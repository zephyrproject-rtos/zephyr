/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "video_ctrls.h"
#include "video_device.h"

#include <fsl_pxp.h>
#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#define DT_DRV_COMPAT nxp_pxp

LOG_MODULE_REGISTER(mcux_pxp, CONFIG_VIDEO_LOG_LEVEL);

struct mcux_pxp_config {
	PXP_Type *base;
	void (*irq_config_func)(void);
};

struct mcux_pxp_ctrls {
	struct {
		struct video_ctrl rotation;
		struct video_ctrl hflip;
		struct video_ctrl vflip;
	};
};

struct pxp_pixfmt_map {
	uint32_t v_pixfmt;
	pxp_ps_pixel_format_t ps_pixfmt;
	pxp_output_pixel_format_t output_pixfmt;
};

static const struct pxp_pixfmt_map pxp_pixfmt_maps[] = {
	/*
	 * ARGB32 is supported so XRGB32 can be supported too though this is not explicitly
	 * declared in the datasheet.
	 * 
	 * FIXME: As PxP's ARGB8888 format corresponds to both VIDEO_PIX_FMT_XRGB32 and
	 * VIDEO_PIX_FMT_ARGB32, we place VIDEO_PIX_FMT_XRGB32 at 1st position to favorite
	 * this format.
	 */
	{VIDEO_PIX_FMT_XRGB32, kPXP_PsPixelFormatARGB8888, kPXP_OutputPixelFormatARGB8888},
	{VIDEO_PIX_FMT_ARGB32, kPXP_PsPixelFormatARGB8888, kPXP_OutputPixelFormatARGB8888},
	{VIDEO_PIX_FMT_RGB24, -1, kPXP_OutputPixelFormatRGB888},
	{VIDEO_PIX_FMT_RGB565, kPXP_PsPixelFormatRGB565, kPXP_OutputPixelFormatRGB565},
	{VIDEO_PIX_FMT_GREY, kPXP_PsPixelFormatY8, kPXP_OutputPixelFormatY8},
	{VIDEO_PIX_FMT_UYVY, kPXP_PsPixelFormatUYVY1P422, kPXP_OutputPixelFormatUYVY1P422},
	{VIDEO_PIX_FMT_VYUY, kPXP_PsPixelFormatVYUY1P422, kPXP_OutputPixelFormatVYUY1P422},
	{VIDEO_PIX_FMT_NV12, kPXP_PsPixelFormatYUV2P420, kPXP_OutputPixelFormatYUV2P420},
	{VIDEO_PIX_FMT_NV21, kPXP_PsPixelFormatYVU2P420, kPXP_OutputPixelFormatYVU2P420},
	{VIDEO_PIX_FMT_NV16, kPXP_PsPixelFormatYUV2P422, kPXP_OutputPixelFormatYUV2P422},
	{VIDEO_PIX_FMT_NV61, kPXP_PsPixelFormatYVU2P422, kPXP_OutputPixelFormatYVU2P422},
	{VIDEO_PIX_FMT_YVU422M, kPXP_PsPixelFormatYVU422, -1},
	{VIDEO_PIX_FMT_YVU420M, kPXP_PsPixelFormatYVU420, -1},
};

#define PXP_MIN_LENGTH  1
#define PXP_MAX_LENGTH  4096
#define PXP_STEP_LENGTH 1

#define PXP_FORMAT_CAP(pixfmt)                                                                     \
	{                                                                                          \
		.pixelformat = pixfmt,                                                             \
		.width_min = PXP_MIN_LENGTH,                                                       \
		.width_max = PXP_MAX_LENGTH,                                                       \
		.height_min = PXP_MIN_LENGTH,                                                      \
		.height_max = PXP_MAX_LENGTH,                                                      \
		.width_step = PXP_STEP_LENGTH,                                                     \
		.height_step = PXP_STEP_LENGTH,                                                    \
	}

static const struct video_format_cap pxp_out_caps[] = {
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_XRGB32),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_ARGB32),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_RGB24),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_GREY),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_UYVY),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_VYUY),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV12),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV21),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV16),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV61),
	{0},
};

static const struct video_format_cap pxp_in_caps[] = {
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_XRGB32),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_ARGB32),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_RGB24),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_GREY),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_UYVY),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_VYUY),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV12),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV21),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV16),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_NV61),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_YVU422M),
	PXP_FORMAT_CAP(VIDEO_PIX_FMT_YVU420M),
	{0},
};

struct mcux_pxp_data {
	struct mcux_pxp_ctrls ctrls;
	uint32_t ps_fmt_height;
	pxp_ps_buffer_config_t ps_buffer_cfg;
	pxp_output_buffer_config_t output_buffer_cfg;
	struct k_fifo fifo_in_queued;
	struct k_fifo fifo_in_done;
	struct k_fifo fifo_out_queued;
	struct k_fifo fifo_out_done;
	bool streaming;
};

