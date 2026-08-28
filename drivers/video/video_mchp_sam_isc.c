/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Microchip SAMA7G5 eXtended Image Sensor Controller (XISC).
 *
 * The controller samples a parallel video stream, either coming from the CSI-2
 * Demux Controller (CSI2DC) or from the parallel camera pins, processes it and
 * writes the frames to memory with its own DMA master:
 *
 *   PFE -> DPC -> WB -> CFA -> CC -> GAM -> VHXS -> CSC -> CBHS -> SUB -> RLP -> DMA
 *
 * Two classes of output formats are supported:
 *
 * - Raw Bayer, where the samples are written to memory unmodified, one pixel per
 *   byte (8-bit) or per 16-bit container (10-bit and 12-bit).
 * - Converted RGB, grayscale and YUV formats, where a raw Bayer input is run
 *   through white balance, Bayer interpolation, color correction and, for the
 *   YUV and grayscale formats, color space conversion and chroma subsampling.
 *
 * The scaler is left bypassed and the image quality modules are programmed with
 * neutral coefficients: the driver performs no automatic white balance or
 * exposure control. Gamma correction encodes the converted frames with the sRGB
 * transfer function, on the boards that ask for it, and the contrast, brightness,
 * hue and saturation of the converted YUV frames are exposed as video controls.
 */

#define DT_DRV_COMPAT microchip_sama7g5_isc

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>
#include <soc.h>

#include "video_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(video_mchp_sam_isc, CONFIG_VIDEO_LOG_LEVEL);

#define ISC_MAX_WIDTH  3264
#define ISC_MAX_HEIGHT 2464

/* Timeout waiting for a clock domain to settle */
#define ISC_CLOCK_TIMEOUT_US 1000

/*
 * Timeout waiting for a colour profile update to be taken into account. The
 * pipeline latches it on a frame boundary, so this has to cover a frame period.
 */
#define ISC_PROFILE_TIMEOUT_MS 1000

/* Timeout waiting for the capture to end after a stop request */
#define ISC_STOP_TIMEOUT_MS 500

/* Interrupts reported by the controller and handled by this driver */
#define ISC_INT_ERRORS                                                                             \
	(ISC_INTEN_WERR_Msk | ISC_INTEN_RERR_Msk | ISC_INTEN_DAOV_Msk | ISC_INTEN_VFPOV_Msk)
/*
 * Conditions that are worth reporting but must not be enabled: the front end
 * arms a synchronization watchdog that expires once per frame whenever the
 * source gates its clock during blanking, so leaving these unmasked would turn
 * every frame into an interrupt storm.
 */
#define ISC_INT_REPORTED                                                                           \
	(ISC_INTEN_VDTO_Msk | ISC_INTEN_HDTO_Msk | ISC_INTEN_GFOV_Msk | ISC_INTEN_DIS_Msk)

/*
 * A capture is a single shot: the front end clears the request once the frame it
 * captured has been written out, and nothing re-arms it. Should a request ever be
 * dropped instead of completed - the front end reports a completed disable and no
 * DMA done - the capture would be lost for good, so the vertical synchronization
 * is enabled as well and used to arm the next frame on a frame boundary.
 */
#define ISC_INT_ENABLED (ISC_INTEN_DDONE_Msk | ISC_INTEN_VD_Msk | ISC_INT_ERRORS)

/* Image processing modules of the pipeline, enabled per output format */
#define ISC_PIPE_BLC    BIT(0) /* black level correction */
#define ISC_PIPE_WB     BIT(1) /* white balance */
#define ISC_PIPE_CFA    BIT(2) /* Bayer interpolation */
#define ISC_PIPE_CC     BIT(3) /* color correction */
#define ISC_PIPE_GAM    BIT(4) /* gamma correction */
#define ISC_PIPE_CSC    BIT(5) /* RGB to YCbCr color space conversion */
#define ISC_PIPE_CBHS   BIT(6) /* contrast, brightness, hue and saturation */
#define ISC_PIPE_SUB422 BIT(7) /* 4:4:4 to 4:2:2 chroma subsampling */

/* Bayer to RGB conversion, without color space conversion */
#define ISC_PIPE_DEBAYER (ISC_PIPE_BLC | ISC_PIPE_WB | ISC_PIPE_CFA | ISC_PIPE_CC | ISC_PIPE_GAM)

/* Bayer to YCbCr conversion */
#define ISC_PIPE_YUV (ISC_PIPE_DEBAYER | ISC_PIPE_CSC | ISC_PIPE_CBHS)

/* Neutral values of the image quality modules */
#define ISC_WB_GAIN_UNITY   BIT(9) /* 1.0 with 9 fractional bits */
#define ISC_CC_GAIN_UNITY   BIT(8) /* 1.0 with 8 fractional bits */
#define ISC_CBHS_CONTRAST   BIT(4) /* 1.0 with 4 fractional bits */
#define ISC_CBHS_SATURATION BIT(4) /* 1.0 with 4 fractional bits */

/*
 * Ranges of the contrast, brightness, hue and saturation controls. Brightness is
 * an offset on the luma and hue an angle in degrees, both signed and written to
 * the register in two's complement; contrast and saturation are unsigned gains
 * with four fractional bits, so unity is 16 and zero is a flat image. The
 * saturation field would hold up to 127, but the values above 100 are reserved.
 */
#define ISC_CBHS_BRIGHTNESS_MIN -1024
#define ISC_CBHS_BRIGHTNESS_MAX 1023
#define ISC_CBHS_CONTRAST_MAX   4095
#define ISC_CBHS_HUE_MIN        -180
#define ISC_CBHS_HUE_MAX        180
#define ISC_CBHS_SATURATION_MAX 100

/* Gamma correction interpolating the table of every color channel */
#define ISC_GAM_ENABLE                                                                             \
	(ISC_GAM_CTRL_ENABLE_Msk | ISC_GAM_CTRL_RENABLE_Msk | ISC_GAM_CTRL_GENABLE_Msk |           \
	 ISC_GAM_CTRL_BENABLE_Msk)

/* Gamma table entry: the curve at the start of a segment, and its slope over it */
#define ISC_GAM_SEG(constant, slope) ISC_FIELD16(slope, constant)

/* The BPS field of ISC_PFE_CFG0 encodes 12 - sample width */
#define ISC_SAMPLE_WIDTH(bps) (12U - (bps))

#define ISC_FIELD16(low, high) (((low) & 0xffff) | (((high) & 0xffff) << 16))

