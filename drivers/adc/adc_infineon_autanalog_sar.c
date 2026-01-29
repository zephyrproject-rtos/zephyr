/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief ADC driver for Infineon AutAnalog SAR ADC used by Edge MCU family.
 */

#define DT_DRV_COMPAT infineon_autanalog_sar_adc

#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <ifx_autanalog.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "cy_pdl.h"

LOG_MODULE_REGISTER(ifx_autanalog_sar_adc, CONFIG_ADC_LOG_LEVEL);

#define ADC_AUTANALOG_SAR_DEFAULT_ACQUISITION_NS (1000u)
#define ADC_AUTANALOG_SAR_RESOLUTION             12

#define IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS           32
#define IFX_AUTANALOG_SAR_NUM_SEQUENCERS             1
#define IFX_AUTANALOG_SAR_NUM_ENABLED_CHANNELS(inst) DT_NUM_CHILDREN(DT_DRV_INST(inst))

#define IFX_AUTANALOG_SAR_SAMPLETIME_COUNT 4

#define IFX_AUTANALOG_HF_CLK_SRC 9

/* Reference Voltage Source definitions from Device Tree Bindings */
enum ifx_autanalog_sar_vref_source {
	IFX_AUTANALOG_SAR_VREF_VDDA = 0,
	IFX_AUTANALOG_SAR_VREF_EXT = 1,
	IFX_AUTANALOG_SAR_VREF_VBGR = 2,
	IFX_AUTANALOG_SAR_VREF_VDDA_BY_2 = 3,
	IFX_AUTANALOG_SAR_VREF_PRB_OUT0 = 4,
	IFX_AUTANALOG_SAR_VREF_PRB_OUT1 = 5,
};

struct ifx_autanalog_sar_adc_config {
	void (*irq_func)(void);
	enum ifx_autanalog_sar_vref_source vref_source;
	bool linear_cal;
	bool offset_cal;
};

struct ifx_autanalog_sar_adc_channel_config {
	/* Hardware supports 4 sample times, so map this channel to one of the sample times
	 * configured in HW.
	 */
	uint8_t sample_time_idx;
};

struct ifx_autanalog_sar_adc_data {
	struct adc_context ctx;
	const struct device *dev;
	/* Conversion Buffer */
	uint16_t *conversion_buffer;
	/* Repeat buffer for continuous sampling */
	uint16_t *repeat_buffer;
	/* Conversion result */
	int conversion_result;
	/* Bitmask of enabled channels */
	uint32_t enabled_channels;

	struct ifx_autanalog_sar_adc_channel_config
		autanalog_channel_cfg[IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS];

	/* The following structures are used by the Infineon PDL API for configuring the ADC. */
	cy_stc_autanalog_sar_t pdl_adc_top_obj;
	cy_stc_autanalog_sar_sta_t pdl_adc_top_static_obj;

	/* PDL structures to initialize the High Speed ADC */
	cy_stc_autanalog_sar_hs_chan_t
		pdl_adc_hs_channel_cfg_obj_arr[IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS];
	cy_stc_autanalog_sar_sta_hs_t pdl_adc_hs_static_obj;
	cy_stc_autanalog_sar_seq_tab_hs_t pdl_adc_seq_hs_cfg_obj[IFX_AUTANALOG_SAR_NUM_SEQUENCERS];
};

/**
 * @brief Initialize PDL structures for the ADC
 *
 * @param data Pointer to driver data structure
 * @param cfg Pointer to driver configuration structure
 *
 * Initializes the PDL structures using default values and values for calibration and vref values
 * derived from the device tree.
 */
