/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_gau_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/irq.h>
#include <errno.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_mcux_gau_adc, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <fsl_adc.h>

#define NUM_ADC_CHANNELS 16

struct mcux_gau_adc_config {
	ADC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	adc_clock_divider_t clock_div;
	adc_analog_portion_power_mode_t power_mode;
	bool input_gain_buffer;
	adc_calibration_ref_t cal_volt;
};

struct mcux_gau_adc_data {
	const struct device *dev;
	struct adc_context ctx;
	adc_channel_source_t channel_sources[NUM_ADC_CHANNELS];
	uint8_t scan_length;
	uint16_t *results;
	size_t results_length;
	uint16_t *repeat;
	struct k_work read_samples_work;
};

static int mcux_gau_adc_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	const struct mcux_gau_adc_config *config = dev->config;
	struct mcux_gau_adc_data *data = dev->data;
	ADC_Type *base = config->base;
	uint8_t channel_id = channel_cfg->channel_id;
	uint8_t source_channel = channel_cfg->input_positive;
	uint32_t tmp_reg;

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels not yet supported");
		return -ENOTSUP;
	}

	if (channel_id >= NUM_ADC_CHANNELS) {
		LOG_ERR("ADC does not support more than %d channels", NUM_ADC_CHANNELS);
		return -ENOTSUP;
	}

	if (source_channel > 12 && source_channel != 15) {
		LOG_ERR("Invalid source channel");
		return -EINVAL;
	}

	/* Set Acquisition/Warmup time */
	tmp_reg = base->ADC_REG_INTERVAL;
	base->ADC_REG_INTERVAL &= ~ADC_ADC_REG_INTERVAL_WARMUP_TIME_MASK;
	base->ADC_REG_INTERVAL &= ~ADC_ADC_REG_INTERVAL_BYPASS_WARMUP_MASK;
	if (channel_cfg->acquisition_time == 0) {
		base->ADC_REG_INTERVAL |= ADC_ADC_REG_INTERVAL_BYPASS_WARMUP_MASK;
	} else if (channel_cfg->acquisition_time <= 32) {
		base->ADC_REG_INTERVAL |=
			ADC_ADC_REG_INTERVAL_WARMUP_TIME(channel_cfg->acquisition_time - 1);
	} else {
		LOG_ERR("Invalid acquisition time requested of ADC");
		return -EINVAL;
	}
	/* If user changed the warmup time, warn  */
	if (base->ADC_REG_INTERVAL != tmp_reg) {
		LOG_WRN("Acquisition/Warmup time is global to entire ADC peripheral, "
		"i.e. channel_setup will override this property for all previous channels.");
	}

	/* Set Input Gain */
	tmp_reg = base->ADC_REG_ANA;
	base->ADC_REG_ANA &= ~ADC_ADC_REG_ANA_INBUF_GAIN_MASK;
	if (channel_cfg->gain == ADC_GAIN_1) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_INBUF_GAIN(kADC_InputGain1);
	} else if (channel_cfg->gain == ADC_GAIN_1_2) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_INBUF_GAIN(kADC_InputGain0P5);
	} else if (channel_cfg->gain == ADC_GAIN_2) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_INBUF_GAIN(kADC_InputGain2);
	} else {
		LOG_ERR("Invalid gain");
		return -EINVAL;
	}
	/* If user changed the gain, warn */
	if (base->ADC_REG_ANA != tmp_reg) {
		LOG_WRN("Input gain is global to entire ADC peripheral, "
		"i.e. channel_setup will override this property for all previous channels.");
	}

	/* Set Reference voltage of ADC */
	tmp_reg = base->ADC_REG_ANA;
	base->ADC_REG_ANA &= ~ADC_ADC_REG_ANA_VREF_SEL_MASK;
	if (channel_cfg->reference == ADC_REF_INTERNAL) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_VREF_SEL(kADC_Vref1P2V);
	} else if (channel_cfg->reference == ADC_REF_EXTERNAL0) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_VREF_SEL(kADC_VrefExternal);
	} else if (channel_cfg->reference == ADC_REF_VDD_1) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_VREF_SEL(kADC_Vref1P8V);
	} else {
		LOG_ERR("Vref not supported");
		return -ENOTSUP;
	}
	/* if user changed the reference voltage, warn */
	if (base->ADC_REG_ANA != tmp_reg) {
		LOG_WRN("Reference voltage is global to entire ADC peripheral, "
		"i.e. channel_setup will override this property for all previous channels.");
	}

	data->channel_sources[channel_id] = source_channel;

	return 0;
}

