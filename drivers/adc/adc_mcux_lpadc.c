/*
 * Copyright 2023-2024 NXP
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
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/opamp.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <fsl_lpadc.h>
LOG_MODULE_REGISTER(nxp_mcux_lpadc);

/*
 * Currently, no instance of the ADC IP has more than
 * 8 channels present. Therefore, we treat channels
 * with an index 8 or higher as a side b channel, with
 * the channel index given by channel_num % 8
 */
#define CHANNELS_PER_SIDE 0x8

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

struct mcux_lpadc_config {
	ADC_Type *base;
	lpadc_reference_voltage_source_t voltage_ref;
	uint8_t power_level;
	uint32_t calibration_average;
	uint32_t offset_a;
	uint32_t offset_b;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	const struct device *ref_supplies;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	int32_t ref_supply_val;
	const struct device *opamp;
	uint8_t opamp_channel; /* ADC channel index that samples OPAMP output */
	uint32_t sample_max;   /* Upper bound of ideal sample range */
	uint32_t sample_min;   /* Lower bound of ideal sample range */
	/* Optional list of programmable OPAMP gain enum values from DT */
	const enum opamp_gain *opamp_gains;
	uint8_t opamp_gain_count;
	/* vVref in mV for the OPAMP output channel
	 * (from that channel node's zephyr,vref-mv)
	 */
	uint16_t opamp_vref_mv;
};

struct mcux_lpadc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	lpadc_conv_command_config_t cmd_config[CONFIG_LPADC_CHANNEL_COUNT];
	/* OPAMP gain control context */
	uint8_t current_gain_index;
	int desired_gain_index;
	/* Precomputed raw thresholds corresponding to
	 * config->ideal_sample_range (mV)
	 */
	uint16_t sample_min_raw;
	uint16_t sample_max_raw;
};

