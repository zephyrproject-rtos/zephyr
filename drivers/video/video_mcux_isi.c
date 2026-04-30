/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_isi

#include <zephyr/devicetree/port-endpoint.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>

#include <fsl_isi.h>

#include "video_ctrls.h"
#include "video_device.h"

/*
 * NOTE: This driver was split into drivers/video/nxp_mcux_isi/.
 * Kept temporarily to ease rebases; it is no longer built.
 */
LOG_MODULE_REGISTER(video_mcux_isi, CONFIG_VIDEO_LOG_LEVEL);

#define ISI_MAX_ACTIVE_BUF 2U

isi_flip_mode_t flip = kISI_FlipDisable;

struct video_mcux_isi_config {
	ISI_Type *base;
	const struct device *source_dev;
	uint8_t input_port;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct video_mcux_isi_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl alpha;
};

struct video_mcux_isi_data {
	const struct device *dev;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;

	/*
	 * Buffers primed before streaming starts.
	 * We take the first two from here to program the HW double-buffer.
	 */
	struct k_fifo fifo_active;
	uint8_t active_primed_cnt;

	isi_config_t isi_config;

	/* Output format (this device endpoint) */
	struct video_format fmt;
	isi_output_format_t isi_format;

	/* Cached upstream/source format (used when CSC is enabled). */
	struct video_format in_fmt; /* upstream/source format */

	struct video_rect crop;
	bool crop_en;
	struct video_rect compose;
	bool compose_en;

	/* Scaler output size before crop (derived from compose or requested format) */
	uint32_t scale_width;
	uint32_t scale_height;

	/* Drop buffer used when no queued buffer is available at frame done. */
	struct video_buffer *drop_vbuf;

	/* Currently programmed HW output buffers (double-buffering). */
	struct video_buffer *active_vbuf[ISI_MAX_ACTIVE_BUF];
	uint8_t buffer_index;

	bool streaming;

	struct video_mcux_isi_ctrls ctrls;

	/* Statistics (updated in ISR) */
	uint32_t drop_cnt;
	uint32_t last_stat_ms;
};

struct isi_format_map {
	uint32_t pixelformat;
	isi_output_format_t isi_format;
};

static const struct isi_format_map isi_formats[] = {
	{.pixelformat = VIDEO_PIX_FMT_RGB565, .isi_format = kISI_OutputRGB565},
	{.pixelformat = VIDEO_PIX_FMT_XRGB32, .isi_format = kISI_OutputXRGB8888},
	{.pixelformat = VIDEO_PIX_FMT_ABGR32, .isi_format = kISI_OutputARGB8888},
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .isi_format = kISI_OutputYUV422_1P8P},
	{.pixelformat = VIDEO_PIX_FMT_XYUV32, .isi_format = kISI_OutputYUV444_1P8P},
	{.pixelformat = VIDEO_PIX_FMT_SRGGB8, .isi_format = kISI_OutputRaw8},
	{.pixelformat = VIDEO_PIX_FMT_SRGGB10, .isi_format = kISI_OutputRaw16P},
	{.pixelformat = VIDEO_PIX_FMT_SRGGB12, .isi_format = kISI_OutputRaw16P},
	{.pixelformat = VIDEO_PIX_FMT_GREY, .isi_format = kISI_OutputRaw8},
};

#define ISI_VIDEO_FORMAT_CAP(width, height, format)                                                \
	{.pixelformat = (format),                                                                  \
	 .width_min = 1,                                                                           \
	 .width_max = (width),                                                                     \
	 .height_min = 1,                                                                          \
	 .height_max = (height),                                                                   \
	 .width_step = 1,                                                                          \
	 .height_step = 1}

/* Limit maximum width to 2K for CSC/resize, 8K for raw passthrough */
static const struct video_format_cap isi_fmts[] = {
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_RGB565),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_XRGB32),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_ABGR32),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_YUYV),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_XYUV32),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_SRGGB8),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_SRGGB10),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_SRGGB12),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_GREY),
	{0},
};

