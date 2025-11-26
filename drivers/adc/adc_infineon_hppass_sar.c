/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief ADC HPPASS SAR driver
 *
 * Driver for the HPPASS SAR ADC used in the Infineon PSOC C3 series.
 */

#define DT_DRV_COMPAT infineon_hppass_sar_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

#include "ifx_hppass_analog.h"
#include "cy_pdl.h"

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define IFX_HPPASS_SAR_ADC_RESOLUTION (12u) /* ADC Resolution for this device is fixed at 12-bit*/

LOG_MODULE_REGISTER(ifx_hppass_sar_adc, CONFIG_ADC_LOG_LEVEL);

/**
 * @brief HPPASS Configuration Structure
 *
 * Basic configuration for the HPPASS analog subsystem.  By default this structure configures the
 * HPPASS AC to enable the ADC.  Other functions of the HPPASS system are not enabled by default.
 */
const cy_stc_hppass_sar_t ifx_hppass_sar_pdl_cfg_struct_default = {
	.vref = CY_HPPASS_SAR_VREF_EXT,
	.lowSupply = false,
	.offsetCal = false,
	.linearCal = false,
	.gainCal = false,
	.chanId = false,
	.aroute = true,
	.dirSampEnMsk = 0,
	.muxSampEnMsk = 0,
	.holdCount = 29U,
	.dirSampGain = {
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
		},
	.muxSampGain = {
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
			CY_HPPASS_SAR_SAMP_GAIN_1,
		},
	.sampTime = {
			32U,
			32U,
			32U,
		},
	.chan = {
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		},
	.grp = {
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
		},
	.limit = {
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
		},
	.muxMode = {
			CY_HPPASS_SAR_MUX_SEQ,
			CY_HPPASS_SAR_MUX_SEQ,
			CY_HPPASS_SAR_MUX_SEQ,
			CY_HPPASS_SAR_MUX_SEQ,
		},
	.fir = {
			NULL,
			NULL,
		},
	.fifo = NULL,
};

/*
 * Device supports up to 28 channels.  Note that channels 12-15, 16-19, 20-23, and 24-27 are
 * multiplexed in hardware and share samplers.
 */
#define HPPASS_SAR_ADC_MAX_CHANNELS       CY_HPPASS_SAR_CHAN_NUM
#define DIRECT_CHANNEL_CNT                CY_HPPASS_SAR_DIR_SAMP_NUM
#define MUXED_CHANNELS_PER_SAMPLER        4
#define IFX_HPPASS_SAR_SAMPLER_GAIN_MSK   0x03
#define IFX_HPPASS_SAR_SAMPLER_GAIN_WIDTH 2

/*
 * Configuration and data structures
 */
struct ifx_hppass_sar_adc_config {
	uint8_t irq_priority;
	IRQn_Type irq_num;
	void (*irq_func)(void);
	uint16_t dir_samp_en_mask;
	uint16_t mux_samp_en_mask;
	bool vref_internal_source;
	bool gain_cal;
	bool offset_cal;
	bool linear_cal;
};

/**
 * @brief HPPASS SAR ADC channel configuration
 */
struct ifx_hppass_sar_adc_channel_config {
	/** Channel number */
	uint8_t id;
	uint8_t input_positive;
	/*
	 * PDL channel configuration structure.  The PDL will reapply channel configurations for all
	 * channels any time a change is made to any channel configuration.  Store the PDL
	 * configuration for this channel so we have a copy to be used for this action.
	 */
	cy_stc_hppass_sar_chan_t pdl_channel_cfg;
};

/**
 * @brief HPPASS SAR ADC device data
 */