static int mcux_lpadc_acquisition_time_setup(const struct device *dev, uint16_t acq_time,
					     lpadc_conv_command_config_t *cmd)
{
	const struct mcux_lpadc_config *config = dev->config;
	uint32_t adc_freq_hz = 0;
	uint32_t conversion_factor = 0;
	uint32_t acquisition_time_value = ADC_ACQ_TIME_VALUE(acq_time);
	uint8_t acquisition_time_unit = ADC_ACQ_TIME_UNIT(acq_time);

	if (ADC_ACQ_TIME_DEFAULT == acquisition_time_value) {
		return 0;
	}

	/* If the acquisition time is expressed in ADC ticks, then directly compare
	 * the acquisition time with configuration items (3, 5, 7, etc. ADC ticks)
	 * supported by the LPADC. The conversion factor is set to 1 (means do not need
	 * to convert configuration items from ADC ticks to nanoseconds).
	 * If the acquisition time is expressed in microseconds or nanoseconds, First
	 * calculate the ADC cycle based on the ADC clock, then convert the configuration
	 * items supported by LPADC into nanoseconds, and finally compare the acquisition
	 * time with configuration items. The conversion factor is equal to the ADC cycle
	 * (means convert configuration items from ADC ticks to nanoseconds).
	 */
	if (ADC_ACQ_TIME_TICKS == acquisition_time_unit) {
		conversion_factor = 1;
	} else {
		if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &adc_freq_hz)) {
			LOG_ERR("Get clock rate failed");
			return -EINVAL;
		}

		conversion_factor = 1000000000 / adc_freq_hz;

		if (ADC_ACQ_TIME_MICROSECONDS == acquisition_time_unit) {
			acquisition_time_value *= 1000;
		}
	}

	if ((3 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK3;
	} else if ((5 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK5;
	} else if ((7 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK7;
	} else if ((11 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK11;
	} else if ((19 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK19;
	} else if ((35 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK35;
	} else if ((67 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK67;
	} else if ((131 * conversion_factor) >= acquisition_time_value) {
		cmd->sampleTimeMode = kLPADC_SampleTimeADCK131;
	} else {
		return -EINVAL;
	}

	return 0;
}

/* Compute 0-based position of channel ch within enabled mask. -1 if not present. */
static int mcux_lpadc_channel_position(uint32_t mask, uint8_t ch)
{
	if ((mask & BIT(ch)) == 0U) {
		return -1;
	}

	int pos = 0;

	for (uint8_t i = 0; i < ch; i++) {
		if (mask & BIT(i)) {
			pos++;
		}
	}

	return pos;
}


static int mcux_lpadc_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	const struct mcux_lpadc_config *config = dev->config;
	const struct device *regulator = config->ref_supplies;
	int32_t vref_uv = config->ref_supply_val * 1000;
	struct mcux_lpadc_data *data = dev->data;
	lpadc_conv_command_config_t *cmd;
	uint8_t channel_side;
	uint8_t channel_num;
	int err;

	/* User may configure maximum number of active channels */
	if (channel_cfg->channel_id >= CONFIG_LPADC_CHANNEL_COUNT) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	/* Select ADC CMD register to configure based off channel ID */
	cmd = &data->cmd_config[channel_cfg->channel_id];

	/* If bit 5 of input_positive is set, then channel side B is used */
	channel_side = 0x20 & channel_cfg->input_positive;
	/* Channel number is selected by lower 4 bits of input_positive */
	channel_num = ADC_CMDL_ADCH(channel_cfg->input_positive);

	LOG_DBG("Channel num: %u, channel side: %c", channel_num,
		channel_side == 0 ? 'A' : 'B');

	LPADC_GetDefaultConvCommandConfig(cmd);

	/* Configure LPADC acquisition time. */
	if (mcux_lpadc_acquisition_time_setup(dev, channel_cfg->acquisition_time, cmd)) {
		LOG_ERR("LPADC acquisition time setting failed");
		return -EINVAL;
	}

#if !(defined(FSL_FEATURE_LPADC_HAS_B_SIDE_CHANNELS) && \
	(FSL_FEATURE_LPADC_HAS_B_SIDE_CHANNELS == 0U))
	if (channel_cfg->differential) {
		/* Channel pairs must match in differential mode */
		if ((ADC_CMDL_ADCH(channel_cfg->input_positive)) !=
		   (ADC_CMDL_ADCH(channel_cfg->input_negative))) {
			return -ENOTSUP;
		}

#if defined(FSL_FEATURE_LPADC_HAS_CMDL_DIFF) && FSL_FEATURE_LPADC_HAS_CMDL_DIFF
		/* Check to see which channel is the positive input */
		if (channel_cfg->input_positive & 0x20) {
			/* Channel B is positive side */
			cmd->sampleChannelMode =
				kLPADC_SampleChannelDiffBothSideBA;
		} else {
			/* Channel A is positive side */
			cmd->sampleChannelMode =
				kLPADC_SampleChannelDiffBothSideAB;
		}
#else
		cmd->sampleChannelMode = kLPADC_SampleChannelDiffBothSide;
#endif
	} else if (channel_side != 0) {
		cmd->sampleChannelMode = kLPADC_SampleChannelSingleEndSideB;
	} else {
		/* Default value for sampleChannelMode is SideA */
	}
#endif
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_CSCALE) && FSL_FEATURE_LPADC_HAS_CMDL_CSCALE
	/*
	 * The true scaling factor used by the LPADC is 30/64, instead of
	 * 1/2. Select 1/2 as this is the closest scaling factor available
	 * in Zephyr.
	 */
	if (channel_cfg->gain == ADC_GAIN_1_2) {
		LOG_INF("Channel gain of 30/64 selected");
		cmd->sampleScaleMode = kLPADC_SamplePartScale;
	} else if (channel_cfg->gain == ADC_GAIN_1) {
		cmd->sampleScaleMode = kLPADC_SampleFullScale;
	} else {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}
#else
	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}
#endif

	/*
	 * ADC_REF_EXTERNAL1: Use SoC internal regulator as LPADC reference voltage.
	 * ADC_REF_EXTERNAL0: Use other voltage source (maybe also within the SoCs)
	 * as LPADC reference voltage, like VREFH, VDDA, etc.
	 */
	if (channel_cfg->reference == ADC_REF_EXTERNAL1) {
		LOG_DBG("ref external1");
		if (regulator != NULL) {
			err = regulator_set_voltage(regulator, vref_uv, vref_uv);
			if (err < 0) {
				return err;
			}
		} else {
			return -EINVAL;
		}
	} else if (channel_cfg->reference == ADC_REF_EXTERNAL0) {
		LOG_DBG("ref external0");
	} else {
		LOG_DBG("ref not support");
		return -EINVAL;
	}

	cmd->channelNumber = channel_num;
	return 0;
}

static int mcux_lpadc_start_read(const struct device *dev,
		 const struct adc_sequence *sequence)
{
	const struct mcux_lpadc_config *config = dev->config;
	struct mcux_lpadc_data *data = dev->data;
	lpadc_hardware_average_mode_t hardware_average_mode;
	uint8_t channel, last_enabled;
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_MODE) \
	&& FSL_FEATURE_LPADC_HAS_CMDL_MODE
	lpadc_conversion_resolution_mode_t resolution_mode;

	switch (sequence->resolution) {
	case 12:
	case 13:
		resolution_mode = kLPADC_ConversionResolutionStandard;
		break;
	case 16:
		resolution_mode = kLPADC_ConversionResolutionHigh;
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
		hardware_average_mode = kLPADC_HardwareAverageCount1;
		break;
	case 1:
		hardware_average_mode = kLPADC_HardwareAverageCount2;
		break;
	case 2:
		hardware_average_mode = kLPADC_HardwareAverageCount4;
		break;
	case 3:
		hardware_average_mode = kLPADC_HardwareAverageCount8;
		break;
	case 4:
		hardware_average_mode = kLPADC_HardwareAverageCount16;
		break;
	case 5:
		hardware_average_mode = kLPADC_HardwareAverageCount32;
		break;
	case 6:
		hardware_average_mode = kLPADC_HardwareAverageCount64;
		break;
	case 7:
		hardware_average_mode = kLPADC_HardwareAverageCount128;
		break;
	default:
		LOG_ERR("Unsupported oversampling value %d",
			sequence->oversampling);
		return -ENOTSUP;
	}

	/*
	 * Now, look at the selected channels to determine which ADC channels
	 * we need to configure, and set those channels up.
	 *
	 * Since this ADC supports chaining channels in hardware, we will
	 * start with the highest channel ID and work downwards, chaining
	 * channels as we go.
	 */
	channel = CONFIG_LPADC_CHANNEL_COUNT;
	last_enabled = 0;
	while (channel-- > 0) {
		if (sequence->channels & BIT(channel)) {
			/* Setup this channel command */
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_MODE) && FSL_FEATURE_LPADC_HAS_CMDL_MODE
			data->cmd_config[channel].conversionResolutionMode =
				resolution_mode;
#endif
			data->cmd_config[channel].hardwareAverageMode =
				hardware_average_mode;
			if (last_enabled) {
				/* Chain channel */
				data->cmd_config[channel].chainedNextCommandNumber =
					last_enabled + 1;
				LOG_DBG("Chaining channel %u to %u",
					channel, last_enabled);
			} else {
				/* End of chain */
				data->cmd_config[channel].chainedNextCommandNumber = 0;
			}
			last_enabled = channel;
			LPADC_SetConvCommandConfig(config->base,
				channel + 1, &data->cmd_config[channel]);
		}
	};

	data->buffer = sequence->buffer;

	/* Precompute raw thresholds for OPAMP channel once per read,
	 * based on current resolution, reference voltage.
	 */
	if (config->opamp) {
		/* Determine effective output width of values we push into the buffer */
		uint32_t max_count = (sequence->resolution < 15) ? 0x0FFF : 0xFFFF;

		if (config->opamp_vref_mv > 0) {
			/* raw = mv * max_count / vref */
			data->sample_min_raw = config->sample_min * max_count /
					config->opamp_vref_mv;
			data->sample_max_raw = config->sample_max * max_count /
					config->opamp_vref_mv;
		} else {
			/* If vref is unknown, fall back to full range */
			data->sample_min_raw = 0u;
			data->sample_max_raw = (uint16_t)max_count;
		}
	}

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
	lpadc_conv_trigger_config_t trigger_config;
	uint8_t first_channel;

	first_channel = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d, input %d", first_channel,
		data->cmd_config[first_channel].channelNumber);

	/* Apply any pending OPAMP gain change synchronously at the start of
	 * the next sampling round, so the adjustment takes effect immediately.
	 * If queued k_work to set the OPAMP gain after a round ended in ISR
	 * may cause the opamp gain to not change successfully before the next round.
	 */
	if (config->opamp && config->opamp_gains && data->desired_gain_index >= 0 &&
		(uint8_t)data->desired_gain_index < config->opamp_gain_count) {
		int idx = data->desired_gain_index;
		int ret = opamp_set_gain(config->opamp, config->opamp_gains[idx]);

		if (ret == 0) {
			data->current_gain_index = (uint8_t)idx;
			data->desired_gain_index = -1;
			LOG_DBG("OPAMP gain set to index %d\r\n", idx);
		} else {
			LOG_DBG("OPAMP gain set failed: %d\r\n", ret);
		}
	}

	LPADC_GetDefaultConvTriggerConfig(&trigger_config);

	trigger_config.targetCommandId = first_channel + 1;

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
	lpadc_sample_channel_mode_t conv_mode;
	int16_t result;
	uint16_t channel;