static const struct isi_format_map *isi_find_format(uint32_t pixelformat)
{
	for (size_t i = 0; i < ARRAY_SIZE(isi_formats); i++) {
		if (isi_formats[i].pixelformat == pixelformat) {
			return &isi_formats[i];
		}
	}

	return NULL;
}

static inline bool isi_pixfmt_is_yuv(uint32_t pixelformat)
{
	switch (pixelformat) {
	case VIDEO_PIX_FMT_YUYV:
	case VIDEO_PIX_FMT_UYVY:
	case VIDEO_PIX_FMT_XYUV32:
		return true;
	default:
		return false;
	}
}

static int isi_configure_csc(const struct device *dev)
{
	struct video_mcux_isi_data *data = dev->data;
	const struct video_mcux_isi_config *config = dev->config;
	const bool in_is_yuv = isi_pixfmt_is_yuv(data->in_fmt.pixelformat);
	const bool out_is_yuv = isi_pixfmt_is_yuv(data->fmt.pixelformat);

	static const isi_csc_config_t csc_yuv2rgb = {
		.mode = kISI_CscYCbCr2RGB,
		.A1 = 1.164f,
		.A2 = 0.0f,
		.A3 = 1.596f,
		.B1 = 1.164f,
		.B2 = -0.392f,
		.B3 = -0.813f,
		.C1 = 1.164f,
		.C2 = 2.017f,
		.C3 = 0.0f,
		.D1 = -16,
		.D2 = -128,
		.D3 = -128,
	};

	static const isi_csc_config_t csc_rgb2yuv = {
		.mode = kISI_CscRGB2YCbCr,
		.A1 = 0.257f,
		.A2 = 0.504f,
		.A3 = 0.098f,
		.B1 = -0.148f,
		.B2 = -0.291f,
		.B3 = 0.439f,
		.C1 = 0.439f,
		.C2 = -0.368f,
		.C3 = -0.071f,
		.D1 = 16,
		.D2 = 128,
		.D3 = 128,
	};

	if (data->in_fmt.pixelformat == 0U || data->fmt.pixelformat == 0U) {
		return -EPIPE;
	}

	/* Tell ISI whether the input is YCbCr (YUV) or RGB. */
	data->isi_config.isYCbCr = in_is_yuv;

	if (in_is_yuv == out_is_yuv) {
		ISI_EnableColorSpaceConversion(config->base, 0, false);
		return 0;
	}

	if (out_is_yuv) {
		ISI_SetColorSpaceConversionConfig(config->base, 0, &csc_rgb2yuv);
		ISI_EnableColorSpaceConversion(config->base, 0, true);
		return 0;
	}

	/* out is RGB */
	ISI_SetColorSpaceConversionConfig(config->base, 0, &csc_yuv2rgb);
	ISI_EnableColorSpaceConversion(config->base, 0, true);

	return 0;
}

static void isi_apply_ctrls(const struct device *dev)
{
	struct video_mcux_isi_data *data = dev->data;

	if (data->ctrls.hflip.val) {
		flip |= kISI_FlipHorizontal;
	} else {
		flip &= ~kISI_FlipHorizontal;
	}

	if (data->ctrls.vflip.val) {
		flip |= kISI_FlipVertical;
	} else {
		flip &= ~kISI_FlipVertical;
	}
}

