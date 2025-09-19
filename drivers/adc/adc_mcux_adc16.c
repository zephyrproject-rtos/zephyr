/*
 * Copyright (c) 2017-2018, 2020, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_adc16

#include <errno.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
#include <zephyr/drivers/dma.h>
#endif

#include <fsl_adc16.h>
#include <zephyr/dt-bindings/adc/mcux-adc16.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(adc_mcux_adc16);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

struct mcux_adc16_config {
	ADC_Type *base;
#ifndef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	void (*irq_config_func)(const struct device *dev);
#endif
	uint32_t clk_source;	/* ADC clock source selection */
	uint32_t long_sample;	/* ADC long sample mode selection */
	uint32_t hw_trigger_src;  /* ADC hardware trigger source */
				/* defined in SIM module SOPT7 */
#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	uint32_t dma_slot;	/* ADC DMA MUX slot */
#endif
	uint32_t trg_offset;
	uint32_t trg_bits;
	uint32_t alt_offset;
	uint32_t alt_bits;
	bool periodic_trigger; /* ADC enable periodic trigger */
	bool channel_mux_b;
	bool high_speed;	/* ADC enable high speed mode*/
	bool continuous_convert; /* ADC enable continuous convert*/
	const struct pinctrl_dev_config *pincfg;
	/* Whether this instance supports differential mode. */
	bool supports_diff;
};

#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
struct adc_edma_config {
	int32_t state;
	uint32_t dma_channel;
	void (*irq_call_back)(void);
	struct dma_config dma_cfg;
	struct dma_block_config dma_block;
};
#endif

struct mcux_adc16_data {
	const struct device *dev;
	struct adc_context ctx;
#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	const struct device *dev_dma;
	struct adc_edma_config adc_dma_config;
#endif
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t channel_id;
	/* Bitmask of channels requested as differential by user config */
	uint32_t diff_channels;
};

#ifdef CONFIG_ADC_MCUX_ADC16_HW_TRIGGER
#define SIM_SOPT7_ADCSET(x, shifts, mask)                                      \
	(((uint32_t)(((uint32_t)(x)) << shifts)) & mask)
#endif

#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
static void adc_dma_callback(const struct device *dma_dev, void *callback_arg,
			     uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)callback_arg;
	struct mcux_adc16_data *data = dev->data;

	LOG_DBG("DMA done");
	data->buffer++;
	adc_context_on_sampling_done(&data->ctx, dev);
}
#endif

#ifdef CONFIG_ADC_MCUX_ADC16_HW_TRIGGER
static void adc_hw_trigger_enable(const struct device *dev)
{
	const struct mcux_adc16_config *config = dev->config;

	/* enable ADC trigger channel */
	SIM->SOPT7 |= SIM_SOPT7_ADCSET(config->hw_trigger_src,
				       config->trg_offset, config->trg_bits) |
		      SIM_SOPT7_ADCSET(1, config->alt_offset, config->alt_bits);
}
#endif

static int mcux_adc16_acquisition_time_setup(const struct device *dev, uint16_t acq_time)
{
	const struct mcux_adc16_config *config = dev->config;
	uint32_t acquisition_time_value = ADC_ACQ_TIME_VALUE(acq_time);
	uint8_t acquisition_time_unit = ADC_ACQ_TIME_UNIT(acq_time);

	if (acquisition_time_value == ADC_ACQ_TIME_DEFAULT) {
		return 0;
	}

	if (acquisition_time_unit == ADC_ACQ_TIME_TICKS) {
		/* acquisition_time_value must directly match ADLSTS field encoding (0..3). */
		if (acquisition_time_value > MCUX_ADC16_ACQUISITION_TIME_6CYCLE) {
			LOG_ERR("Invalid acquisition time ticks value: %u", acquisition_time_value);
			return -EINVAL;
		}
		config->base->CFG2 = ((config->base->CFG2 & ~ADC_CFG2_ADLSTS_MASK) |
					ADC_CFG2_ADLSTS(acquisition_time_value));
	} else {
		LOG_ERR("Acquisition time unit not support");
		return -ENOTSUP;
	}

	return 0;
}