static void mcux_pxp_isr(const struct device *const dev)
{
	const struct mcux_pxp_config *config = dev->config;
	struct mcux_pxp_data *data = dev->data;

	PXP_ClearStatusFlags(config->base, kPXP_CompleteFlag);

	data->streaming = false;

	struct video_buffer *vbuf_in = k_fifo_get(&data->fifo_in_queued, K_NO_WAIT);
	struct video_buffer *vbuf_out = k_fifo_get(&data->fifo_out_queued, K_NO_WAIT);

	/* Something went wrong, should never happen */
	if ((uint32_t)vbuf_in->buffer != data->ps_buffer_cfg.bufferAddr ||
	    (uint32_t)vbuf_out->buffer != data->output_buffer_cfg.buffer0Addr) {
		return;
	}

	vbuf_out->timestamp = k_uptime_get_32();
	vbuf_out->bytesused = data->output_buffer_cfg.pitchBytes * data->output_buffer_cfg.height;

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange((uint32_t)vbuf_out->buffer, vbuf_out->bytesused);
#endif

	k_fifo_put(&data->fifo_in_done, vbuf_in);
	k_fifo_put(&data->fifo_out_done, vbuf_out);
}

static int mcux_pxp_get_caps(const struct device *dev, struct video_caps *caps)
{
	if (caps->type == VIDEO_BUF_TYPE_INPUT) {
		caps->format_caps = pxp_in_caps;
	} else {
		caps->format_caps = pxp_out_caps;
	}

	caps->min_vbuf_count = 1;
	caps->buf_align = 64;

	return 0;
}

static int mcux_pxp_transform_cap(const struct device *const dev,
				  const struct video_format_cap *const cap,
				  struct video_format_cap *const res_cap,
				  enum video_buf_type direction, uint16_t ind)
{
	struct mcux_pxp_data *data = dev->data;
	struct mcux_pxp_ctrls *ctrls = &data->ctrls;

	if (ind > 0) {
		return -ERANGE;
	}

	if (ctrls->rotation.val == 0 || ctrls->rotation.val == 180) {
		*res_cap = *cap;
	} else {
		/* For 90 or 270 deg rotation, swap width and height */
		res_cap->pixelformat = cap->pixelformat;

		res_cap->width_min = cap->height_min;
		res_cap->width_max = cap->height_max;
		res_cap->width_step = cap->height_step;

		res_cap->height_min = cap->width_min;
		res_cap->height_max = cap->width_max;
		res_cap->height_step = cap->width_step;
	}

	res_cap->width_min = MAX(res_cap->width_min, PXP_MIN_LENGTH);
	res_cap->height_min = MAX(res_cap->height_min, PXP_MIN_LENGTH);
	res_cap->width_max = MIN(res_cap->width_max, PXP_MAX_LENGTH);
	res_cap->height_max = MIN(res_cap->height_max, PXP_MAX_LENGTH);

	return 0;
}

static int mcux_pxp_get_fmt(const struct device *dev, struct video_format *fmt)
{
	uint8_t i;
	unsigned int bpp;
	struct mcux_pxp_data *data = dev->data;

	if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
		// Find the video format that matches the PXP ps format
		for (i = 0; i < ARRAY_SIZE(pxp_pixfmt_maps); i++) {
			if (pxp_pixfmt_maps[i].ps_pixfmt == data->ps_buffer_cfg.pixelFormat) {
				fmt->pixelformat = pxp_pixfmt_maps[i].v_pixfmt;
				break;
			}
		}
		if (i == ARRAY_SIZE(pxp_pixfmt_maps)) {
			return -EINVAL;
		}

		fmt->pitch = data->ps_buffer_cfg.pitchBytes;

		bpp = video_bits_per_pixel(fmt->pixelformat);
		if (bpp == 0) {
			return -EINVAL;
		}

		fmt->width = fmt->pitch / bpp * BITS_PER_BYTE;
		fmt->height = data->ps_fmt_height;
	} else {
		// Find the video format that matches the PXP output format
		for (i = 0; i < ARRAY_SIZE(pxp_pixfmt_maps); i++) {
			if (pxp_pixfmt_maps[i].output_pixfmt ==
			    data->output_buffer_cfg.pixelFormat) {
				fmt->pixelformat = pxp_pixfmt_maps[i].v_pixfmt;
				break;
			}
		}
		if (i == ARRAY_SIZE(pxp_pixfmt_maps)) {
			return -EINVAL;
		}

		fmt->pitch = data->output_buffer_cfg.pitchBytes;
		fmt->width = data->output_buffer_cfg.width;
		fmt->height = data->output_buffer_cfg.height;
	}

	fmt->size = fmt->pitch * fmt->height;

	return 0;
}