static int isi_channel_configure(const struct device *dev)
{
	struct video_mcux_isi_data *data = dev->data;
	const struct video_mcux_isi_config *config = dev->config;
	ISI_Type *base = config->base;
	uint32_t scale_w;
	uint32_t scale_h;
	uint32_t out_w;
	uint32_t out_h;
	bool hw_crop_en = false;
	int ret;

	if (data->fmt.pixelformat == 0U) {
		LOG_ERR("Video format not configured");
		return -EPIPE;
	}

	/* Derive input color space from upstream format. */
	if (data->in_fmt.pixelformat == 0U) {
		return -EPIPE;
	}
	data->isi_config.isYCbCr = isi_pixfmt_is_yuv(data->in_fmt.pixelformat);

	data->isi_config.inputWidth = data->in_fmt.width;
	data->isi_config.inputHeight = data->in_fmt.height;
	data->isi_config.outputFormat = data->isi_format;

	/* Default: no scaling unless crop/compose requires it. */
	scale_w = data->in_fmt.width;
	scale_h = data->in_fmt.height;

	if (data->compose_en) {
		scale_w = data->scale_width;
		scale_h = data->scale_height;
	}

	out_w = scale_w;
	out_h = scale_h;

	if (data->crop_en) {
		if ((data->crop.left + data->crop.width) > scale_w ||
		    (data->crop.top + data->crop.height) > scale_h) {
			return -EINVAL;
		}

		/* Enable crop only when it actually crops. */
		hw_crop_en = !((data->crop.left == 0U) && (data->crop.top == 0U) &&
			       (data->crop.width == scale_w) && (data->crop.height == scale_h));

		out_w = hw_crop_en ? data->crop.width : scale_w;
		out_h = hw_crop_en ? data->crop.height : scale_h;
	}

	data->fmt.width = out_w;
	data->fmt.height = out_h;
	ret = video_estimate_fmt_size(&data->fmt);
	if (ret) {
		return ret;
	}

	data->isi_config.outputLinePitchBytes = data->fmt.pitch;

	ISI_Init(base, 0);
	ISI_SetConfig(base, 0, &data->isi_config);

	ISI_SetScalerConfig(base, 0, data->in_fmt.width, data->in_fmt.height, scale_w, scale_h);

	if (hw_crop_en) {
		const isi_crop_config_t crop_cfg = {
			.upperLeftX = data->crop.left,
			.upperLeftY = data->crop.top,
			.lowerRightX = data->crop.left + data->crop.width,
			.lowerRightY = data->crop.top + data->crop.height,
		};
		ISI_SetCropConfig(base, 0, &crop_cfg);
		ISI_EnableCrop(base, 0, true);
	} else {
		ISI_EnableCrop(base, 0, false);
	}

	ret = isi_configure_csc(dev);
	if (ret) {
		return ret;
	}

	LOG_DBG("ISI configured: in=%ux%u out=%ux%u pitch=%u", data->in_fmt.width,
		data->in_fmt.height, data->fmt.width, data->fmt.height,
		data->isi_config.outputLinePitchBytes);

	return 0;
}

static int video_mcux_isi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	const struct isi_format_map *map;
	struct video_format in_fmt;
	struct video_format out_fmt;
	int ret;

	map = isi_find_format(fmt->pixelformat);
	if (map == NULL) {
		return -ENOTSUP;
	}

	if (!device_is_ready(config->source_dev)) {
		return -ENODEV;
	}

	in_fmt = *fmt;

	ret = video_estimate_fmt_size(&in_fmt);
	if (ret) {
		return ret;
	}

	ret = video_set_format(config->source_dev, &in_fmt);
	if (ret) {
		return ret;
	}

	ret = video_get_format(config->source_dev, &data->in_fmt);
	if (ret) {
		return ret;
	}

	out_fmt = in_fmt;

	if (data->compose_en) {
		out_fmt.width = data->scale_width;
		out_fmt.height = data->scale_height;
	}

	if (data->crop_en) {
		out_fmt.width = data->crop.width;
		out_fmt.height = data->crop.height;
	}

	ret = video_estimate_fmt_size(&out_fmt);
	if (ret) {
		return ret;
	}

	data->fmt = out_fmt;
	data->isi_format = map->isi_format;

	*fmt = out_fmt;

	LOG_INF("ISI format set: %c%c%c%c, %ux%u, pitch=%u, size=%u", (char)fmt->pixelformat,
		(char)(fmt->pixelformat >> 8), (char)(fmt->pixelformat >> 16),
		(char)(fmt->pixelformat >> 24), fmt->width, fmt->height, fmt->pitch, fmt->size);

	return 0;
}