/**
 * @brief Format accepted on the parallel input of the front end
 *
 * @param pixelformat format presented by the CSI2DC or the parallel sensor
 * @param pfe_bps ISC_PFE_CFG0.BPS field value, which encodes 12 - sample width
 * @param bay_cfg Bayer pattern of the first row, meaningless when not raw
 * @param raw whether the samples are raw Bayer, hence convertible by the pipeline
 */
struct isc_input_format {
	uint32_t pixelformat;
	uint32_t pfe_bps;
	uint32_t bay_cfg;
	bool raw;
};

static const struct isc_input_format isc_input_formats[] = {
	{VIDEO_PIX_FMT_SBGGR8, ISC_PFE_CFG0_BPS_EIGHT_Val, ISC_CFA_CFG_BAYCFG_BGBG_Val, true},
	{VIDEO_PIX_FMT_SGBRG8, ISC_PFE_CFG0_BPS_EIGHT_Val, ISC_CFA_CFG_BAYCFG_GBGB_Val, true},
	{VIDEO_PIX_FMT_SGRBG8, ISC_PFE_CFG0_BPS_EIGHT_Val, ISC_CFA_CFG_BAYCFG_GRGR_Val, true},
	{VIDEO_PIX_FMT_SRGGB8, ISC_PFE_CFG0_BPS_EIGHT_Val, ISC_CFA_CFG_BAYCFG_RGRG_Val, true},
	{VIDEO_PIX_FMT_SBGGR10, ISC_PFE_CFG0_BPS_TEN_Val, ISC_CFA_CFG_BAYCFG_BGBG_Val, true},
	{VIDEO_PIX_FMT_SGBRG10, ISC_PFE_CFG0_BPS_TEN_Val, ISC_CFA_CFG_BAYCFG_GBGB_Val, true},
	{VIDEO_PIX_FMT_SGRBG10, ISC_PFE_CFG0_BPS_TEN_Val, ISC_CFA_CFG_BAYCFG_GRGR_Val, true},
	{VIDEO_PIX_FMT_SRGGB10, ISC_PFE_CFG0_BPS_TEN_Val, ISC_CFA_CFG_BAYCFG_RGRG_Val, true},
	{VIDEO_PIX_FMT_SBGGR12, ISC_PFE_CFG0_BPS_TWELVE_Val, ISC_CFA_CFG_BAYCFG_BGBG_Val, true},
	{VIDEO_PIX_FMT_SGBRG12, ISC_PFE_CFG0_BPS_TWELVE_Val, ISC_CFA_CFG_BAYCFG_GBGB_Val, true},
	{VIDEO_PIX_FMT_SGRBG12, ISC_PFE_CFG0_BPS_TWELVE_Val, ISC_CFA_CFG_BAYCFG_GRGR_Val, true},
	{VIDEO_PIX_FMT_SRGGB12, ISC_PFE_CFG0_BPS_TWELVE_Val, ISC_CFA_CFG_BAYCFG_RGRG_Val, true},
	{VIDEO_PIX_FMT_GREY, ISC_PFE_CFG0_BPS_EIGHT_Val, 0, false},
	{VIDEO_PIX_FMT_YUYV, ISC_PFE_CFG0_BPS_EIGHT_Val, 0, false},
};

/**
 * @brief Format the controller is able to write to memory
 *
 * @param pixelformat format written to memory
 * @param rlp_cfg rounding, limiting and packing configuration
 * @param dcfg_imode DMA input mode
 * @param dctrl_dview DMA descriptor view
 * @param pipeline image processing modules to enable, 0 for a raw dump
 */
struct isc_output_format {
	uint32_t pixelformat;
	uint32_t rlp_cfg;
	uint32_t dcfg_imode;
	uint32_t dctrl_dview;
	uint16_t pipeline;
};

#define ISC_RAW_OUTPUT(fmt, rlp, imode)                                                            \
	{                                                                                          \
		.pixelformat = VIDEO_PIX_FMT_##fmt,                                                \
		.rlp_cfg = ISC_RLP_CFG_MODE_##rlp,                                                 \
		.dcfg_imode = ISC_DCFG_IMODE_##imode,                                              \
		.dctrl_dview = ISC_DCTRL_DVIEW_PACKED,                                             \
		.pipeline = 0,                                                                     \
	}

static const struct isc_output_format isc_output_formats[] = {
	/* Raw Bayer dumped to memory as sampled */
	ISC_RAW_OUTPUT(SBGGR8, DAT8, PACKED8),
	ISC_RAW_OUTPUT(SGBRG8, DAT8, PACKED8),
	ISC_RAW_OUTPUT(SGRBG8, DAT8, PACKED8),
	ISC_RAW_OUTPUT(SRGGB8, DAT8, PACKED8),
	ISC_RAW_OUTPUT(SBGGR10, DAT10, PACKED16),
	ISC_RAW_OUTPUT(SGBRG10, DAT10, PACKED16),
	ISC_RAW_OUTPUT(SGRBG10, DAT10, PACKED16),
	ISC_RAW_OUTPUT(SRGGB10, DAT10, PACKED16),
	ISC_RAW_OUTPUT(SBGGR12, DAT12, PACKED16),
	ISC_RAW_OUTPUT(SGBRG12, DAT12, PACKED16),
	ISC_RAW_OUTPUT(SGRBG12, DAT12, PACKED16),
	ISC_RAW_OUTPUT(SRGGB12, DAT12, PACKED16),
	/* Converted formats */
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.rlp_cfg = ISC_RLP_CFG_MODE_RGB565,
		.dcfg_imode = ISC_DCFG_IMODE_PACKED16,
		.dctrl_dview = ISC_DCTRL_DVIEW_PACKED,
		.pipeline = ISC_PIPE_DEBAYER,
	},
	{
		/* The controller writes blue, green, red then the alpha byte */
		.pixelformat = VIDEO_PIX_FMT_BGRX32,
		.rlp_cfg = ISC_RLP_CFG_MODE_ARGB32,
		.dcfg_imode = ISC_DCFG_IMODE_PACKED32,
		.dctrl_dview = ISC_DCTRL_DVIEW_PACKED,
		.pipeline = ISC_PIPE_DEBAYER,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_YUYV,
		.rlp_cfg = ISC_RLP_CFG_MODE_YCYC | ISC_RLP_CFG_YMODE_RLP_CRYCBY,
		.dcfg_imode = ISC_DCFG_IMODE_PACKED32,
		.dctrl_dview = ISC_DCTRL_DVIEW_PACKED,
		.pipeline = ISC_PIPE_YUV | ISC_PIPE_SUB422,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.rlp_cfg = ISC_RLP_CFG_MODE_DATY8,
		.dcfg_imode = ISC_DCFG_IMODE_PACKED8,
		.dctrl_dview = ISC_DCTRL_DVIEW_PACKED,
		.pipeline = ISC_PIPE_YUV,
	},
};