struct ifx_hppass_sar_adc_data {
	/* ADC context for async operations */
	struct adc_context ctx;
	const struct device *dev;
	/*	PDL ADC configuration structure */
	cy_stc_hppass_sar_t hppass_sar_obj;
	/* channel configurations for all channels (used or not)*/
	struct ifx_hppass_sar_adc_channel_config hppass_sar_chan_obj[HPPASS_SAR_ADC_MAX_CHANNELS];
	/* Bitmask of enabled channels */
	uint32_t enabled_channels;
	/* Conversion buffer */
	uint16_t *buffer;
	/* Repeat buffer for continuous sampling */
	uint16_t *repeat_buffer;
	/** Conversion result */
	int result;
};

/*
 * ADC Channels 12-28 are grouped together in hardware using a mux.  The grouping is as follows:
 * Sampler 12: Channels 12-15,
 * Sampler 13: Channels 16-19,
 * Sampler 14: Channels 20-23,
 * Sampler 15: Channels 24-27
 */
#define ADC_SAMPLER_12_CHANNEL_GROUP 0x0000F000
#define ADC_SAMPLER_13_CHANNEL_GROUP 0x000F0000
#define ADC_SAMPLER_14_CHANNEL_GROUP 0x00F00000
#define ADC_SAMPLER_15_CHANNEL_GROUP 0x0F000000
#define ADC_SAMPLER_DIRECT_MASK      0x0FFF

/**
 * @brief Configure HPPASS SAR ADC group
 *
 * @param channels Bitmask of channels to be enabled in the group
 * @param group Group number to configure (0-7)
 *
 * HPPASS SAR ADC has 8 groups.  ADC samplers can be added to a group, and will be sampled
 * simultaneously and converted sequentially when the group is triggered.  Note that only one MUXed
 * channel can be included in a mux group.
 */
static int ifx_hppass_sar_configure_group(uint32_t channels, uint32_t group)
{
	cy_stc_hppass_sar_grp_t group_cfg = {0};

	/* Check that no more than one channel is selected from each muxed group */
	if (POPCOUNT(channels & ADC_SAMPLER_12_CHANNEL_GROUP) > 1 ||
	    POPCOUNT(channels & ADC_SAMPLER_13_CHANNEL_GROUP) > 1 ||
	    POPCOUNT(channels & ADC_SAMPLER_14_CHANNEL_GROUP) > 1 ||
	    POPCOUNT(channels & ADC_SAMPLER_15_CHANNEL_GROUP) > 1) {

		return -EINVAL;
	}

	group_cfg.trig = CY_HPPASS_SAR_TRIG_0; /* TRIG_0 used for SW Trigger */
	group_cfg.sampTime = CY_HPPASS_SAR_SAMP_TIME_DISABLED;

	/* Enable directly sampled channels. */
	group_cfg.dirSampMsk = channels & ADC_SAMPLER_DIRECT_MASK;

	/* Enable Muxed channels.  We need to determine if each sampler is enabled and what the mux
	 * should be set to for the sampler.
	 */
	group_cfg.muxSampMsk = 0;
	for (int channel_num = DIRECT_CHANNEL_CNT; channel_num < HPPASS_SAR_ADC_MAX_CHANNELS;
	     channel_num++) {
		if (channels & (1 << channel_num)) {
			int sampler_num =
				(channel_num - DIRECT_CHANNEL_CNT) / MUXED_CHANNELS_PER_SAMPLER;
			int mux_setting =
				(channel_num - DIRECT_CHANNEL_CNT) % MUXED_CHANNELS_PER_SAMPLER;
			group_cfg.muxSampMsk |= (1 << sampler_num);
			group_cfg.muxChanIdx[sampler_num] = mux_setting;
		}
	}

	if (Cy_HPPASS_SAR_GroupConfig(group, &group_cfg) != 0) {
		LOG_ERR("ADC Group configuration failed");
		return -EINVAL;
	}

	/* CrossTalkAdjust must be called any time groups are reconfigured. */
	Cy_HPPASS_SAR_CrossTalkAdjust((uint8_t)1U << group);
	return 0;
}

/**
 * @brief Read results of the specified group of channels
 *
 * @param channels Bitmask of channels to read results for
 * @param data Pointer to ADC data structure
 *
 * Helper function to read all the results for the specified channels into the data buffer.
 */