#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) \
	&& (FSL_FEATURE_LPADC_FIFO_COUNT == 2U))
	LPADC_GetConvResult(base, &conv_result, 0U);
#else
	LPADC_GetConvResult(base, &conv_result);
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */

	channel = conv_result.commandIdSource - 1;
	LOG_DBG("Finished channel %d. Raw result is 0x%04x",
		channel, conv_result.convValue);
	/*
	 * For 12 or 13 bit resolution the LSBs will be 0, so a bit shift
	 * is needed. For differential modes, the ADC conversion to
	 * millivolts expects to use a shift one less than the resolution.
	 *
	 * For 16 bit modes, the adc value can be left untouched. ADC
	 * API should treat the value as signed if the channel is
	 * in differential mode
	 */
	conv_mode = data->cmd_config[channel].sampleChannelMode;
	if (data->ctx.sequence.resolution < 15) {
		result = ((conv_result.convValue >> 3) & 0xFFF);
#if !(defined(FSL_FEATURE_LPADC_HAS_B_SIDE_CHANNELS) && \
	(FSL_FEATURE_LPADC_HAS_B_SIDE_CHANNELS == 0U))
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_DIFF) && FSL_FEATURE_LPADC_HAS_CMDL_DIFF
		if (conv_mode == kLPADC_SampleChannelDiffBothSideAB ||
		    conv_mode == kLPADC_SampleChannelDiffBothSideBA) {
#else
		if (conv_mode == kLPADC_SampleChannelDiffBothSide) {
#endif
			if ((conv_result.convValue & 0x8000)) {
				/* 13 bit mode, MSB is sign bit. (2's complement) */
				result -= 0x1000;
			}
		}
		*data->buffer++ = result;
#endif
	} else {
		*data->buffer++ = conv_result.convValue;
	}


	data->channels &= ~BIT(channel);

	/*
	 * Hardware will automatically continue sampling, so no need
	 * to issue new trigger
	 */
	if (data->channels == 0) {
		/* End of one round: decide whether to adjust OPAMP gain */
		if (config->opamp && config->opamp_gain_count > 0U && config->opamp_gains != NULL) {
			int pos = mcux_lpadc_channel_position(data->ctx.sequence.channels,
						config->opamp_channel);

			if (pos >= 0) {
				uint16_t sample = data->repeat_buffer[pos];
				int new_index = (int)data->current_gain_index;

				if (sample < data->sample_min_raw) {
					if ((data->current_gain_index + 1U) <
							config->opamp_gain_count) {
						new_index = (int)(data->current_gain_index + 1U);
					}
				} else if (sample > data->sample_max_raw) {
					if (data->current_gain_index > 0U) {
						new_index = (int)(data->current_gain_index - 1U);
					}
				}
				if (new_index != (int)data->current_gain_index) {
					/* Stage desired gain; it will be applied synchronously
					 * at the start of the next sampling round.
					 */
					data->desired_gain_index = new_index;
				}
			}
		}
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int mcux_lpadc_init(const struct device *dev)
{
	const struct mcux_lpadc_config *config = dev->config;
	struct mcux_lpadc_data *data = dev->data;
	ADC_Type *base = config->base;
	lpadc_config_t adc_config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	/* Enable necessary regulators */
	const struct device *regulator = config->ref_supplies;

	if (regulator != NULL) {
		err = regulator_enable(regulator);
		if (err) {
			return err;
		}
	}

	LPADC_GetDefaultConfig(&adc_config);

	adc_config.enableAnalogPreliminary = true;
	adc_config.referenceVoltageSource = config->voltage_ref;

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS) \
	&& FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS
	adc_config.conversionAverageMode = config->calibration_average;
#endif /* FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS */

#if !(DT_ANY_INST_HAS_PROP_STATUS_OKAY(no_power_level))
		adc_config.powerLevelMode = config->power_level;
#endif

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

	/* Initialize OPAMP gain control context */
	data->current_gain_index = 0U;
	data->desired_gain_index = -1;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, mcux_lpadc_driver_api) = {
	.channel_setup = mcux_lpadc_channel_setup,
	.read = mcux_lpadc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcux_lpadc_read_async,
#endif
};