static int mcux_adc16_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	const struct mcux_adc16_config *config = dev->config;
	uint8_t channel_id = channel_cfg->channel_id;
	int ret;

	if (channel_id > (ADC_SC1_ADCH_MASK >> ADC_SC1_ADCH_SHIFT)) {
		LOG_ERR("Channel %d is not valid", channel_id);
		return -EINVAL;
	}

	ret = mcux_adc16_acquisition_time_setup(dev, channel_cfg->acquisition_time);
	if (ret) {
		LOG_ERR("ADC16 acquisition time setting failed");
		return ret;
	}

	if (channel_cfg->differential) {
		if (!config->supports_diff) {
			LOG_ERR("Differential channels are not supported on %s", dev->name);
			return -ENOTSUP;
		}
		/* Record user's differential request for this channel */
		((struct mcux_adc16_data *)dev->data)->diff_channels |= BIT(channel_id);
	}

	if (channel_cfg->reference == ADC_REF_EXTERNAL0) {
		/* Select Vrefh and Vrefl as reference */
		config->base->SC2 &= ~ADC_SC2_REFSEL_MASK;
	} else if (channel_cfg->reference == ADC_REF_VDD_1) {
		/* Select Valth and Valtl as reference */
		config->base->SC2 = ((config->base->SC2 & ~ADC_SC2_REFSEL_MASK) |
					ADC_SC2_REFSEL(1));
	} else {
		LOG_DBG("ref not support");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

#ifdef CONFIG_ADC_MCUX_ADC16_HW_TRIGGER
	adc_hw_trigger_enable(dev);
#endif

	return 0;
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	const struct mcux_adc16_config *config = dev->config;
	struct mcux_adc16_data *data = dev->data;
	adc16_hardware_average_mode_t mode;
	adc16_resolution_t resolution;
	int error;
	uint32_t tmp32;
	ADC_Type *base = config->base;

	switch (sequence->resolution) {
	case 8:
	case 9:
		resolution = kADC16_Resolution8or9Bit;
		break;
	case 10:
	case 11:
		resolution = kADC16_Resolution10or11Bit;
		break;
	case 12:
	case 13:
		resolution = kADC16_Resolution12or13Bit;
		break;
#if defined(FSL_FEATURE_ADC16_MAX_RESOLUTION) &&                               \
	(FSL_FEATURE_ADC16_MAX_RESOLUTION >= 16U)
	case 16:
		resolution = kADC16_Resolution16Bit;
		break;
#endif
	default:
		LOG_ERR("Invalid resolution");
		return -EINVAL;
	}

	tmp32 = base->CFG1 & ~(ADC_CFG1_MODE_MASK);
	tmp32 |= ADC_CFG1_MODE(resolution);
	base->CFG1 = tmp32;

	switch (sequence->oversampling) {
	case 0:
		mode = kADC16_HardwareAverageDisabled;
		break;
	case 2:
		mode = kADC16_HardwareAverageCount4;
		break;
	case 3:
		mode = kADC16_HardwareAverageCount8;
		break;
	case 4:
		mode = kADC16_HardwareAverageCount16;
		break;
	case 5:
		mode = kADC16_HardwareAverageCount32;
		break;
	default:
		LOG_ERR("Invalid oversampling");
		return -EINVAL;
	}
	ADC16_SetHardwareAverage(config->base, mode);

	if (sequence->buffer_size < 2) {
		LOG_ERR("sequence buffer size too small %d < 2", sequence->buffer_size);
		return -EINVAL;
	}

	if (sequence->options) {
		if (sequence->buffer_size <
			2 * (sequence->options->extra_samplings + 1)) {
			LOG_ERR("sequence buffer size too small < 2 * extra + 2");
			return -EINVAL;
		}
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	error = adc_context_wait_for_completion(&data->ctx);
#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	dma_stop(data->dev_dma, data->adc_dma_config.dma_channel);
#endif
	return error;
}

static int mcux_adc16_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	struct mcux_adc16_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int mcux_adc16_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct mcux_adc16_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static void mcux_adc16_start_channel(const struct device *dev)
{
	const struct mcux_adc16_config *config = dev->config;
	struct mcux_adc16_data *data = dev->data;

	adc16_channel_config_t channel_config;
	uint32_t channel_group = 0U;

	data->channel_id = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d", data->channel_id);

	/* Configure differential per channel if supported/requested */
	if (config->supports_diff && (data->diff_channels & BIT(data->channel_id))) {
		channel_config.enableDifferentialConversion = true;
	} else {
		channel_config.enableDifferentialConversion = false;
	}

	channel_config.enableInterruptOnConversionCompleted = true;
	channel_config.channelNumber = data->channel_id;
	ADC16_SetChannelConfig(config->base, channel_group, &channel_config);
#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	LOG_DBG("Starting EDMA");
	dma_start(data->dev_dma, data->adc_dma_config.dma_channel);
#endif
	LOG_DBG("Starting channel done");
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcux_adc16_data *data =
		CONTAINER_OF(ctx, struct mcux_adc16_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	LOG_DBG("config dma");
	data->adc_dma_config.dma_block.block_size = 2;
	data->adc_dma_config.dma_block.dest_address = (uint32_t)data->buffer;
	data->adc_dma_config.dma_cfg.head_block =
		&(data->adc_dma_config.dma_block);
	dma_config(data->dev_dma, data->adc_dma_config.dma_channel,
		   &data->adc_dma_config.dma_cfg);
#endif

	mcux_adc16_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct mcux_adc16_data *data =
		CONTAINER_OF(ctx, struct mcux_adc16_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

#ifndef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
static void mcux_adc16_isr(const struct device *dev)
{
	const struct mcux_adc16_config *config = dev->config;
	struct mcux_adc16_data *data = dev->data;
	ADC_Type *base = config->base;
	uint32_t channel_group = 0U;
	uint16_t result;

	result = ADC16_GetChannelConversionValue(base, channel_group);
	LOG_DBG("Finished channel %d. Result is 0x%04x", data->channel_id,
		result);

	*data->buffer++ = result;
	data->channels &= ~BIT(data->channel_id);

	if (data->channels) {
		mcux_adc16_start_channel(dev);
	} else {
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}
#endif

static int mcux_adc16_init(const struct device *dev)
{
	const struct mcux_adc16_config *config = dev->config;
	struct mcux_adc16_data *data = dev->data;
	ADC_Type *base = config->base;
	adc16_config_t adc_config;
	int err;

	LOG_DBG("init adc");
	ADC16_GetDefaultConfig(&adc_config);

#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	adc_config.clockSource = (adc16_clock_source_t)config->clk_source;
	adc_config.longSampleMode =
		(adc16_long_sample_mode_t)config->long_sample;
	adc_config.enableHighSpeed = config->high_speed;
	adc_config.enableContinuousConversion = config->continuous_convert;
#endif

#if CONFIG_ADC_MCUX_ADC16_VREF_DEFAULT
	adc_config.referenceVoltageSource = kADC16_ReferenceVoltageSourceVref;
#else /* CONFIG_ADC_MCUX_ADC16_VREF_ALTERNATE */
	adc_config.referenceVoltageSource = kADC16_ReferenceVoltageSourceValt;
#endif

#if CONFIG_ADC_MCUX_ADC16_CLK_DIV_RATIO_1
	adc_config.clockDivider = kADC16_ClockDivider1;
#elif CONFIG_ADC_MCUX_ADC16_CLK_DIV_RATIO_2
	adc_config.clockDivider = kADC16_ClockDivider2;
#elif CONFIG_ADC_MCUX_ADC16_CLK_DIV_RATIO_4
	adc_config.clockDivider = kADC16_ClockDivider4;
#else /* CONFIG_ADC_MCUX_ADC16_CLK_DIV_RATIO_8 */
	adc_config.clockDivider = kADC16_ClockDivider8;
#endif

	ADC16_Init(base, &adc_config);
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && \
	    FSL_FEATURE_ADC16_HAS_CALIBRATION
	ADC16_SetHardwareAverage(base, kADC16_HardwareAverageCount32);
	ADC16_DoAutoCalibration(base);
#endif
	if (config->channel_mux_b) {
		ADC16_SetChannelMuxMode(base, kADC16_ChannelMuxB);
	}

	if (IS_ENABLED(CONFIG_ADC_MCUX_ADC16_HW_TRIGGER)) {
		ADC16_EnableHardwareTrigger(base, true);
	} else {
		ADC16_EnableHardwareTrigger(base, false);
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	data->dev = dev;

	/* dma related init */
#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
	/* Enable DMA. */
	ADC16_EnableDMA(base, true);
	data->adc_dma_config.dma_cfg.block_count = 1U;
	data->adc_dma_config.dma_cfg.dma_slot = config->dma_slot;
	data->adc_dma_config.dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	data->adc_dma_config.dma_cfg.source_burst_length = 2U;
	data->adc_dma_config.dma_cfg.dest_burst_length = 2U;
	data->adc_dma_config.dma_cfg.channel_priority = 0U;
	data->adc_dma_config.dma_cfg.dma_callback = adc_dma_callback;
	data->adc_dma_config.dma_cfg.user_data = (void *)dev;

	data->adc_dma_config.dma_cfg.source_data_size = 2U;
	data->adc_dma_config.dma_cfg.dest_data_size = 2U;
	data->adc_dma_config.dma_block.source_address = (uint32_t)&base->R[0];

	if (data->dev_dma == NULL || !device_is_ready(data->dev_dma)) {
		LOG_ERR("dma binding fail");
		return -EINVAL;
	}

	if (config->periodic_trigger) {
		enum dma_channel_filter adc_filter = DMA_CHANNEL_PERIODIC;

		data->adc_dma_config.dma_channel =
			dma_request_channel(data->dev_dma, (void *)&adc_filter);
	} else {
		enum dma_channel_filter adc_filter = DMA_CHANNEL_NORMAL;

		data->adc_dma_config.dma_channel =
			dma_request_channel(data->dev_dma, (void *)&adc_filter);
	}
	if (data->adc_dma_config.dma_channel == -EINVAL) {
		LOG_ERR("can not allocate dma channel");
		return -EINVAL;
	}
	LOG_DBG("dma allocated channel %d", data->adc_dma_config.dma_channel);
#else
	config->irq_config_func(dev);
#endif
	LOG_DBG("adc init done");

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, mcux_adc16_driver_api) = {
	.channel_setup = mcux_adc16_channel_setup,
	.read = mcux_adc16_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcux_adc16_read_async,
#endif
};

#ifdef CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA
#define ADC16_MCUX_EDMA_INIT(n)					\
	.hw_trigger_src =					\
		DT_INST_PROP_OR(n, hw_trigger_src, 0),		\
	.dma_slot = DT_INST_DMAS_CELL_BY_IDX(n, 0, source),	\
	.trg_offset = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, offset),	\
	.trg_bits = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, bits),	\
	.alt_offset = DT_INST_CLOCKS_CELL_BY_IDX(n, 1, offset),	\
	.alt_bits = DT_INST_CLOCKS_CELL_BY_IDX(n, 1, bits),
#define ADC16_MCUX_EDMA_DATA(n)					\
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, adc##n))
#define ADC16_MCUX_IRQ_INIT(n)
#define ADC16_MCUX_IRQ_DECLARE(n)
#else
#define ADC16_MCUX_EDMA_INIT(n)
#define ADC16_MCUX_EDMA_DATA(n)
#define ADC16_MCUX_IRQ_INIT(n) .irq_config_func = mcux_adc16_config_func_##n,
#define ADC16_MCUX_IRQ_DECLARE(n)					\
	static void mcux_adc16_config_func_##n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    mcux_adc16_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}
#endif /* CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA */


#define ACD16_MCUX_INIT(n)					\
	ADC16_MCUX_IRQ_DECLARE(n)				\
	PINCTRL_DT_INST_DEFINE(n);				\
								\
	static const struct mcux_adc16_config mcux_adc16_config_##n = {	\
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),		\
		ADC16_MCUX_IRQ_INIT(n)					\
		.channel_mux_b = DT_INST_PROP(n, channel_mux_b),	\
		.clk_source = DT_INST_PROP_OR(n, clk_source, 0),	\
		.long_sample = DT_INST_PROP_OR(n, long_sample, 0),	\
		.high_speed = DT_INST_PROP(n, high_speed),		\
		.periodic_trigger = DT_INST_PROP(n, periodic_trigger),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.continuous_convert =				\
			DT_INST_PROP(n, continuous_convert),	\
		.supports_diff = DT_INST_PROP(n, has_differential_mode),\
		ADC16_MCUX_EDMA_INIT(n)				\
	};								\
									\
	static struct mcux_adc16_data mcux_adc16_data_##n = {		\
		ADC_CONTEXT_INIT_TIMER(mcux_adc16_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(mcux_adc16_data_##n, ctx),	\
		ADC_CONTEXT_INIT_SYNC(mcux_adc16_data_##n, ctx),	\
		.diff_channels = 0U,					\
		ADC16_MCUX_EDMA_DATA(n)					\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &mcux_adc16_init,	\
			      NULL,	\
			      &mcux_adc16_data_##n,	\
			      &mcux_adc16_config_##n,	\
			      POST_KERNEL,		\
			      CONFIG_ADC_INIT_PRIORITY,	\
			      &mcux_adc16_driver_api);	\

DT_INST_FOREACH_STATUS_OKAY(ACD16_MCUX_INIT)