struct isc_config {
	isc_registers_t *regs;
	const struct device *source_dev;
	struct sam_clk_cfg clock_cfg;
	uint32_t pfe_cfg0;
	unsigned int irq;
	uint16_t black_level_offset;
	uint8_t mck_div;
	bool srgb_gamma;
};

struct isc_ctrls {
	struct video_ctrl brightness;
	struct video_ctrl contrast;
	struct video_ctrl hue;
	struct video_ctrl saturation;
};

struct isc_data {
	bool clock_enabled;
	struct isc_ctrls ctrls;
	const struct isc_output_format *out_fmt;
	const struct isc_input_format *in_fmt;
	struct video_format fmt;
	struct video_format_cap caps[ARRAY_SIZE(isc_output_formats) + 1];
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_poll_signal *sig_out;
	struct video_buffer *cur_buf;
	struct k_spinlock lock;
	bool streaming;
	bool capture_armed;
	uint8_t vd_since_arm;
};

static const struct isc_output_format *isc_get_output_format(uint32_t pixelformat)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(isc_output_formats); i++) {
		if (isc_output_formats[i].pixelformat == pixelformat) {
			return &isc_output_formats[i];
		}
	}

	return NULL;
}

static const struct isc_input_format *isc_get_input_format(uint32_t pixelformat)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(isc_input_formats); i++) {
		if (isc_input_formats[i].pixelformat == pixelformat) {
			return &isc_input_formats[i];
		}
	}

	return NULL;
}

static int isc_wait_reg_clear(const volatile uint32_t *reg, uint32_t mask)
{
	for (int wait_us = 0; wait_us < ISC_CLOCK_TIMEOUT_US; wait_us += 10) {
		if ((*reg & mask) == 0) {
			return 0;
		}
		k_busy_wait(10);
	}

	return -ETIMEDOUT;
}

/*
 * ISC_CTRLEN and ISC_CTRLDIS cross into a second clock domain, and writing them
 * while a synchronization is already in progress is forbidden, so the caller has
 * to wait for ISC_CTRLSR.SIP to clear first.
 */
static void isc_wait_sync_done(const struct isc_config *cfg)
{
	if (isc_wait_reg_clear(&cfg->regs->ISC_CTRLSR, ISC_CTRLSR_SIP_Msk) < 0) {
		LOG_WRN("Domain synchronization still in progress");
	}
}

static int isc_update_profile(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;

	isc_wait_sync_done(cfg);
	cfg->regs->ISC_CTRLEN = ISC_CTRLEN_UPPRO_Msk;

	for (int wait_ms = 0; wait_ms < ISC_PROFILE_TIMEOUT_MS; wait_ms++) {
		if ((cfg->regs->ISC_CTRLSR & ISC_CTRLSR_UPPRO_Msk) == 0) {
			return 0;
		}
		k_msleep(1);
	}

	LOG_ERR("Timed out updating the color profile");

	return -ETIMEDOUT;
}

/*
 * Scale the black level offset of the sensor, given at a 10-bit sample width, to
 * the width actually sampled. The hardware field is 9-bit wide.
 */
static uint32_t isc_black_level_offset(uint32_t offset, uint32_t bps)
{
	int shift = (int)ISC_SAMPLE_WIDTH(bps) - 10;

	return (shift >= 0) ? MIN(offset << shift, 511U) : (offset >> -shift);
}

/*
 * Bring the controller back to its reset state. Issued once, before the internal
 * clocks are configured, since the reset also clears ISC_CLKEN.
 */
static void isc_software_reset(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;

	isc_wait_sync_done(cfg);
	cfg->regs->ISC_CTRLDIS = ISC_CTRLDIS_SWRST_Msk;
	isc_wait_sync_done(cfg);
}

/*
 * The gamma block compresses the 12 bits the pipeline carries down to the 10 bits
 * the color space converter and the rate limiter work on. Disabled it drops the
 * two low bits, which keeps the samples linear; enabled it interpolates a curve
 * described by 64 segments of 64 input codes, one table per color channel.
 *
 * Displays expect their input to be encoded, so the curve is the sRGB transfer
 * function. Each entry holds the value of the curve at the start of its segment
 * and its slope over the segment:
 *
 *   srgb(u)      = u <= 0.0031308 ? 12.92 * u : 1.055 * u^(1/2.4) - 0.055
 *   f(x)         = 1023 * srgb(x / 4095)
 *   GCONSTANT(k) = round(f(64 * k))
 *   GSLOPE(k)    = round(f(64 * (k + 1)) - f(64 * k))
 *
 * The slope is signed fixed point with six fractional bits, so the rise over the
 * 64 codes of a segment is also the slope in the units of the field. The three
 * channel registers share that layout, hence a single packing macro and, since
 * nothing here treats the channels differently, a single table.
 *
 * Uniform segments describe a curve that is not, which costs up to 20 counts of
 * 1023 inside the first segment, where the knee of the curve is. The remaining
 * 98 percent of the range is within three counts.
 */