static void ifx_hppass_get_group_results(uint32_t channels, struct ifx_hppass_sar_adc_data *data)
{
	if (data->buffer == NULL) {
		LOG_ERR("ADC data buffer is NULL");
		return;
	}

	for (size_t i = 0; i < HPPASS_SAR_ADC_MAX_CHANNELS; i++) {
		if (channels & (1 << i)) {
			int16_t result = Cy_HPPASS_SAR_Result_ChannelRead(i);
			*data->buffer++ = result;
		}
	}
}

/**
 * @brief Start ADC conversion
 *
 * @param ctx ADC context
 *
 * The HPPASS SAR ADC uses a grouping functionality to simultaneously sample then convert multiple
 * channels with one trigger input.  All channels in the ADC Sequence are added to a group and a
 * conversion is triggered.
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ifx_hppass_sar_adc_data *data =
		CONTAINER_OF(ctx, struct ifx_hppass_sar_adc_data, ctx);
	const struct adc_sequence *sequence = &ctx->sequence;
	uint32_t result_status;

	data->repeat_buffer = data->buffer;
	if (data->buffer == NULL || sequence->buffer_size == 0) {
		data->result = -ENOMEM;
		return;
	}

	if (sequence->channels == 0) {
		LOG_ERR("No channels specified");
		data->result = -EINVAL;
	} else if (ifx_hppass_sar_configure_group(sequence->channels, 0) != 0) {
		LOG_ERR("Invalid channel group selection");
		data->result = -EINVAL;
	} else {
		/* Trigger SAR ADC group 0 conversion */
		Cy_HPPASS_SAR_Result_ClearStatus(sequence->channels);
		Cy_HPPASS_SetFwTrigger(CY_HPPASS_TRIG_0_MSK);

#if defined(CONFIG_ADC_ASYNC)
		if (!data->ctx.asynchronous) {
#endif /* CONFIG_ADC_ASYNC */
			/* Wait for channel conversion done */
			do {
				result_status = Cy_HPPASS_SAR_Result_GetStatus();
			} while ((result_status & sequence->channels) != sequence->channels);

			ifx_hppass_get_group_results(sequence->channels, data);
			adc_context_on_sampling_done(&data->ctx, data->dev);
#if defined(CONFIG_ADC_ASYNC)
		}
