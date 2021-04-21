/*
 * Copyright (c) 2017-2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_adc16

#include <errno.h>
#include <drivers/adc.h>
#include <fsl_adc16.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_mcux_adc16);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

struct mcux_adc16_config {
	ADC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	bool channel_mux_b;
};

struct mcux_adc16_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t channel_id;
};

static int mcux_adc16_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id > (ADC_SC1_ADCH_MASK >> ADC_SC1_ADCH_SHIFT)) {
		LOG_ERR("Channel %d is not valid", channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid channel acquisition time");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

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
#if defined(FSL_FEATURE_ADC16_MAX_RESOLUTION) && (FSL_FEATURE_ADC16_MAX_RESOLUTION >= 16U)
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

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	error = adc_context_wait_for_completion(&data->ctx);
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

#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
	channel_config.enableDifferentialConversion = false;
#endif
	channel_config.enableInterruptOnConversionCompleted = true;
	channel_config.channelNumber = data->channel_id;
	ADC16_SetChannelConfig(config->base, channel_group, &channel_config);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcux_adc16_data *data =
		CONTAINER_OF(ctx, struct mcux_adc16_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

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

static void mcux_adc16_isr(const struct device *dev)
{
	const struct mcux_adc16_config *config = dev->config;
	struct mcux_adc16_data *data = dev->data;
	ADC_Type *base = config->base;
	uint32_t channel_group = 0U;
	uint16_t result;

	result = ADC16_GetChannelConversionValue(base, channel_group);
	LOG_DBG("Finished channel %d. Result is 0x%04x",
		    data->channel_id, result);

	*data->buffer++ = result;
	data->channels &= ~BIT(data->channel_id);

	if (data->channels) {
		mcux_adc16_start_channel(dev);
	} else {
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int mcux_adc16_init(const struct device *dev)
{
	const struct mcux_adc16_config *config = dev->config;
	struct mcux_adc16_data *data = dev->data;
	ADC_Type *base = config->base;
	adc16_config_t adc_config;

	ADC16_GetDefaultConfig(&adc_config);

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
	ADC16_EnableHardwareTrigger(base, false);

	config->irq_config_func(dev);
	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api mcux_adc16_driver_api = {
	.channel_setup = mcux_adc16_channel_setup,
	.read = mcux_adc16_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcux_adc16_read_async,
#endif
};

#define ACD16_MCUX_INIT(n)						\
	static void mcux_adc16_config_func_##n(const struct device *dev); \
									\
	static const struct mcux_adc16_config mcux_adc16_config_##n = {	\
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),		\
		.irq_config_func = mcux_adc16_config_func_##n,		\
		.channel_mux_b = DT_INST_PROP(n, channel_mux_b),	\
	};								\
									\
	static struct mcux_adc16_data mcux_adc16_data_##n = {		\
		ADC_CONTEXT_INIT_TIMER(mcux_adc16_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(mcux_adc16_data_##n, ctx),	\
		ADC_CONTEXT_INIT_SYNC(mcux_adc16_data_##n, ctx),	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &mcux_adc16_init,			\
			    device_pm_control_nop, &mcux_adc16_data_##n,\
			    &mcux_adc16_config_##n, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &mcux_adc16_driver_api);			\
									\
	static void mcux_adc16_config_func_##n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    mcux_adc16_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(ACD16_MCUX_INIT)
