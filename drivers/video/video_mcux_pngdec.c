/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/video.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fsl_pngdec.h>
#include "video_common.h"

#define DT_DRV_COMPAT nxp_pngdec

LOG_MODULE_REGISTER(mcux_pngdec, CONFIG_VIDEO_LOG_LEVEL);

struct mcux_pngdec_data {
	struct video_device vdev; /* keep first */
	struct k_work decode_work;
	struct k_sem decode_done;    /* Signaled when the decoder goes idle. */
	bool running;         /* Decoder is actively decoding. */
	bool first_frame_rcv; /* First incoming frame has been fed. */
	pngdec_image_t image_info;
};

struct mcux_pngdec_config {
	PNGDEC_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

#define MCUX_PNGDEC_WIDTH_MIN  1U
#define MCUX_PNGDEC_WIDTH_MAX  1024U
#define MCUX_PNGDEC_HEIGHT_MIN 1U
#define MCUX_PNGDEC_HEIGHT_MAX 0xFFFFU
#define MCUX_PNGDEC_STEP       1U

#define MCUX_PNGDEC_FORMAT_CAP(format)			\
	{.pixelformat = (format),			\
	 .width_min = (MCUX_PNGDEC_WIDTH_MIN),		\
	 .width_max = (MCUX_PNGDEC_WIDTH_MAX),		\
	 .height_min = (MCUX_PNGDEC_HEIGHT_MIN),	\
	 .height_max = (MCUX_PNGDEC_HEIGHT_MAX),	\
	 .width_step = (MCUX_PNGDEC_STEP),		\
	 .height_step = (MCUX_PNGDEC_STEP)}

/* PNG decoder only outputs VIDEO_PIX_FMT_RGBA32 format. */
static const struct video_format_cap mcux_pngdec_out_fmts[2] = {
	MCUX_PNGDEC_FORMAT_CAP(VIDEO_PIX_FMT_RGBA32), {0}};

/* PNG decoder only supports PNG input. */
static const struct video_format_cap mcux_pngdec_in_fmts[2] = {
	MCUX_PNGDEC_FORMAT_CAP(VIDEO_PIX_FMT_PNG), /* PNG compressed image format */
	{0}
};

static void mcux_pngdec_decode_one_frame(const struct device *dev)
{
	const struct mcux_pngdec_config *config = dev->config;
	struct mcux_pngdec_data *data = dev->data;
	struct video_buffer *current_in = NULL;
	struct video_buffer *current_out = NULL;


	if (!video_stream_busy(&data->vdev) ||
	    k_fifo_is_empty(in_fifo_in) || k_fifo_is_empty(out_fifo_in)) {
		/* Nothing can be done if streaming is off or either queue is empty */
		data->running = false;
		k_sem_give(&data->decode_done);
		return;
	}

	/* Get the input buffer from the input queue. */
	current_in = k_fifo_peek_head(in_fifo_in);
	current_out = k_fifo_peek_head(out_fifo_in);

	if (current_in == NULL || current_out == NULL) {
		return;
	}

	PNGDEC_SetPngBuffer(config->base, (uint8_t *)current_in->buffer, current_in->size);
	PNGDEC_SetOutputBuffer(config->base, current_out->buffer, NULL);

	/* Clear dirty counter, write 1 then clear. */
	config->base->CNT_CTRL_CLR = PNGDEC_CNT_CTRL_CLR_CNT_CTRL_CLR_MASK;
	config->base->CNT_CTRL_CLR = 0U;

	/* Clear all status. */
	PNGDEC_ClearStatusFlags(config->base, 0xFFFFU);

	/*
	 * Drop any stale idle signal from a previous starve/stop before this frame's
	 * decode is started, so a stop issued while this frame is in flight waits for
	 * this frame specifically.
	 */
	k_sem_reset(&data->decode_done);

	/* Start decoding. */
	PNGDEC_StartDecode(config->base);

	data->running = true;
}

static int mcux_pngdec_set_fmt(const struct device *dev, struct video_format *fmt, size_t idx)
{
	struct mcux_pngdec_data *data = dev->data;

	/*
	 * The input size and output format is obtained after parsing the first enqueued
	 * input frame buffer. So only validate the format after queueing the first frame,
	 * Note: It is assumed that all the following PNG frames in the stream have the
	 * same sizes as the first one.
	 */
	fmt->pitch = 0;
	fmt->size = 0;
	if (data->first_frame_rcv) {
		struct video_format *current_format =
			(fmt->type == VIDEO_BUF_TYPE_INPUT) ? &(data->vdev.in_fmt) :
			&(data->vdev.out_fmt);

		if ((fmt->pixelformat != current_format->pixelformat) ||
			(fmt->width != current_format->width) ||
			(fmt->height != current_format->height) ||
			(fmt->pitch != current_format->pitch) ||
			(fmt->size != current_format->size)) {
			LOG_ERR("The input/output format is determined by the PNG header"
				" and cannot be changed.");
			return -EINVAL;
		}
	}

	return 0;
}

static int mcux_pngdec_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct mcux_pngdec_data *data = dev->data;