#endif /* CONFIG_ADC_ASYNC */

		data->result = 0;
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ifx_hppass_sar_adc_data *data =
		CONTAINER_OF(ctx, struct ifx_hppass_sar_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/**
 * @brief Start read operation
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param sequence Pointer to the adc_sequence structure.
 *
 * This function validates the read parameters, sets up the buffer, and initiates the read
 * operation using the ADC context.
 */
static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ifx_hppass_sar_adc_data *data = dev->data;

	if (sequence->buffer_size < (sizeof(int16_t) * POPCOUNT(sequence->channels))) {
		LOG_ERR("Buffer too small");
		return -ENOMEM;
	}

	if (sequence->resolution != IFX_HPPASS_SAR_ADC_RESOLUTION) {
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

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

/**
 * @brief ADC interrupt handler
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * Interrupt Handler for the combined group results interrupt.  This handler is common all group
 * completion interrupts.  Individual group completion interrupts are available if needed for more
 * advanced ADC control.
 */
static void ifx_hppass_sar_adc_isr(const struct device *dev)
{
#if defined(CONFIG_ADC_ASYNC)
	struct ifx_hppass_sar_adc_data *data = dev->data;
#else
	ARG_UNUSED(dev);
#endif /* CONFIG_ADC_ASYNC */
	LOG_DBG("SAR ADC combined results interrupt");

	/* Check which SAR result groups have completed */
	uint32_t result_intr_status = Cy_HPPASS_SAR_Result_GetInterruptStatusMasked();

	/* Clear the specific SAR result interrupts that fired */
	Cy_HPPASS_SAR_Result_ClearInterrupt(result_intr_status);

	/* Check if Group 0 completed (which is what we're using) */
	if (result_intr_status & CY_HPPASS_INTR_SAR_RESULT_GROUP_0) {
		LOG_DBG("SAR Group 0 conversion complete");

#if defined(CONFIG_ADC_ASYNC)
		if (data->ctx.asynchronous) {
			const struct adc_sequence *sequence = &data->ctx.sequence;
			uint32_t result_status = Cy_HPPASS_SAR_Result_GetStatus();

			/* Make sure all requested channels have completed */
			if ((result_status & sequence->channels) == sequence->channels) {
				ifx_hppass_get_group_results(sequence->channels, data);
				/* Clear the result status for the channels we read */
				Cy_HPPASS_SAR_Result_ClearStatus(result_status &
								 sequence->channels);

				adc_context_on_sampling_done(&data->ctx, dev);
			} else {
				/*
				 * Not all channels have completed yet.  This shouldn't happen, if
				 * configured correctly all channels in the group will be complete
				 * when this interrupt occurs.
				 */
				LOG_ERR("SAR Group 0: Not all channels completed.");
			}
		}
#endif /* CONFIG_ADC_ASYNC */
	}

	/*
	 * This implementation only uses Group 0. Any other interrupts indicates a configuration
	 * error.
	 */
	if (result_intr_status & ~CY_HPPASS_INTR_SAR_RESULT_GROUP_0) {
		LOG_ERR("SAR Results Interrupt for unhandled groups: 0x%08X",
			(uint32_t)(result_intr_status & ~CY_HPPASS_INTR_SAR_RESULT_GROUP_0));
	}
}

/**
 * @brief Initialize pdl adc configuration structure
 *
 * This function initializes the pdl adc configuration with values derived from the device tree and
 * other default values.  Channel and Group configurations are set to NULL intially.  Channels will
 * be configured later in the channel setup function.  Groups are configured when a conversion is
 * started.
 */
static void ifx_init_pdl_struct(struct ifx_hppass_sar_adc_data *data,
				const struct ifx_hppass_sar_adc_config *cfg)
{
	data->hppass_sar_obj = ifx_hppass_sar_pdl_cfg_struct_default;
	data->hppass_sar_obj.vref =
		cfg->vref_internal_source ? CY_HPPASS_SAR_VREF_VDDA : CY_HPPASS_SAR_VREF_EXT;
	data->hppass_sar_obj.offsetCal = cfg->offset_cal;
	data->hppass_sar_obj.linearCal = cfg->linear_cal;
	data->hppass_sar_obj.gainCal = cfg->gain_cal;
	data->hppass_sar_obj.dirSampEnMsk = cfg->dir_samp_en_mask;
	data->hppass_sar_obj.muxSampEnMsk = cfg->mux_samp_en_mask;
}

/**
 * @brief Initialize channel configuration structures
 *
 * This function initializes the channel configuration structures with default values.  All
 * channels are initially disabled.  Channels will be enabled and configured in the channel setup
 * function.
 */
static void ifx_init_channel_cfg(struct ifx_hppass_sar_adc_data *data)
{
	for (int i = 0; i < HPPASS_SAR_ADC_MAX_CHANNELS; i++) {
		data->hppass_sar_chan_obj[i] = (struct ifx_hppass_sar_adc_channel_config){
			.id = (uint8_t)i,
			.input_positive = 0,
			.pdl_channel_cfg =
				(cy_stc_hppass_sar_chan_t){
					.diff = false,
					.sign = false,
					.avg = CY_HPPASS_SAR_AVG_DISABLED,
					.limit = CY_HPPASS_SAR_LIMIT_DISABLED,
					.result = false,
					.fifo = CY_HPPASS_FIFO_DISABLED,
				},
		};

		data->hppass_sar_obj.chan[i] = NULL;
	}
}

/*
 * Zephyr Driver API Functions
 */

/**
 * @brief ADC read implementation
 */
static int ifx_hppass_sar_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ifx_hppass_sar_adc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

/**
 * @brief ADC read async implementation
 */
#ifdef CONFIG_ADC_ASYNC
static int ifx_hppass_sar_adc_read_async(const struct device *dev,
					 const struct adc_sequence *sequence,
					 struct k_poll_signal *async)
{
	struct ifx_hppass_sar_adc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);
	return ret;
}
#endif

/**
 * @brief Configure ADC channel
 *
 * Implements the Zephyr ADC channel configuration API.
 */
static int ifx_hppass_sar_adc_channel_setup(const struct device *dev,
					    const struct adc_channel_cfg *channel_cfg)
{
	struct ifx_hppass_sar_adc_data *data = dev->data;

	if (channel_cfg->channel_id >= HPPASS_SAR_ADC_MAX_CHANNELS) {
		LOG_ERR("Invalid channel ID: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1 && channel_cfg->gain != ADC_GAIN_3 &&
	    channel_cfg->gain != ADC_GAIN_6 && channel_cfg->gain != ADC_GAIN_12) {
		LOG_ERR("Gain setting not supported");
		return -EINVAL;
	}

	/*
	 * The HPPASS SAR hardware block does not support setting the reference individually per
	 * channel.  The device can select internal or external reference that will apply to all ADC
	 * channels.
	 */
	if (channel_cfg->reference != ADC_REF_INTERNAL &&
	    channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("Reference setting not supported");
		return -EINVAL;
	}

	/*
	 * The HPPASS SAR Hardware block does not support setting acquisition time per channel.  The
	 * device has three sampling time configuration registers.  These registers are used to
	 * configure the sample time for a group rather than individual channels.
	 */
	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid channel acquisition time, expected ADC_ACQ_TIME_DEFAULT");
		return -EINVAL;
	}

	data->hppass_sar_chan_obj[channel_cfg->channel_id].id = channel_cfg->channel_id;
	data->hppass_sar_chan_obj[channel_cfg->channel_id].pdl_channel_cfg =
		(cy_stc_hppass_sar_chan_t){
			.diff = (channel_cfg->differential == 0) ? false : true,
			.sign = false,
			.avg = CY_HPPASS_SAR_AVG_DISABLED,
			.limit = CY_HPPASS_SAR_LIMIT_DISABLED,
			.result = true,
			.fifo = CY_HPPASS_FIFO_DISABLED,
		};

	data->hppass_sar_obj.chan[channel_cfg->channel_id] =
		&data->hppass_sar_chan_obj[channel_cfg->channel_id].pdl_channel_cfg;

	if (Cy_HPPASS_SAR_ChannelConfig(
		    channel_cfg->channel_id,
		    &data->hppass_sar_chan_obj[channel_cfg->channel_id].pdl_channel_cfg) !=
	    CY_HPPASS_SUCCESS) {
		LOG_ERR("Channel %d configuration failed", channel_cfg->channel_id);
		return -EIO;
	}

	/*
	 * PDL doesn't support configuring gain except during device initialization.  Initialize the
	 * gain registers directly here.
	 */
	uint32_t sampler_gain;

	HPPASS_SAR_SAMP_GAIN(HPPASS_BASE) &=
		~(IFX_HPPASS_SAR_SAMPLER_GAIN_MSK
		  << (channel_cfg->channel_id * IFX_HPPASS_SAR_SAMPLER_GAIN_WIDTH));
	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		sampler_gain = 0;
		break;
	case ADC_GAIN_3:
		sampler_gain = 1;
		break;
	case ADC_GAIN_6:
		sampler_gain = 2;
		break;
	case ADC_GAIN_12:
		sampler_gain = 3;
		break;
	default:
		sampler_gain = 0;
		break;
	}

	HPPASS_SAR_SAMP_GAIN(HPPASS_BASE) |=
		(sampler_gain << (channel_cfg->channel_id * IFX_HPPASS_SAR_SAMPLER_GAIN_WIDTH));

	data->enabled_channels |= BIT(channel_cfg->channel_id);

	return 0;
}