static const uint32_t isc_gamma_srgb[64] = {
	ISC_GAM_SEG(0, 135),  ISC_GAM_SEG(135, 64), ISC_GAM_SEG(198, 47), ISC_GAM_SEG(245, 38),
	ISC_GAM_SEG(284, 33), ISC_GAM_SEG(317, 29), ISC_GAM_SEG(346, 27), ISC_GAM_SEG(373, 25),
	ISC_GAM_SEG(398, 23), ISC_GAM_SEG(420, 21), ISC_GAM_SEG(442, 20), ISC_GAM_SEG(462, 19),
	ISC_GAM_SEG(481, 18), ISC_GAM_SEG(499, 17), ISC_GAM_SEG(517, 17), ISC_GAM_SEG(533, 16),
	ISC_GAM_SEG(550, 15), ISC_GAM_SEG(565, 15), ISC_GAM_SEG(580, 14), ISC_GAM_SEG(594, 14),
	ISC_GAM_SEG(609, 14), ISC_GAM_SEG(622, 13), ISC_GAM_SEG(635, 13), ISC_GAM_SEG(648, 13),
	ISC_GAM_SEG(661, 12), ISC_GAM_SEG(673, 12), ISC_GAM_SEG(685, 12), ISC_GAM_SEG(697, 12),
	ISC_GAM_SEG(709, 11), ISC_GAM_SEG(720, 11), ISC_GAM_SEG(731, 11), ISC_GAM_SEG(742, 11),
	ISC_GAM_SEG(752, 10), ISC_GAM_SEG(763, 10), ISC_GAM_SEG(773, 10), ISC_GAM_SEG(783, 10),
	ISC_GAM_SEG(793, 10), ISC_GAM_SEG(803, 10), ISC_GAM_SEG(812, 9),  ISC_GAM_SEG(822, 9),
	ISC_GAM_SEG(831, 9),  ISC_GAM_SEG(840, 9),  ISC_GAM_SEG(849, 9),  ISC_GAM_SEG(858, 9),
	ISC_GAM_SEG(867, 9),  ISC_GAM_SEG(876, 9),  ISC_GAM_SEG(884, 8),  ISC_GAM_SEG(893, 8),
	ISC_GAM_SEG(901, 8),  ISC_GAM_SEG(909, 8),  ISC_GAM_SEG(918, 8),  ISC_GAM_SEG(926, 8),
	ISC_GAM_SEG(934, 8),  ISC_GAM_SEG(942, 8),  ISC_GAM_SEG(949, 8),  ISC_GAM_SEG(957, 8),
	ISC_GAM_SEG(965, 8),  ISC_GAM_SEG(972, 7),  ISC_GAM_SEG(980, 7),  ISC_GAM_SEG(987, 7),
	ISC_GAM_SEG(994, 7),  ISC_GAM_SEG(1002, 7), ISC_GAM_SEG(1009, 7), ISC_GAM_SEG(1016, 7),
};

static void isc_configure_gamma(isc_registers_t *regs)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(isc_gamma_srgb); i++) {
		regs->ISC_GAM_RENTRY[i] = isc_gamma_srgb[i];
		regs->ISC_GAM_GENTRY[i] = isc_gamma_srgb[i];
		regs->ISC_GAM_BENTRY[i] = isc_gamma_srgb[i];
	}
}

/*
 * The contrast, brightness, hue and saturation block works on the output of the
 * color space converter, so it only ever sees the formats that carry luma and
 * chroma. Its four registers hold the current value of the matching controls,
 * masked to their field width so that the signed ones land as two's complement.
 */
static void isc_configure_cbhs(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	struct isc_ctrls *ctrls = &data->ctrls;

	cfg->regs->ISC_CBHS_BRIGHT = ISC_CBHS_BRIGHT_BRIGHT(ctrls->brightness.val);
	cfg->regs->ISC_CBHS_CONT = ISC_CBHS_CONT_CONTRAST(ctrls->contrast.val);
	cfg->regs->ISC_CBHS_HUE = ISC_CBHS_HUE_HUE(ctrls->hue.val);
	cfg->regs->ISC_CBHS_SAT = ISC_CBHS_SAT_SATURATION(ctrls->saturation.val);
}

static void isc_activate_ctrl(struct video_ctrl *ctrl, bool active)
{
	if (active) {
		ctrl->flags &= ~VIDEO_CTRL_FLAG_INACTIVE;
	} else {
		ctrl->flags |= VIDEO_CTRL_FLAG_INACTIVE;
	}
}

/*
 * Contrast and brightness only reach the frames of the formats the color space
 * converter feeds, and hue and saturation only those of them that keep their
 * chroma channels. The controls that the selected format leaves nothing to do
 * are reported as inactive rather than accepting a value and ignoring it.
 */
static void isc_update_ctrl_activity(const struct device *dev)
{
	struct isc_data *data = dev->data;
	struct isc_ctrls *ctrls = &data->ctrls;
	uint16_t pipeline = data->out_fmt->pipeline;
	bool cbhs = pipeline & ISC_PIPE_CBHS;
	/* Chroma subsampling marks the output formats that carry chroma at all */
	bool chroma = pipeline & ISC_PIPE_SUB422;

	isc_activate_ctrl(&ctrls->brightness, cbhs);
	isc_activate_ctrl(&ctrls->contrast, cbhs);
	isc_activate_ctrl(&ctrls->hue, chroma);
	isc_activate_ctrl(&ctrls->saturation, chroma);
}

/*
 * The contrast, brightness, hue and saturation registers are part of the color
 * profile, so a write only reaches the pipeline once the profile is updated, and
 * that needs the controller to be clocked. Until the first stream is started the
 * value is only kept, and isc_configure_pipeline() programs it.
 */
static int isc_set_ctrl(const struct device *dev, uint32_t id)
{
	struct isc_data *data = dev->data;

	switch (id) {
	case VIDEO_CID_BRIGHTNESS:
	case VIDEO_CID_CONTRAST:
	case VIDEO_CID_HUE:
	case VIDEO_CID_SATURATION:
		break;
	default:
		return -ENOTSUP;
	}

	if (!data->clock_enabled) {
		return 0;
	}

	isc_configure_cbhs(dev);

	return isc_update_profile(dev);
}

