/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_adc

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>

#include <errno.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_adc.h>
#include <adc_lib.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_adc.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_bee, CONFIG_ADC_LOG_LEVEL);

#define ADC_12BIT_FULL_SCALE   4096
#define ADC_BEE_VREF_BYPASS_MV 900
#define ADC_BEE_VREF_DIVIDE_MV 3300

#if defined(CONFIG_SOC_SERIES_RTL8752H)
#define ADC_BEE_RTL8752H_UNSUPPORTED_CH 6
#endif

struct adc_bee_config {
	uint32_t reg;
	uint16_t clkid;
	uint8_t channels;
	const struct pinctrl_dev_config *pcfg;
	uint8_t irq_num;
	void (*irq_config_func)(const struct device *dev);
};

struct adc_bee_data {
	struct adc_context ctx;
	const struct device *dev;
	uint32_t is_bypass_mode;
	uint32_t seq_map;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
};

static int adc_bee_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_bee_data *data = dev->data;
	const struct adc_bee_config *cfg = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)cfg->reg;

	if (sequence->resolution != 12U) {
		LOG_ERR("resolution is not valid");
		return -ENOTSUP;
	}

#if defined(CONFIG_SOC_SERIES_RTL8752H)
	if (sequence->channels & BIT(ADC_BEE_RTL8752H_UNSUPPORTED_CH)) {
		LOG_ERR("ADC channel %d is not available on rtl8752h!",
			ADC_BEE_RTL8752H_UNSUPPORTED_CH);
		return -ENOTSUP;
	}
#endif

	data->seq_map = sequence->channels;

	for (uint8_t i = 0; i < cfg->channels; i++) {
		if (sequence->channels & BIT(i)) {
			if (data->is_bypass_mode & BIT(i)) {
				ADC_BypassCmd(i, ENABLE);
			} else {
				ADC_BypassCmd(i, DISABLE);
			}
		}
	}

	ADC_BitMapConfig(adc, sequence->channels);

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_bee_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_bee_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_bee_start_read(dev, seq);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_bee_read_async(const struct device *dev, const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_bee_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = adc_bee_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_bee_channel_setup(const struct device *dev, const struct adc_channel_cfg *chan_cfg)
{
	const struct adc_bee_config *cfg = dev->config;

	if (chan_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not valid, only ADC_GAIN_1 supported");
		return -ENOTSUP;
	}

	if (chan_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Reference is not valid, only ADC_REF_INTERNAL supported");
		return -ENOTSUP;
	}

	if (chan_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported acquisition_time, only default supported");
		return -ENOTSUP;
	}

	if (chan_cfg->differential) {
		LOG_ERR("Differential sampling not supported");
		return -ENOTSUP;
	}

	if (chan_cfg->channel_id >= cfg->channels) {
		LOG_ERR("Invalid channel (%u)", chan_cfg->channel_id);
		return -EINVAL;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_bee_data *data = CONTAINER_OF(ctx, struct adc_bee_data, ctx);
	const struct device *dev = data->dev;
	const struct adc_bee_config *cfg = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)cfg->reg;

	data->repeat_buffer = data->buffer;

	ADC_INTConfig(adc, ADC_INT_ONE_SHOT_DONE, ENABLE);
	ADC_Cmd(adc, ADC_ONE_SHOT_MODE, ENABLE);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_bee_data *data = CONTAINER_OF(ctx, struct adc_bee_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_bee_isr(const struct device *dev)
{
	struct adc_bee_data *data = dev->data;
	const struct adc_bee_config *cfg = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)cfg->reg;
	uint16_t raw_data;
	int32_t voltage;
	uint32_t fake_raw;
	ADC_ErrorStatus error_status = NO_ERROR;

	if (ADC_GetINTStatus(adc, ADC_INT_ONE_SHOT_DONE)) {
		for (uint32_t i = 0; i < cfg->channels; i++) {
			if (data->seq_map & BIT(i)) {
				raw_data = (uint16_t)ADC_ReadRawData(adc, i);
				if (data->is_bypass_mode & BIT(i)) {
					voltage = ADC_GetVoltage(BYPASS_SINGLE_MODE, raw_data,
								 &error_status);
					fake_raw = (voltage * ADC_12BIT_FULL_SCALE) /
						   ADC_BEE_VREF_BYPASS_MV;
				} else {
					voltage = ADC_GetVoltage(DIVIDE_SINGLE_MODE, raw_data,
								 &error_status);
					fake_raw = (voltage * ADC_12BIT_FULL_SCALE) /
						   ADC_BEE_VREF_DIVIDE_MV;
				}

				if (fake_raw > 4095) {
					fake_raw = 4095;
				}

				if (voltage < 0) {
					fake_raw = 0;
				}

				*data->buffer++ = (uint16_t)fake_raw;
			}
		}

		ADC_Cmd(adc, ADC_ONE_SHOT_MODE, DISABLE);
		ADC_INTConfig(adc, ADC_INT_ONE_SHOT_DONE, DISABLE);
		ADC_ClearINTPendingBit(adc, ADC_INT_ONE_SHOT_DONE);
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static DEVICE_API(adc, adc_bee_driver_api) = {
	.channel_setup = adc_bee_channel_setup,
	.read = adc_bee_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_bee_read_async,
#endif /* CONFIG_ADC_ASYNC */
};

static int adc_bee_init(const struct device *dev)
{
	struct adc_bee_data *data = dev->data;
	const struct adc_bee_config *cfg = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)cfg->reg;
	int ret;

	data->dev = dev;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

	ADC_CalibrationInit();

	ADC_InitTypeDef adc_init_struct;

	ADC_StructInit(&adc_init_struct);
	adc_init_struct.ADC_DataAvgEn = DISABLE;
	adc_init_struct.ADC_PowerAlwaysOnEn = ENABLE;
	for (uint32_t i = 0; i < cfg->channels; i++) {
		adc_init_struct.ADC_SchIndex[i] = EXT_SINGLE_ENDED(i);
	}

#if defined(CONFIG_SOC_SERIES_RTL8752H)
	adc_init_struct.ADC_SchIndex[ADC_BEE_RTL8752H_UNSUPPORTED_CH] = 0;
#endif

	adc_init_struct.ADC_SchIndex[cfg->channels - 1] = INTERNAL_VBAT_MODE;

	ADC_Init(adc, &adc_init_struct);

	cfg->irq_config_func(dev);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define BEE_ADC_INIT(index)                                                                        \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static void adc_bee_irq_config_func(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), adc_bee_isr,        \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}                                                                                          \
	static struct adc_bee_data adc_bee_data_##index = {                                        \
		ADC_CONTEXT_INIT_TIMER(adc_bee_data_##index, ctx),                                 \
		ADC_CONTEXT_INIT_LOCK(adc_bee_data_##index, ctx),                                  \
		ADC_CONTEXT_INIT_SYNC(adc_bee_data_##index, ctx),                                  \
		.is_bypass_mode = DT_INST_PROP_OR(index, is_bypass_mode, 0),                       \
	};                                                                                         \
	const static struct adc_bee_config adc_bee_config_##index = {                              \
		.reg = DT_INST_REG_ADDR(index),                                                    \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.channels = DT_INST_PROP(index, channels),                                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_num = DT_INST_IRQN(index),                                                    \
		.irq_config_func = adc_bee_irq_config_func,                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, &adc_bee_init, NULL, &adc_bee_data_##index,                   \
			      &adc_bee_config_##index, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,      \
			      &adc_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BEE_ADC_INIT)