static void ifx_init_pdl_structs(struct ifx_autanalog_sar_adc_data *data,
				 const struct ifx_autanalog_sar_adc_config *cfg)
{
	data->pdl_adc_top_obj = (cy_stc_autanalog_sar_t){
		.sarStaCfg = &data->pdl_adc_top_static_obj,
		/* This driver implementation uses only a single sequencer.  The sequencer is
		 * reconfigured every time an adc read is started.  Hardware supports up to 32
		 * sequencers, which can be used for more advanced ADC configurations.
		 */
		.hsSeqTabNum = IFX_AUTANALOG_SAR_NUM_SEQUENCERS,
		.hsSeqTabArr = &data->pdl_adc_seq_hs_cfg_obj[0],
		.lpSeqTabNum = 0U,
		.lpSeqTabArr = NULL,
		.firNum = 0U,
		.firCfg = NULL,
		.fifoCfg = NULL,
	};

	data->pdl_adc_seq_hs_cfg_obj[0] = (cy_stc_autanalog_sar_seq_tab_hs_t){
		.chanEn = CY_AUTANALOG_SAR_CHAN_MASK_GPIO_DISABLED,
		.muxMode = CY_AUTANALOG_SAR_CHAN_CFG_MUX_DISABLED,
		.mux0Sel = CY_AUTANALOG_SAR_CHAN_CFG_MUX0,
		.mux1Sel = CY_AUTANALOG_SAR_CHAN_CFG_MUX0,
		.sampleTimeEn = true,
		.sampleTime = CY_AUTANALOG_SAR_SAMPLE_TIME0,
		.accEn = false,
		.accCount = CY_AUTANALOG_SAR_ACC_CNT2,
		.calReq = CY_AUTANALOG_SAR_CAL_DISABLED,
		.nextAction = CY_AUTANALOG_SAR_NEXT_ACTION_STATE_STOP, /* Single Shot mode  */

	};

	data->pdl_adc_top_static_obj = (cy_stc_autanalog_sar_sta_t){
		.lpStaCfg = NULL, /* This driver implementation only implements HS mode.*/
		.hsStaCfg = &data->pdl_adc_hs_static_obj,
		.posBufPwr = CY_AUTANALOG_SAR_BUF_PWR_OFF,
		.negBufPwr = CY_AUTANALOG_SAR_BUF_PWR_OFF,
		/* Note:  This setting chooses "accumulate and dump" vs. "interleaved" for channels
		 * where averaging is enabled.  The selection for "accumulate" vs. "accumlate and
		 * divide" is tracked by adc->average_is_accumulate and is applied in the hardware
		 * on a per-channel basis.
		 */
		.accMode = CY_AUTANALOG_SAR_ACC_DISABLED,
		.startupCal = (cfg->offset_cal ? CY_AUTANALOG_SAR_CAL_OFFSET
					       : CY_AUTANALOG_SAR_CAL_DISABLED) |
			      (cfg->linear_cal ? CY_AUTANALOG_SAR_CAL_LINEARITY
					       : CY_AUTANALOG_SAR_CAL_DISABLED),
		.chanID = false, /* We don't use the FIFO features */
		/* When accShift is set for a channel, shift back down to 12 bits */
		.shiftMode = false,
		.intMuxChan = {NULL}, /* We don't expose mux channels */
		.limitCond = {NULL},  /* We don't expose the range detection */
		.muxResultMask = 0u,  /* We don't expose mux channels */
		.firResultMask = 0u,  /* We don't expose FIR functionality */
	};

	data->pdl_adc_hs_static_obj = (cy_stc_autanalog_sar_sta_hs_t){
		.hsVref = cfg->vref_source,
		.hsSampleTime = {
				0,
				0,
				0,
				0,
			},
		/* These will be configured during channel setup */
		.hsGpioChan = {
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
			},
		.hsGpioResultMask = 0,
	};

	/* Map the vref_source from the device tree to the PDL enum */
	switch (cfg->vref_source) {
	case IFX_AUTANALOG_SAR_VREF_VDDA:
		data->pdl_adc_hs_static_obj.hsVref = CY_AUTANALOG_SAR_VREF_VDDA;
		break;
	case IFX_AUTANALOG_SAR_VREF_EXT:
		data->pdl_adc_hs_static_obj.hsVref = CY_AUTANALOG_SAR_VREF_EXT;
		break;
	case IFX_AUTANALOG_SAR_VREF_VDDA_BY_2:
		data->pdl_adc_hs_static_obj.hsVref = CY_AUTANALOG_SAR_VREF_VDDA_BY_2;
		break;
	case IFX_AUTANALOG_SAR_VREF_VBGR:
		data->pdl_adc_hs_static_obj.hsVref = CY_AUTANALOG_SAR_VREF_VBGR;
		break;
	case IFX_AUTANALOG_SAR_VREF_PRB_OUT0:
		data->pdl_adc_hs_static_obj.hsVref = CY_AUTANALOG_SAR_VREF_PRB_OUT0;
		break;
	case IFX_AUTANALOG_SAR_VREF_PRB_OUT1:
		data->pdl_adc_hs_static_obj.hsVref = CY_AUTANALOG_SAR_VREF_PRB_OUT1;
		break;
	default:
		LOG_ERR("Unsupported VREF source, using VDDA");
		data->pdl_adc_hs_static_obj.hsVref = CY_AUTANALOG_SAR_VREF_VDDA;
		break;
	}
} /* ifx_init_pdl_struct() */

