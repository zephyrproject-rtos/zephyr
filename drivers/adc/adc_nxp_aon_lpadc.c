/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT nxp_aon_lpadc

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_common.h"
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_nxp_aon_lpadc, CONFIG_ADC_LOG_LEVEL);

/* Multi-channel sequences are converted in hardware: each selected sequence
 * channel is mapped to one conversion command (command N drives channel id
 * N-1), the commands are chained through CMDH[NEXT], and a single software
 * trigger on trigger 0 walks the chain. Every conversion pushes a result into
 * FIFO 0, which the FIFO-watermark interrupt drains in channel order.
 */

#define LPADC_AON_CMD_COUNT 7U

/* The converter is fixed at 12-bit single-ended resolution; results are
 * left-justified in RESFIFO[D], so with hardware averaging disabled the
 * data occupies D[15:4].
 */
#define LPADC_AON_RESOLUTION   12U
#define LPADC_AON_RESULT_SHIFT 4U

/* Offset calibration averages 128 conversions, so allow for a slow ADCK. */
#define LPADC_AON_CAL_TIMEOUT_US 100000U

struct adc_aon_lpadc_config {
	LPADC_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint8_t voltage_ref;
	uint8_t power_level;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
};

struct adc_aon_lpadc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t channel_input[LPADC_AON_CMD_COUNT];
	uint8_t configured_channels;
	uint8_t first_command;
	uint8_t total;
	uint8_t collected;
};

static inline volatile uint32_t *lpadc_cmdl(LPADC_Type *base, uint8_t command)
{
	return &base->CMDL1 + (command - 1U) * 2U;
}

static inline volatile uint32_t *lpadc_cmdh(LPADC_Type *base, uint8_t command)
{
	return &base->CMDL1 + (command - 1U) * 2U + 1U;
}

static int adc_aon_lpadc_channel_setup(const struct device *dev,
				       const struct adc_channel_cfg *channel_cfg)
{
	struct adc_aon_lpadc_data *data = dev->data;

	if (channel_cfg->channel_id >= LPADC_AON_CMD_COUNT) {
		LOG_ERR("Channel %u not supported (max %u)", channel_cfg->channel_id,
			LPADC_AON_CMD_COUNT - 1U);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL &&
	    channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("Unsupported channel reference 0x%02x", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported acquisition time 0x%02x", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->input_positive > (LPADC_CMDL1_ADCH_MASK >> LPADC_CMDL1_ADCH_SHIFT)) {
		LOG_ERR("Invalid input %u", channel_cfg->input_positive);
		return -EINVAL;
	}

	data->channel_input[channel_cfg->channel_id] = channel_cfg->input_positive;
	data->configured_channels |= BIT(channel_cfg->channel_id);

	return 0;
}

/* Offset calibration is mandatory after every converter reset and before the
 * first conversion. The converter must be enabled while it runs.
 */
static int adc_aon_lpadc_calibrate(LPADC_Type *base)
{
	base->CTRL |= LPADC_CTRL_CALOFS_MASK;

	if (!WAIT_FOR((base->STAT & LPADC_STAT_CAL_RDY_MASK) != 0U, LPADC_AON_CAL_TIMEOUT_US,
		      k_busy_wait(10))) {
		LOG_ERR("Offset calibration timed out");
		return -ETIMEDOUT;
	}

	return 0;
}

static int adc_aon_lpadc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_aon_lpadc_config *config = dev->config;
	struct adc_aon_lpadc_data *data = dev->data;
	LPADC_Type *base = config->base;
	uint8_t next = 0U;
	uint8_t first = 0U;
	int ret;

	if (sequence->channels == 0U) {
		LOG_ERR("No channels selected");
		return -EINVAL;
	}

	if ((sequence->channels >> LPADC_AON_CMD_COUNT) != 0U) {
		LOG_ERR("Channel id out of range (max %u)", LPADC_AON_CMD_COUNT - 1U);
		return -EINVAL;
	}

	if ((sequence->channels & ~(uint32_t)data->configured_channels) != 0U) {
		LOG_ERR("Sequence contains unconfigured channels (mask 0x%08x)",
			sequence->channels);
		return -EINVAL;
	}

	if (sequence->resolution != LPADC_AON_RESOLUTION) {
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -EINVAL;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("Oversampling is not supported");
		return -ENOTSUP;
	}

	ret = adc_sequence_validate_buffer(sequence, POPCOUNT(sequence->channels),
					   sizeof(uint16_t));
	if (ret < 0) {
		LOG_ERR("Buffer too small: %u", sequence->buffer_size);
		return ret;
	}

	if (sequence->calibrate) {
		ret = adc_aon_lpadc_calibrate(base);
		if (ret < 0) {
			return ret;
		}
	}

	/*
	 * Build the command chain over the selected channels, walking from the
	 * highest channel id down so each command's CMDH[NEXT] points at the
	 * next-higher channel's command (the highest ends the chain). The chain
	 * therefore runs lowest -> highest, producing results in channel order.
	 */
	for (int8_t ch = LPADC_AON_CMD_COUNT - 1; ch >= 0; ch--) {
		uint8_t command;

		if ((sequence->channels & BIT(ch)) == 0U) {
			continue;
		}

		command = (uint8_t)ch + 1U;
		*lpadc_cmdl(base, command) = LPADC_CMDL1_ADCH(data->channel_input[ch]);
		*lpadc_cmdh(base, command) = LPADC_CMDH1_NEXT(next);
		next = command;
		first = command;
	}

	data->first_command = first;
	data->total = (uint8_t)POPCOUNT(sequence->channels);
	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_aon_lpadc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				    struct k_poll_signal *async)
{
	struct adc_aon_lpadc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = adc_aon_lpadc_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int adc_aon_lpadc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_aon_lpadc_read_async(dev, sequence, NULL);
}