static int video_mcux_isi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_mcux_isi_data *data = dev->data;

	if (data->fmt.pixelformat == 0U) {
		return -EPIPE;
	}

	*fmt = data->fmt;

	return 0;
}

static int video_mcux_isi_stream_start(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	ISI_Type *base = config->base;
	int ret;

	if (!device_is_ready(config->source_dev)) {
		LOG_ERR("%s: source device not ready", dev->name);
		return -ENODEV;
	}

	ret = isi_channel_configure(dev);
	if (ret) {
		return ret;
	}

	struct video_buffer *vb0;
	struct video_buffer *vb1;

	if (data->drop_vbuf == NULL || data->active_primed_cnt != ISI_MAX_ACTIVE_BUF) {
		LOG_ERR("ISI requires 1 drop buffer + %u active buffers", ISI_MAX_ACTIVE_BUF);
		return -ENOMEM;
	}

	vb0 = k_fifo_get(&data->fifo_active, K_NO_WAIT);
	vb1 = k_fifo_get(&data->fifo_active, K_NO_WAIT);
	if (vb0 == NULL || vb1 == NULL) {
		LOG_ERR("ISI requires %u active buffers", ISI_MAX_ACTIVE_BUF);
		return -ENOMEM;
	}

	data->active_vbuf[0] = vb0;
	data->active_vbuf[1] = vb1;

	ISI_SetOutputBufferAddr(base, 0, 0, (uint32_t)(uintptr_t)vb0->buffer, 0, 0);
	ISI_SetOutputBufferAddr(base, 0, 1, (uint32_t)(uintptr_t)vb1->buffer, 0, 0);

	data->buffer_index = 0U;
	data->streaming = true;

	ISI_ClearInterruptStatus(base, 0, (uint32_t)kISI_FrameReceivedInterrupt);
	ISI_EnableInterrupts(base, 0, (uint32_t)kISI_FrameReceivedInterrupt);
	ISI_Start(base, 0);

	ret = video_stream_start(config->source_dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret) {
		ISI_Stop(base, 0);
		ISI_DisableInterrupts(base, 0, (uint32_t)kISI_FrameReceivedInterrupt);
		data->streaming = false;
		return ret;
	}

	return 0;
}

static void isi_return_buffer(struct video_mcux_isi_data *data, struct video_buffer *vbuf)
{
	if (vbuf == NULL) {
		return;
	}

	/* Mark as not containing a valid frame when returning to user. */
	vbuf->bytesused = 0U;

	k_fifo_put(&data->fifo_out, vbuf);
}

static void isi_return_all_buffers(struct video_mcux_isi_data *data)
{
	struct video_buffer *vbuf;
	struct video_buffer *drop;
	struct video_buffer *active0;
	struct video_buffer *active1;

	/*
	 * Take local copies first and clear state, so that we don't accidentally
	 * re-return buffers due to aliasing (active buffer can be drop buffer).
	 */
	drop = data->drop_vbuf;
	active0 = data->active_vbuf[0];
	active1 = data->active_vbuf[1];

	data->drop_vbuf = NULL;
	data->active_vbuf[0] = NULL;
	data->active_vbuf[1] = NULL;
	data->active_primed_cnt = 0U;

	/* Return special drop buffer first. */
	isi_return_buffer(data, drop);

	/* Return HW active buffers if distinct from drop. */
	if (active0 != drop) {
		isi_return_buffer(data, active0);
	}
	if (active1 != drop && active1 != active0) {
		isi_return_buffer(data, active1);
	}

	/* Return primed buffers (pre-stream). */
	while ((vbuf = k_fifo_get(&data->fifo_active, K_NO_WAIT)) != NULL) {
		if (vbuf != drop && vbuf != active0 && vbuf != active1) {
			isi_return_buffer(data, vbuf);
		}
	}

	/* Return queued input buffers. */
	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
		if (vbuf != drop && vbuf != active0 && vbuf != active1) {
			isi_return_buffer(data, vbuf);
		}
	}
}