	ARG_UNUSED(type);

	if (enable) {
		mcux_pngdec_decode_one_frame(dev);
	} else {
		data->first_frame_rcv = false;
		if (data->running) {
			/* Let the frame currently being decoded finish, then stop decoding. */
			k_sem_take(&data->decode_done, K_FOREVER);
		}
	}

	return 0;
}

static int mcux_pngdec_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	int ret = 0;
	struct mcux_pngdec_data *data = dev->data;
	struct k_fifo *fifo = vbuf->type == VIDEO_BUF_TYPE_INPUT
		? &data->vdev.in_buf_submitted : &data->vdev.out_buf_submitted;
	bool do_decode = false;

	if (((uint32_t)vbuf->buffer & 0x7U) != 0U) {
		LOG_ERR("Buffer address must be 8-byte aligned");
		return -EINVAL;
	}

	/*
	 * The PNG decoder outputs VIDEO_PIX_FMT_RGBA32 format only.
	 * The output size is obtained after parsing the first enqueued input frame buffer,
	 * and it is assumed that all the following PNG frames in the stream have the same
	 * format as the first one, so the parsing is done only once.
	 */
	if (vbuf->type == VIDEO_BUF_TYPE_INPUT) {
		/*
		 * Check if it is the first frame, if so parse the PNG header
		 * and obtain the frame info.
		 */
		if (!data->first_frame_rcv) {
			if (PNGDEC_ParseHeader(&(data->image_info), vbuf->buffer) !=
			    kStatus_Success) {
				LOG_ERR("PNG header parsing failed");
				return -ENOTSUP;
			}

			/*
			 * Set output pixel format, width and height
			 * based on parsed PNG header.
			 */
			data->vdev.out_fmt.width = data->image_info.width;
			data->vdev.out_fmt.height = data->image_info.height;

			/* Calculate the default pitch and buffer size. */
			video_estimate_fmt_size(&data->vdev.out_fmt);

			/*
			 * The input width/height is the same as output. PNG files do
			 * not have pitch and the size can be different for each file
			 * due to compression ratio, so will not set them.
			 */
			data->vdev.in_fmt.width = data->vdev.out_fmt.width;
			data->vdev.in_fmt.height = data->vdev.out_fmt.height;

			data->first_frame_rcv = true;
		}
	}

	k_fifo_put(fifo, vbuf);

	/*
	 * If the streaming is on but the decoder is not running due to lack of
	 * in/out buffer, start to decode new frame.
	 */
	if (!data->running && video_stream_busy(dev)) {
		mcux_pngdec_decode_one_frame(dev);
	}

	return ret;
}

static int mcux_pngdec_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->min_vbuf_count = 1;

	if (caps->type == VIDEO_BUF_TYPE_OUTPUT) {
		caps->format_caps = mcux_pngdec_out_fmts;
	} else {
		caps->format_caps = mcux_pngdec_in_fmts;
	}

	caps->buf_align = 8U;

	return 0;
}

static int mcux_pngdec_transform_cap(const struct device *const dev,
				const struct video_format_cap *const cap,
				struct video_format_cap *const res_cap,
				enum video_buf_type direction, uint16_t ind)
{
	struct mcux_pngdec_data *data = dev->data;

	if (ind > 0) {
		return -ERANGE;
	}

	*res_cap = *cap;
	if (direction == VIDEO_BUF_TYPE_OUTPUT) {
		res_cap->pixelformat = data->vdev.in_fmt.pixelformat;
	} else {
		res_cap->pixelformat = data->vdev.out_fmt.pixelformat;
	}

	return 0;
}

static DEVICE_API(video, mcux_pngdec_driver_api) = {
	.set_format = mcux_pngdec_set_fmt,
	.set_stream = mcux_pngdec_set_stream,
	.get_caps = mcux_pngdec_get_caps,
	.transform_cap = mcux_pngdec_transform_cap,
	.enqueue = mcux_pngdec_enqueue,
};

