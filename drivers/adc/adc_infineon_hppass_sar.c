/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief HPPASS SAR ADC driver for PSoC C3.
 *
 * 12-bit fixed resolution ADC, software-triggered via FW pulse.  Uses Group 0
 * for all reads.  Async mode completes via SAR result interrupt.
 *
 * HPPASS is shared across multiple Infineon device families. The TRM
 * citations throughout this file refer to the PSOC Control C3 Mainline
 * documentation:
 *   - <em>PSOC Control C3 Mainline Architecture TRM</em>, 002-39348
 *     (§27.2.2 SAR ADC)
 *   - <em>PSOC Control C3 Mainline Registers TRM</em>, 002-39445
 *     (HPPASS_SAR_*)
 */

#define DT_DRV_COMPAT infineon_hppass_sar_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>

#include <infineon_hppass.h>
#include "cy_hppass_sar.h"

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define IFX_HPPASS_SAR_ADC_RESOLUTION (12u)

LOG_MODULE_REGISTER(ifx_hppass_sar_adc, CONFIG_ADC_LOG_LEVEL);

/**
 * @brief HPPASS Configuration Structure
 *
 * Default arg for the Cy_HPPASS_SAR_Init() call.  Per-channel programming goes through
 * SAR_CHAN_CFG, SAMP_GAIN, and RESULT_MASK directly.  Unsupported fields (sampTime, fir, fifo,
 * limit, chan/grp, lowSupply, chanId, holdCount) are set to PDL-recommended values.
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
 * Device supports 28 channels.
 * Channels 12..27 are multiplexed: samplers 12-15 each select one of four
 * channels (12-15, 16-19, 20-23, 24-27).
 */
#define HPPASS_SAR_ADC_MAX_CHANNELS       CY_HPPASS_SAR_CHAN_NUM
#define DIRECT_CHANNEL_CNT                CY_HPPASS_SAR_DIR_SAMP_NUM
#define MUXED_CHANNELS_PER_SAMPLER \
	((CY_HPPASS_SAR_CHAN_NUM - CY_HPPASS_SAR_DIR_SAMP_NUM) / CY_HPPASS_SAR_MUX_SAMP_NUM)
#define IFX_HPPASS_SAR_SAMPLER_GAIN_MSK   0x03
#define IFX_HPPASS_SAR_SAMPLER_GAIN_WIDTH 2

/* 28-channel mask, OR'd into CFG_RESULT_MASK (Regs TRM §20.1.18). */
#define IFX_HPPASS_SAR_CHAN_MSK            ((1UL << HPPASS_SAR_ADC_MAX_CHANNELS) - 1UL)
/* Group 0 done bit, bit 0 of the ENTRY_DONE field in CFG_SAR_RESULT_INTR/_SET/_MASK/_MASKED
 * (Regs TRM §20.1.26-29).  Derived from the IP register header so the bit position tracks
 * silicon revisions rather than a hand-coded literal.
 */
#define IFX_HPPASS_SAR_INTR_RESULT_GROUP_0 BIT(HPPASS_SAR_CFG_SAR_RESULT_INTR_ENTRY_DONE_Pos)

/*
 * SAR controller register offsets relative to the SAR controller reg
 * region (HPPASS_SAR_Type), wired in via cfg->ctrl_base.  The FwGen
 * HPPASS_SAR_*() accessor macros key off the parent HPPASS base and
 * re-add the SAR controller offset, so they cannot address this region
 * directly; access it with sys_read32()/sys_write32() instead.
 */
#define IFX_HPPASS_SAR_SEQ_ENTRY(grp)     (0x100U + (grp) * 4U) /* SAR.SEQ_ENTRY[grp] */
#define IFX_HPPASS_SAR_SAMP_GAIN          0x408U                /* SAR.CFG.SAMP_GAIN */
#define IFX_HPPASS_SAR_CHAN_CFG(ch)       (0x430U + (ch) * 4U)  /* SAR.CFG.CHAN_CFG[ch] */
#define IFX_HPPASS_SAR_CHAN_RESULT(ch)    (0x510U + (ch) * 4U)  /* SAR.CFG.CHAN_RESULT[ch] */
#define IFX_HPPASS_SAR_RESULT_MASK        0x590U                /* SAR.CFG.RESULT_MASK */
#define IFX_HPPASS_SAR_RESULT_UPDATED     0x594U                /* SAR.CFG.RESULT_UPDATED */
#define IFX_HPPASS_SAR_RESULT_INTR        0x600U                /* SAR.CFG.SAR_RESULT_INTR */
#define IFX_HPPASS_SAR_RESULT_INTR_MASK   0x608U                /* SAR.CFG.SAR_RESULT_INTR_MASK */
#define IFX_HPPASS_SAR_RESULT_INTR_MASKED 0x60CU                /* SAR.CFG.SAR_RESULT_INTR_MASKED */