/**
 * @brief Read results from the ADC
 *
 * @param channels Bitmask of channels to read
 * @param data Pointer to driver data structure
 *
 * Reads the conversion results for the specified channels from the ADC and stores them in the
 * buffer pointed to by data->buffer.  The buffer pointer is incremented as results are stored.
 * It is assumed that the buffer is large enough to hold all requested results.
 */
static void ifx_autanalog_sar_get_results(uint32_t channels,
					  struct ifx_autanalog_sar_adc_data *data)
{
	if (data->conversion_buffer == NULL) {
		LOG_ERR("ADC data buffer is NULL");
		return;
	}

	for (size_t i = 0; i < IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS; i++) {
		if ((channels & (1 << i)) != 0) {
			*data->conversion_buffer++ =
				Cy_AutAnalog_SAR_ReadResult(0, CY_AUTANALOG_SAR_INPUT_GPIO, i);
		}
	}
} /* ifx_autanalog_sar_get_results() */

/**
 * @brief Build a sequencer entry for the specified channels
 *
 * @param channels Bitmask of channels to include in the sequencer entry
 * @param data Pointer to driver data structure
 *
 * Builds a sequencer entry for the specified channels.  All channels in the entry must have the
 * same acquisition time and must map to one of the four sample times configured in hardware.
 *
 * @return 0 on success, -EINVAL if an error occurs.
 */
static int ifx_build_hs_sequencer_entry(uint32_t channels, struct ifx_autanalog_sar_adc_data *data)
{
	uint8_t timer_index = IFX_AUTANALOG_SAR_SAMPLETIME_COUNT;
	cy_stc_autanalog_sar_seq_tab_hs_t *seq_entry = &data->pdl_adc_seq_hs_cfg_obj[0];

	/* Verify that all channels in the sequence have the same acquisition time and the sample
	 * time is configured in hardware.
	 */
	for (uint8_t i = 0; i < IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS; i++) {
		if ((channels & (1 << i)) != 0) {
			if (data->autanalog_channel_cfg[i].sample_time_idx >=
			    IFX_AUTANALOG_SAR_SAMPLETIME_COUNT) {
				LOG_ERR("Invalid sample time index for channel %d", i);
				return -EINVAL;
			}

			if (timer_index == IFX_AUTANALOG_SAR_SAMPLETIME_COUNT) {
				timer_index = data->autanalog_channel_cfg[i].sample_time_idx;
			} else if (timer_index != data->autanalog_channel_cfg[i].sample_time_idx) {
				LOG_ERR("All channels in a sequence must have the same sample "
					"time");
				return -EINVAL;
			}
		}
	}

	if (timer_index >= IFX_AUTANALOG_SAR_SAMPLETIME_COUNT) {
		LOG_ERR("No sample time configured for selected channels");
		return -EINVAL;
	}

	seq_entry->sampleTime = (cy_en_autanalog_sar_sample_time_t)timer_index;
	seq_entry->chanEn = channels;
	seq_entry->muxMode = CY_AUTANALOG_SAR_CHAN_CFG_MUX_DISABLED;
	seq_entry->mux0Sel = CY_AUTANALOG_SAR_CHAN_CFG_MUX0;
	seq_entry->mux1Sel = CY_AUTANALOG_SAR_CHAN_CFG_MUX0;
	seq_entry->sampleTimeEn = true;
	seq_entry->accEn = false;
	seq_entry->accCount = CY_AUTANALOG_SAR_ACC_CNT2;
	seq_entry->calReq = CY_AUTANALOG_SAR_CAL_DISABLED;
	seq_entry->nextAction = CY_AUTANALOG_SAR_NEXT_ACTION_STATE_STOP;

	return 0;
} /* ifx_build_hs_sequencer_entry() */

