/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_gpadc

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <register.h>

LOG_MODULE_REGISTER(adc_sf32lb, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_CFG_REG1 offsetof(GPADC_TypeDef, ADC_CFG_REG1)
#define ADC_SLOT_REG offsetof(GPADC_TypeDef, ADC_SLOT0_REG)
#define ADC_RDATA    offsetof(GPADC_TypeDef, ADC_RDATA0)
#define ADC_CTRL_REG offsetof(GPADC_TypeDef, ADC_CTRL_REG)
#define GPADC_IRQ    offsetof(GPADC_TypeDef, GPADC_IRQ)

#define SYS_CFG_ANAU_CR offsetof(HPSYS_CFG_TypeDef, ANAU_CR)

#define ADC_MAX_CH (8U)
#define ADC_RDATAX(n)    (ADC_RDATA + (((n) >> 1) * 4U))
#define ADC_SLOT_REGX(n) (ADC_SLOT_REG + (n) * 4U)

#define ADC_SF32LB_DEFAULT_VREF_INTERNAL 3300

struct adc_sf32lb_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
};

struct adc_sf32lb_config {
	uintptr_t base;
	uintptr_t cfg_base;
	const struct pinctrl_dev_config *pcfg;
	struct sf32lb_clock_dt_spec clock;
	void (*irq_config_func)(void);
};

static void adc_sf32lb_isr(const struct device *dev)
{
	const struct adc_sf32lb_config *config = dev->config;
	struct adc_sf32lb_data *data = dev->data;
	uint16_t channel;
	uint32_t adc_data;

	if (!sys_test_bit(config->base + GPADC_IRQ, GPADC_GPADC_IRQ_GPADC_IRSR_Pos)) {
		return;
	}

	sys_set_bit(config->base + GPADC_IRQ, GPADC_GPADC_IRQ_GPADC_ICR_Pos);

	while (data->channels) {
		channel = find_lsb_set(data->channels) - 1;
		adc_data = sys_read32(config->base + ADC_RDATAX(channel));

		if (channel & 1) {
			*data->buffer++ = FIELD_GET(GPADC_ADC_RDATA0_SLOT1_RDATA, adc_data);
		} else {
			*data->buffer++ = FIELD_GET(GPADC_ADC_RDATA0_SLOT0_RDATA, adc_data);
		}

		data->channels &= ~BIT(channel);
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}

static int adc_sf32lb_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_sf32lb_config *config = dev->config;
	uint8_t channel_id;
	uint32_t adc_slot = 0;

	channel_id = channel_cfg->channel_id;

	if (channel_cfg->channel_id >= ADC_MAX_CH) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Acquisition time is not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("External reference is not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		adc_slot |= FIELD_PREP(GPADC_ADC_SLOT0_REG_PCHNL_SEL, channel_id);
		adc_slot |= FIELD_PREP(GPADC_ADC_SLOT0_REG_NCHNL_SEL, channel_id);
	} else {
		adc_slot |= FIELD_PREP(GPADC_ADC_SLOT0_REG_PCHNL_SEL, channel_id);
	}

	adc_slot |= GPADC_ADC_SLOT0_REG_SLOT_EN;
	sys_write32(adc_slot, config->base + ADC_SLOT_REGX(channel_id));

	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_sf32lb_data *data =
		CONTAINER_OF(ctx, struct adc_sf32lb_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);
	if (sequence->options) {
		needed_buffer_size *= (1U + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)",
			sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}
	return 0;
}

static void adc_sf32lb_start_conversion(const struct device *dev)
{
	const struct adc_sf32lb_config *const cfg = dev->config;

	sys_set_bit(cfg->base + ADC_CTRL_REG, GPADC_ADC_CTRL_REG_ADC_START_Pos);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_sf32lb_data *data = CONTAINER_OF(ctx, struct adc_sf32lb_data, ctx);

	adc_sf32lb_start_conversion(data->dev);
}

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_sf32lb_data *data = dev->data;
	uint8_t num_active_channels;
	int error;

	data->channels = sequence->channels;

	num_active_channels = sys_count_bits(&data->channels, sizeof(data->channels));
	error = check_buffer_size(sequence, num_active_channels);
	if (error < 0) {
		return error;
	}

	data->buffer = sequence->buffer;
	data->repeat_buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	error = adc_context_wait_for_completion(&data->ctx);

	return error;
}

static int adc_sf32lb_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_sf32lb_data *data = dev->data;
	int error;

	if (sequence->resolution != 12U) {
		LOG_ERR("Resolution %d is not supported", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling is not supported");
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		LOG_ERR("Calibration is not supported");
		return -ENOTSUP;
	}

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static DEVICE_API(adc, adc_sf32lb_driver_api) = {
	.channel_setup = adc_sf32lb_channel_setup,
	.read = adc_sf32lb_read,
	.ref_internal = ADC_SF32LB_DEFAULT_VREF_INTERNAL,
};

static int adc_sf32lb_init(const struct device *dev)
{
	const struct adc_sf32lb_config *config = dev->config;
	struct adc_sf32lb_data *data = dev->data;
	int ret;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	/* enable bandgap*/
	sys_set_bit(config->cfg_base + SYS_CFG_ANAU_CR, HPSYS_CFG_ANAU_CR_EN_BG_Pos);

	sys_clear_bits(config->base + ADC_CTRL_REG, GPADC_ADC_CTRL_REG_TIMER_TRIG_EN |
		GPADC_ADC_CTRL_REG_DMA_EN);
	sys_set_bits(config->base + ADC_CTRL_REG, GPADC_ADC_CTRL_REG_FRC_EN_ADC |
		GPADC_ADC_CTRL_REG_CHNL_SEL_FRC_EN);
	/* enable ref ldo */
	sys_set_bits(config->base + ADC_CFG_REG1, GPADC_ADC_CFG_REG1_ANAU_GPADC_SE |
		GPADC_ADC_CFG_REG1_ANAU_GPADC_LDOREF_EN);

	/* disable all slots */
	for (uint8_t i = 0; i < 8U; i++) {
		sys_clear_bit(config->base + ADC_SLOT_REGX(i), GPADC_ADC_SLOT0_REG_SLOT_EN_Pos);
	}

	config->irq_config_func();

	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return ret;
}

#define ADC_SF32LB_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void adc_sf32lb_irq_config_func_##n(void);                                          \
	static struct adc_sf32lb_data adc_sf32lb_data_##n = {                                      \
		ADC_CONTEXT_INIT_TIMER(adc_sf32lb_data_##n, ctx),                                  \
		ADC_CONTEXT_INIT_LOCK(adc_sf32lb_data_##n, ctx),                                   \
		ADC_CONTEXT_INIT_SYNC(adc_sf32lb_data_##n, ctx),                                   \
	};                                                                                         \
	static const struct adc_sf32lb_config adc_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.cfg_base = DT_REG_ADDR(DT_INST_PHANDLE(n, sifli_cfg)),                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
		.irq_config_func = adc_sf32lb_irq_config_func_##n,                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, adc_sf32lb_init, NULL, &adc_sf32lb_data_##n,                      \
			      &adc_sf32lb_config_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,       \
			      &adc_sf32lb_driver_api);                                             \
	static void adc_sf32lb_irq_config_func_##n(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), adc_sf32lb_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(ADC_SF32LB_DEFINE)