static void isc_configure_pipeline(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	isc_registers_t *regs = cfg->regs;
	uint16_t pipeline = data->out_fmt->pipeline;
	uint32_t bay_cfg = data->in_fmt->bay_cfg;
	uint32_t blofst = isc_black_level_offset(cfg->black_level_offset, data->in_fmt->pfe_bps);
	/*
	 * Subtracting a pedestal the sensor does not add darkens the image and
	 * clips the low end, so black level correction stays out of the pipeline
	 * until a board describes what its sensor adds.
	 */
	bool blc = (pipeline & ISC_PIPE_BLC) && blofst != 0;
	/*
	 * Gamma correction is what makes the frames displayable, and also what
	 * makes them non-linear, so whether to apply it is left to the board.
	 */
	bool gam = (pipeline & ISC_PIPE_GAM) && cfg->srgb_gamma;

	regs->ISC_DPC_CTRL = blc ? ISC_DPC_CTRL_BLCEN_Msk : 0;
	regs->ISC_WB_CTRL = (pipeline & ISC_PIPE_WB) ? ISC_WB_CTRL_ENABLE_Msk : 0;
	regs->ISC_CFA_CTRL = (pipeline & ISC_PIPE_CFA) ? ISC_CFA_CTRL_ENABLE_Msk : 0;
	regs->ISC_CC_CTRL = (pipeline & ISC_PIPE_CC) ? ISC_CC_CTRL_ENABLE_Msk : 0;
	regs->ISC_GAM_CTRL = gam ? ISC_GAM_ENABLE : 0;
	regs->ISC_CSC_CTRL = (pipeline & ISC_PIPE_CSC) ? ISC_CSC_CTRL_ENABLE_Msk : 0;
	regs->ISC_CBHS_CTRL = (pipeline & ISC_PIPE_CBHS) ? ISC_CBHS_CTRL_ENABLE_Msk : 0;
	regs->ISC_SUB422_CTRL = (pipeline & ISC_PIPE_SUB422) ? ISC_SUB422_CTRL_ENABLE_Msk : 0;

	/* The scaler and 4:2:0 subsampling are not used */
	regs->ISC_VHXS_CTRL = 0;
	regs->ISC_SUB420_CTRL = 0;

	if (pipeline == 0) {
		return;
	}

	regs->ISC_DPC_CFG = ISC_DPC_CFG_BAYCFG(bay_cfg) | ISC_DPC_CFG_BLOFST(blofst);

	/* Neutral white balance: unity gain and no offset on every channel */
	regs->ISC_WB_CFG = ISC_WB_CFG_BAYCFG(bay_cfg);
	regs->ISC_WB_O_RGR = 0;
	regs->ISC_WB_O_BGB = 0;
	regs->ISC_WB_G_RGR = ISC_FIELD16(ISC_WB_GAIN_UNITY, ISC_WB_GAIN_UNITY);
	regs->ISC_WB_G_BGB = ISC_FIELD16(ISC_WB_GAIN_UNITY, ISC_WB_GAIN_UNITY);

	regs->ISC_CFA_CFG = ISC_CFA_CFG_BAYCFG(bay_cfg) | ISC_CFA_CFG_EITPOL_Msk;

	/* Identity color correction matrix, without offsets */
	regs->ISC_CC_RR_RG = ISC_FIELD16(ISC_CC_GAIN_UNITY, 0);
	regs->ISC_CC_RB_OR = 0;
	regs->ISC_CC_GR_GG = ISC_FIELD16(0, ISC_CC_GAIN_UNITY);
	regs->ISC_CC_GB_OG = 0;
	regs->ISC_CC_BR_BG = 0;
	regs->ISC_CC_BB_OB = ISC_FIELD16(ISC_CC_GAIN_UNITY, 0);

	if (gam) {
		isc_configure_gamma(regs);
	}

	if (pipeline & ISC_PIPE_CSC) {
		/*
		 * ITU-R BT.601 RGB to YCbCr conversion with 8 fractional bits,
		 * negative coefficients in two's complement over 12 bits:
		 *   Y  =  0.257 R + 0.504 G + 0.098 B + 16
		 *   Cb = -0.148 R - 0.291 G + 0.439 B + 128
		 *   Cr =  0.439 R - 0.368 G - 0.071 B + 128
		 */
		regs->ISC_CSC_YR_YG = ISC_FIELD16(66, 129);
		regs->ISC_CSC_YB_OY = ISC_FIELD16(25, 16);
		regs->ISC_CSC_CBR_CBG = ISC_FIELD16(0xfda, 0xfb6);
		regs->ISC_CSC_CBB_OCB = ISC_FIELD16(112, 128);
		regs->ISC_CSC_CRR_CRG = ISC_FIELD16(112, 0xfa2);
		regs->ISC_CSC_CRB_OCR = ISC_FIELD16(0xfee, 128);
	}

	if (pipeline & ISC_PIPE_CBHS) {
		isc_configure_cbhs(dev);
	}
}

static int isc_configure(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	isc_registers_t *regs = cfg->regs;
	uint32_t width = data->fmt.width;
	uint32_t height = data->fmt.height;
	uint32_t pfe_cfg0;

	pfe_cfg0 = cfg->pfe_cfg0 | ISC_PFE_CFG0_BPS(data->in_fmt->pfe_bps) |
		   ISC_PFE_CFG0_MODE_PROGRESSIVE | ISC_PFE_CFG0_COLEN_Msk |
		   ISC_PFE_CFG0_ROWEN_Msk;

	/*
	 * A non-raw sensor sends one pixel as two samples on the parallel bus,
	 * and the front end counts samples, so the cropping window has to be
	 * expressed with twice the pixel count.
	 */
	if (!data->in_fmt->raw) {
		width <<= 1;
		height <<= 1;
	}

	regs->ISC_PFE_CFG0 = pfe_cfg0;

	/*
	 * Bound the number of columns and rows the front end forwards, so that a
	 * misconfigured sensor sending more data than expected cannot make the
	 * DMA write past the end of the frame buffer.
	 */
	regs->ISC_PFE_CFG1 = ISC_PFE_CFG1_COLMIN(0) | ISC_PFE_CFG1_COLMAX(width - 1);
	regs->ISC_PFE_CFG2 = ISC_PFE_CFG2_ROWMIN(0) | ISC_PFE_CFG2_ROWMAX(height - 1);

	regs->ISC_RLP_CFG = data->out_fmt->rlp_cfg;
	regs->ISC_DCFG = data->out_fmt->dcfg_imode | ISC_DCFG_YMBSIZE_BEATS32 |
			 ISC_DCFG_CMBSIZE_BEATS32;

	isc_configure_pipeline(dev);

	return isc_update_profile(dev);
}

/* Program the DMA with the current buffer and request the capture of one frame */
static void isc_start_dma(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;

	cfg->regs->ISC_DAD0 = (uint32_t)(uintptr_t)data->cur_buf->buffer;
	/* ISC_DCTRL.IE suppresses the DMA done interrupt, so it is left cleared */
	cfg->regs->ISC_DCTRL = data->out_fmt->dctrl_dview;
	isc_wait_sync_done(cfg);
	cfg->regs->ISC_CTRLEN = ISC_CTRLEN_CAPTURE_Msk;
	data->capture_armed = true;
	data->vd_since_arm = 0;
}

/*
 * Pick the format the source has to output for a given ISC output format: raw
 * output is a straight dump of the input, while the converted formats need a raw
 * Bayer input which is looked up in the capabilities of the source.
 */