/*
 * Field masks for SAR registers written as multi-field aggregates.
 * Values are composed with FIELD_PREP(); PSC3 runs little-endian (CFGEND=0).
 */

/* SEQ_ENTRY0: per-group sampler enables, mux select, trigger source (Regs TRM 002-39445 §20.1.5).
 */
#define IFX_HPPASS_SAR_SEQ_DIRECT_SAMPLER_EN GENMASK(11, 0)  /* per direct sampler */
#define IFX_HPPASS_SAR_SEQ_MUXED_SAMPLER_EN  GENMASK(15, 12) /* per mux sampler */
#define IFX_HPPASS_SAR_SEQ_MUX0_SEL          GENMASK(17, 16) /* mux 0 channel select */
#define IFX_HPPASS_SAR_SEQ_MUX1_SEL          GENMASK(19, 18) /* mux 1 channel select */
#define IFX_HPPASS_SAR_SEQ_MUX2_SEL          GENMASK(21, 20) /* mux 2 channel select */
#define IFX_HPPASS_SAR_SEQ_MUX3_SEL          GENMASK(23, 22) /* mux 3 channel select */
#define IFX_HPPASS_SAR_SEQ_TR_SEL            GENMASK(27, 24) /* trigger source */
#define IFX_HPPASS_SAR_SEQ_SAMP_TIME_SEL     GENMASK(29, 28) /* sample-time gen */
#define IFX_HPPASS_SAR_SEQ_PRIORITY          BIT(30)         /* group priority */
#define IFX_HPPASS_SAR_SEQ_CONT              BIT(31)         /* continuous conversion */

/* CFG_CHAN_CFG0: per-channel differential/signed/range/avg/FIFO (Regs TRM 002-39445 §20.1.12). */
#define IFX_HPPASS_SAR_CHAN_CFG_DIFFERENTIAL BIT(0)          /* differential input mode */
#define IFX_HPPASS_SAR_CHAN_CFG_IS_SIGNED    BIT(1)          /* signed result format */
#define IFX_HPPASS_SAR_CHAN_CFG_RANGE_SEL    GENMASK(23, 20) /* input range select */
#define IFX_HPPASS_SAR_CHAN_CFG_AVG_SEL      GENMASK(26, 24) /* averaging select */
#define IFX_HPPASS_SAR_CHAN_CFG_FIFO_SEL     GENMASK(30, 28) /* result FIFO select */

/*
 * Configuration and data structures
 */