/** @brief Start ADC sampling
 *
 * @param ctx Pointer to ADC context
 *
 * This function is called by the ADC context.  Configures ADC Sequencer and starts the ADC
 * sampling.
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ifx_autanalog_sar_adc_data *data =
		CONTAINER_OF(ctx, struct ifx_autanalog_sar_adc_data, ctx);
	const struct adc_sequence *sequence = &ctx->sequence;
	uint32_t result_status;

	data->repeat_buffer = data->conversion_buffer;
	if (data->conversion_buffer == NULL || sequence->buffer_size == 0) {
		data->conversion_result = -ENOMEM;
		return;
	}

	if (sequence->channels == 0) {
		LOG_ERR("No channels specified");
		data->conversion_result = -EINVAL;

		return;
	}

	/* This implementation uses a single sequencer which is reconfigured for every ADC
	 * read operation.  If needed, this can be extended to use multiple sequencers.
	 */
	if (ifx_build_hs_sequencer_entry(sequence->channels, data) != 0) {
		LOG_ERR("Error building ADC Sequencer Configuration");
		data->conversion_result = -EINVAL;
		return;
	}

	/* Stop the Autonomous Controller while we reconfigure the sequencer */
	ifx_autanalog_pause_sar_autonomous_control();
	result_status = Cy_AutAnalog_SAR_LoadHSseqTable(0, IFX_AUTANALOG_SAR_NUM_SEQUENCERS,
							&data->pdl_adc_seq_hs_cfg_obj[0]);
	if (result_status != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Error Loading ADC Sequencer Configuration: %u",
			(unsigned int)result_status);
		data->conversion_result = -EIO;
		return;
	}

	ifx_autanalog_start_sar_autonomous_control();
	Cy_AutAnalog_SAR_ClearHSchanResultStatus(0, sequence->channels);
	Cy_AutAnalog_FwTrigger(CY_AUTANALOG_FW_TRIGGER0);

#if defined(CONFIG_ADC_ASYNC)
	if (!data->ctx.asynchronous) {
#endif
		/* Wait for conversion to complete */
		do {
			result_status = Cy_AutAnalog_SAR_GetHSchanResultStatus(0);
		} while ((result_status & sequence->channels) != sequence->channels);

		ifx_autanalog_sar_get_results(sequence->channels, data);
		adc_context_on_sampling_done(&data->ctx, data->dev);
#if defined(CONFIG_ADC_ASYNC)
	}
#endif
	data->conversion_result = 0;

} /* adc_context_start_sampling() */

/**
 * @brief Update buffer pointer for next read
 *
 * @param ctx Pointer to ADC context
 * @param repeat_sampling True if the current sampling is to be repeated
 */
static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ifx_autanalog_sar_adc_data *data =
		CONTAINER_OF(ctx, struct ifx_autanalog_sar_adc_data, ctx);

	if (repeat_sampling) {
		data->conversion_buffer = data->repeat_buffer;
	}
} /* adc_context_update_buffer_pointer() */

/**
 * @brief Start an ADC read operation
 *
 * @param dev Pointer to device structure
 * @param sequence Pointer to ADC sequence structure
 *
 * Validates the requested sequence is supported by hardware and starts the ADC read operation.
 *
 * @return 0 on success, negative error code on failure.
 */
static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ifx_autanalog_sar_adc_data *data = dev->data;

	if (sequence->buffer_size < (sizeof(int16_t) * POPCOUNT(sequence->channels))) {
		LOG_ERR("Buffer too small");
		return -ENOMEM;
	}

	if (sequence->resolution != ADC_AUTANALOG_SAR_RESOLUTION) {
		LOG_ERR("Unsupported resolution: %d", sequence->resolution);
		return -EINVAL;
	}

	if (sequence->channels == 0) {
		LOG_ERR("No channels specified");
		return -EINVAL;
	}

	if ((sequence->channels ^ (data->enabled_channels & sequence->channels)) != 0) {
		LOG_ERR("Channels not configured");
		return -EINVAL;
	}

	if (sequence->oversampling != 0) {
		LOG_ERR("Oversampling not supported");
		return -EINVAL;
	}

	data->conversion_buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
} /* start_read() */

/**
 * @brief ADC interrupt handler
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * All interrupts for the AutAnalog subsystem are handled by a single IRQ.  This function implements
 * the AutAnalog SAR ADC interrupt handling, and is expected to be called from the AutAnalog ISR.
 */
