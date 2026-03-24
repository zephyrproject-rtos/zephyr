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
#include <zephyr/dt-bindings/adc/infineon-autanalog-sar.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <infineon_autanalog.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include "cy_pdl.h"

LOG_MODULE_REGISTER(ifx_autanalog_sar_adc, CONFIG_ADC_LOG_LEVEL);

#define ADC_AUTANALOG_SAR_DEFAULT_ACQUISITION_NS (1000u)
#define ADC_AUTANALOG_SAR_RESOLUTION             12

#define IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS           32
#define IFX_AUTANALOG_SAR_MAX_GPIO_CHANNELS          8
#define IFX_AUTANALOG_SAR_MAX_MUX_CHANNELS           16
#define IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET         8
#define IFX_AUTANALOG_SAR_NUM_SEQUENCERS             1
#define IFX_AUTANALOG_SAR_NUM_ENABLED_CHANNELS(inst) DT_NUM_CHILDREN(DT_DRV_INST(inst))

#define IFX_AUTANALOG_SAR_SAMPLETIME_COUNT 4

#define IFX_AUTANALOG_HF_CLK_SRC 9

/* clang-format off */

/* Helpers to split a combined channel bitmask into GPIO and MUX parts */
#define IFX_GPIO_CHANNELS_MASK(channels) ((channels) & 0xFFu)
#define IFX_MUX_CHANNELS_MASK(channels)                                                        \
	(((channels) >> IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET) & 0xFFFFu)

/* clang-format on */
struct ifx_autanalog_sar_channel_dt_config {
	uint8_t channel_id;
	bool mux_buf_bypass;
};

struct ifx_autanalog_sar_adc_config {
	void (*irq_func)(void);
	const struct device *mfd;
	uint8_t vref_source;
	bool linear_cal;
	bool offset_cal;
	uint8_t pos_buf_pwr;
	uint8_t neg_buf_pwr;
	uint8_t acc_mode;
	bool shift_mode;
	bool fifo_chan_id;
	uint8_t num_dt_channels;
	const struct ifx_autanalog_sar_channel_dt_config *dt_channels;
};

struct ifx_autanalog_sar_adc_channel_config {
	/* Hardware supports 4 sample times, so map this channel to one of the sample times
	 * configured in HW.
	 */
	uint8_t sample_time_idx;
	/* Whether the MUX input buffer should be bypassed for this channel */
	bool mux_buf_bypass;
};

struct ifx_autanalog_sar_adc_data {
	struct adc_context ctx;
	const struct device *dev;
	/* Conversion Buffer */
	uint32_t *conversion_buffer;
	/* Repeat buffer for continuous sampling */
	uint32_t *repeat_buffer;
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
		pdl_adc_hs_channel_cfg_obj_arr[IFX_AUTANALOG_SAR_MAX_GPIO_CHANNELS];
	cy_stc_autanalog_sar_sta_hs_t pdl_adc_hs_static_obj;
	cy_stc_autanalog_sar_seq_tab_hs_t pdl_adc_seq_hs_cfg_obj[IFX_AUTANALOG_SAR_NUM_SEQUENCERS];