/**
 * @brief Initialize ADC device
 */
static int ifx_hppass_sar_adc_init(const struct device *dev)
{
	const struct ifx_hppass_sar_adc_config *cfg = dev->config;
	struct ifx_hppass_sar_adc_data *data = dev->data;

	data->dev = dev;

	LOG_DBG("Initializing HPPASS SAR ADC");

	/*
	 * Initialize the data structure.  The data structure contains a pdl device initialization
	 * object which we store to be able to reinitialize the ADC if needed.
	 */
	ifx_init_pdl_struct(data, cfg);
	ifx_init_channel_cfg(data);

	if (Cy_HPPASS_SAR_Init(&data->hppass_sar_obj) != CY_RSLT_SUCCESS) {
		LOG_ERR("Failed to initialize HPPASS SAR ADC");
		return -EIO;
	}

	if (ifx_hppass_ac_init_adc() != CY_RSLT_SUCCESS) {
		LOG_ERR("HPPASS AC failed to initialize ADC");
		return -EIO;
	}

#if defined(CONFIG_ADC_ASYNC)
	Cy_HPPASS_SAR_Result_SetInterruptMask(CY_HPPASS_INTR_SAR_RESULT_GROUP_0);
	cfg->irq_func();
#endif /* CONFIG_ADC_ASYNC */

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
#define ADC_IFX_HPPASS_SAR_DRIVER_API(n)                                                           \
	static DEVICE_API(adc, adc_ifx_hppass_sar_driver_api_##n) = {                              \
		.channel_setup = ifx_hppass_sar_adc_channel_setup,                                 \
		.read = ifx_hppass_sar_adc_read,                                                   \
		.read_async = ifx_hppass_sar_adc_read_async,                                       \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};
#else
#define ADC_IFX_HPPASS_SAR_DRIVER_API(n)                                                           \
	static DEVICE_API(adc, adc_ifx_hppass_sar_driver_api_##n) = {                              \
		.channel_setup = ifx_hppass_sar_adc_channel_setup,                                 \
		.read = ifx_hppass_sar_adc_read,                                                   \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};
#endif

/*
 * Devicetree channel mask generation
 *
 * dir_samp_en_mask:
 *   One bit per direct sampler channel (0..11) that has a child node.
 *
 * mux_samp_en_mask:
 *   One bit per mux sampler group:
 *     Bit0 -> any of channels 12..15 present
 *     Bit1 -> any of channels 16..19 present
 *     Bit2 -> any of channels 20..23 present
 *     Bit3 -> any of channels 24..27 present
 */

#define IFX_HPPASS_SAR_CH_EXISTS(inst, ch) DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(inst), channel_##ch))

/* Direct sampler bitmap (0..11) */
#define IFX_HPPASS_SAR_DIR_MASK(inst)                                                              \
	((IFX_HPPASS_SAR_CH_EXISTS(inst, 0) ? BIT(0) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 1) ? BIT(1) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 2) ? BIT(2) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 3) ? BIT(3) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 4) ? BIT(4) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 5) ? BIT(5) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 6) ? BIT(6) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 7) ? BIT(7) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 8) ? BIT(8) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 9) ? BIT(9) : 0) |                                        \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 10) ? BIT(10) : 0) |                                      \
	 (IFX_HPPASS_SAR_CH_EXISTS(inst, 11) ? BIT(11) : 0))