static void ifx_autanalog_sar_adc_isr(const struct device *dev)
{
#if defined(CONFIG_ADC_ASYNC)
	struct ifx_autanalog_sar_adc_data *data = dev->data;
	const struct adc_sequence *sequence = &data->ctx.sequence;
	uint32_t result_status;

	if (data->ctx.asynchronous) {
		result_status = Cy_AutAnalog_SAR_GetHSchanResultStatus(0);

		if ((result_status & sequence->channels) == sequence->channels) {
			Cy_AutAnalog_SAR_ClearHSchanResultStatus(0, sequence->channels);
			ifx_autanalog_sar_get_results(sequence->channels, data);
			adc_context_on_sampling_done(&data->ctx, data->dev);
		} else {
			/* Not all channels have completed yet.  This shouldn't happen in
			 * normal operation
			 */
			LOG_ERR("ADC ISR: Not all channels completed yet.");
		}
	}
#else
	ARG_UNUSED(dev);
#endif
} /* ifx_autanalog_sar_adc_isr() */

/**
 * @brief Calculate the sample time register value based on requested acquisition
 * time
 *
 * @param acquisition_time_ns Requested acquisition time in nanoseconds
 *
 * @return Acquisition clock cycles, 0 if error
 */
static uint16_t ifx_calc_acquisition_timer_val(uint32_t acquisition_time_ns)
{
	const uint32_t ACQUISITION_CLOCKS_MIN = 1;
	const uint32_t ACQUISITION_CLOCKS_MAX = 1024;

	uint32_t timer_clock_cycles;
	uint32_t clock_frequency_hz;
	uint32_t clock_period_ns;

	clock_frequency_hz = Cy_SysClk_ClkHfGetFrequency(IFX_AUTANALOG_HF_CLK_SRC);
	if (clock_frequency_hz == 0) {
		LOG_ERR("Failed to get AutAnalog clock frequency");
		return 0;
	}

	clock_period_ns = NSEC_PER_SEC / clock_frequency_hz;
	timer_clock_cycles = (acquisition_time_ns + (clock_period_ns - 1)) / clock_period_ns;
	if (timer_clock_cycles < ACQUISITION_CLOCKS_MIN) {
		timer_clock_cycles = ACQUISITION_CLOCKS_MIN;
		LOG_WRN("ADC acquisition time too short, using minimum");
	} else if (timer_clock_cycles > ACQUISITION_CLOCKS_MAX) {
		timer_clock_cycles = ACQUISITION_CLOCKS_MAX;
		LOG_WRN("ADC acquisition time too long, using maximum");
	}

	/* Per the register map, the timer value should be one less than the actual
	 * desired sampling cycle count
	 */
	return (uint16_t)(timer_clock_cycles - 1);
} /* ifx_calc_acquisition_timer_val() */

/* Zephyr Driver API Functions
 */

/**
 * @brief Autanalog SAR ADC read function
 *
 * @param dev Pointer to device structure
 * @param sequence Pointer to ADC sequence structure
 *
 * @return 0 on success, error code on failure
 */
static int ifx_autanalog_sar_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ifx_autanalog_sar_adc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
} /* ifx_autanalog_sar_adc_read() */

#ifdef CONFIG_ADC_ASYNC
/**
 * @brief Autanalog SAR ADC asynchronous read function
 *
 * @param dev Pointer to device structure
 * @param sequence Pointer to ADC sequence structure
 * @param async Pointer to poll signal structure for asynchronous operation
 *
 * @return 0 on success, error code on failure
 */
static int ifx_autanalog_sar_adc_read_async(const struct device *dev,
					    const struct adc_sequence *sequence,
					    struct k_poll_signal *async)
{
	struct ifx_autanalog_sar_adc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
} /* ifx_autanalog_sar_adc_read_async() */
#endif

/**
 * @brief API Function to configure an ADC channel
 *
 * @param dev Pointer to device structure
 * @param channel_cfg Pointer to channel configuration structure
 *
 * @return 0 on success, negative error code on failure
 */