#define OPAMP_NODE(n) DT_INST_PHANDLE(n, nxp_opamps)
/* Helpers to select the ADC child node matching channel reg value */
#define LPADC_FOREACH_INPUT(node, ch) \
	IF_ENABLED(IS_EQ(DT_REG_ADDR_RAW(node), ch), (node))
#define OPAMP_CH_ID(n) DT_PHA_BY_IDX(DT_DRV_INST(n), nxp_opamps, 0, channel_id)
#define OPAMP_CH_NODE(n) DT_FOREACH_CHILD_VARGS(DT_DRV_INST(n),	\
	LPADC_FOREACH_INPUT, OPAMP_CH_ID(n))
#define OPAMP_GAINS_INIT(n)								\
	.opamp_gains = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, nxp_opamps),		\
		(COND_CODE_1(DT_NODE_HAS_PROP(OPAMP_NODE(n), programmable_gain),	\
		((const enum opamp_gain[]){DT_FOREACH_PROP_ELEM_SEP(OPAMP_NODE(n),	\
		programmable_gain, DT_ENUM_IDX_BY_IDX, (,))}), (NULL))), (NULL)),	\
											\
	.opamp_gain_count = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, nxp_opamps),		\
		(COND_CODE_1(DT_NODE_HAS_PROP(OPAMP_NODE(n), programmable_gain),	\
		(DT_PROP_LEN(OPAMP_NODE(n), programmable_gain)), (0))), (0)),