static int video_mcux_isi_stream_stop(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	ISI_Type *base = config->base;
	int ret;

	ret = video_stream_stop(config->source_dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret) {
		return ret;
	}

	ISI_Stop(base, 0);
	ISI_DisableInterrupts(base, 0, (uint32_t)kISI_FrameReceivedInterrupt);
	ISI_ClearInterruptStatus(base, 0, (uint32_t)kISI_FrameReceivedInterrupt);

	data->streaming = false;
	isi_return_all_buffers(data);

	return 0;
}

static int video_mcux_isi_set_stream(const struct device *dev, bool enable,
				     enum video_buf_type type)
{
	ARG_UNUSED(type);

	if (enable) {
		return video_mcux_isi_stream_start(dev);
	}

	return video_mcux_isi_stream_stop(dev);
}

static int video_mcux_isi_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_mcux_isi_data *data = dev->data;

	if (data->fmt.pixelformat == 0U) {
		return -EPIPE;
	}

	vbuf->bytesused = data->fmt.size;
	if (vbuf->size < vbuf->bytesused) {
		return -ENOMEM;
	}

	if (!data->streaming) {
		if (data->drop_vbuf == NULL) {
			data->drop_vbuf = vbuf;
			LOG_DBG("Drop frame buffer set: %p", vbuf->buffer);
			return 0;
		}

		/* Prime the two active HW buffers before streaming starts. */
		if (data->active_primed_cnt < ISI_MAX_ACTIVE_BUF) {
			k_fifo_put(&data->fifo_active, vbuf);
			data->active_primed_cnt++;
			LOG_DBG("Active buffer primed (%u/%u): %p", data->active_primed_cnt,
				ISI_MAX_ACTIVE_BUF, vbuf->buffer);
			return 0;
		}
	}

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_mcux_isi_dequeue(const struct device *dev, struct video_buffer **vbuf,
				  k_timeout_t timeout)
{
	struct video_mcux_isi_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_mcux_isi_flush(const struct device *dev, bool cancel)
{
	struct video_mcux_isi_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}
		return 0;
	}

	/*
	 * Cancel all queued buffers.
	 *
	 * If streaming, do not touch active HW buffers here. Users should stop the
	 * stream first to reclaim those buffers.
	 */
	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
		k_fifo_put(&data->fifo_out, vbuf);
	}

	if (!data->streaming) {
		/* Also return drop + primed buffers when not streaming. */
		isi_return_all_buffers(data);
	}

	return 0;
}

static int video_mcux_isi_get_caps(const struct device *dev, struct video_caps *caps)
{
	ARG_UNUSED(dev);

	if (caps == NULL) {
		return -EINVAL;
	}

	caps->format_caps = isi_fmts;
	/* ISI requires at least 3 buffers (2 active + 1 drop frame). */
	caps->min_vbuf_count = 3;

	return 0;
}

static int video_mcux_isi_set_ctrl(const struct device *dev, uint32_t id)
{
	struct video_mcux_isi_data *data = dev->data;

	switch (id) {
	case VIDEO_CID_HFLIP:
	case VIDEO_CID_VFLIP:
	case VIDEO_CID_ALPHA_COMPONENT:
		if (data->streaming) {
			return -EBUSY;
		}

		isi_apply_ctrls(dev);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int video_mcux_isi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_mcux_isi_config *config = dev->config;

	return video_set_frmival(config->source_dev, frmival);
}

static int video_mcux_isi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_mcux_isi_config *config = dev->config;

	return video_get_frmival(config->source_dev, frmival);
}

static int video_mcux_isi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct video_mcux_isi_config *config = dev->config;

	return video_enum_frmival(config->source_dev, fie);
}

