/*
 * Copyright (c) 2021 Antonio Tessarolo
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_vf610_adc

#include <errno.h>
#include <zephyr/drivers/adc.h>
#include <adc_imx6sx.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vf610_adc, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

struct vf610_adc_config {
	ADC_Type *base;
	uint8_t clock_source;
	uint8_t divide_ratio;
	void (*irq_config_func)(const struct device *dev);
};

struct vf610_adc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t channel_id;
};

static int vf610_adc_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id > (ADC_HC0_ADCH_MASK >> ADC_HC0_ADCH_SHIFT)) {
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

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct vf610_adc_config *config = dev->config;
	struct vf610_adc_data *data = dev->data;
	enum _adc_average_number mode;
	enum _adc_resolution_mode resolution;
	int error;
	ADC_Type *base = config->base;

	switch (sequence->resolution) {
	case 8:
		resolution = adcResolutionBit8;
		break;
	case 10:
		resolution = adcResolutionBit10;
		break;
	case 12:
		resolution = adcResolutionBit12;
		break;
	default:
		LOG_ERR("Invalid resolution");
		return -EINVAL;
	}

	ADC_SetResolutionMode(base, resolution);

	switch (sequence->oversampling) {
	case 0:
		mode = adcAvgNumNone;
		break;
	case 2:
		mode = adcAvgNum4;
		break;
	case 3:
		mode = adcAvgNum8;
		break;
	case 4:
		mode = adcAvgNum16;
		break;
	case 5:
		mode = adcAvgNum32;
		break;
	default:
		LOG_ERR("Invalid oversampling");
		return -EINVAL;
	}
	ADC_SetAverageNum(config->base, mode);

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	error = adc_context_wait_for_completion(&data->ctx);
	return error;
}

static int vf610_adc_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	struct vf610_adc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int vf610_adc_read_async(struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct vf610_adc_data *data = dev->driver_data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static void vf610_adc_start_channel(const struct device *dev)
{
	const struct vf610_adc_config *config = dev->config;
	struct vf610_adc_data *data = dev->data;

	data->channel_id = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d", data->channel_id);

	ADC_SetIntCmd(config->base, true);

	ADC_TriggerSingleConvert(config->base, data->channel_id);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct vf610_adc_data *data =
		CONTAINER_OF(ctx, struct vf610_adc_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	vf610_adc_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct vf610_adc_data *data =
		CONTAINER_OF(ctx, struct vf610_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void vf610_adc_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct vf610_adc_config *config = dev->config;
	struct vf610_adc_data *data = dev->data;
	ADC_Type *base = config->base;
	uint16_t result;

	result = ADC_GetConvertResult(base);
	LOG_DBG("Finished channel %d. Result is 0x%04x",
		data->channel_id, result);

	*data->buffer++ = result;
	data->channels &= ~BIT(data->channel_id);

	if (data->channels) {
		vf610_adc_start_channel(dev);
	} else {
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int vf610_adc_init(const struct device *dev)
{
	const struct vf610_adc_config *config = dev->config;
	struct vf610_adc_data *data = dev->data;
	ADC_Type *base = config->base;
	adc_init_config_t adc_config;

	adc_config.averageNumber = adcAvgNumNone;
	adc_config.resolutionMode = adcResolutionBit12;
	adc_config.clockSource = config->clock_source;
	adc_config.divideRatio = config->divide_ratio;

	ADC_Init(base, &adc_config);

	ADC_SetConvertTrigMode(base, adcSoftwareTrigger);

	ADC_SetCalibration(base, true);

	config->irq_config_func(dev);
	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, vf610_adc_driver_api) = {
	.channel_setup = vf610_adc_channel_setup,
	.read = vf610_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = vf610_adc_read_async,
#endif
};

#define VF610_ADC_INIT(n)						\
	static void vf610_adc_config_func_##n(const struct device *dev);\
									\
	static const struct vf610_adc_config vf610_adc_config_##n = {	\
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),		\
		.clock_source = DT_INST_PROP(n, clk_source),		\
		.divide_ratio = DT_INST_PROP(n, clk_divider),		\
		.irq_config_func = vf610_adc_config_func_##n,		\
	};								\
									\
	static struct vf610_adc_data vf610_adc_data_##n = {		\
		ADC_CONTEXT_INIT_TIMER(vf610_adc_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(vf610_adc_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(vf610_adc_data_##n, ctx),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &vf610_adc_init,			\
			      NULL, &vf610_adc_data_##n,		\
			      &vf610_adc_config_##n, POST_KERNEL,	\
			      CONFIG_ADC_INIT_PRIORITY,			\
			      &vf610_adc_driver_api);			\
									\
	static void vf610_adc_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    vf610_adc_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(VF610_ADC_INIT)