static void mcux_gau_adc_read_samples(struct k_work *work)
{
	struct mcux_gau_adc_data *data =
				CONTAINER_OF(work, struct mcux_gau_adc_data,
						read_samples_work);
	const struct device *dev = data->dev;
	const struct mcux_gau_adc_config *config = dev->config;
	ADC_Type *base = config->base;

	/* using this variable to prevent buffer overflow */
	size_t length = data->results_length;

	while ((ADC_GetFifoDataCount(base) > 0) && (--length > 0)) {
		*(data->results++) = (uint16_t)ADC_GetConversionResult(base);
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}


static void mcux_gau_adc_isr(const struct device *dev)
{
	const struct mcux_gau_adc_config *config = dev->config;
	struct mcux_gau_adc_data *data = dev->data;
	ADC_Type *base = config->base;

	if (ADC_GetStatusFlags(base) & kADC_DataReadyInterruptFlag) {
		/* Clear flag to avoid infinite interrupt */
		ADC_ClearStatusFlags(base, kADC_DataReadyInterruptFlag);

		/* offload and do not block during irq */
		k_work_submit(&data->read_samples_work);
	} else {
		LOG_ERR("ADC received unimplemented interrupt");
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcux_gau_adc_data *data =
		CONTAINER_OF(ctx, struct mcux_gau_adc_data, ctx);
	const struct mcux_gau_adc_config *config = data->dev->config;
	ADC_Type *base = config->base;

	ADC_StopConversion(base);
	ADC_DoSoftwareTrigger(base);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct mcux_gau_adc_data *data =
		CONTAINER_OF(ctx, struct mcux_gau_adc_data, ctx);

	if (repeat_sampling) {
		data->results = data->repeat;
	}
}

static int mcux_gau_adc_do_read(const struct device *dev,
		   const struct adc_sequence *sequence)
{
	const struct mcux_gau_adc_config *config = dev->config;
	ADC_Type *base = config->base;
	struct mcux_gau_adc_data *data = dev->data;
	uint8_t num_channels = 0;

	/* if user selected channel >= NUM_ADC_CHANNELS that is invalid */
	if (sequence->channels & (0xFFFF << NUM_ADC_CHANNELS)) {
		LOG_ERR("Invalid channels selected for sequence");
		return -EINVAL;
	}

	/* Count channels */
	for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
		num_channels += ((sequence->channels & (0x1 << i)) ? 1 : 0);
	}

	/* Buffer must hold (number of samples per channel) * (number of channels) samples */
	if ((sequence->options != NULL && sequence->buffer_size <
	    ((1 + sequence->options->extra_samplings) * num_channels)) ||
	    (sequence->options == NULL && sequence->buffer_size < num_channels)) {
		LOG_ERR("Buffer size too small");
		return -ENOMEM;
	}

	/* Set scan length in data struct for isr to understand & set scan length register */
	base->ADC_REG_CONFIG &= ~ADC_ADC_REG_CONFIG_SCAN_LENGTH_MASK;
	data->scan_length = num_channels;
	/* Register Value is 1 less than what it represents */
	base->ADC_REG_CONFIG |= ADC_ADC_REG_CONFIG_SCAN_LENGTH(data->scan_length - 1);

	/* Set up scan channels */
	for (int channel = 0; channel < NUM_ADC_CHANNELS; channel++) {
		if (sequence->channels & (0x1 << channel)) {
			ADC_SetScanChannel(base,
				data->scan_length - num_channels--,
				data->channel_sources[channel]);
		}
	}

	/* Set resolution of ADC */
	base->ADC_REG_ANA &= ~ADC_ADC_REG_ANA_RES_SEL_MASK;
	/* odd numbers are for differential channels */
	if (sequence->resolution == 12 || sequence->resolution == 11) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_RES_SEL(kADC_Resolution12Bit);
	} else if (sequence->resolution == 14 || sequence->resolution == 13) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_RES_SEL(kADC_Resolution14Bit);
	} else if (sequence->resolution == 16 || sequence->resolution == 15) {
		base->ADC_REG_ANA |= ADC_ADC_REG_ANA_RES_SEL(kADC_Resolution16Bit);
	} else {
		LOG_ERR("Invalid resolution");
		return -EINVAL;
	}

	/* Set oversampling */
	base->ADC_REG_CONFIG &= ~ADC_ADC_REG_CONFIG_AVG_SEL_MASK;
	if (sequence->oversampling == 0) {
		base->ADC_REG_CONFIG |= ADC_ADC_REG_CONFIG_AVG_SEL(kADC_AverageNone);
	} else if (sequence->oversampling == 1) {
		base->ADC_REG_CONFIG |= ADC_ADC_REG_CONFIG_AVG_SEL(kADC_Average2);
	} else if (sequence->oversampling == 2) {
		base->ADC_REG_CONFIG |= ADC_ADC_REG_CONFIG_AVG_SEL(kADC_Average4);
	} else if (sequence->oversampling == 3) {
		base->ADC_REG_CONFIG |= ADC_ADC_REG_CONFIG_AVG_SEL(kADC_Average8);
	} else if (sequence->oversampling == 4) {
		base->ADC_REG_CONFIG |= ADC_ADC_REG_CONFIG_AVG_SEL(kADC_Average16);
	} else {
		LOG_ERR("Invalid oversampling setting");
		return -EINVAL;
	}

	/* Calibrate if requested */
	if (sequence->calibrate) {
		if (ADC_DoAutoCalibration(base, config->cal_volt)) {
			LOG_WRN("Calibration of ADC failed!");
		}
	}

	data->results = sequence->buffer;
	data->results_length = sequence->buffer_size;
	data->repeat = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int mcux_gau_adc_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	struct mcux_gau_adc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = mcux_gau_adc_do_read(dev, sequence);
	adc_context_release(&data->ctx, error);
	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int mcux_gau_adc_read_async(const struct device *dev,
				   const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct mcux_gau_adc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = mcux_gau_adc_do_read(dev, sequence);
	adc_context_release(&data->ctx, error);
	return error;
}
#endif