	/* PDL structures for MUX channels */
	cy_stc_autanalog_sar_mux_chan_t
		pdl_adc_mux_channel_cfg_obj_arr[IFX_AUTANALOG_SAR_MAX_MUX_CHANNELS];
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
		.posBufPwr = (cy_en_autanalog_sar_buf_pwr_t)cfg->pos_buf_pwr,
		.negBufPwr = (cy_en_autanalog_sar_buf_pwr_t)cfg->neg_buf_pwr,
		/* Note:  This setting chooses "accumulate and dump" vs. "interleaved" for channels
		 * where averaging is enabled.  The selection for "accumulate" vs. "accumulate and
		 * divide" is tracked by adc->average_is_accumulate and is applied in the hardware
		 * on a per-channel basis.
		 */
		.accMode = (cy_en_autanalog_sar_acc_mode_t)cfg->acc_mode,
		.startupCal = (cfg->offset_cal ? CY_AUTANALOG_SAR_CAL_OFFSET
					       : CY_AUTANALOG_SAR_CAL_DISABLED) |
			      (cfg->linear_cal ? CY_AUTANALOG_SAR_CAL_LINEARITY
					       : CY_AUTANALOG_SAR_CAL_DISABLED),
		.chanID = cfg->fifo_chan_id,
		.shiftMode = cfg->shift_mode,
		.intMuxChan = {NULL}, /* MUX channels configured during channel setup */
		.limitCond = {NULL},  /* We don't expose the range detection */
		.muxResultMask = 0u,  /* MUX result mask updated during channel setup */
		.firResultMask = 0u,  /* We don't expose FIR functionality */
	};

	data->pdl_adc_hs_static_obj = (cy_stc_autanalog_sar_sta_hs_t){
		.hsVref = (cy_en_autanalog_sar_vref_t)cfg->vref_source,
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

	/* Read GPIO channel results (channels 0-7) */
	for (size_t i = 0; i < IFX_AUTANALOG_SAR_MAX_GPIO_CHANNELS; i++) {
		if ((channels & BIT(i)) != 0) {
			*data->conversion_buffer++ =
				Cy_AutAnalog_SAR_ReadResult(0, CY_AUTANALOG_SAR_INPUT_GPIO, i);
		}
	}

	/* Read MUX channel results (channels 8-23 map to MUX channels 0-15) */
	for (size_t i = 0; i < IFX_AUTANALOG_SAR_MAX_MUX_CHANNELS; i++) {
		if ((channels & BIT(i + IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET)) != 0) {
			*data->conversion_buffer++ =
				Cy_AutAnalog_SAR_ReadResult(0, CY_AUTANALOG_SAR_INPUT_MUX, i);
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
	uint8_t gpio_channels = IFX_GPIO_CHANNELS_MASK(channels);
	uint16_t mux_channels = IFX_MUX_CHANNELS_MASK(channels);
	uint8_t mux_mode = CY_AUTANALOG_SAR_CHAN_CFG_MUX_DISABLED;
	uint8_t mux0_sel = CY_AUTANALOG_SAR_CHAN_CFG_MUX0;
	uint8_t mux1_sel = CY_AUTANALOG_SAR_CHAN_CFG_MUX0;

	/* Verify that all channels in the sequence have the same acquisition time and the sample
	 * time is configured in hardware.
	 */
	for (uint8_t i = 0; i < IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS; i++) {
		if ((channels & BIT(i)) == 0) {
			continue;
		}

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

	if (timer_index >= IFX_AUTANALOG_SAR_SAMPLETIME_COUNT) {
		LOG_ERR("No sample time configured for selected channels");
		return -EINVAL;
	}

	/* Determine MUX mode based on which MUX channels are requested.
	 * Hardware supports sampling MUX channel 0 only, MUX channel 1 only,
	 * both MUX channels simultaneously, or pseudo-differential mode.
	 * This driver currently supports single-ended MUX sampling only.
	 */
	if (mux_channels != 0) {
		uint8_t mux_count = 0;
		uint8_t first_mux = 0xFF;
		uint8_t second_mux = 0xFF;

		for (uint8_t i = 0; i < IFX_AUTANALOG_SAR_MAX_MUX_CHANNELS; i++) {
			if ((mux_channels & BIT(i)) == 0) {
				continue;
			}

			mux_count++;
			if (first_mux == 0xFF) {
				first_mux = i;
			} else if (second_mux == 0xFF) {
				second_mux = i;
			} else {
				/* Both MUX slots already assigned */
			}
		}

		if (mux_count > 2) {
			LOG_ERR("At most 2 MUX channels can be sampled per sequencer entry");
			return -EINVAL;
		}

		if (mux_count == 1) {
			mux_mode = CY_AUTANALOG_SAR_CHAN_CFG_MUX0_SINGLE_ENDED;
			mux0_sel = first_mux;
		} else {
			mux_mode = CY_AUTANALOG_SAR_CHAN_CFG_MUX0_MUX1_SINGLE_ENDED;
			mux0_sel = first_mux;
			mux1_sel = second_mux;
		}
	}

	seq_entry->sampleTime = (cy_en_autanalog_sar_sample_time_t)timer_index;
	seq_entry->chanEn = gpio_channels;
	seq_entry->muxMode = mux_mode;
	seq_entry->mux0Sel = mux0_sel;
	seq_entry->mux1Sel = mux1_sel;
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
	const struct ifx_autanalog_sar_adc_config *cfg = data->dev->config;
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
	ifx_autanalog_pause_autonomous_control(cfg->mfd);
	result_status = Cy_AutAnalog_SAR_LoadHSseqTable(0, IFX_AUTANALOG_SAR_NUM_SEQUENCERS,
							&data->pdl_adc_seq_hs_cfg_obj[0]);
	if (result_status != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Error Loading ADC Sequencer Configuration: %u",
			(unsigned int)result_status);
		data->conversion_result = -EIO;
		return;
	}

	ifx_autanalog_start_autonomous_control(cfg->mfd);

	{
		uint8_t gpio_ch = IFX_GPIO_CHANNELS_MASK(sequence->channels);
		uint16_t mux_ch = IFX_MUX_CHANNELS_MASK(sequence->channels);

		if (gpio_ch != 0) {
			Cy_AutAnalog_SAR_ClearHSchanResultStatus(0, gpio_ch);
		}
		if (mux_ch != 0) {
			Cy_AutAnalog_SAR_ClearMuxChanResultStatus(0, mux_ch);
		}
	}

#if defined(CONFIG_ADC_ASYNC)
	if (!data->ctx.asynchronous) {
#endif
		/* Wait for conversion to complete */
		{
			uint8_t gpio_ch = IFX_GPIO_CHANNELS_MASK(sequence->channels);
			uint16_t mux_ch = IFX_MUX_CHANNELS_MASK(sequence->channels);
			bool gpio_done, mux_done;

			do {
				gpio_done = (gpio_ch == 0) ||
					    ((Cy_AutAnalog_SAR_GetHSchanResultStatus(0) &
					      gpio_ch) == gpio_ch);
				mux_done = (mux_ch == 0) ||
					   ((Cy_AutAnalog_SAR_GetMuxChanResultStatus(0) & mux_ch) ==
					    mux_ch);
			} while (!gpio_done || !mux_done);
		}

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

	if (sequence->buffer_size < (sizeof(int32_t) * POPCOUNT(sequence->channels))) {
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

	if (data->ctx.asynchronous) {
		uint8_t gpio_ch = IFX_GPIO_CHANNELS_MASK(sequence->channels);
		uint16_t mux_ch = IFX_MUX_CHANNELS_MASK(sequence->channels);
		bool gpio_done = (gpio_ch == 0) ||
				 ((Cy_AutAnalog_SAR_GetHSchanResultStatus(0) & gpio_ch) == gpio_ch);
		bool mux_done = (mux_ch == 0) ||
				((Cy_AutAnalog_SAR_GetMuxChanResultStatus(0) & mux_ch) == mux_ch);

		if (gpio_done && mux_done) {
			if (gpio_ch != 0) {
				Cy_AutAnalog_SAR_ClearHSchanResultStatus(0, gpio_ch);
			}
			if (mux_ch != 0) {
				Cy_AutAnalog_SAR_ClearMuxChanResultStatus(0, mux_ch);
			}
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
static int ifx_autanalog_sar_setup_gpio_channel(struct ifx_autanalog_sar_adc_data *data,
						const struct adc_channel_cfg *channel_cfg,
						uint8_t sample_time_idx)
{
	cy_stc_autanalog_sar_hs_chan_t *pdl_channel;
	uint8_t hw_channel = channel_cfg->channel_id;

	if (hw_channel >= IFX_AUTANALOG_SAR_MAX_GPIO_CHANNELS) {
		LOG_ERR("GPIO channel ID %d out of range (max %d)", hw_channel,
			IFX_AUTANALOG_SAR_MAX_GPIO_CHANNELS - 1);
		return -EINVAL;
	}

	if (channel_cfg->input_positive >= PASS_SAR_SAR_GPIO_CHANNELS) {
		LOG_ERR("Invalid ADC input pin for GPIO channel %d: %d", hw_channel,
			channel_cfg->input_positive);
		return -EINVAL;
	}

	pdl_channel = &data->pdl_adc_hs_channel_cfg_obj_arr[hw_channel];
	data->pdl_adc_hs_static_obj.hsGpioChan[hw_channel] = pdl_channel;

	pdl_channel->posPin = channel_cfg->input_positive;
	pdl_channel->hsDiffEn = false;
	pdl_channel->sign = false;
	pdl_channel->posCoeff = CY_AUTANALOG_SAR_CH_COEFF_DISABLED;
	pdl_channel->negPin = CY_AUTANALOG_SAR_PIN_GPIO0;
	pdl_channel->accShift = false;
	pdl_channel->negCoeff = CY_AUTANALOG_SAR_CH_COEFF_DISABLED;
	pdl_channel->hsLimit = CY_AUTANALOG_SAR_LIMIT_STATUS_DISABLED;
	pdl_channel->fifoSel = CY_AUTANALOG_FIFO_DISABLED;

	data->pdl_adc_hs_static_obj.hsGpioResultMask |= BIT(hw_channel);
	data->autanalog_channel_cfg[channel_cfg->channel_id].sample_time_idx = sample_time_idx;
	if (Cy_AutAnalog_SAR_LoadStaticConfig(0, &data->pdl_adc_top_static_obj) !=
	    CY_AUTANALOG_SUCCESS) {
		data->pdl_adc_hs_static_obj.hsGpioChan[hw_channel] = NULL;
		data->pdl_adc_hs_static_obj.hsGpioResultMask &= ~BIT(hw_channel);
		LOG_ERR("Failed to configure GPIO ADC Channel %d", hw_channel);
		return -EIO;
	}

	data->enabled_channels |= BIT(channel_cfg->channel_id);
	return 0;
}

static int ifx_autanalog_sar_setup_mux_channel(struct ifx_autanalog_sar_adc_data *data,
					       const struct adc_channel_cfg *channel_cfg,
					       uint8_t sample_time_idx)
{
	cy_stc_autanalog_sar_mux_chan_t *pdl_mux_channel;
	uint8_t mux_idx;

	if (channel_cfg->channel_id < IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET) {
		LOG_ERR("MUX channel ID must be >= %d", IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET);
		return -EINVAL;
	}

	mux_idx = channel_cfg->channel_id - IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET;
	if (mux_idx >= IFX_AUTANALOG_SAR_MAX_MUX_CHANNELS) {
		LOG_ERR("MUX channel index %d out of range (max %d)", mux_idx,
			IFX_AUTANALOG_SAR_MAX_MUX_CHANNELS - 1);
		return -EINVAL;
	}

	pdl_mux_channel = &data->pdl_adc_mux_channel_cfg_obj_arr[mux_idx];
	data->pdl_adc_top_static_obj.intMuxChan[mux_idx] = pdl_mux_channel;

	pdl_mux_channel->posPin = (cy_en_autanalog_sar_pin_mux_t)channel_cfg->input_positive;
	pdl_mux_channel->sign = false;
	pdl_mux_channel->posCoeff = CY_AUTANALOG_SAR_CH_COEFF_DISABLED;
	pdl_mux_channel->negPin = (cy_en_autanalog_sar_pin_mux_t)channel_cfg->input_negative;
	pdl_mux_channel->buffBypass =
		data->autanalog_channel_cfg[channel_cfg->channel_id].mux_buf_bypass;
	pdl_mux_channel->accShift = false;
	pdl_mux_channel->negCoeff = CY_AUTANALOG_SAR_CH_COEFF_DISABLED;
	pdl_mux_channel->muxLimit = CY_AUTANALOG_SAR_LIMIT_STATUS_DISABLED;
	pdl_mux_channel->fifoSel = CY_AUTANALOG_FIFO_DISABLED;

	data->pdl_adc_top_static_obj.muxResultMask |= BIT(mux_idx);
	data->autanalog_channel_cfg[channel_cfg->channel_id].sample_time_idx = sample_time_idx;
	if (Cy_AutAnalog_SAR_LoadStaticConfig(0, &data->pdl_adc_top_static_obj) !=
	    CY_AUTANALOG_SUCCESS) {
		data->pdl_adc_top_static_obj.intMuxChan[mux_idx] = NULL;
		data->pdl_adc_top_static_obj.muxResultMask &= ~BIT(mux_idx);
		LOG_ERR("Failed to configure MUX ADC Channel %d (mux idx %d)",
			channel_cfg->channel_id, mux_idx);
		return -EIO;
	}

	data->enabled_channels |= BIT(channel_cfg->channel_id);
	return 0;
}

static int ifx_autanalog_sar_adc_channel_setup(const struct device *dev,
					       const struct adc_channel_cfg *channel_cfg)
{
	struct ifx_autanalog_sar_adc_data *data = dev->data;
	uint8_t sample_time_idx;
	uint16_t timer_clock_cycles;
	bool is_mux_channel;

	if (channel_cfg->channel_id >= IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS) {
		LOG_ERR("Invalid channel ID: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	/* Determine if this is a MUX channel based on channel_id range */
	is_mux_channel = (channel_cfg->channel_id >= IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET);

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

	if (is_mux_channel) {
		return ifx_autanalog_sar_setup_mux_channel(data, channel_cfg, sample_time_idx);
	}

	return ifx_autanalog_sar_setup_gpio_channel(data, channel_cfg, sample_time_idx);
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

	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("AutAnalog MFD device not ready");
		return -ENODEV;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(data->autanalog_channel_cfg); i++) {
		data->autanalog_channel_cfg[i].sample_time_idx = 0xFF;
		data->autanalog_channel_cfg[i].mux_buf_bypass = false;
	}

	data->dev = dev;
	data->enabled_channels = 0;

	/* Pre-populate per-channel DT settings that are not carried in adc_channel_cfg. */
	for (uint8_t i = 0; i < cfg->num_dt_channels; i++) {
		uint8_t ch = cfg->dt_channels[i].channel_id;

		if (ch < IFX_AUTANALOG_SAR_MAX_NUM_CHANNELS) {
			data->autanalog_channel_cfg[ch].mux_buf_bypass =
				cfg->dt_channels[i].mux_buf_bypass;
		}
	}

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
	static DEVICE_API(adc, ifx_autanalog_sar_adc_api_##n) = {                                  \
		.channel_setup = ifx_autanalog_sar_adc_channel_setup,                              \
		.read = ifx_autanalog_sar_adc_read,                                                \
		.read_async = ifx_autanalog_sar_adc_read_async,                                    \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};
#else
#define ADC_IFX_AUTANALOG_SAR_DRIVER_API(n)                                                        \
	static DEVICE_API(adc, ifx_autanalog_sar_adc_api_##n) = {                                  \
		.channel_setup = ifx_autanalog_sar_adc_channel_setup,                              \
		.read = ifx_autanalog_sar_adc_read,                                                \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};
#endif /* CONFIG_ADC_ASYNC */

/* Per-channel DT config: extract channel_id and DT-only channel settings. */
#define IFX_CHAN_DT_CFG_ENTRY(child_node_id)                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(child_node_id, reg),                                         \
		({                                                                                 \
			.channel_id = (uint8_t)DT_REG_ADDR(child_node_id),                         \
			.mux_buf_bypass = DT_PROP_OR(child_node_id, mux_buf_bypass, 0),            \
		},), ())

#define IFX_CHAN_DT_CFG_ARRAY(n)                                                                   \
	static const struct ifx_autanalog_sar_channel_dt_config ifx_sar_chan_dt_cfg_##n[] = {      \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IFX_CHAN_DT_CFG_ENTRY)}

/* Device Instantiation */
#define IFX_AUTANALOG_SAR_ADC_INIT(n)                                                              \
	ADC_IFX_AUTANALOG_SAR_DRIVER_API(n);                                                       \
	static void ifx_autanalog_sar_adc_config_func_##n(void);                                   \
	/* Per-channel DT config array */                                                          \
	IFX_CHAN_DT_CFG_ARRAY(n);                                                                  \
	static void ifx_autanalog_sar_adc_config_func_##n(void);                                   \
	static const struct ifx_autanalog_sar_adc_config ifx_autanalog_sar_adc_config_##n = {      \
		.irq_func = ifx_autanalog_sar_adc_config_func_##n,                                 \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.vref_source = DT_INST_PROP(n, vref_source),                                       \
		.linear_cal = DT_INST_PROP(n, linear_cal),                                         \
		.offset_cal = DT_INST_PROP(n, offset_cal),                                         \
		.pos_buf_pwr = DT_INST_PROP(n, pos_buf_pwr),                                       \
		.neg_buf_pwr = DT_INST_PROP(n, neg_buf_pwr),                                       \
		.acc_mode = DT_INST_PROP(n, acc_mode),                                             \
		.shift_mode = DT_INST_PROP(n, shift_mode),                                         \
		.fifo_chan_id = DT_INST_PROP(n, fifo_chan_id),                                     \
		.num_dt_channels = ARRAY_SIZE(ifx_sar_chan_dt_cfg_##n),                            \
		.dt_channels = ifx_sar_chan_dt_cfg_##n,                                            \
	};                                                                                         \
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
		ifx_autanalog_set_irq_handler(DEVICE_DT_GET(DT_INST_PARENT(n)),                    \
					      DEVICE_DT_INST_GET(n), IFX_AUTANALOG_PERIPH_SAR_ADC, \
					      ifx_autanalog_sar_adc_isr);                          \
	}

DT_INST_FOREACH_STATUS_OKAY(IFX_AUTANALOG_SAR_ADC_INIT)