static int isc_select_input_format(const struct device *dev, uint32_t out_pixelformat,
				   const struct isc_input_format **in_fmt)
{
	const struct isc_config *cfg = dev->config;
	const struct isc_output_format *out;
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	const struct isc_input_format *best = NULL;
	int ret;

	out = isc_get_output_format(out_pixelformat);
	if (out == NULL) {
		return -ENOTSUP;
	}

	if (out->pipeline == 0) {
		/* The output is the input, unmodified */
		*in_fmt = isc_get_input_format(out_pixelformat);
		return (*in_fmt == NULL) ? -ENOTSUP : 0;
	}

	ret = video_get_caps(cfg->source_dev, &caps);
	if (ret < 0) {
		return ret;
	}

	/* Prefer the deepest raw format the source is able to produce */
	for (unsigned int i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		const struct isc_input_format *in =
			isc_get_input_format(caps.format_caps[i].pixelformat);

		if (in == NULL || !in->raw) {
			continue;
		}

		if (best == NULL || in->pfe_bps < best->pfe_bps) {
			/* A smaller BPS field encodes a larger sample width */
			best = in;
		}
	}

	if (best == NULL) {
		LOG_ERR("The source provides no raw Bayer format to convert from");
		return -ENOTSUP;
	}

	*in_fmt = best;

	return 0;
}

static int isc_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	const struct isc_output_format *out_fmt;
	const struct isc_input_format *in_fmt;
	struct video_format source_fmt = *fmt;
	int ret;

	if (!IN_RANGE(fmt->width, 1, ISC_MAX_WIDTH) ||
	    !IN_RANGE(fmt->height, 1, ISC_MAX_HEIGHT)) {
		LOG_ERR("Resolution %ux%u exceeds %ux%u", fmt->width, fmt->height, ISC_MAX_WIDTH,
			ISC_MAX_HEIGHT);
		return -EINVAL;
	}

	out_fmt = isc_get_output_format(fmt->pixelformat);
	if (out_fmt == NULL) {
		LOG_ERR("Format %s is not supported", VIDEO_FOURCC_TO_STR(fmt->pixelformat));
		return -ENOTSUP;
	}

	ret = isc_select_input_format(dev, fmt->pixelformat, &in_fmt);
	if (ret < 0) {
		return ret;
	}

	source_fmt.pixelformat = in_fmt->pixelformat;

	ret = video_set_format(cfg->source_dev, &source_fmt);
	if (ret < 0) {
		return ret;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	data->fmt = *fmt;
	data->out_fmt = out_fmt;
	data->in_fmt = in_fmt;

	isc_update_ctrl_activity(dev);

	LOG_DBG("%ux%u, input %s, output %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(in_fmt->pixelformat), VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	return 0;
}

static int isc_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	int ret;

	if (data->out_fmt == NULL) {
		/* Default to dumping whatever the source produces */
		ret = video_get_format(cfg->source_dev, fmt);
		if (ret < 0) {
			return ret;
		}

		ret = isc_set_fmt(dev, fmt);
		if (ret < 0) {
			return ret;
		}
	}

	*fmt = data->fmt;

	return 0;
}

static int isc_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	struct video_caps source_caps = {.type = caps->type};
	uint16_t width_min = 4, height_min = 4, width_step = 1, height_step = 1;
	uint16_t width_max = ISC_MAX_WIDTH, height_max = ISC_MAX_HEIGHT;
	bool source_has_raw = false;
	unsigned int nb = 0;
	int ret;

	ret = video_get_caps(cfg->source_dev, &source_caps);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The controller does not resize, so the frame sizes it supports are the
	 * ones of the source, capped to what the front end can sample.
	 */
	for (unsigned int i = 0; source_caps.format_caps[i].pixelformat != 0; i++) {
		const struct video_format_cap *cap = &source_caps.format_caps[i];
		const struct isc_input_format *in = isc_get_input_format(cap->pixelformat);

		if (in == NULL) {
			continue;
		}

		if (in->raw) {
			source_has_raw = true;
		}

		width_min = MAX(width_min, cap->width_min);
		height_min = MAX(height_min, cap->height_min);
		width_max = MIN(width_max, cap->width_max);
		height_max = MIN(height_max, cap->height_max);
		width_step = MAX(width_step, cap->width_step);
		height_step = MAX(height_step, cap->height_step);
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(isc_output_formats); i++) {
		const struct isc_output_format *out = &isc_output_formats[i];

		if (out->pipeline == 0) {
			/* Raw dump, only if the source produces that very format */
			bool supported = false;

			for (unsigned int j = 0; source_caps.format_caps[j].pixelformat != 0; j++) {
				if (source_caps.format_caps[j].pixelformat == out->pixelformat) {
					supported = true;
					break;
				}
			}

			if (!supported) {
				continue;
			}
		} else if (!source_has_raw) {
			/* Conversion requires a raw Bayer input */
			continue;
		}

		data->caps[nb++] = (struct video_format_cap){
			.pixelformat = out->pixelformat,
			.width_min = width_min,
			.width_max = width_max,
			.width_step = width_step,
			.height_min = height_min,
			.height_max = height_max,
			.height_step = height_step,
		};
	}

	memset(&data->caps[nb], 0, sizeof(data->caps[nb]));

	caps->format_caps = data->caps;
	caps->min_vbuf_count = 1;
	/* Frames are invalidated in the cache, so buffers must not share a line */
	caps->buf_align = sys_cache_data_line_size_get();

	return 0;
}

/*
 * Bring up the two clock domains of the controller: the ISP clock, which clocks
 * the processing pipeline and the double domain synchronization behind
 * ISC_CTRLEN, and the sensor clock, which also clocks the CSI2DC video pipeline.
 * Both have to be running before any control command is issued.
 */
static int isc_enable_clocks(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;
	isc_registers_t *regs = cfg->regs;
	uint32_t expected = ISC_CLKSR_ICSR_Msk | ISC_CLKSR_MCSR_Msk;
	int ret;

	isc_software_reset(dev);

	ret = isc_wait_reg_clear(&regs->ISC_CLKSR, ISC_CLKSR_SIP_Msk);
	if (ret < 0) {
		LOG_ERR("The clock domains did not settle");
		return ret;
	}

	regs->ISC_CLKCFG = ISC_CLKCFG_MCDIV(cfg->mck_div);
	regs->ISC_CLKEN = ISC_CLKEN_MCEN_Msk;
	regs->ISC_CLKEN = ISC_CLKEN_ICEN_Msk;

	for (int wait_us = 0; wait_us < ISC_CLOCK_TIMEOUT_US; wait_us += 10) {
		uint32_t status = regs->ISC_CLKSR;

		if ((status & (expected | ISC_CLKSR_SIP_Msk)) == expected) {
			return 0;
		}
		k_busy_wait(10);
	}

	LOG_ERR("Clocks did not start, ISC_CLKSR is 0x%08x, expected 0x%08x",
		regs->ISC_CLKSR, expected);

	return -EIO;
}