static void mcux_pngdec_decode_work_handler(struct k_work *work)
{
	struct mcux_pngdec_data *data = CONTAINER_OF(work, struct mcux_pngdec_data, decode_work);

	k_mutex_lock(data->vdev.lock);
	mcux_pngdec_decode_one_frame(data->vdev.dev);
	k_mutex_unlock(&data->vdev.lock);
}

static void mcux_pngdec_isr(const struct device *dev)
{
	const struct mcux_pngdec_config *config = dev->config;
	struct mcux_pngdec_data *data = dev->data;
	struct video_buffer *current_in = k_fifo_get(&data->vdev.in_buf_submitted, K_NO_WAIT);
	struct video_buffer *current_out = k_fifo_get(&data->vdev.out_buf_submitted, K_NO_WAIT);
	uint32_t status_flags = PNGDEC_GetStatusFlags(config->base);

	if ((status_flags & kPNGDEC_DecodePixelDoneFlag) != 0U) {
		current_out->bytesused = config->base->DEC_PIXEL_DMA_DES_LEN;
	}

	if ((status_flags & kPNGDEC_ErrorFlags) != 0U) {
		LOG_ERR("PNG decode error");
		current_out->bytesused = 0;
	}

	/* Clear status flags. */
	PNGDEC_ClearStatusFlags(config->base, status_flags);

	current_out->size = data->vdev.out_fmt.size;

	video_buffer_done(dev, current_in, 0);
	video_buffer_done(dev, current_out, 0);

	/* Defer the next decode to a workqueue: decode_one_frame needs to be called from a
	 * locked state, which cannot be done from ISR context.
	 */
	k_work_submit(&data->decode_work);
}

static int mcux_pngdec_init(const struct device *dev)
{
	const struct mcux_pngdec_config *config = dev->config;
	struct mcux_pngdec_data *data = dev->data;
	pngdec_config_t init_config;

	k_work_init(&data->decode_work, mcux_pngdec_decode_work_handler);
	k_sem_init(&data->decode_done, 0, 1);

	/* Initialise default input / output formats */
	data->vdev.in_fmt.type = VIDEO_BUF_TYPE_INPUT;
	data->vdev.in_fmt.pixelformat = VIDEO_PIX_FMT_PNG;
	data->vdev.in_fmt.width = MCUX_PNGDEC_WIDTH_MIN;
	data->vdev.in_fmt.height = MCUX_PNGDEC_HEIGHT_MIN;

	data->vdev.out_fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	data->vdev.out_fmt.pixelformat = VIDEO_PIX_FMT_RGBA32;
	data->vdev.out_fmt.width = MCUX_PNGDEC_WIDTH_MIN;
	data->vdev.out_fmt.height = MCUX_PNGDEC_HEIGHT_MIN;
	data->vdev.out_fmt.pitch = MCUX_PNGDEC_WIDTH_MIN * 4U; /* 4 bytes per pixel for RGBA8888 */
	data->vdev.out_fmt.size = MCUX_PNGDEC_WIDTH_MIN * MCUX_PNGDEC_HEIGHT_MIN * 4U;

	/* Init PNG decoder module. */
	PNGDEC_GetDefaultConfig(&init_config);
	PNGDEC_Init(config->base, &init_config);

	PNGDEC_EnableInterrupts(config->base, kPNGDEC_DecodePixelDoneFlag | kPNGDEC_ErrorFlags);

	config->irq_config_func(dev);

	return 0;
}

#define MCUX_PNGDEC_INIT(n)									\
	static void mcux_pngdec_irq_config_##n(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_pngdec_isr,		\
			    DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_INST_IRQN(n));							\
	}											\
												\
	static const struct mcux_pngdec_config mcux_pngdec_config_##n = {			\
		.base = (PNGDEC_Type *)DT_INST_REG_ADDR(n),					\
		.irq_config_func = mcux_pngdec_irq_config_##n,					\
	};											\
												\
	static struct mcux_pngdec_data mcux_pngdec_data_##n = {					\
		.vdev = VIDEO_DEVICE_INIT(mcux_pngdec_data_##n, DEVICE_DT_GET(n), NULL),	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, mcux_pngdec_init, NULL, &mcux_pngdec_data_##n,			\
			      &mcux_pngdec_config_##n, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,	\
			      &mcux_pngdec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_PNGDEC_INIT)