#define LPADC_MCUX_INIT(n)						\
									\
	static void mcux_lpadc_config_func_##n(const struct device *dev);	\
									\
	PINCTRL_DT_INST_DEFINE(n);						\
	static const struct mcux_lpadc_config mcux_lpadc_config_##n = {	\
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),	\
		.voltage_ref =	DT_INST_PROP(n, voltage_ref),	\
		.calibration_average = DT_INST_ENUM_IDX_OR(n, calibration_average, 0),	\
		.power_level = DT_INST_PROP_OR(n, power_level, 0),	\
		.offset_a = DT_INST_PROP(n, offset_value_a),	\
		.offset_b = DT_INST_PROP(n, offset_value_b),	\
		.irq_config_func = mcux_lpadc_config_func_##n,				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.ref_supplies = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, nxp_references),\
						(DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n),\
						nxp_references))), (NULL)),\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.ref_supply_val = COND_CODE_1(\
						DT_INST_NODE_HAS_PROP(n, nxp_references),\
						(DT_PHA(DT_DRV_INST(n), nxp_references, vref_mv)), \
						(0)),\
		.opamp = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, nxp_opamps),			\
			(DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(n, nxp_opamps, 0))), (NULL)),	\
		.opamp_channel = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, nxp_opamps),		\
			(DT_PHA_BY_IDX(DT_DRV_INST(n), nxp_opamps, 0, channel_id)), (0)),	\
		.opamp_vref_mv = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, nxp_opamps),		\
			(COND_CODE_1(DT_NODE_EXISTS(OPAMP_CH_NODE(n)),				\
			(DT_PROP_OR(OPAMP_CH_NODE(n), zephyr_vref_mv, 0)), (0))), (0)),		\
		.sample_min = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, ideal_sample_range),		\
			(DT_PROP_BY_IDX(DT_DRV_INST(n), ideal_sample_range, 0)), (INT32_MIN)),	\
		.sample_max = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, ideal_sample_range),		\
			(DT_PROP_BY_IDX(DT_DRV_INST(n), ideal_sample_range, 1)), (UINT32_MAX)),	\
			OPAMP_GAINS_INIT(n)							\
	};									\
	static struct mcux_lpadc_data mcux_lpadc_data_##n = {	\
		ADC_CONTEXT_INIT_TIMER(mcux_lpadc_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(mcux_lpadc_data_##n, ctx),	\
		ADC_CONTEXT_INIT_SYNC(mcux_lpadc_data_##n, ctx),	\
	};														\
										\
	DEVICE_DT_INST_DEFINE(n,						\
		&mcux_lpadc_init, NULL, &mcux_lpadc_data_##n,			\
		&mcux_lpadc_config_##n, POST_KERNEL,				\
		CONFIG_ADC_INIT_PRIORITY,					\
		&mcux_lpadc_driver_api);							\
										\
	static void mcux_lpadc_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			DT_INST_IRQ(n, priority), mcux_lpadc_isr,	\
			DEVICE_DT_INST_GET(n), 0);				\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}	\
										\
	BUILD_ASSERT((DT_INST_PROP_OR(n, power_level, 0) >= 0) && \
		(DT_INST_PROP_OR(n, power_level, 0) <= 3), "power_level: wrong value");

DT_INST_FOREACH_STATUS_OKAY(LPADC_MCUX_INIT)
