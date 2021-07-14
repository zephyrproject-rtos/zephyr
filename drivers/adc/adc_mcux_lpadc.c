/*
 * Copyright (c) 2020 Toby Firth
 *
 * Based on adc_mcux_adc16.c and adc_mcux_adc12.c, which are:
 * Copyright (c) 2017-2018, NXP
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_lpadc

#include <errno.h>
#include <drivers/adc.h>
#include <fsl_lpadc.h>

#if !defined(CONFIG_SOC_SERIES_IMX_RT11XX)
#include <fsl_power.h>
#endif

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(nxp_mcux_lpadc);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"


struct mcux_lpadc_config {
	ADC_Type *base;
	uint32_t clock_div;
	uint32_t clock_source;
	lpadc_reference_voltage_source_t voltage_ref;
#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS)\
	&& FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS
	lpadc_conversion_average_mode_t calibration_average;
#else
	uint32_t calibration_average;
#endif /* FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS */
	lpadc_power_level_mode_t power_level;
	uint32_t offset_a;
	uint32_t offset_b;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_lpadc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t channel_id;
	lpadc_hardware_average_mode_t average;
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_MODE) \
	&& FSL_FEATURE_LPADC_HAS_CMDL_MODE
	lpadc_conversion_resolution_mode_t resolution;
#endif /* FSL_FEATURE_LPADC_HAS_CMDL_MODE */
};

static int mcux_lpadc_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id > 31) {
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

	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	return 0;
}

static int mcux_lpadc_start_read(const struct device *dev,
		 const struct adc_sequence *sequence)
{
	struct mcux_lpadc_data *data = dev->data;
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_MODE) \
	&& FSL_FEATURE_LPADC_HAS_CMDL_MODE
	switch (sequence->resolution) {
	case 12:
	case 13:
		data->resolution = kLPADC_ConversionResolutionStandard;
		break;
	case 16:
		data->resolution = kLPADC_ConversionResolutionHigh;
		break;
	default:
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}
#else
	/* If FSL_FEATURE_LPADC_HAS_CMDL_MODE is not defined
	   only 12/13 bit resolution is supported. */
	if (sequence->resolution != 12 && sequence->resolution != 13) {
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}
#endif /* FSL_FEATURE_LPADC_HAS_CMDL_MODE */

	switch (sequence->oversampling) {
	case 0:
		data->average = kLPADC_HardwareAverageCount1;
		break;
	case 1:
		data->average = kLPADC_HardwareAverageCount2;
		break;
	case 2:
		data->average = kLPADC_HardwareAverageCount4;
		break;
	case 3:
		data->average = kLPADC_HardwareAverageCount8;
		break;
	case 4:
		data->average = kLPADC_HardwareAverageCount16;
		break;
	case 5:
		data->average = kLPADC_HardwareAverageCount32;
		break;
	case 6:
		data->average = kLPADC_HardwareAverageCount64;
		break;
	case 7:
		data->average = kLPADC_HardwareAverageCount128;
		break;
	default:
		LOG_ERR("Unsupported oversampling value %d",
			sequence->oversampling);
		return -ENOTSUP;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);
	int error = adc_context_wait_for_completion(&data->ctx);

	return error;
}

static int mcux_lpadc_read_async(const struct device *dev,
			const struct adc_sequence *sequence,
			struct k_poll_signal *async)
{
	struct mcux_lpadc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = mcux_lpadc_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int mcux_lpadc_read(const struct device *dev,
		   const struct adc_sequence *sequence)
{
	return mcux_lpadc_read_async(dev, sequence, NULL);
}

static void mcux_lpadc_start_channel(const struct device *dev)
{
	const struct mcux_lpadc_config *config = dev->config;
	struct mcux_lpadc_data *data = dev->data;

	data->channel_id = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d", data->channel_id);

	lpadc_conv_command_config_t cmd_config;

	LPADC_GetDefaultConvCommandConfig(&cmd_config);
	cmd_config.channelNumber = data->channel_id;
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_MODE) \
	&& FSL_FEATURE_LPADC_HAS_CMDL_MODE
	cmd_config.conversionResolutionMode = data->resolution;
#endif /* FSL_FEATURE_LPADC_HAS_CMDL_MODE */
	cmd_config.hardwareAverageMode = data->average;
	LPADC_SetConvCommandConfig(config->base, 1, &cmd_config);

	lpadc_conv_trigger_config_t trigger_config;