static int video_mcux_isi_set_selection(const struct device *dev, struct video_selection *sel)
{
	struct video_mcux_isi_data *data = dev->data;

	if (sel == NULL) {
		return -EINVAL;
	}

	if (data->streaming) {
		return -EBUSY;
	}

	if (sel->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -ENOTSUP;
	}

	switch (sel->target) {
	case VIDEO_SEL_TGT_CROP:
		data->crop = sel->rect;
		data->crop_en = (sel->rect.width != 0U) && (sel->rect.height != 0U);
		return 0;
	case VIDEO_SEL_TGT_COMPOSE:
		data->compose = sel->rect;
		data->compose_en = (sel->rect.width != 0U) && (sel->rect.height != 0U);
		if (data->compose_en) {
			/* Compose defines scaler output size (pre-crop). */
			data->scale_width = data->compose.width;
			data->scale_height = data->compose.height;
		}
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int video_mcux_isi_get_selection(const struct device *dev, struct video_selection *sel)
{
	struct video_mcux_isi_data *data = dev->data;

	if (sel == NULL) {
		return -EINVAL;
	}

	if (sel->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -ENOTSUP;
	}

	switch (sel->target) {
	case VIDEO_SEL_TGT_CROP:
		if (data->crop_en) {
			sel->rect = data->crop;
		} else {
			sel->rect = (struct video_rect){0};
		}
		return 0;
	case VIDEO_SEL_TGT_COMPOSE:
		if (data->compose_en) {
			sel->rect = data->compose;
		} else {
			sel->rect = (struct video_rect){0};
		}
		return 0;
	case VIDEO_SEL_TGT_NATIVE_SIZE:
		if (data->fmt.pixelformat == 0U) {
			return -EPIPE;
		}
		sel->rect.left = 0;
		sel->rect.top = 0;
		sel->rect.width = data->fmt.width;
		sel->rect.height = data->fmt.height;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(video, video_mcux_isi_driver_api) = {
	.set_format = video_mcux_isi_set_fmt,
	.get_format = video_mcux_isi_get_fmt,
	.set_stream = video_mcux_isi_set_stream,
	.enqueue = video_mcux_isi_enqueue,
	.dequeue = video_mcux_isi_dequeue,
	.flush = video_mcux_isi_flush,
	.get_caps = video_mcux_isi_get_caps,
	.set_ctrl = video_mcux_isi_set_ctrl,
	.set_selection = video_mcux_isi_set_selection,
	.get_selection = video_mcux_isi_get_selection,
	.set_frmival = video_mcux_isi_set_frmival,
	.get_frmival = video_mcux_isi_get_frmival,
	.enum_frmival = video_mcux_isi_enum_frmival,
};

static void video_mcux_isi_isr(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	ISI_Type *base = config->base;
	uint32_t int_status;
	struct video_buffer *vbuf;
	uint32_t buffer_addr;
	uint32_t now_ms;

	int_status = ISI_GetInterruptStatus(base, 0);
	ISI_ClearInterruptStatus(base, 0, int_status);

	if ((int_status & (uint32_t)kISI_FrameReceivedInterrupt) == 0U) {
		return;
	}

	/* If this triggers, streaming started without required drop buffer. */
	__ASSERT_NO_MSG(data->drop_vbuf != NULL);
	if (data->drop_vbuf == NULL) {
		return;
	}

	vbuf = data->active_vbuf[data->buffer_index];
	if (vbuf != NULL && vbuf != data->drop_vbuf) {
		vbuf->timestamp = k_uptime_get_32();
		vbuf->bytesused = data->fmt.size;
		k_fifo_put(&data->fifo_out, vbuf);
	}

	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		vbuf = data->drop_vbuf;
		data->drop_cnt++;
	}

	buffer_addr = (uint32_t)(uintptr_t)vbuf->buffer;
	data->active_vbuf[data->buffer_index] = vbuf;

	ISI_SetOutputBufferAddr(base, 0, data->buffer_index, buffer_addr, 0, 0);
	data->buffer_index ^= 1U;

	/* Print at most once per second (avoid log spam / ISR overhead). */
	now_ms = k_uptime_get_32();
	if ((now_ms - data->last_stat_ms) >= 1000U) {
		if (data->drop_cnt) {
			LOG_INF("ISI dropped %u frame(s) (no input buffer)", data->drop_cnt);
			data->drop_cnt = 0U;
		}
		data->last_stat_ms = now_ms;
	}
}

static int video_mcux_isi_init_ctrls(const struct device *dev)
{
	struct video_mcux_isi_data *data = dev->data;
	int ret;

	ret = video_init_ctrl(&data->ctrls.hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&data->ctrls.vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	return video_init_ctrl(
		&data->ctrls.alpha, dev, VIDEO_CID_ALPHA_COMPONENT,
		(struct video_ctrl_range){.min = 0, .max = 255, .step = 1, .def = 0});
}

static int video_mcux_isi_init(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	int ret;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_fifo_init(&data->fifo_active);

	data->dev = dev;
	data->buffer_index = 0U;
	data->streaming = false;
	data->drop_vbuf = NULL;
	data->active_primed_cnt = 0U;
	data->buffer_index = 0U;
	data->drop_cnt = 0U;
	data->last_stat_ms = k_uptime_get_32();
	memset(&data->fmt, 0, sizeof(data->fmt));
	memset(&data->in_fmt, 0, sizeof(data->in_fmt));
	memset(&data->ctrls, 0, sizeof(data->ctrls));
	memset(&data->crop, 0, sizeof(data->crop));
	data->crop_en = false;
	memset(&data->compose, 0, sizeof(data->compose));
	data->compose_en = false;

	ISI_GetDefaultConfig(&data->isi_config);
	data->isi_config.isChannelBypassed = false;
	data->isi_config.isSourceMemory = false;
	data->isi_config.sourcePort = config->input_port;
	data->isi_config.mipiChannel = 0;

	data->fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	data->fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
	data->fmt.width = 1080;
	data->fmt.height = 2340;
	(void)video_estimate_fmt_size(&data->fmt);

	data->in_fmt = data->fmt;
	data->isi_format = kISI_OutputRGB565;

	data->scale_width = data->fmt.width;
	data->scale_height = data->fmt.height;

	data->active_vbuf[0] = NULL;
	data->active_vbuf[1] = NULL;

	ret = video_mcux_isi_init_ctrls(dev);
	if (ret) {
		return ret;
	}

	isi_apply_ctrls(dev);

	if (config->clock_dev) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("%s: clock device not ready", dev->name);
			return -ENODEV;
		}

		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret) {
			LOG_ERR("%s: failed to enable clock", dev->name);
			return ret;
		}
	}

	return 0;
}

#define ISI_SOURCE_DEV(inst)                                                                       \
	DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))

#define VIDEO_MCUX_ISI_DEVICE(inst)                                                                \
	static const struct video_mcux_isi_config video_mcux_isi_config_##inst = {                 \
		.base = (ISI_Type *)DT_INST_REG_ADDR(inst),                                        \
		.source_dev = ISI_SOURCE_DEV(inst),                                                \
		.input_port = DT_INST_PROP_OR(inst, input_port, 0),                                \
		.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks), \
			(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))), (NULL)),                        \
			 .clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks), \
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name)), (NULL)), \
	};                                                                                         \
	static struct video_mcux_isi_data video_mcux_isi_data_##inst;                              \
	static int video_mcux_isi_init_##inst(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), video_mcux_isi_isr,   \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
		return video_mcux_isi_init(dev);                                                   \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, &video_mcux_isi_init_##inst, NULL,                             \
			      &video_mcux_isi_data_##inst, &video_mcux_isi_config_##inst,          \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,                             \
			      &video_mcux_isi_driver_api);                                         \
	VIDEO_DEVICE_DEFINE(video_mcux_isi_##inst, DEVICE_DT_INST_GET(inst), ISI_SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(VIDEO_MCUX_ISI_DEVICE)