static int isc_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	isc_registers_t *regs = cfg->regs;
	k_spinlock_key_t key;
	int ret;

	if (!enable) {
		regs->ISC_INTDIS = ISC_INT_ENABLED;
		irq_disable(cfg->irq);

		key = k_spin_lock(&data->lock);
		data->streaming = false;
		k_spin_unlock(&data->lock, key);

		/* End the capture at the next vertical synchronization */
		isc_wait_sync_done(cfg);
		regs->ISC_CTRLDIS = ISC_CTRLDIS_DISABLE_Msk;

		for (int wait_ms = 0; wait_ms < ISC_STOP_TIMEOUT_MS; wait_ms++) {
			if ((regs->ISC_CTRLSR & ISC_CTRLSR_CAPTURE_Msk) == 0) {
				break;
			}
			k_msleep(1);
		}

		ret = video_stream_stop(cfg->source_dev, type);

		/* Hand the buffer being captured back to the incoming queue */
		key = k_spin_lock(&data->lock);
		if (data->cur_buf != NULL) {
			k_fifo_put(&data->fifo_in, data->cur_buf);
			data->cur_buf = NULL;
		}
		k_spin_unlock(&data->lock, key);

		return ret;
	}

	if (data->out_fmt == NULL) {
		LOG_ERR("The format must be set before starting the stream");
		return -EIO;
	}

	/*
	 * First register access of this device. Both the ISP clock and the sensor
	 * clock are started here: the sensor clock also clocks the CSI2DC video
	 * pipeline, so it has to run before the source is started, and the ISP
	 * clock before any control command is issued.
	 */
	if (!data->clock_enabled) {
		ret = isc_enable_clocks(dev);
		if (ret < 0) {
			return ret;
		}

		data->clock_enabled = true;
	}

	/* Mask and clear every interrupt source before unmasking the line */
	regs->ISC_INTDIS = ISC_INTDIS_Msk;
	(void)regs->ISC_INTSR;

	irq_enable(cfg->irq);

	/*
	 * The source has to be streaming before the controller is configured: the
	 * colour profile update is committed on a frame boundary of the incoming
	 * video, so it never completes while the pixel clock is idle.
	 */
	ret = video_stream_start(cfg->source_dev, type);
	if (ret < 0) {
		irq_disable(cfg->irq);
		return ret;
	}

	ret = isc_configure(dev);
	if (ret < 0) {
		video_stream_stop(cfg->source_dev, type);
		irq_disable(cfg->irq);
		return ret;
	}

	regs->ISC_INTEN = ISC_INT_ENABLED;

	key = k_spin_lock(&data->lock);
	data->streaming = true;
	data->capture_armed = false;
	if (data->cur_buf == NULL) {
		data->cur_buf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	}
	if (data->cur_buf != NULL) {
		isc_start_dma(dev);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int isc_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct isc_data *data = dev->data;
	k_spinlock_key_t key;

	if (data->out_fmt == NULL) {
		return -EIO;
	}

	if (vbuf->buffer == NULL) {
		LOG_ERR("Buffer has no memory attached");
		return -EINVAL;
	}

	if (vbuf->size < data->fmt.size) {
		LOG_ERR("Buffer of %u bytes is too small for a %u byte frame", vbuf->size,
			data->fmt.size);
		return -EINVAL;
	}

	vbuf->bytesused = data->fmt.size;
	vbuf->line_offset = 0;

	/*
	 * The DMA is about to overwrite the buffer, so its cache lines are
	 * dropped here rather than written back over the captured frame.
	 */
	sys_cache_data_invd_range(vbuf->buffer, vbuf->bytesused);

	key = k_spin_lock(&data->lock);
	if (data->streaming && data->cur_buf == NULL) {
		data->cur_buf = vbuf;
		isc_start_dma(dev);
	} else {
		k_fifo_put(&data->fifo_in, vbuf);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int isc_dequeue(const struct device *dev, struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct isc_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	/* Drop the cache lines the DMA wrote behind, before the caller reads them */
	if ((*vbuf)->buffer != NULL) {
		sys_cache_data_invd_range((*vbuf)->buffer, (*vbuf)->bytesused);
	}

	return 0;
}

static int isc_flush(const struct device *dev, bool cancel)
{
	struct isc_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		/* Wait for every pending buffer to be captured */
		while (data->streaming &&
		       (!k_fifo_is_empty(&data->fifo_in) || data->cur_buf != NULL)) {
			k_msleep(1);
		}

		return 0;
	}

	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
		k_fifo_put(&data->fifo_out, vbuf);

		if (IS_ENABLED(CONFIG_POLL) && data->sig_out != NULL) {
			k_poll_signal_raise(data->sig_out, VIDEO_BUF_ABORTED);
		}
	}

	return 0;
}

static int isc_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct isc_config *cfg = dev->config;

	return video_set_frmival(cfg->source_dev, frmival);
}

static int isc_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct isc_config *cfg = dev->config;

	return video_get_frmival(cfg->source_dev, frmival);
}

static int isc_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct isc_config *cfg = dev->config;
	const struct isc_input_format *in_fmt;
	struct video_format source_fmt = *fie->format;
	struct video_frmival_enum source_fie = *fie;
	int ret;

	ret = isc_select_input_format(dev, fie->format->pixelformat, &in_fmt);
	if (ret < 0) {
		return ret;
	}

	source_fmt.pixelformat = in_fmt->pixelformat;
	source_fie.format = &source_fmt;

	ret = video_enum_frmival(cfg->source_dev, &source_fie);
	if (ret < 0) {
		return ret;
	}

	source_fie.format = fie->format;
	*fie = source_fie;

	return 0;
}