	LPADC_GetDefaultConvTriggerConfig(&trigger_config);

	trigger_config.targetCommandId = 1;

	/* configures trigger0. */
	LPADC_SetConvTriggerConfig(config->base, 0, &trigger_config);

	/* 1 is trigger0 mask. */
	LPADC_DoSoftwareTrigger(config->base, 1);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcux_lpadc_data *data =
	CONTAINER_OF(ctx, struct mcux_lpadc_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	mcux_lpadc_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
			  bool repeat_sampling)
{
	struct mcux_lpadc_data *data =
		CONTAINER_OF(ctx, struct mcux_lpadc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void mcux_lpadc_isr(const struct device *dev)
{
	const struct mcux_lpadc_config *config = dev->config;
	struct mcux_lpadc_data *data = dev->data;
	ADC_Type *base = config->base;

	lpadc_conv_result_t conv_result;

#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) \
	&& (FSL_FEATURE_LPADC_FIFO_COUNT == 2U))
	LPADC_GetConvResult(base, &conv_result, 0U);
#else
	LPADC_GetConvResult(base, &conv_result);
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */

	/* For 12-bit resolution the MSB will be 0.
	   So a 3 bit shift is also needed. */
	uint16_t result = data->ctx.sequence.resolution < 16 ?
			conv_result.convValue >> 3 : conv_result.convValue;

	LOG_DBG("Finished channel %d. Result is 0x%04x",
		data->channel_id, result);

	*data->buffer++ = result;

	data->channels &= ~BIT(data->channel_id);

	if (data->channels) {
		mcux_lpadc_start_channel(dev);
	} else {
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int mcux_lpadc_init(const struct device *dev)
{
	const struct mcux_lpadc_config *config = dev->config;
	struct mcux_lpadc_data *data = dev->data;
	ADC_Type *base = config->base;
	lpadc_config_t adc_config;

#if !defined(CONFIG_SOC_SERIES_IMX_RT11XX)
	CLOCK_SetClkDiv(kCLOCK_DivAdcAsyncClk, config->clock_div, true);
	CLOCK_AttachClk(config->clock_source);

	/* Power up the ADC */
	POWER_DisablePD(kPDRUNCFG_PD_LDOGPADC);
#endif

	LPADC_GetDefaultConfig(&adc_config);

	adc_config.enableAnalogPreliminary = true;
	adc_config.referenceVoltageSource = config->voltage_ref;

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS) \
	&& FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS
	adc_config.conversionAverageMode = config->calibration_average;
#endif /* FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS */

	adc_config.powerLevelMode = config->power_level;

	LPADC_Init(base, &adc_config);

	/* Do ADC calibration. */
#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CALOFS) \
	&& FSL_FEATURE_LPADC_HAS_CTRL_CALOFS
#if defined(FSL_FEATURE_LPADC_HAS_OFSTRIM) \
	&& FSL_FEATURE_LPADC_HAS_OFSTRIM
	/* Request offset calibration. */
#if defined(CONFIG_LPADC_DO_OFFSET_CALIBRATION) \
	&& CONFIG_LPADC_DO_OFFSET_CALIBRATION
	LPADC_DoOffsetCalibration(base);
#else
	LPADC_SetOffsetValue(base,
			config->offset_a,
			config->offset_b);
#endif  /* DEMO_LPADC_DO_OFFSET_CALIBRATION */
#endif  /* FSL_FEATURE_LPADC_HAS_OFSTRIM */
	/* Request gain calibration. */
	LPADC_DoAutoCalibration(base);
#endif /* FSL_FEATURE_LPADC_HAS_CTRL_CALOFS */

#if (defined(FSL_FEATURE_LPADC_HAS_CFG_CALOFS) \
	&& FSL_FEATURE_LPADC_HAS_CFG_CALOFS)
	/* Do auto calibration. */
	LPADC_DoAutoCalibration(base);
#endif /* FSL_FEATURE_LPADC_HAS_CFG_CALOFS */

/* Enable the watermark interrupt. */
#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) \
	&& (FSL_FEATURE_LPADC_FIFO_COUNT == 2U))
	LPADC_EnableInterrupts(base, kLPADC_FIFO0WatermarkInterruptEnable);
#else
	LPADC_EnableInterrupts(base, kLPADC_FIFOWatermarkInterruptEnable);
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */

	config->irq_config_func(dev);
	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api mcux_lpadc_driver_api = {
	.channel_setup = mcux_lpadc_channel_setup,
	.read = mcux_lpadc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcux_lpadc_read_async,
#endif
};


#define ASSERT_LPADC_CLK_SOURCE_VALID(val, str)	\
	BUILD_ASSERT(val == 0 || val == 1 || val == 2 || val == 7, str)

#define ASSERT_LPADC_CLK_DIV_VALID(val, str) \
	BUILD_ASSERT(val == 1 || val == 2 || val == 4 || val == 8, str)

#define ASSERT_LPADC_CALIBRATION_AVERAGE_VALID(val, str) \
	BUILD_ASSERT(val == 1 || val == 2 || val == 4 || val == 8 \
		|| val == 16 || val == 32 || val == 64 || val == 128, str) \

#define ASSERT_WITHIN_RANGE(val, min, max, str)	\
	BUILD_ASSERT(val >= min && val <= max, str)

#if defined(CONFIG_SOC_SERIES_IMX_RT11XX)
#define TO_LPADC_CLOCK_SOURCE(val) 0
#else
#define TO_LPADC_CLOCK_SOURCE(val) \
	MUX_A(CM_ADCASYNCCLKSEL, val)
#endif

#define TO_LPADC_REFERENCE_VOLTAGE(val) \
	_DO_CONCAT(kLPADC_ReferenceVoltageAlt, val)

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS)\
	&& FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS
#define TO_LPADC_CALIBRATION_AVERAGE(val) \
	_DO_CONCAT(kLPADC_ConversionAverage, val)
#else
#define TO_LPADC_CALIBRATION_AVERAGE(val) 0
#endif

#define TO_LPADC_POWER_LEVEL(val) \
	_DO_CONCAT(kLPADC_PowerLevelAlt, val)

#define LPADC_MCUX_INIT(n)						\
	static void mcux_lpadc_config_func_##n(const struct device *dev);	\
									\
	ASSERT_LPADC_CLK_SOURCE_VALID(DT_INST_PROP(n, clk_source),	\
			  "Invalid clock source");			\
	ASSERT_LPADC_CLK_DIV_VALID(DT_INST_PROP(n, clk_divider),	\
		   "Invalid clock divider");			\
	ASSERT_WITHIN_RANGE(DT_INST_PROP(n, voltage_ref), 2, 3,	\
		"Invalid voltage reference source");	\
	ASSERT_LPADC_CALIBRATION_AVERAGE_VALID(		\
		DT_INST_PROP(n, calibration_average),	\
		"Invalid converion average number for auto-calibration time");	\
	ASSERT_WITHIN_RANGE(DT_INST_PROP(n, power_level), 1, 4,		\
		"Invalid power level");					\
	static const struct mcux_lpadc_config mcux_lpadc_config_##n = {	\
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),	\
		.clock_source = TO_LPADC_CLOCK_SOURCE(DT_INST_PROP(n, clk_source)),	\
		.clock_div = DT_INST_PROP(n, clk_divider),					\
		.voltage_ref =												\
			TO_LPADC_REFERENCE_VOLTAGE(DT_INST_PROP(n, voltage_ref)),	\
		.calibration_average =										\
			TO_LPADC_CALIBRATION_AVERAGE(DT_INST_PROP(n, calibration_average)),	\
		.power_level = TO_LPADC_POWER_LEVEL(DT_INST_PROP(n, power_level)),	\
		.offset_a = DT_INST_PROP(n, offset_value_a),	\
		.offset_a = DT_INST_PROP(n, offset_value_b),	\
		.irq_config_func = mcux_lpadc_config_func_##n,				\
	};									\
										\
	static struct mcux_lpadc_data mcux_lpadc_data_##n = {	\
		ADC_CONTEXT_INIT_TIMER(mcux_lpadc_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(mcux_lpadc_data_##n, ctx),	\
		ADC_CONTEXT_INIT_SYNC(mcux_lpadc_data_##n, ctx),	\
	};														\
										\
	DEVICE_DT_INST_DEFINE(n,						\
		&mcux_lpadc_init, NULL, &mcux_lpadc_data_##n,			\
		&mcux_lpadc_config_##n, POST_KERNEL,				\
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,					\
		&mcux_lpadc_driver_api);							\
										\
	static void mcux_lpadc_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			DT_INST_IRQ(n, priority), mcux_lpadc_isr,	\
			DEVICE_DT_INST_GET(n), 0);				\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(LPADC_MCUX_INIT)