struct ifx_hppass_sar_adc_config {
	const struct device *mfd;
	/*
	 * SAR controller register base (HPPASS_SAR).  All direct register
	 * access in this driver goes through here.  Calibration registers
	 * (HPPASS_SARADC) are owned by the HPPASS MFD parent.
	 */
	mem_addr_t ctrl_base;
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
 * @brief HPPASS SAR ADC device data
 */
struct ifx_hppass_sar_adc_data {
	struct adc_context ctx;
	const struct device *dev;
	uint32_t enabled_channels;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
};

/*
 * ADC Channels 12-28 are grouped together in hardware using a mux.  The groupings are:
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
 * Configure SAR_SEQ_GROUP[group] to fire the requested channels on the next trigger.
 * Each mux sampler can select only one of its four inputs; returns -EINVAL if more than
 * one channel per mux group is requested.
 */
static int ifx_hppass_sar_configure_group(mem_addr_t ctrl_base, uint32_t channels, uint32_t group)
{
	uint32_t mux_samp_msk = 0;
	uint32_t mux_sel[4] = {0};

	/* Check that no more than one channel is selected from each muxed group */
	if (POPCOUNT(channels & ADC_SAMPLER_12_CHANNEL_GROUP) > 1 ||
	    POPCOUNT(channels & ADC_SAMPLER_13_CHANNEL_GROUP) > 1 ||
	    POPCOUNT(channels & ADC_SAMPLER_14_CHANNEL_GROUP) > 1 ||
	    POPCOUNT(channels & ADC_SAMPLER_15_CHANNEL_GROUP) > 1) {

		return -EINVAL;
	}

	/* Determine which muxed samplers are needed and their mux select values. */
	for (int channel_num = DIRECT_CHANNEL_CNT; channel_num < HPPASS_SAR_ADC_MAX_CHANNELS;
	     channel_num++) {
		if (channels & (1 << channel_num)) {
			int sampler_num =
				(channel_num - DIRECT_CHANNEL_CNT) / MUXED_CHANNELS_PER_SAMPLER;
			int mux_setting =
				(channel_num - DIRECT_CHANNEL_CNT) % MUXED_CHANNELS_PER_SAMPLER;
			mux_samp_msk |= (1 << sampler_num);
			mux_sel[sampler_num] = mux_setting;
		}
	}

	/* Single-shot conversion via SEQ_ENTRY0 (Regs TRM §20.1.5). */
	uint32_t seq_entry_reg =
		FIELD_PREP(IFX_HPPASS_SAR_SEQ_DIRECT_SAMPLER_EN,
			   channels & ADC_SAMPLER_DIRECT_MASK) |
		FIELD_PREP(IFX_HPPASS_SAR_SEQ_MUXED_SAMPLER_EN, mux_samp_msk) |
		FIELD_PREP(IFX_HPPASS_SAR_SEQ_MUX0_SEL, mux_sel[0]) |
		FIELD_PREP(IFX_HPPASS_SAR_SEQ_MUX1_SEL, mux_sel[1]) |
		FIELD_PREP(IFX_HPPASS_SAR_SEQ_MUX2_SEL, mux_sel[2]) |
		FIELD_PREP(IFX_HPPASS_SAR_SEQ_MUX3_SEL, mux_sel[3]) |
		/* MFD wires TR0 to FW_PULSE via trig-in-0-type */
		FIELD_PREP(IFX_HPPASS_SAR_SEQ_TR_SEL, CY_HPPASS_SAR_TRIG_0);

	sys_write32(seq_entry_reg, ctrl_base + IFX_HPPASS_SAR_SEQ_ENTRY(group));

	/*
	 * Cross-talk compensation: adjusts per-sampler offset trim based on
	 * the number of simultaneously active samplers.  Must be called each
	 * time the active sampler set changes (per cy_hppass_sar.h:1172-1177).
	 * Reads calOffsetMirror[] populated by Cy_HPPASS_SAR_Init().
	 */
	Cy_HPPASS_SAR_CrossTalkAdjust((uint8_t)1U << group);
	return 0;
}

/**
 * @brief Read results of the specified group of channels
 *
 * @param channels Bitmask of channels to read results for
 * @param data Pointer to ADC data structure
 *
 * Reads all results for the specified channels into the data buffer.
 */
static void ifx_hppass_get_group_results(mem_addr_t ctrl_base, uint32_t channels,
					 struct ifx_hppass_sar_adc_data *data)
{
	if (data->buffer == NULL) {
		LOG_ERR("ADC data buffer is NULL");
		return;
	}

