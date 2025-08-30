/*
 * Copyright 2025 NXP
 *
 * Based on adc_mcux_sar_adc.c, which is:
 * Copyright 2023-2024 NXP
 * Copyright (c) 2020 Toby Firth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sar_adc

#include <errno.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <fsl_sar_adc.h>
LOG_MODULE_REGISTER(adc_mcux_sar_adc);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

struct mcux_sar_adc_config {
	ADC_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_sar_adc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
};

static int mcux_sar_adc_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	/* User may configure maximum number of active channels */
	if (channel_cfg->channel_id >= CONFIG_SAR_ADC_CHANNEL_COUNT) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference");
		return -ENOTSUP;
	}

	return 0;
}

static int mcux_sar_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct mcux_sar_adc_config *config = dev->config;
	struct mcux_sar_adc_data *data = dev->data;
	ADC_Type *base = config->base;
	uint8_t channel_id;

	if (sequence->resolution != 12) {
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	channel_id = CONFIG_SAR_ADC_CHANNEL_COUNT;
	while (channel_id-- > 0) {
		if (sequence->channels & BIT(channel_id)) {
			ADC_EnableSpecificChannelNormalConv(base, channel_id);
		} else {
			ADC_DisableSpecificChannelNormalConv(base, channel_id);
		}
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);
	int error = adc_context_wait_for_completion(&data->ctx);

	return error;
}

static int mcux_sar_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct mcux_sar_adc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = mcux_sar_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int mcux_sar_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return mcux_sar_adc_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcux_sar_adc_data *data = CONTAINER_OF(ctx, struct mcux_sar_adc_data, ctx);
	const struct mcux_sar_adc_config *config = data->dev->config;
	ADC_Type *base = config->base;

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	ADC_StartConvChain(base, kADC_NormalConvOneShotMode);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct mcux_sar_adc_data *data = CONTAINER_OF(ctx, struct mcux_sar_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void mcux_sar_adc_isr(const struct device *dev)
{
	const struct mcux_sar_adc_config *config = dev->config;
	struct mcux_sar_adc_data *data = dev->data;
	ADC_Type *base = config->base;
	adc_conv_result_t conv_result;
	uint16_t channel_id;

	if (((ADC_GetConvIntStatus(base) & kADC_NormalConvChainEndIntFlag))) {
		ADC_ClearConvIntStatus(base, kADC_NormalConvChainEndIntFlag);
	}

	for (channel_id = 0; channel_id < CONFIG_SAR_ADC_CHANNEL_COUNT; channel_id++) {
		if (ADC_GetChannelConvResult(base, &conv_result, channel_id)) {
			data->channels &= ~BIT(channel_id);
			*(data->buffer++) = conv_result.convData;
			if (data->channels == 0) {
				adc_context_on_sampling_done(&data->ctx, dev);
			}
		}
	}
}

static int mcux_sar_adc_init(const struct device *dev)
{
	const struct mcux_sar_adc_config *config = dev->config;
	struct mcux_sar_adc_data *data = dev->data;
	ADC_Type *base = config->base;
	adc_config_t adc_config;
	adc_calibration_config_t calibrationConfig;

	ADC_GetDefaultConfig(&adc_config);
	ADC_Init(base, &adc_config);
	ADC_SetConvMode(base, kADC_NormalConvOneShotMode);
	ADC_EnableConvInt(base, (uint32_t)kADC_NormalConvChainEndIntEnable);

	/* Do calibration to reduce or eliminate the various error contribution effects. */
	calibrationConfig.enableAverage = true;
	calibrationConfig.sampleTime = kADC_SampleTime22;
#if (defined(FSL_FEATURE_ADC_HAS_CALBISTREG) && (FSL_FEATURE_ADC_HAS_CALBISTREG == 1U))
	calibrationConfig.averageSampleNumbers = kADC_AverageSampleNumbers32;
#else
	calibrationConfig.averageSampleNumbers = kADC_AverageSampleNumbers512;
#endif /* FSL_FEATURE_ADC_HAS_CALBISTREG */

	if (!(ADC_DoCalibration(base, &calibrationConfig))) {
		LOG_WRN("Calibration failed.");
	}

	config->irq_config_func(dev);
	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, mcux_sar_adc_driver_api) = {
	.channel_setup = mcux_sar_adc_channel_setup,
	.read = mcux_sar_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcux_sar_adc_read_async,
#endif
};

#define SAR_ADC_MCUX_INIT(n)                                                                       \
                                                                                                   \
	static void mcux_sar_adc_config_func_##n(const struct device *dev);                        \
                                                                                                   \
	static const struct mcux_sar_adc_config mcux_sar_adc_config_##n = {                        \
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),                                           \
		.irq_config_func = mcux_sar_adc_config_func_##n,                                   \
	};                                                                                         \
	static struct mcux_sar_adc_data mcux_sar_adc_data_##n = {                                  \
		ADC_CONTEXT_INIT_TIMER(mcux_sar_adc_data_##n, ctx),                                \
		ADC_CONTEXT_INIT_LOCK(mcux_sar_adc_data_##n, ctx),                                 \
		ADC_CONTEXT_INIT_SYNC(mcux_sar_adc_data_##n, ctx),                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &mcux_sar_adc_init, NULL, &mcux_sar_adc_data_##n,                 \
			      &mcux_sar_adc_config_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,     \
			      &mcux_sar_adc_driver_api);                                           \
                                                                                                   \
	static void mcux_sar_adc_config_func_##n(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_sar_adc_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(SAR_ADC_MCUX_INIT)
