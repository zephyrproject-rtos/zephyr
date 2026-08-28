/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Microchip SAMA7G5 CSI-2 Demux Controller (CSI2DC).
 *
 * The CSI2DC receives the CSI-2 packet stream from the MIPI CSI-2 host
 * controller over the internal IDI interface. Its video pipeline extracts one
 * virtual channel and one data type and outputs the pixels as an unpacked
 * parallel stream towards the Image Sensor Controller (ISC):
 *
 *   sensor --CSI-2--> csi2host --IDI--> csi2dc --parallel--> isc --> memory
 *
 * Since the pipeline unpacks the CSI-2 payload, the format seen on the parallel
 * side differs from the one on the CSI-2 side: a packed 10-bit Bayer stream
 * (VIDEO_PIX_FMT_SBGGR10P) is presented to the ISC as one 10-bit sample per
 * pixel (VIDEO_PIX_FMT_SBGGR10). This driver performs that translation.
 */

#define DT_DRV_COMPAT microchip_sama7g5_csi2dc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>
#include <soc.h>

#include "video_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(video_mchp_sam_csi2dc, CONFIG_VIDEO_LOG_LEVEL);

/*
 * Inter-line minimum delay inserted in the output waveform, in other words how
 * long the parallel output towards the image sensor controller is held back
 * between lines. None is needed.
 *
 * Linux carries a comment claiming the SAMA7G5 requires a delay of 15, but the
 * value it computes is (15 & GENMASK(7, 4)), which is zero, so the delay it
 * actually programs is none. The bare metal reference code programs none either.
 */
#define CSI2DC_HLC_DELAY 0

/* Maximum number of formats reported by get_caps() */
#define CSI2DC_MAX_CAPS 8

/**
 * @brief Description of a format handled by the video pipeline
 *
 * @param csi_pixelformat format as received on the CSI-2 link
 * @param pixelformat format as output on the parallel link to the ISC
 * @param datatype CSI-2 data type carrying this format
 */
struct csi2dc_format {
	uint32_t csi_pixelformat;
	uint32_t pixelformat;
	uint8_t datatype;
};

#define CSI2DC_FMT(csi_fmt, fmt, dt)                                                               \
	{                                                                                          \
		.csi_pixelformat = VIDEO_PIX_FMT_##csi_fmt,                                        \
		.pixelformat = VIDEO_PIX_FMT_##fmt,                                                \
		.datatype = dt,                                                                    \
	}

static const struct csi2dc_format csi2dc_formats[] = {
	/* Raw 8-bit */
	CSI2DC_FMT(SBGGR8, SBGGR8, VIDEO_MIPI_CSI2_DT_RAW8),
	CSI2DC_FMT(SGBRG8, SGBRG8, VIDEO_MIPI_CSI2_DT_RAW8),
	CSI2DC_FMT(SGRBG8, SGRBG8, VIDEO_MIPI_CSI2_DT_RAW8),
	CSI2DC_FMT(SRGGB8, SRGGB8, VIDEO_MIPI_CSI2_DT_RAW8),
	/* Raw 10-bit, unpacked into one 10-bit sample per pixel */
	CSI2DC_FMT(SBGGR10P, SBGGR10, VIDEO_MIPI_CSI2_DT_RAW10),
	CSI2DC_FMT(SGBRG10P, SGBRG10, VIDEO_MIPI_CSI2_DT_RAW10),
	CSI2DC_FMT(SGRBG10P, SGRBG10, VIDEO_MIPI_CSI2_DT_RAW10),
	CSI2DC_FMT(SRGGB10P, SRGGB10, VIDEO_MIPI_CSI2_DT_RAW10),
	/* Raw 12-bit, unpacked into one 12-bit sample per pixel */
	CSI2DC_FMT(SBGGR12P, SBGGR12, VIDEO_MIPI_CSI2_DT_RAW12),
	CSI2DC_FMT(SGBRG12P, SGBRG12, VIDEO_MIPI_CSI2_DT_RAW12),
	CSI2DC_FMT(SGRBG12P, SGRBG12, VIDEO_MIPI_CSI2_DT_RAW12),
	CSI2DC_FMT(SRGGB12P, SRGGB12, VIDEO_MIPI_CSI2_DT_RAW12),
	/* YUV 4:2:2, two samples per pixel on the parallel bus */
	CSI2DC_FMT(YUYV, YUYV, VIDEO_MIPI_CSI2_DT_YUV422_8),
	/* Grayscale */
	CSI2DC_FMT(GREY, GREY, VIDEO_MIPI_CSI2_DT_RAW8),
};