static int ifx_autanalog_sar_adc_channel_setup(const struct device *dev,
					       const struct adc_channel_cfg *channel_cfg)
{
	struct ifx_autanalog_sar_adc_data *data = dev->data;
	struct ifx_autanalog_sar_adc_channel_config *channel_zephyr_cfg =
		&data->autanalog_channel_cfg[channel_cfg->channel_id];
	cy_stc_autanalog_sar_hs_chan_t *pdl_channel =
		&data->pdl_adc_hs_channel_cfg_obj_arr[channel_cfg->channel_id];
	data->pdl_adc_hs_static_obj.hsGpioChan[channel_cfg->channel_id] = pdl_channel;
	uint8_t sample_time_idx;
	uint16_t timer_clock_cycles;

	if (channel_cfg->channel_id >= IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS) {
		LOG_ERR("Invalid channel ID: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("AutAnalog SAR ADC Hardware only supports unity gain.");
		return -EINVAL;
	}

	/* NOTE: This ADC hardware does not support reference settings per channel.
	 * Reference must be set for all channels.  Use vref-source property in devicetree
	 * for the adc instance.
	 */
	if (channel_cfg->reference != ADC_REF_INTERNAL &&
	    channel_cfg->reference != ADC_REF_EXTERNAL0 &&
	    channel_cfg->reference != ADC_REF_VDD_1_2) {
		LOG_ERR("Reference setting not supported.");
		return -EINVAL;
	}

	/* This driver implementation only supports direct GPIO channel inputs and not the
	 * MUXed inputs
	 */
	if (channel_cfg->input_positive >= PASS_SAR_SAR_GPIO_CHANNELS) {
		LOG_ERR("Invalid ADC input pin for channel %d: %d", channel_cfg->channel_id,
			channel_cfg->input_positive);
		return -EINVAL;
	}

	/* Calculate sample time and try to map it to one of the 4 available sample time
	 * configurations. If all sample time slots have been used and don't match the
	 * requested time, return an error and stop configuring the ADC channel.
	 */
	timer_clock_cycles = ifx_calc_acquisition_timer_val(channel_cfg->acquisition_time);
	sample_time_idx = 0xFF;
	for (size_t i = 0; i < IFX_AUTANALOG_SAR_SAMPLETIME_COUNT; i++) {
		if (data->pdl_adc_hs_static_obj.hsSampleTime[i] == timer_clock_cycles) {
			sample_time_idx = i;
			break;
		} else if (data->pdl_adc_hs_static_obj.hsSampleTime[i] == 0) {
			data->pdl_adc_hs_static_obj.hsSampleTime[i] = timer_clock_cycles;
			sample_time_idx = i;
			break;
		}
	}

	if (sample_time_idx == 0xFF) {
		LOG_ERR("No available sample time slots for requested acquisition time");
		return -EINVAL;
	}

	channel_zephyr_cfg->sample_time_idx = sample_time_idx;

	pdl_channel->posPin = channel_cfg->input_positive;
	pdl_channel->hsDiffEn = false;
	pdl_channel->sign = false;
	pdl_channel->posCoeff = CY_AUTANALOG_SAR_CH_COEFF_DISABLED;
	pdl_channel->negPin = CY_AUTANALOG_SAR_PIN_GPIO0;
	pdl_channel->accShift = false;
	pdl_channel->negCoeff = CY_AUTANALOG_SAR_CH_COEFF_DISABLED;
	pdl_channel->hsLimit = CY_AUTANALOG_SAR_LIMIT_STATUS_DISABLED;
	pdl_channel->fifoSel = CY_AUTANALOG_FIFO_DISABLED;

	data->pdl_adc_hs_static_obj.hsGpioResultMask |= (1 << channel_cfg->channel_id);

	if (Cy_AutAnalog_SAR_LoadStaticConfig(0, &data->pdl_adc_top_static_obj) !=
	    CY_AUTANALOG_SUCCESS) {
		data->pdl_adc_hs_static_obj.hsGpioChan[channel_cfg->channel_id] = NULL;
		data->pdl_adc_hs_static_obj.hsGpioResultMask &= ~(1 << channel_cfg->channel_id);
		LOG_ERR("Failed to configure ADC Channel %d", channel_cfg->channel_id);

		return -EIO;
	}

	data->enabled_channels |= BIT(channel_cfg->channel_id);

	return 0;
} /* ifx_autanalog_sar_adc_channel_setup() */

/**
 * @brief Initialize the ADC driver
 *
 * @param dev Pointer to device structure
 *
 * Initializes the ADC driver, configures the Infineon PDL data structures, and initializes the
 * AutAnalog SAR ADC hardware.
 *
 * @return 0 on success, negative error code on failure
 */
static int ifx_autanalog_sar_adc_init(const struct device *dev)
{
	const struct ifx_autanalog_sar_adc_config *cfg = dev->config;
	struct ifx_autanalog_sar_adc_data *data = dev->data;
	cy_en_autanalog_status_t result_val;

	memset(data->autanalog_channel_cfg, 0xFF, sizeof(data->autanalog_channel_cfg));

	data->dev = dev;
	data->enabled_channels = 0;

	/* Initialize the pdl data structures based on the device tree configuration, then
	 * use Infineon PDL APIs to initialize the ADC.
	 */
	ifx_init_pdl_structs(data, cfg);
	result_val = Cy_AutAnalog_SAR_LoadConfig(0, &data->pdl_adc_top_obj);
	if (result_val != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to initialize AutAnalog SAR ADC");
		return -EIO;
	}

	/* Note: We can only partially initialize the AutAnalog system here.  If we try to
	 * run the Autonomous Controller here, the ADC will not function correctly.  We need
	 * to wait until at least one channel is configured before starting the AC.
	 */

#if defined(CONFIG_ADC_ASYNC)
	cfg->irq_func();
#endif /* CONFIG_ADC_ASYNC */

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
} /* ifx_autanalog_sar_adc_init() */

#ifdef CONFIG_ADC_ASYNC
#define ADC_IFX_AUTANALOG_SAR_DRIVER_API(n)                                                        \
	static const struct adc_driver_api ifx_autanalog_sar_adc_api_##n = {                       \
		.channel_setup = ifx_autanalog_sar_adc_channel_setup,                              \
		.read = ifx_autanalog_sar_adc_read,                                                \
		.read_async = ifx_autanalog_sar_adc_read_async,                                    \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};