static int mcux_gau_adc_init(const struct device *dev)
{
	const struct mcux_gau_adc_config *config = dev->config;
	struct mcux_gau_adc_data *data = dev->data;
	ADC_Type *base = config->base;
	adc_config_t adc_config;

	data->dev = dev;

	LOG_DBG("Initializing ADC");

	ADC_GetDefaultConfig(&adc_config);

	/* DT configs */
	adc_config.clockDivider = config->clock_div;
	adc_config.powerMode = config->power_mode;
	adc_config.enableInputGainBuffer = config->input_gain_buffer;
	adc_config.triggerSource = kADC_TriggerSourceSoftware;

	adc_config.inputMode = kADC_InputSingleEnded;
	/* One shot meets the needs of the current zephyr adc context/api */
	adc_config.conversionMode = kADC_ConversionOneShot;
	/* since using one shot mode, just interrupt on one sample (agnostic to # channels) */
	adc_config.fifoThreshold = kADC_FifoThresholdData1;
	/* 32 bit width not supported in this driver; zephyr seems to use 16 bit */
	adc_config.resultWidth = kADC_ResultWidth16;
	adc_config.enableDMA = false;
	adc_config.enableADC = true;

	ADC_Init(base, &adc_config);

	if (ADC_DoAutoCalibration(base, config->cal_volt)) {
		LOG_WRN("Calibration of ADC failed!");
	}

	ADC_ClearStatusFlags(base, kADC_DataReadyInterruptFlag);

	config->irq_config_func(dev);
	ADC_EnableInterrupts(base, kADC_DataReadyInterruptEnable);

	k_work_init(&data->read_samples_work, &mcux_gau_adc_read_samples);

	adc_context_init(&data->ctx);
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api mcux_gau_adc_driver_api = {
	.channel_setup = mcux_gau_adc_channel_setup,
	.read = mcux_gau_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcux_gau_adc_read_async,
#endif
	.ref_internal = 1200,
};


#define GAU_ADC_MCUX_INIT(n)							\
										\
	static void mcux_gau_adc_config_func_##n(const struct device *dev);     \
										\
	static const struct mcux_gau_adc_config mcux_gau_adc_config_##n = {	\
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),			\
		.irq_config_func = mcux_gau_adc_config_func_##n,		\
		/* Minus one because DT starts at 1, HAL enum starts at 0 */	\
		.clock_div = DT_INST_PROP(n, nxp_clock_divider) - 1,		\
		.power_mode = DT_INST_ENUM_IDX(n, nxp_power_mode),		\
		.input_gain_buffer = DT_INST_PROP(n, nxp_input_buffer),		\
		.cal_volt = DT_INST_ENUM_IDX(n, nxp_calibration_voltage),	\
	};									\
										\
	static struct mcux_gau_adc_data mcux_gau_adc_data_##n = {0};		\
										\
	DEVICE_DT_INST_DEFINE(n, &mcux_gau_adc_init, NULL,			\
			      &mcux_gau_adc_data_##n, &mcux_gau_adc_config_##n,	\
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,		\
			      &mcux_gau_adc_driver_api);			\
										\
	static void mcux_gau_adc_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
				mcux_gau_adc_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(GAU_ADC_MCUX_INIT)
