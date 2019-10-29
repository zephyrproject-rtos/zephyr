/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Based on adc_mcux_adc16.c, which is:
 * Copyright (c) 2017-2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/adc.h>
#include <fsl_adc12.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_mcux_adc12);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

struct mcux_adc12_config {
	ADC_Type *base;
	adc12_clock_source_t clock_src;
	adc12_clock_divider_t clock_div;
	adc12_reference_voltage_source_t ref_src;
	uint32_t sample_clk_count;
	void (*irq_config_func)(struct device *dev);
};

struct mcux_adc12_data {
	struct device *dev;
	struct adc_context ctx;
	u16_t *buffer;
	u16_t *repeat_buffer;
	u32_t channels;
	u8_t channel_id;
};

static int mcux_adc12_channel_setup(struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	u8_t channel_id = channel_cfg->channel_id;

	if (channel_id > (ADC_SC1_ADCH_MASK >> ADC_SC1_ADCH_SHIFT)) {
		LOG_ERR("Invalid channel %d", channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
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

static int mcux_adc12_start_read(struct device *dev,
				 const struct adc_sequence *sequence)
{
	const struct mcux_adc12_config *config = dev->config->config_info;
	struct mcux_adc12_data *data = dev->driver_data;
	adc12_hardware_average_mode_t mode;
	adc12_resolution_t resolution;
	ADC_Type *base = config->base;
	int error;
	u32_t tmp32;

	switch (sequence->resolution) {
	case 8:
		resolution = kADC12_Resolution8Bit;
		break;
	case 10:
		resolution = kADC12_Resolution10Bit;
		break;
	case 12:
		resolution = kADC12_Resolution12Bit;
		break;
	default:
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	tmp32 = base->CFG1 & ~(ADC_CFG1_MODE_MASK);
	tmp32 |= ADC_CFG1_MODE(resolution);
	base->CFG1 = tmp32;

	switch (sequence->oversampling) {
	case 0:
		mode = kADC12_HardwareAverageDisabled;
		break;
	case 2:
		mode = kADC12_HardwareAverageCount4;
		break;
	case 3:
		mode = kADC12_HardwareAverageCount8;
		break;
	case 4:
		mode = kADC12_HardwareAverageCount16;
		break;
	case 5:
		mode = kADC12_HardwareAverageCount32;
		break;
	default:
		LOG_ERR("Unsupported oversampling value %d",
			sequence->oversampling);
		return -ENOTSUP;
	}
	ADC12_SetHardwareAverage(config->base, mode);

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);
	error = adc_context_wait_for_completion(&data->ctx);

	return error;
}

static int mcux_adc12_read_async(struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct mcux_adc12_data *data = dev->driver_data;
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = mcux_adc12_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int mcux_adc12_read(struct device *dev,
			   const struct adc_sequence *sequence)
{
	return mcux_adc12_read_async(dev, sequence, NULL);
}

static void mcux_adc12_start_channel(struct device *dev)
{
	const struct mcux_adc12_config *config = dev->config->config_info;
	struct mcux_adc12_data *data = dev->driver_data;

	adc12_channel_config_t channel_config;
	u32_t channel_group = 0U;

	data->channel_id = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d", data->channel_id);
	channel_config.enableInterruptOnConversionCompleted = true;
	channel_config.channelNumber = data->channel_id;
	ADC12_SetChannelConfig(config->base, channel_group, &channel_config);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcux_adc12_data *data =
		CONTAINER_OF(ctx, struct mcux_adc12_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	mcux_adc12_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct mcux_adc12_data *data =
		CONTAINER_OF(ctx, struct mcux_adc12_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void mcux_adc12_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct mcux_adc12_config *config = dev->config->config_info;
	struct mcux_adc12_data *data = dev->driver_data;
	ADC_Type *base = config->base;
	u32_t channel_group = 0U;
	u16_t result;

	result = ADC12_GetChannelConversionValue(base, channel_group);
	LOG_DBG("Finished channel %d. Result is 0x%04x",
		data->channel_id, result);

	*data->buffer++ = result;
	data->channels &= ~BIT(data->channel_id);

	if (data->channels) {
		mcux_adc12_start_channel(dev);
	} else {
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int mcux_adc12_init(struct device *dev)
{
	const struct mcux_adc12_config *config = dev->config->config_info;
	struct mcux_adc12_data *data = dev->driver_data;
	ADC_Type *base = config->base;
	adc12_config_t adc_config;

	ADC12_GetDefaultConfig(&adc_config);

	adc_config.referenceVoltageSource = config->ref_src;
	adc_config.clockSource = config->clock_src;
	adc_config.clockDivider = config->clock_div;
	adc_config.sampleClockCount = config->sample_clk_count;
	adc_config.resolution = kADC12_Resolution12Bit;
	adc_config.enableContinuousConversion = false;

	ADC12_Init(base, &adc_config);
	ADC12_DoAutoCalibration(base);
	ADC12_EnableHardwareTrigger(base, false);

	config->irq_config_func(dev);
	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api mcux_adc12_driver_api = {
	.channel_setup = mcux_adc12_channel_setup,
	.read = mcux_adc12_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcux_adc12_read_async,
#endif
};

#define ASSERT_WITHIN_RANGE(val, min, max, str) \
	BUILD_ASSERT_MSG(val >= min && val <= max, str)
#define ASSERT_ADC12_CLK_DIV_VALID(val, str) \
	BUILD_ASSERT_MSG(val == 1 || val == 2 || val == 4 || val == 8, str)
#define TO_ADC12_CLOCK_SRC(val) _DO_CONCAT(kADC12_ClockSourceAlt, val)
#define TO_ADC12_CLOCK_DIV(val) _DO_CONCAT(kADC12_ClockDivider, val)

#if DT_INST_0_NXP_KINETIS_ADC12
static void mcux_adc12_config_func_0(struct device *dev);

ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_ADC12_CLK_SOURCE, 0, 3,
		    "Invalid clock source");
ASSERT_ADC12_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_ADC12_CLK_DIVIDER,
			   "Invalid clock divider");
ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_ADC12_SAMPLE_TIME, 2, 256,
		    "Invalid sample time");
static const struct mcux_adc12_config mcux_adc12_config_0 = {
	.base = (ADC_Type *)DT_INST_0_NXP_KINETIS_ADC12_BASE_ADDRESS,
	.clock_src = TO_ADC12_CLOCK_SRC(DT_INST_0_NXP_KINETIS_ADC12_CLK_SOURCE),
	.clock_div =
		TO_ADC12_CLOCK_DIV(DT_INST_0_NXP_KINETIS_ADC12_CLK_DIVIDER),
#if DT_INST_0_NXP_KINETIS_ADC12_ALTERNATE_VOLTAGE_REFERENCE == 1
	.ref_src = kADC12_ReferenceVoltageSourceValt,
#else
	.ref_src = kADC12_ReferenceVoltageSourceVref,
#endif
	.sample_clk_count = DT_INST_0_NXP_KINETIS_ADC12_SAMPLE_TIME,
	.irq_config_func = mcux_adc12_config_func_0,
};

static struct mcux_adc12_data mcux_adc12_data_0 = {
	ADC_CONTEXT_INIT_TIMER(mcux_adc12_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(mcux_adc12_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(mcux_adc12_data_0, ctx),
};

DEVICE_AND_API_INIT(mcux_adc12_0, DT_INST_0_NXP_KINETIS_ADC12_LABEL,
		    &mcux_adc12_init, &mcux_adc12_data_0, &mcux_adc12_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_adc12_driver_api);

static void mcux_adc12_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_INST_0_NXP_KINETIS_ADC12_IRQ_0,
		    DT_INST_0_NXP_KINETIS_ADC12_IRQ_0_PRIORITY, mcux_adc12_isr,
		    DEVICE_GET(mcux_adc12_0), 0);

	irq_enable(DT_INST_0_NXP_KINETIS_ADC12_IRQ_0);
}
#endif /* DT_INST_0_NXP_KINETIS_ADC12 */

#if DT_INST_1_NXP_KINETIS_ADC12
static void mcux_adc12_config_func_1(struct device *dev);

ASSERT_WITHIN_RANGE(DT_INST_1_NXP_KINETIS_ADC12_CLK_SOURCE, 0, 3,
		    "Invalid clock source");
ASSERT_ADC12_CLK_DIV_VALID(DT_INST_1_NXP_KINETIS_ADC12_CLK_DIVIDER,
			   "Invalid clock divider");
ASSERT_WITHIN_RANGE(DT_INST_1_NXP_KINETIS_ADC12_SAMPLE_TIME, 2, 256,
		    "Invalid sample time");
static const struct mcux_adc12_config mcux_adc12_config_1 = {
	.base = (ADC_Type *)DT_INST_1_NXP_KINETIS_ADC12_BASE_ADDRESS,
	.clock_src = TO_ADC12_CLOCK_SRC(DT_INST_1_NXP_KINETIS_ADC12_CLK_SOURCE),
	.clock_div =
		TO_ADC12_CLOCK_DIV(DT_INST_1_NXP_KINETIS_ADC12_CLK_DIVIDER),
#if DT_INST_1_NXP_KINETIS_ADC12_ALTERNATE_VOLTAGE_REFERENCE == 1
	.ref_src = kADC12_ReferenceVoltageSourceValt,
#else
	.ref_src = kADC12_ReferenceVoltageSourceVref,
#endif
	.sample_clk_count = DT_INST_1_NXP_KINETIS_ADC12_SAMPLE_TIME,
	.irq_config_func = mcux_adc12_config_func_1,
};

static struct mcux_adc12_data mcux_adc12_data_1 = {
	ADC_CONTEXT_INIT_TIMER(mcux_adc12_data_1, ctx),
	ADC_CONTEXT_INIT_LOCK(mcux_adc12_data_1, ctx),
	ADC_CONTEXT_INIT_SYNC(mcux_adc12_data_1, ctx),
};

DEVICE_AND_API_INIT(mcux_adc12_1, DT_INST_1_NXP_KINETIS_ADC12_LABEL,
		    &mcux_adc12_init, &mcux_adc12_data_1, &mcux_adc12_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_adc12_driver_api);

static void mcux_adc12_config_func_1(struct device *dev)
{
	IRQ_CONNECT(DT_INST_1_NXP_KINETIS_ADC12_IRQ_0,
		    DT_INST_1_NXP_KINETIS_ADC12_IRQ_0_PRIORITY, mcux_adc12_isr,
		    DEVICE_GET(mcux_adc12_1), 0);

	irq_enable(DT_INST_1_NXP_KINETIS_ADC12_IRQ_0);
}
#endif /* DT_INST_1_NXP_KINETIS_ADC12 */

#if DT_INST_2_NXP_KINETIS_ADC12
static void mcux_adc12_config_func_2(struct device *dev);

ASSERT_WITHIN_RANGE(DT_INST_2_NXP_KINETIS_ADC12_ADC_CLK_SOURCE, 0, 3,
		    "Invalid clock source");
ASSERT_ADC12_CLK_DIV_VALID(DT_INST_2_NXP_KINETIS_ADC12_ADC_CLK_DIVIDER,
			   "Invalid clock divider");
ASSERT_WITHIN_RANGE(DT_INST_2_NXP_KINETIS_ADC12_ADC_SAMPLE_TIME, 2, 256,
		    "Invalid sample time");
static const struct mcux_adc12_config mcux_adc12_config_2 = {
	.base = (ADC_Type *)DT_INST_2_NXP_KINETIS_ADC12_ADC_BASE_ADDRESS,
	.clock_src = TO_ADC12_CLOCK_SRC(DT_INST_2_NXP_KINETIS_ADC12_ADC_CLK_SOURCE),
	.clock_div = TO_ADC12_CLOCK_DIV(DT_INST_2_NXP_KINETIS_ADC12_ADC_CLK_DIVIDER),
#if DT_INST_2_NXP_KINETIS_ADC12_ADC_ALTERNATE_VOLTAGE_REFERENCE == 1
	.ref_src = kADC12_ReferenceVoltageSourceValt,
#else
	.ref_src = kADC12_ReferenceVoltageSourceVref,
#endif
	.sample_clk_count = DT_INST_2_NXP_KINETIS_ADC12_ADC_SAMPLE_TIME,
	.irq_config_func = mcux_adc12_config_func_2,
};

static struct mcux_adc12_data mcux_adc12_data_2 = {
	ADC_CONTEXT_INIT_TIMER(mcux_adc12_data_2, ctx),
	ADC_CONTEXT_INIT_LOCK(mcux_adc12_data_2, ctx),
	ADC_CONTEXT_INIT_SYNC(mcux_adc12_data_2, ctx),
};

DEVICE_AND_API_INIT(mcux_adc12_2, DT_INST_2_NXP_KINETIS_ADC12_ADC_LABEL,
		    &mcux_adc12_init, &mcux_adc12_data_2, &mcux_adc12_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_adc12_driver_api);

static void mcux_adc12_config_func_2(struct device *dev)
{
	IRQ_CONNECT(DT_INST_2_NXP_KINETIS_ADC12_ADC_IRQ,
		    DT_INST_2_NXP_KINETIS_ADC12_ADC_IRQ_PRIORITY, mcux_adc12_isr,
		    DEVICE_GET(mcux_adc12_2), 0);

	irq_enable(DT_INST_2_NXP_KINETIS_ADC12_ADC_IRQ);
}
#endif /* DT_INST_2_NXP_KINETIS_ADC12 */