#else
#define ADC_IFX_AUTANALOG_SAR_DRIVER_API(n)                                                        \
	static const struct adc_driver_api ifx_autanalog_sar_adc_api_##n = {                       \
		.channel_setup = ifx_autanalog_sar_adc_channel_setup,                              \
		.read = ifx_autanalog_sar_adc_read,                                                \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};
#endif /* CONFIG_ADC_ASYNC */

/* Device Instantiation */
#define IFX_AUTANALOG_SAR_ADC_INIT(n)                                                              \
	ADC_IFX_AUTANALOG_SAR_DRIVER_API(n);                                                       \
	static void ifx_autanalog_sar_adc_config_func_##n(void);                                   \
	static const struct ifx_autanalog_sar_adc_config ifx_autanalog_sar_adc_config_##n = {      \
		.irq_func = ifx_autanalog_sar_adc_config_func_##n,                                 \
		.vref_source = DT_INST_ENUM_IDX(n, vref_source),                                   \
		.linear_cal = DT_INST_PROP(n, linear_cal),                                         \
		.offset_cal = DT_INST_PROP(n, offset_cal)};                                        \
	static struct ifx_autanalog_sar_adc_data ifx_autanalog_sar_adc_data_##n = {                \
		ADC_CONTEXT_INIT_LOCK(ifx_autanalog_sar_adc_data_##n, ctx),                        \
		ADC_CONTEXT_INIT_TIMER(ifx_autanalog_sar_adc_data_##n, ctx),                       \
		ADC_CONTEXT_INIT_SYNC(ifx_autanalog_sar_adc_data_##n, ctx),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &ifx_autanalog_sar_adc_init, NULL,                                \
			      &ifx_autanalog_sar_adc_data_##n, &ifx_autanalog_sar_adc_config_##n,  \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,                               \
			      &ifx_autanalog_sar_adc_api_##n);                                     \
                                                                                                   \
	static void ifx_autanalog_sar_adc_config_func_##n(void)                                    \
	{                                                                                          \
		ifx_autanalog_register_adc_handler(ifx_autanalog_sar_adc_isr,                      \
						   DEVICE_DT_INST_GET(n));                         \
	}

DT_INST_FOREACH_STATUS_OKAY(IFX_AUTANALOG_SAR_ADC_INIT)