/* Group presence helpers */
#define IFX_HPPASS_SAR_GRP0_PRESENT(inst)                                                          \
	(IFX_HPPASS_SAR_CH_EXISTS(inst, 12) || IFX_HPPASS_SAR_CH_EXISTS(inst, 13) ||               \
	 IFX_HPPASS_SAR_CH_EXISTS(inst, 14) || IFX_HPPASS_SAR_CH_EXISTS(inst, 15))

#define IFX_HPPASS_SAR_GRP1_PRESENT(inst)                                                          \
	(IFX_HPPASS_SAR_CH_EXISTS(inst, 16) || IFX_HPPASS_SAR_CH_EXISTS(inst, 17) ||               \
	 IFX_HPPASS_SAR_CH_EXISTS(inst, 18) || IFX_HPPASS_SAR_CH_EXISTS(inst, 19))

#define IFX_HPPASS_SAR_GRP2_PRESENT(inst)                                                          \
	(IFX_HPPASS_SAR_CH_EXISTS(inst, 20) || IFX_HPPASS_SAR_CH_EXISTS(inst, 21) ||               \
	 IFX_HPPASS_SAR_CH_EXISTS(inst, 22) || IFX_HPPASS_SAR_CH_EXISTS(inst, 23))

#define IFX_HPPASS_SAR_GRP3_PRESENT(inst)                                                          \
	(IFX_HPPASS_SAR_CH_EXISTS(inst, 24) || IFX_HPPASS_SAR_CH_EXISTS(inst, 25) ||               \
	 IFX_HPPASS_SAR_CH_EXISTS(inst, 26) || IFX_HPPASS_SAR_CH_EXISTS(inst, 27))

