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

struct mcux_pxp_data {
	struct mcux_pxp_ctrls ctrls;
	pxp_ps_buffer_config_t ps_buffer_cfg;
	pxp_output_buffer_config_t output_buffer_cfg;
	struct k_fifo fifo_in_queued;
	struct k_fifo fifo_in_done;
	struct k_fifo fifo_out_queued;
	struct k_fifo fifo_out_done;
};

static void mcux_pxp_isr(const struct device *const dev)
{
	const struct mcux_pxp_config *config = dev->config;
	struct mcux_pxp_data *data = dev->data;

	PXP_ClearStatusFlags(config->base, kPXP_CompleteFlag);

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
	/* TODO */
	return 0;
}

static int mcux_pxp_get_fmt(const struct device *dev, struct video_format *fmt)
{
	/* TODO */
	return 0;
}

static int mcux_pxp_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct mcux_pxp_data *data = dev->data;

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	switch (fmt->type) {
	case VIDEO_BUF_TYPE_INPUT:
		data->ps_buffer_cfg.pitchBytes = fmt->pitch;
		switch (fmt->pixelformat) {
		case VIDEO_PIX_FMT_RGB565:
			data->ps_buffer_cfg.pixelFormat = kPXP_PsPixelFormatRGB565;
			break;
		case VIDEO_PIX_FMT_RGB24:
#if (!(defined(FSL_FEATURE_PXP_HAS_NO_EXTEND_PIXEL_FORMAT) &&                                      \
FSL_FEATURE_PXP_HAS_NO_EXTEND_PIXEL_FORMAT)) &&                                             \
	(!(defined(FSL_FEATURE_PXP_V3) && FSL_FEATURE_PXP_V3))
			data->ps_buffer_cfg.pixelFormat = kPXP_PsPixelFormatARGB8888;
#else
			data->ps_buffer_cfg.pixelFormat = kPXP_PsPixelFormatRGB888;
#endif
			break;
		case VIDEO_PIX_FMT_ARGB32:
		case VIDEO_PIX_FMT_XRGB32:
			data->ps_buffer_cfg.pixelFormat = kPXP_PsPixelFormatARGB8888;
			break;
		default:
			return -ENOTSUP;
		}
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		data->output_buffer_cfg.pitchBytes = fmt->pitch;
		data->output_buffer_cfg.width = fmt->width;
		data->output_buffer_cfg.height = fmt->height;
		switch (fmt->pixelformat) {
		case VIDEO_PIX_FMT_RGB565:
			data->output_buffer_cfg.pixelFormat = kPXP_OutputPixelFormatRGB565;
			break;
		case VIDEO_PIX_FMT_RGB24:
			data->output_buffer_cfg.pixelFormat = kPXP_OutputPixelFormatRGB888;
			break;
		case VIDEO_PIX_FMT_XRGB32:
		case VIDEO_PIX_FMT_ARGB32:
			data->output_buffer_cfg.pixelFormat = kPXP_OutputPixelFormatARGB8888;
			break;
		default:
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
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

static int mcux_pxp_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	const struct mcux_pxp_config *const config = dev->config;
	struct mcux_pxp_data *data = dev->data;

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t)vbuf->buffer, vbuf->size);
#endif

	switch (vbuf->type) {
	case VIDEO_BUF_TYPE_INPUT:
		k_fifo_put(&data->fifo_in_queued, vbuf);

		data->ps_buffer_cfg.swapByte = false;
		data->ps_buffer_cfg.bufferAddr = (uint32_t)vbuf->buffer;
		data->ps_buffer_cfg.bufferAddrU = 0U;
		data->ps_buffer_cfg.bufferAddrV = 0U;

		PXP_SetProcessSurfaceBufferConfig(config->base, &data->ps_buffer_cfg);

		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		k_fifo_put(&data->fifo_out_queued, vbuf);

		data->output_buffer_cfg.interlacedMode = kPXP_OutputProgressive;
		data->output_buffer_cfg.buffer0Addr = (uint32_t)vbuf->buffer;
		data->output_buffer_cfg.buffer1Addr = 0U;

		PXP_SetOutputBufferConfig(config->base, &data->output_buffer_cfg);
		/* We only support a process surface that covers the full buffer */
		PXP_SetProcessSurfacePosition(config->base, 0U, 0U, data->output_buffer_cfg.width,
					      data->output_buffer_cfg.height);

		break;
	default:
		return -ENOTSUP;
	}

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

static int mcux_pxp_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct mcux_pxp_config *const config = dev->config;
	struct mcux_pxp_data *data = dev->data;

	if (enable) {
#ifdef CONFIG_HAS_MCUX_CACHE
		DCACHE_CleanByRange((uint32_t)data->ps_buffer_cfg.bufferAddr,
				    data->output_buffer_cfg.pitchBytes *
					    data->output_buffer_cfg.height);
#endif
		PXP_Start(config->base);
	}

	return 0;
}

static DEVICE_API(video, mcux_pxp_driver_api) = {
	.get_caps = mcux_pxp_get_caps,
	.get_format = mcux_pxp_get_fmt,
	.set_format = mcux_pxp_set_fmt,
	.set_ctrl = mcux_pxp_set_ctrl,
	.enqueue = mcux_pxp_enqueue,
	.dequeue = mcux_pxp_dequeue,
	.set_stream = mcux_pxp_set_stream,
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