static void isc_isr(const struct device *dev)
{
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	uint32_t status = cfg->regs->ISC_INTSR & cfg->regs->ISC_INTMASK;
	k_spinlock_key_t key;

	if (status & ISC_INT_ERRORS) {
		LOG_DBG("DMA error, status 0x%08x", status);
	}

	key = k_spin_lock(&data->lock);

	if (status & ISC_INTEN_DDONE_Msk) {
		data->capture_armed = false;

		if (data->cur_buf != NULL) {
			struct video_buffer *vbuf = data->cur_buf;

			vbuf->timestamp = k_uptime_get_32();
			data->cur_buf = NULL;
			k_fifo_put(&data->fifo_out, vbuf);

			if (IS_ENABLED(CONFIG_POLL) && data->sig_out != NULL) {
				k_poll_signal_raise(data->sig_out, VIDEO_BUF_DONE);
			}
		}
	}

	if (status & ISC_INTEN_VD_Msk) {
		data->vd_since_arm++;
		/*
		 * A request taken at one vertical synchronization has to have written its
		 * frame out by the next one. Two without a completion mean the front end
		 * dropped the request rather than honoring it, and since a capture is a
		 * single shot nothing else would ever take it again.
		 */
		if (data->capture_armed && data->vd_since_arm >= 2) {
			data->capture_armed = false;
			LOG_DBG("capture request dropped, arming again");
		}
	}

	if (data->streaming && !data->capture_armed) {
		if (data->cur_buf == NULL) {
			data->cur_buf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
		}
		if (data->cur_buf != NULL) {
			isc_start_dma(dev);
		}
	}

	k_spin_unlock(&data->lock, key);
}

#ifdef CONFIG_POLL
/*
 * Signal raised when a frame lands in the outgoing queue, so that a consumer can
 * wait on it instead of polling. A NULL signal unregisters the previous one.
 */
static int isc_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct isc_data *data = dev->data;

	data->sig_out = sig;

	return 0;
}
#endif /* CONFIG_POLL */

static DEVICE_API(video, isc_driver_api) = {
	.set_format = isc_set_fmt,
	.set_ctrl = isc_set_ctrl,
#ifdef CONFIG_POLL
	.set_signal = isc_set_signal,
#endif
	.get_format = isc_get_fmt,
	.get_caps = isc_get_caps,
	.set_stream = isc_set_stream,
	.enqueue = isc_enqueue,
	.dequeue = isc_dequeue,
	.flush = isc_flush,
	.set_frmival = isc_set_frmival,
	.get_frmival = isc_get_frmival,
	.enum_frmival = isc_enum_frmival,
};

static int isc_init_ctrls(const struct device *dev)
{
	struct isc_data *data = dev->data;
	struct isc_ctrls *ctrls = &data->ctrls;
	int ret;

	/*
	 * This brightness shadows the one the sensor exposes, which is the black
	 * level pedestal the sensor adds to its raw samples. The one here is an
	 * offset on the luma of the converted frames, which is what a host asking
	 * for brightness means.
	 */
	ret = video_init_ctrl(
		&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
		(struct video_ctrl_range){.min = ISC_CBHS_BRIGHTNESS_MIN,
					  .max = ISC_CBHS_BRIGHTNESS_MAX,
					  .step = 1,
					  .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->contrast, dev, VIDEO_CID_CONTRAST,
		(struct video_ctrl_range){.min = 0,
					  .max = ISC_CBHS_CONTRAST_MAX,
					  .step = 1,
					  .def = ISC_CBHS_CONTRAST});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->hue, dev, VIDEO_CID_HUE,
		(struct video_ctrl_range){.min = ISC_CBHS_HUE_MIN,
					  .max = ISC_CBHS_HUE_MAX,
					  .step = 1,
					  .def = 0});
	if (ret < 0) {
		return ret;
	}

	return video_init_ctrl(
		&ctrls->saturation, dev, VIDEO_CID_SATURATION,
		(struct video_ctrl_range){.min = 0,
					  .max = ISC_CBHS_SATURATION_MAX,
					  .step = 1,
					  .def = ISC_CBHS_SATURATION});
}

static int isc_init(const struct device *dev)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct isc_config *cfg = dev->config;
	struct isc_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->source_dev)) {
		LOG_ERR("Source device %s is not ready", cfg->source_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(pmc)) {
		LOG_ERR("Power Management Controller device not ready");
		return -ENODEV;
	}

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	ret = isc_init_ctrls(dev);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(pmc, (clock_control_subsys_t)&cfg->clock_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to enable the peripheral clock (%d)", ret);
		return ret;
	}

	/*
	 * No register is accessed and the interrupt line is left masked here:
	 * the controller is reset, configured and unmasked when the stream is
	 * started. Keeping the boot path free of register access avoids stalling
	 * the bus before the console is up, which would be undebuggable.
	 */
	LOG_DBG("ready, source is %s", cfg->source_dev->name);

	return 0;
}

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

/* Polarities of the parallel input, inactive states are the hardware defaults */
#define ISC_PFE_POLARITIES(n)                                                                      \
	((DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(n, 0, 0), hsync_active, 1) ? 0                          \
									: ISC_PFE_CFG0_HPOL_Msk) |  \
	 (DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(n, 0, 0), vsync_active, 1) ? 0                          \
									: ISC_PFE_CFG0_VPOL_Msk) |  \
	 (DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(n, 0, 0), pclk_sample, 1) ? 0                           \
								      : ISC_PFE_CFG0_PPOL_Msk))

#define ISC_INIT(n)                                                                                \
	static struct isc_data isc_data_##n;                                                       \
                                                                                                   \
	static const struct isc_config isc_cfg_##n = {                                             \
		.regs = (isc_registers_t *)DT_INST_REG_ADDR(n),                                    \
		.source_dev = SOURCE_DEV(n),                                                       \
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),                                         \
		.pfe_cfg0 = ISC_PFE_POLARITIES(n) |                                                \
			    (DT_INST_PROP(n, microchip_mipi_mode) ? ISC_PFE_CFG0_MIPI_Msk : 0),     \
		.irq = DT_INST_IRQN(n),                                                            \
		.black_level_offset = DT_INST_PROP(n, microchip_black_level_offset),               \
		.srgb_gamma = DT_INST_PROP(n, microchip_srgb_gamma),                               \
		.mck_div = DT_INST_PROP(n, microchip_sensor_clock_div),                             \
	};                                                                                         \
                                                                                                   \
	static int isc_init_##n(const struct device *dev)                                          \
	{                                                                                          \
		/* Enabled only while streaming, see isc_set_stream() */                            \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), isc_isr,                     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		return isc_init(dev);                                                              \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &isc_init_##n, NULL, &isc_data_##n, &isc_cfg_##n, POST_KERNEL,     \
			      CONFIG_VIDEO_MCHP_SAM_ISC_INIT_PRIORITY, &isc_driver_api);            \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(isc_##n, DEVICE_DT_INST_GET(n), SOURCE_DEV(n));

DT_INST_FOREACH_STATUS_OKAY(ISC_INIT)