static void adc_aon_lpadc_isr(const struct device *dev)
{
	const struct adc_aon_lpadc_config *config = dev->config;
	struct adc_aon_lpadc_data *data = dev->data;
	LPADC_Type *base = config->base;
	uint32_t result;

	/* Drain every result currently in FIFO 0 (clears the watermark flag). */
	while (((result = base->RESFIFO0) & LPADC_RESFIFO0_VALID_MASK) != 0U) {
		if (data->collected < data->total) {
			*data->buffer++ = (uint16_t)((result & LPADC_RESFIFO0_D_MASK) >>
						     LPADC_AON_RESULT_SHIFT);
			data->collected++;
		}
	}

	if (data->collected >= data->total) {
		base->IE &= ~LPADC_IE_FWMIE0_MASK;
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_aon_lpadc_data *data = CONTAINER_OF(ctx, struct adc_aon_lpadc_data, ctx);
	const struct adc_aon_lpadc_config *config = data->dev->config;
	LPADC_Type *base = config->base;

	data->repeat_buffer = data->buffer;
	data->collected = 0U;

	/* Flush FIFO 0 and set the watermark so a single interrupt fires once the
	 * whole command chain has produced all of this round's results.
	 */
	base->CTRL |= LPADC_CTRL_RSTFIFO0_MASK;
	base->FCTRL0 = LPADC_FCTRL0_FWMARK(data->total - 1U);
	base->TCTRL[0] = LPADC_TCTRL_TCMD(data->first_command);
	base->IE |= LPADC_IE_FWMIE0_MASK;
	base->SWTRIG = LPADC_SWTRIG_SWT0_MASK;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_aon_lpadc_data *data = CONTAINER_OF(ctx, struct adc_aon_lpadc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_aon_lpadc_init(const struct device *dev)
{
	const struct adc_aon_lpadc_config *config = dev->config;
	struct adc_aon_lpadc_data *data = dev->data;
	LPADC_Type *base = config->base;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Register access faults while the AON CGU clock gate is closed. */
	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	/* Reset the converter and its result FIFO. */
	base->CTRL = LPADC_CTRL_RST_MASK;
	base->CTRL = 0U;
	base->CTRL = LPADC_CTRL_RSTFIFO0_MASK;

	base->CFG = LPADC_CFG_PWRSEL(config->power_level) | LPADC_CFG_REFSEL(config->voltage_ref);

	/* Calibrate with the maximum calibration sample time, averaging over
	 * 128 conversions.
	 */
	base->CTRL |= LPADC_CTRL_CAL_AVGS(7) | LPADC_CTRL_CAL_STS(7);
	base->CTRL |= LPADC_CTRL_ADCEN_MASK;

	ret = adc_aon_lpadc_calibrate(base);
	if (ret < 0) {
		return ret;
	}

	config->irq_config_func(dev);

	data->dev = dev;
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_aon_lpadc_driver_api) = {
	.channel_setup = adc_aon_lpadc_channel_setup,
	.read = adc_aon_lpadc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_aon_lpadc_read_async,
#endif
};

#define ADC_AON_LPADC_INIT(n)                                                                      \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void adc_aon_lpadc_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), adc_aon_lpadc_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct adc_aon_lpadc_config adc_aon_lpadc_config_##n = {                      \
		.base = (LPADC_Type *)DT_INST_REG_ADDR(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.voltage_ref = DT_INST_PROP(n, voltage_ref),                                       \
		.power_level = DT_INST_PROP(n, power_level),                                       \
		.irq_config_func = adc_aon_lpadc_config_func_##n,                                  \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
	};                                                                                         \
                                                                                                   \
	static struct adc_aon_lpadc_data adc_aon_lpadc_data_##n = {                                \
		ADC_CONTEXT_INIT_TIMER(adc_aon_lpadc_data_##n, ctx),                               \
		ADC_CONTEXT_INIT_LOCK(adc_aon_lpadc_data_##n, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(adc_aon_lpadc_data_##n, ctx),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_aon_lpadc_init, NULL, &adc_aon_lpadc_data_##n,                \
			      &adc_aon_lpadc_config_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,    \
			      &adc_aon_lpadc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_AON_LPADC_INIT)