/* Mux sampler enable mask (bit per group if any channel in that group exists) */
#define IFX_HPPASS_SAR_MUX_MASK(inst)                                                              \
	((IFX_HPPASS_SAR_GRP0_PRESENT(inst) ? BIT(0) : 0) |                                        \
	 (IFX_HPPASS_SAR_GRP1_PRESENT(inst) ? BIT(1) : 0) |                                        \
	 (IFX_HPPASS_SAR_GRP2_PRESENT(inst) ? BIT(2) : 0) |                                        \
	 (IFX_HPPASS_SAR_GRP3_PRESENT(inst) ? BIT(3) : 0))

/* Device instantiation */
#define IFX_HPPASS_SAR_ADC_INIT(n)                                                                 \
	ADC_IFX_HPPASS_SAR_DRIVER_API(n);                                                          \
	static void ifx_hppass_sar_adc_config_func_##n(void);                                      \
	static const struct ifx_hppass_sar_adc_config ifx_hppass_sar_adc_config_##n = {            \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.irq_func = ifx_hppass_sar_adc_config_func_##n,                                    \
		.dir_samp_en_mask = IFX_HPPASS_SAR_DIR_MASK(n),                                    \
		.mux_samp_en_mask = IFX_HPPASS_SAR_MUX_MASK(n),                                    \
		.vref_internal_source = DT_INST_PROP(n, ref_internal_source),                      \
		.gain_cal = DT_INST_PROP(n, gain_cal),                                             \
		.offset_cal = DT_INST_PROP(n, offset_cal),                                         \
		.linear_cal = DT_INST_PROP(n, linear_cal)};                                        \
	static struct ifx_hppass_sar_adc_data ifx_hppass_sar_adc_data_##n = {                      \
		ADC_CONTEXT_INIT_TIMER(ifx_hppass_sar_adc_data_##n, ctx),                          \
		ADC_CONTEXT_INIT_LOCK(ifx_hppass_sar_adc_data_##n, ctx),                           \
		ADC_CONTEXT_INIT_SYNC(ifx_hppass_sar_adc_data_##n, ctx),                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &ifx_hppass_sar_adc_init, NULL, &ifx_hppass_sar_adc_data_##n,     \
			      &ifx_hppass_sar_adc_config_##n, POST_KERNEL,                         \
			      CONFIG_ADC_INFINEON_HPPASS_SAR_INIT_PRIORITY,                        \
			      &adc_ifx_hppass_sar_driver_api_##n);                                 \
	static void ifx_hppass_sar_adc_config_func_##n(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ifx_hppass_sar_adc_isr,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(IFX_HPPASS_SAR_ADC_INIT)