struct csi2dc_config {
	csi2dc_registers_t *regs;
	const struct device *source_dev;
	struct sam_clk_cfg clock_cfg;
	uint8_t virtual_channel;
	bool non_continuous_clock;
};

struct csi2dc_data {
	const struct csi2dc_format *fmt_desc;
	struct video_format fmt;
	struct video_format_cap caps[CSI2DC_MAX_CAPS + 1];
};

static const struct csi2dc_format *csi2dc_get_csi_format(uint32_t csi_pixelformat)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(csi2dc_formats); i++) {
		if (csi2dc_formats[i].csi_pixelformat == csi_pixelformat) {
			return &csi2dc_formats[i];
		}
	}

	return NULL;
}

static const struct csi2dc_format *csi2dc_get_format(uint32_t pixelformat)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(csi2dc_formats); i++) {
		if (csi2dc_formats[i].pixelformat == pixelformat) {
			return &csi2dc_formats[i];
		}
	}

	return NULL;
}

static int csi2dc_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct csi2dc_config *cfg = dev->config;
	struct csi2dc_data *data = dev->data;
	const struct csi2dc_format *desc;
	struct video_format source_fmt = *fmt;
	int ret;

	desc = csi2dc_get_format(fmt->pixelformat);
	if (desc == NULL) {
		LOG_ERR("Format %s is not supported by the video pipeline",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat));
		return -ENOTSUP;
	}

	source_fmt.pixelformat = desc->csi_pixelformat;

	ret = video_set_format(cfg->source_dev, &source_fmt);
	if (ret < 0) {
		return ret;
	}

	/* The parallel side is unpacked, so its line size differs from the CSI-2 one */
	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	data->fmt = *fmt;
	data->fmt_desc = desc;

	return 0;
}

static int csi2dc_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct csi2dc_config *cfg = dev->config;
	struct csi2dc_data *data = dev->data;
	const struct csi2dc_format *desc;
	int ret;

	if (data->fmt_desc == NULL) {
		ret = video_get_format(cfg->source_dev, fmt);
		if (ret < 0) {
			return ret;
		}

		desc = csi2dc_get_csi_format(fmt->pixelformat);
		if (desc == NULL) {
			LOG_ERR("Source format %s is not supported by the video pipeline",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat));
			return -ENOTSUP;
		}

		fmt->pixelformat = desc->pixelformat;
		ret = video_estimate_fmt_size(fmt);
		if (ret < 0) {
			return ret;
		}

		data->fmt = *fmt;
		data->fmt_desc = desc;
	}

	*fmt = data->fmt;

	return 0;
}

static int csi2dc_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct csi2dc_config *cfg = dev->config;
	struct csi2dc_data *data = dev->data;
	unsigned int nb = 0;
	int ret;

	ret = video_get_caps(cfg->source_dev, caps);
	if (ret < 0) {
		return ret;
	}

	/* Translate the CSI-2 formats of the source into the parallel formats */
	for (unsigned int i = 0; caps->format_caps[i].pixelformat != 0; i++) {
		const struct csi2dc_format *desc =
			csi2dc_get_csi_format(caps->format_caps[i].pixelformat);

		if (desc == NULL) {
			continue;
		}

		if (nb == CSI2DC_MAX_CAPS) {
			LOG_WRN("Source reports more formats than the %u supported ones",
				CSI2DC_MAX_CAPS);
			break;
		}

		data->caps[nb] = caps->format_caps[i];
		data->caps[nb].pixelformat = desc->pixelformat;
		nb++;
	}

	memset(&data->caps[nb], 0, sizeof(data->caps[nb]));
	caps->format_caps = data->caps;

	return 0;
}