static int mcux_pxp_set_fmt(const struct device *dev, struct video_format *fmt)
{
	uint8_t sz, i;
	const struct video_format_cap *src_caps;
	struct mcux_pxp_data *data = dev->data;
	int ret = video_estimate_fmt_size(fmt);

	if (ret < 0) {
		return ret;
	}

	if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
		src_caps = pxp_in_caps;
		sz = ARRAY_SIZE(pxp_in_caps);
	} else {
		src_caps = pxp_out_caps;
		sz = ARRAY_SIZE(pxp_out_caps);
	}

	for (i = 0; i < sz; i++) {
		if (src_caps[i].pixelformat == fmt->pixelformat) {
			break;
		}
	}

	if (i == sz || !IN_RANGE(fmt->width, PXP_MIN_LENGTH, PXP_MAX_LENGTH) ||
	    !IN_RANGE(fmt->height, PXP_MIN_LENGTH, PXP_MAX_LENGTH)) {
		return -EINVAL;
	}

	if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
		data->ps_buffer_cfg.pixelFormat = pxp_pixfmt_maps[i].ps_pixfmt;
		data->ps_buffer_cfg.pitchBytes = fmt->pitch;
		data->ps_fmt_height = fmt->height;
	} else {
		data->output_buffer_cfg.pixelFormat =
			pxp_pixfmt_maps[i].output_pixfmt;
		data->output_buffer_cfg.pitchBytes = fmt->pitch;
		data->output_buffer_cfg.width = fmt->width;
		data->output_buffer_cfg.height = fmt->height;
	}

	return 0;
}

static int mcux_pxp_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct mcux_pxp_config *const config = dev->config;
	struct mcux_pxp_data *data = dev->data;
	struct mcux_pxp_ctrls *ctrls = &data->ctrls;
	pxp_flip_mode_t flip = ctrls->hflip.val | ctrls->vflip.val;
	pxp_rotate_degree_t rotate = kPXP_Rotate0;

	ARG_UNUSED(id);

	switch (ctrls->rotation.val) {
	case 0:
		rotate = kPXP_Rotate0;
		break;
	case 90:
		rotate = kPXP_Rotate90;
		break;
	case 180:
		rotate = kPXP_Rotate180;
		break;
	case 270:
		rotate = kPXP_Rotate270;
		break;
	}

	PXP_SetRotateConfig(config->base, kPXP_RotateProcessSurface, rotate, flip);

	return 0;
}

static int mcux_pxp_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct mcux_pxp_config *const config = dev->config;
	struct mcux_pxp_data *data = dev->data;

	if (!data->streaming && enable && !k_fifo_is_empty(&data->fifo_in_queued) &&
	    !k_fifo_is_empty(&data->fifo_out_queued)) {

		/* Valide the input / output formats and rotation before start streaming */
		uint8_t i;

		for (i = 0; i < ARRAY_SIZE(pxp_pixfmt_maps); i++) {
			if (pxp_pixfmt_maps[i].ps_pixfmt == data->ps_buffer_cfg.pixelFormat) {
				break;
			}
		}
		if (i == ARRAY_SIZE(pxp_pixfmt_maps)) {
			return -EINVAL;
		}

		unsigned int bpp = video_bits_per_pixel(pxp_pixfmt_maps[i].v_pixfmt);
		if (bpp == 0) {
			return -EINVAL;
		}
		uint32_t ps_width = data->ps_buffer_cfg.pitchBytes / bpp * BITS_PER_BYTE;
		struct mcux_pxp_ctrls *ctrls = &data->ctrls;
		bool cond1 = (ctrls->rotation.val == 0 || ctrls->rotation.val == 180) &&
				 (data->output_buffer_cfg.width != ps_width ||
				  data->output_buffer_cfg.height != data->ps_fmt_height);
		bool cond2 = (ctrls->rotation.val == 90 || ctrls->rotation.val == 270) &&
			     (data->output_buffer_cfg.width != data->ps_fmt_height ||
			      data->output_buffer_cfg.height != ps_width);

		if (cond1 || cond2) {
			return -ENOTSUP;
		}

#ifdef CONFIG_HAS_MCUX_CACHE
		DCACHE_CleanByRange((uint32_t)data->ps_buffer_cfg.bufferAddr,
				    data->output_buffer_cfg.pitchBytes *
					    data->output_buffer_cfg.height);
#endif
		PXP_Start(config->base);

		data->streaming = true;
	}

	return 0;
}