	for (size_t i = 0; i < HPPASS_SAR_ADC_MAX_CHANNELS; i++) {
		if (channels & (1 << i)) {
			int16_t result =
				(int16_t)sys_read32(ctrl_base + IFX_HPPASS_SAR_CHAN_RESULT(i));
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
	const struct ifx_hppass_sar_adc_config *cfg = data->dev->config;
	mem_addr_t ctrl_base = cfg->ctrl_base;
	const struct adc_sequence *sequence = &ctx->sequence;
	uint32_t result_status;

	data->repeat_buffer = data->buffer;
	if (data->buffer == NULL || sequence->buffer_size == 0) {
		adc_context_complete(&data->ctx, -ENOMEM);
		return;
	}

	if (sequence->channels == 0) {
		LOG_ERR("No channels specified");
		adc_context_complete(&data->ctx, -EINVAL);
	} else if (ifx_hppass_sar_configure_group(ctrl_base, sequence->channels, 0) != 0) {
		LOG_ERR("Invalid channel group selection");
		adc_context_complete(&data->ctx, -EINVAL);
	} else {
		/* Clear previous results before triggering the next conversion. */
		sys_write32(sequence->channels & IFX_HPPASS_SAR_CHAN_MSK,
			    ctrl_base + IFX_HPPASS_SAR_RESULT_UPDATED);
		ifx_hppass_ac_set_fw_trigger_pulse(cfg->mfd, IFX_HPPASS_TRIG_0_MSK);

#if defined(CONFIG_ADC_ASYNC)
		if (!data->ctx.asynchronous) {
#endif /* CONFIG_ADC_ASYNC */
			/*
			 * A synchronous conversion completes only once the MFD
			 * parent's autonomous controller is running and the
			 * firmware trigger reaches this group, both outside this
			 * driver's control.  Bound the poll so a stalled or
			 * mis-routed trigger fails the read with -ETIMEDOUT
			 * instead of spinning the calling thread forever.
			 */
			bool completed = false;

			for (uint32_t i = 0U;
			     i < CONFIG_ADC_INFINEON_HPPASS_SAR_POLL_TIMEOUT_US; i++) {
				result_status =
					sys_read32(ctrl_base + IFX_HPPASS_SAR_RESULT_UPDATED);
				if ((result_status & sequence->channels) ==
				    sequence->channels) {
					completed = true;
					break;
				}
				k_busy_wait(1);
			}

			if (!completed) {
				LOG_ERR("SAR conversion timed out (AC not running or "
					"trigger not routed?)");
				adc_context_complete(&data->ctx, -ETIMEDOUT);
				return;
			}

			ifx_hppass_get_group_results(ctrl_base, sequence->channels, data);
			adc_context_on_sampling_done(&data->ctx, data->dev);
#if defined(CONFIG_ADC_ASYNC)
		}
#endif /* CONFIG_ADC_ASYNC */
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
 * Validates the read parameters, sets buffer pointer, and calls adc_context_start_read().
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
	const struct ifx_hppass_sar_adc_config *cfg = dev->config;
	mem_addr_t ctrl_base = cfg->ctrl_base;
#if defined(CONFIG_ADC_ASYNC)
	struct ifx_hppass_sar_adc_data *data = dev->data;
#endif /* CONFIG_ADC_ASYNC */
	LOG_DBG("SAR ADC combined results interrupt");

	uint32_t result_intr_status = sys_read32(ctrl_base + IFX_HPPASS_SAR_RESULT_INTR_MASKED);

	/* RESULT_INTR is write-1-to-clear (TRM 002-39445 §20.1.26). Read it
	 * back after clearing to force the write to complete before the ISR
	 * returns, so the same interrupt isn't re-triggered.
	 */
	sys_write32(result_intr_status, ctrl_base + IFX_HPPASS_SAR_RESULT_INTR);
	(void)sys_read32(ctrl_base + IFX_HPPASS_SAR_RESULT_INTR);

	if (result_intr_status & IFX_HPPASS_SAR_INTR_RESULT_GROUP_0) {
		LOG_DBG("SAR Group 0 conversion complete");

#if defined(CONFIG_ADC_ASYNC)
		if (data->ctx.asynchronous) {
			const struct adc_sequence *sequence = &data->ctx.sequence;
			uint32_t result_status =
				sys_read32(ctrl_base + IFX_HPPASS_SAR_RESULT_UPDATED);

			if ((result_status & sequence->channels) == sequence->channels) {
				ifx_hppass_get_group_results(ctrl_base, sequence->channels, data);
				sys_write32(result_status & sequence->channels &
						    IFX_HPPASS_SAR_CHAN_MSK,
					    ctrl_base + IFX_HPPASS_SAR_RESULT_UPDATED);

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
	if (result_intr_status & ~IFX_HPPASS_SAR_INTR_RESULT_GROUP_0) {
		LOG_ERR("SAR Results Interrupt for unhandled groups: 0x%08X",
			(uint32_t)(result_intr_status & ~IFX_HPPASS_SAR_INTR_RESULT_GROUP_0));
	}
}

/**
 * @brief Initialize PDL ADC configuration structure
 *
 * Builds the one-shot Cy_HPPASS_SAR_Init() argument from the device-tree
 * configuration on top of the fully initialized file-scope default.  The
 * default supplies a complete baseline so the structure is deterministic
 * regardless of which fields are overridden here.  Channel and group
 * pointers stay NULL; per-channel programming uses direct SAR_CHAN_CFG /
 * SAMP_GAIN / RESULT_MASK access at runtime.
 */
static void ifx_init_pdl_struct(cy_stc_hppass_sar_t *sar_cfg,
				const struct ifx_hppass_sar_adc_config *cfg)
{
	*sar_cfg = ifx_hppass_sar_pdl_cfg_struct_default;
	sar_cfg->vref =
		cfg->vref_internal_source ? CY_HPPASS_SAR_VREF_VDDA : CY_HPPASS_SAR_VREF_EXT;
	sar_cfg->offsetCal = cfg->offset_cal;
	sar_cfg->linearCal = cfg->linear_cal;
	sar_cfg->gainCal = cfg->gain_cal;
	sar_cfg->dirSampEnMsk = cfg->dir_samp_en_mask;
	sar_cfg->muxSampEnMsk = cfg->mux_samp_en_mask;
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
 * Validates params, writes SAR_CHAN_CFG, RESULT_MASK, and SAMP_GAIN for one channel.
 */
static int ifx_hppass_sar_adc_channel_setup(const struct device *dev,
					    const struct adc_channel_cfg *channel_cfg)
{
	const struct ifx_hppass_sar_adc_config *cfg = dev->config;
	struct ifx_hppass_sar_adc_data *data = dev->data;
	mem_addr_t ctrl_base = cfg->ctrl_base;

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

	/*
	 * Per-channel CFG_CHAN_CFG0 defaults to all-zero: single-ended,
	 * unsigned, full-range, no averaging, no FIFO.  RESULT_MASK then
	 * gates this channel's result register.
	 */
	uint32_t chan_cfg_reg = 0U;

	/*
	 * RESULT_MASK and SAMP_GAIN are shared across all channels and are
	 * updated read-modify-write below.  Serialize against concurrent
	 * channel setup and against an in-flight conversion via the existing
	 * adc_context lock to avoid lost updates.
	 */
	adc_context_lock(&data->ctx, false, NULL);

	sys_write32(chan_cfg_reg, ctrl_base + IFX_HPPASS_SAR_CHAN_CFG(channel_cfg->channel_id));

	sys_write32(sys_read32(ctrl_base + IFX_HPPASS_SAR_RESULT_MASK) |
			    (1UL << channel_cfg->channel_id),
		    ctrl_base + IFX_HPPASS_SAR_RESULT_MASK);

	/*
	 * SAMP_GAIN has one 2-bit field per sampler (12 direct in bits [23:0],
	 * 4 muxed in [31:24]).  Muxed samplers are shared by several channels,
	 * so map the channel to its sampler to pick the field.
	 */
	uint32_t sampler_gain;
	uint32_t sampler_idx =
		(channel_cfg->channel_id < DIRECT_CHANNEL_CNT)
			? channel_cfg->channel_id
			: (DIRECT_CHANNEL_CNT +
			   (channel_cfg->channel_id - DIRECT_CHANNEL_CNT) /
				   MUXED_CHANNELS_PER_SAMPLER);
	uint32_t gain_shift = sampler_idx * IFX_HPPASS_SAR_SAMPLER_GAIN_WIDTH;

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

	uint32_t samp_gain = sys_read32(ctrl_base + IFX_HPPASS_SAR_SAMP_GAIN);

	samp_gain &= ~(IFX_HPPASS_SAR_SAMPLER_GAIN_MSK << gain_shift);
	samp_gain |= (sampler_gain << gain_shift);
	sys_write32(samp_gain, ctrl_base + IFX_HPPASS_SAR_SAMP_GAIN);

	data->enabled_channels |= BIT(channel_cfg->channel_id);

	adc_context_release(&data->ctx, 0);

	return 0;
}

/**
 * @brief Initialize ADC device
 */
static int ifx_hppass_sar_adc_init(const struct device *dev)
{
	const struct ifx_hppass_sar_adc_config *cfg = dev->config;
	struct ifx_hppass_sar_adc_data *data = dev->data;
	cy_stc_hppass_sar_t sar_cfg;

	data->dev = dev;

	LOG_DBG("Initializing HPPASS SAR ADC");

	/*
	 * The MFD parent (infineon,hppass-analog) enables the HPPASS block and
	 * brings up the clock, power, and AREF/Vref that the SAR depends on.
	 * It must be ready before any SAR register access below.
	 */
	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("HPPASS MFD parent not ready");
		return -ENODEV;
	}

	ifx_init_pdl_struct(&sar_cfg, cfg);

	/*
	 * Cy_HPPASS_SAR_Init() copies SFLASH calibration into
	 * SAR_CALOFFST[0..3]/CALGAINC/CALGAINF and populates the PDL
	 * calOffsetMirror[] table.  The cross-talk compensation in
	 * ifx_hppass_sar_configure_group() reads that table via
	 * Cy_HPPASS_SAR_CrossTalkAdjust(), so this call must stay.
	 */
	if (Cy_HPPASS_SAR_Init(&sar_cfg) != CY_RSLT_SUCCESS) {
		LOG_ERR("Failed to initialize HPPASS SAR ADC");
		return -EIO;
	}

#if defined(CONFIG_ADC_ASYNC)
	/* Group 0 result interrupt drives async completions. */
	sys_write32(IFX_HPPASS_SAR_INTR_RESULT_GROUP_0,
		    cfg->ctrl_base + IFX_HPPASS_SAR_RESULT_INTR_MASK);
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

#define IFX_HPPASS_SAR_CH_EXISTS(inst, ch) \
	DT_NODE_EXISTS(DT_CHILD_BY_UNIT_ADDR_INT(DT_DRV_INST(inst), ch))

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
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.ctrl_base = (mem_addr_t)DT_INST_REG_ADDR(n),                                      \
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