static int csi2dc_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct csi2dc_config *cfg = dev->config;
	struct csi2dc_data *data = dev->data;
	csi2dc_registers_t *regs = cfg->regs;
	int ret;

	if (!enable) {
		ret = video_stream_stop(cfg->source_dev, type);

		LOG_DBG("last frame: %u columns, %u rows, pipe status 0x%08x",
			regs->CSI2DC_VPCOLR, regs->CSI2DC_VPROWR, regs->CSI2DC_VPISR);

		regs->CSI2DC_VPER = 0;
		regs->CSI2DC_PUR = CSI2DC_PUR_VP_Msk;

		return ret;
	}

	if (data->fmt_desc == NULL) {
		LOG_ERR("The format must be set before starting the stream");
		return -EIO;
	}

	/* Reset the controller now that the ISC provides the pipeline clock */
	regs->CSI2DC_GCTLR = CSI2DC_GCTLR_SWRST_Msk;

	/*
	 * MIPIFRN runs the video pipe on the controller's own clock instead of
	 * synchronizing it to the serial clock recovered from the link. It is set
	 * by default, and clearing it is only correct when the clock feeding this
	 * controller is itself gated - not merely because the sensor gates its
	 * clock lane, since the pipe then cannot drain between packets and the
	 * packet buffer overflows.
	 */
	regs->CSI2DC_GCFGR = CSI2DC_GCFGR_HLC(CSI2DC_HLC_DELAY) |
			     (cfg->non_continuous_clock ? 0 : CSI2DC_GCFGR_MIPIFRN_Msk);

	/*
	 * Extract the configured virtual channel and data type. Payload
	 * decompression is left disabled and post adjustment enabled, so that
	 * samples are presented to the ISC aligned on the parallel bus.
	 */
	regs->CSI2DC_VPCFGR = CSI2DC_VPCFGR_DT(data->fmt_desc->datatype) |
			      CSI2DC_VPCFGR_VC(cfg->virtual_channel) | CSI2DC_VPCFGR_PA_Msk;

	regs->CSI2DC_VPER = CSI2DC_VPER_ENABLE_Msk;
	regs->CSI2DC_PUR = CSI2DC_PUR_VP_Msk;

	return video_stream_start(cfg->source_dev, type);
}

static int csi2dc_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct csi2dc_config *cfg = dev->config;

	return video_set_frmival(cfg->source_dev, frmival);
}

static int csi2dc_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct csi2dc_config *cfg = dev->config;

	return video_get_frmival(cfg->source_dev, frmival);
}

static int csi2dc_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct csi2dc_config *cfg = dev->config;
	struct video_format source_fmt;
	const struct csi2dc_format *desc;
	struct video_frmival_enum source_fie = *fie;
	int ret;

	desc = csi2dc_get_format(fie->format->pixelformat);
	if (desc == NULL) {
		return -ENOTSUP;
	}

	source_fmt = *fie->format;
	source_fmt.pixelformat = desc->csi_pixelformat;
	source_fie.format = &source_fmt;

	ret = video_enum_frmival(cfg->source_dev, &source_fie);
	if (ret < 0) {
		return ret;
	}

	source_fie.format = fie->format;
	*fie = source_fie;

	return 0;
}

static DEVICE_API(video, csi2dc_driver_api) = {
	.set_format = csi2dc_set_fmt,
	.get_format = csi2dc_get_fmt,
	.get_caps = csi2dc_get_caps,
	.set_stream = csi2dc_set_stream,
	.set_frmival = csi2dc_set_frmival,
	.get_frmival = csi2dc_get_frmival,
	.enum_frmival = csi2dc_enum_frmival,
};

static int csi2dc_init(const struct device *dev)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct csi2dc_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->source_dev)) {
		LOG_ERR("Source device %s is not ready", cfg->source_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(pmc)) {
		LOG_ERR("Power Management Controller device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(pmc, (clock_control_subsys_t)&cfg->clock_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to enable the peripheral clock (%d)", ret);
		return ret;
	}

	/*
	 * No register is accessed here on purpose. Besides the peripheral clock
	 * enabled above, the controller is clocked by the sensor controller
	 * clock generated by the ISC, which the ISC driver enables when it
	 * initializes, after this device. Accessing a register whose clock
	 * domain is not running can stall the bus, so the reset and the whole
	 * configuration are deferred to the moment the stream is started.
	 */
	LOG_DBG("ready, source is %s", cfg->source_dev->name);

	return 0;
}

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

#define CSI2DC_INIT(n)                                                                             \
	static struct csi2dc_data csi2dc_data_##n;                                                  \
                                                                                                   \
	static const struct csi2dc_config csi2dc_cfg_##n = {                                       \
		.regs = (csi2dc_registers_t *)DT_INST_REG_ADDR(n),                                 \
		.source_dev = SOURCE_DEV(n),                                                       \
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),                                         \
		.virtual_channel = DT_INST_PROP(n, microchip_virtual_channel),                     \
		.non_continuous_clock = DT_INST_PROP(n, microchip_non_continuous_clock),            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &csi2dc_init, NULL, &csi2dc_data_##n, &csi2dc_cfg_##n,             \
			      POST_KERNEL, CONFIG_VIDEO_MCHP_SAM_CSI2DC_INIT_PRIORITY,             \
			      &csi2dc_driver_api);                                                 \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(csi2dc_##n, DEVICE_DT_INST_GET(n), SOURCE_DEV(n));

DT_INST_FOREACH_STATUS_OKAY(CSI2DC_INIT)