static int mcux_pxp_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	const struct mcux_pxp_config *const config = dev->config;
	struct mcux_pxp_data *data = dev->data;

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t)vbuf->buffer, vbuf->size);
#endif

	if (vbuf->type == VIDEO_BUF_TYPE_INPUT) {
		k_fifo_put(&data->fifo_in_queued, vbuf);

		data->ps_buffer_cfg.swapByte = false;
		data->ps_buffer_cfg.bufferAddr = (uint32_t)vbuf->buffer;

		switch (data->ps_buffer_cfg.pixelFormat) {
		case kPXP_PsPixelFormatYVU422:
		case kPXP_PsPixelFormatYVU420:
			uint32_t y_size =
				data->output_buffer_cfg.width * data->output_buffer_cfg.height;
			uint32_t v_size = y_size / 2;

			if (data->ps_buffer_cfg.pixelFormat == VIDEO_PIX_FMT_YVU420M) {
				v_size = v_size / 2;
			}

			data->ps_buffer_cfg.bufferAddrV = (uint32_t)vbuf->buffer + y_size;
			data->ps_buffer_cfg.bufferAddrU = (uint32_t)vbuf->buffer + y_size + v_size;
			break;
		default:
			data->ps_buffer_cfg.bufferAddrU = 0U;
			data->ps_buffer_cfg.bufferAddrV = 0U;
			break;
		}

		PXP_SetProcessSurfaceBufferConfig(config->base, &data->ps_buffer_cfg);
	} else {
		k_fifo_put(&data->fifo_out_queued, vbuf);

		data->output_buffer_cfg.interlacedMode = kPXP_OutputProgressive;
		data->output_buffer_cfg.buffer0Addr = (uint32_t)vbuf->buffer;
		data->output_buffer_cfg.buffer1Addr = 0U;

		PXP_SetOutputBufferConfig(config->base, &data->output_buffer_cfg);
		/* We only support a process surface that covers the full buffer */
		PXP_SetProcessSurfacePosition(config->base, 0U, 0U, data->output_buffer_cfg.width,
					      data->output_buffer_cfg.height);
	}

	mcux_pxp_set_stream(dev, true, vbuf->type);

	return 0;
}

static int mcux_pxp_dequeue(const struct device *dev, struct video_buffer **vbuf,
			    k_timeout_t timeout)
{
	struct mcux_pxp_data *data = dev->data;

	switch ((*vbuf)->type) {
	case VIDEO_BUF_TYPE_INPUT:
		*vbuf = k_fifo_get(&data->fifo_in_done, timeout);
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		*vbuf = k_fifo_get(&data->fifo_out_done, timeout);
		break;
	default:
		return -ENOTSUP;
	}

	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static DEVICE_API(video, mcux_pxp_driver_api) = {
	.get_caps = mcux_pxp_get_caps,
	.transform_cap = mcux_pxp_transform_cap,
	.get_format = mcux_pxp_get_fmt,
	.set_format = mcux_pxp_set_fmt,
	.set_ctrl = mcux_pxp_set_ctrl,
	.set_stream = mcux_pxp_set_stream,
	.enqueue = mcux_pxp_enqueue,
	.dequeue = mcux_pxp_dequeue,
};

static int mcux_pxp_init(const struct device *const dev)
{
	const struct mcux_pxp_config *const config = dev->config;
	struct mcux_pxp_data *data = dev->data;
	struct mcux_pxp_ctrls *ctrls = &data->ctrls;
	int ret;

	k_fifo_init(&data->fifo_in_queued);
	k_fifo_init(&data->fifo_out_queued);
	k_fifo_init(&data->fifo_in_done);
	k_fifo_init(&data->fifo_out_done);

	PXP_Init(config->base);

	PXP_SetProcessSurfaceBackGroundColor(config->base, 0U);
	/* Disable alpha surface and CSC1 */
	PXP_SetAlphaSurfacePosition(config->base, 0xFFFFU, 0xFFFFU, 0U, 0U);
	PXP_EnableCsc1(config->base, false);
	PXP_EnableInterrupts(config->base, kPXP_CompleteInterruptEnable);

	config->irq_config_func();

	ret = video_init_ctrl(
		&ctrls->rotation, dev, VIDEO_CID_ROTATE,
		(struct video_ctrl_range){.min = 0, .max = 270, .step = 90, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	video_auto_cluster_ctrl(&ctrls->rotation, 3, false);

	return 0;
}

#define MCUX_PXP_INIT(inst)                                                                        \
	static void mcux_pxp_irq_config_##inst(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), mcux_pxp_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static const struct mcux_pxp_config mcux_pxp_config_##inst = {                             \
		.base = (PXP_Type *)DT_INST_REG_ADDR(inst),                                        \
		.irq_config_func = mcux_pxp_irq_config_##inst,                                     \
	};                                                                                         \
                                                                                                   \
	static struct mcux_pxp_data mcux_pxp_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &mcux_pxp_init, NULL, &mcux_pxp_data_##inst,                   \
			      &mcux_pxp_config_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &mcux_pxp_driver_api);                                               \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(pxp_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(MCUX_PXP_INIT)
