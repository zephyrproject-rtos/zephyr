/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <fsl_pngdec.h>
#include "video_device.h"

#define DT_DRV_COMPAT nxp_pngdec

LOG_MODULE_REGISTER(mcux_pngdec, CONFIG_VIDEO_LOG_LEVEL);

struct video_common_header {
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
};

struct video_m2m_common {
	struct video_common_header in;  /* Incoming PNG stream. */
	struct video_common_header out; /* Outgoing decoded image data. */
};

struct mcux_pngdec_data {
	struct video_m2m_common m2m;
	struct k_mutex lock;
	bool is_streaming;    /* Application has set the in/out stream on. */
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
#define MCUX_PNGDEC_WIDTH_STEP 1U

#define MCUX_PNGDEC_FORMAT_CAP(format)                                                             \
	{.pixelformat = (format),                                                                  \
	 .width_min = (MCUX_PNGDEC_WIDTH_MIN),                                                     \
	 .width_max = (MCUX_PNGDEC_WIDTH_MAX),                                                     \
	 .height_min = (MCUX_PNGDEC_HEIGHT_MIN),                                                   \
	 .height_max = (MCUX_PNGDEC_HEIGHT_MAX),                                                   \
	 .width_step = (MCUX_PNGDEC_WIDTH_STEP),                                                   \
	 .height_step = (MCUX_PNGDEC_WIDTH_STEP)}

/* PNG decoder only outputs ABGR8888 format. */
static const struct video_format_cap mcux_pngdec_out_fmts[2] = {
	MCUX_PNGDEC_FORMAT_CAP(VIDEO_PIX_FMT_ABGR32), {0}};

/* PNG decoder only supports PNG input. */
static const struct video_format_cap mcux_pngdec_in_fmts[2] = {
	MCUX_PNGDEC_FORMAT_CAP(VIDEO_PIX_FMT_PNG), /* PNG compressed image format */
	{0}};

static void mcux_pngdec_decode_one_frame(const struct device *dev)
{
	const struct mcux_pngdec_config *config = dev->config;
	struct mcux_pngdec_data *data = dev->data;
	struct k_fifo *in_fifo_in = &data->m2m.in.fifo_in;
	struct k_fifo *out_fifo_in = &data->m2m.out.fifo_in;
	struct video_buffer *current_in;
	struct video_buffer *current_out;

	if (k_fifo_is_empty(in_fifo_in) || k_fifo_is_empty(out_fifo_in)) {
		/* Nothing can be done if either input or output queue is empty */
		data->running = false;
		return;
	}

	/* Get the input buffer from the input queue. */
	current_in = k_fifo_peek_head(in_fifo_in);
	current_out = k_fifo_peek_head(out_fifo_in);

	PNGDEC_SetPngBuffer(config->base, (uint8_t *)current_in->buffer, current_in->size);
	PNGDEC_SetOutputBuffer(config->base, current_out->buffer, NULL);

	/* Clear dirty counter, write 1 then clear. */
	config->base->CNT_CTRL_CLR = PNGDEC_CNT_CTRL_CLR_CNT_CTRL_CLR_MASK;
	config->base->CNT_CTRL_CLR = 0U;

	/* Clear all status. */
	PNGDEC_ClearStatusFlags(config->base, 0xFFFFU);

	/* Start decoding. */
	PNGDEC_StartDecode(config->base);

	data->running = true;
}

static int mcux_pngdec_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct mcux_pngdec_data *data = dev->data;

	*fmt = fmt->type == VIDEO_BUF_TYPE_INPUT ? data->m2m.in.fmt : data->m2m.out.fmt;

	return 0;
}

static int mcux_pngdec_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct mcux_pngdec_data *data = dev->data;
	bool is_input = (fmt->type == VIDEO_BUF_TYPE_INPUT) ? true : false;
	struct video_format *current_format =
		(fmt->type == VIDEO_BUF_TYPE_INPUT) ? &(data->m2m.in.fmt) : &(data->m2m.out.fmt);
	int ret = 0;

	/* Check whether the pixel format is supported by the driver. */
	if (is_input) {
		if (fmt->pixelformat != VIDEO_PIX_FMT_PNG) {
			LOG_ERR("PNG decoder only supports PNG input format");
			return -EINVAL;
		}
	} else if (fmt->pixelformat != VIDEO_PIX_FMT_ABGR32) {
		LOG_ERR("PNG decoder only supports ABGR8888 output format");
		return -EINVAL;
	}

	/*
	 * The input pixel format can only be VIDEO_PIX_FMT_PNG, and the output
	 * format is fixed to ABGR8888.
	 * The input size and output format is obtained after parsing the first enqueued
	 * input frame buffer.
	 * Note: It is assumed that all the following PNG frames in the stream have the
	 * same dimensions as the first one.
	 */
	if (data->first_frame_rcv) {
		/*
		 * Validate the settings. Only need to check the input width/height since output is
		 * the same.
		 */
		if ((fmt->width != data->m2m.in.fmt.width) ||
		    (fmt->height != data->m2m.in.fmt.height)) {
			LOG_ERR("The input/output width/height are fixed and cannot be "
				"configured.");
			return -EINVAL;
		}

		if (is_input) {
			current_format->size = fmt->pitch * fmt->height;
		} else if (fmt->pitch != data->m2m.in.fmt.pitch) {
			LOG_ERR("The output pitch is fixed and cannot be configured.");
			return -EINVAL;
		}
	} else {
		current_format->width = fmt->width;
		current_format->height = fmt->height;
		/* Input PNG does not have pitch. */
		if (is_input) {
			current_format->size = fmt->size;
		} else {
			current_format->pitch = fmt->width * 4U;
			current_format->size = fmt->pitch * fmt->height;
		}
	}

	return ret;
}

static int mcux_pngdec_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct mcux_pngdec_data *data = dev->data;
	int ret = 0;

	ARG_UNUSED(type);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (enable == data->is_streaming) {
		goto out;
	}

	data->is_streaming = enable;

	/* Start decoding if streaming is on otherwise stop the decoder. */
	if (enable) {
		mcux_pngdec_decode_one_frame(dev);
	} else {
		data->running = false;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mcux_pngdec_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct mcux_pngdec_data *data = dev->data;
	bool is_input = (vbuf->type == VIDEO_BUF_TYPE_INPUT) ? true : false;
	struct video_common_header *common =
		vbuf->type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;

	if (vbuf->buffer == NULL) {
		LOG_ERR("Buffer pointer is NULL");
		return -EINVAL;
	}

	if (((uint32_t)vbuf->buffer & 0x7U) != 0U) {
		LOG_ERR("Buffer address must be 8-byte aligned");
		return -EINVAL;
	}

	/*
	 * The PNG decoder outputs ABGR8888 format only.
	 * The output size is obtained after parsing the first enqueued input frame buffer,
	 * and it is assumed that all the following PNG frames in the stream have the same
	 * format as the first one, so the parsing is done only once.
	 */
	if (is_input) {
		if (vbuf->size == 0U) {
			LOG_ERR("Input buffer size must be non-zero");
			return -EINVAL;
		}

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

			/* Set output pixel format, width and height based on parsed PNG header */
			data->m2m.out.fmt.width = data->image_info.width;
			data->m2m.out.fmt.height = data->image_info.height;

			/*
			 * Calculate the default pitch and buffer size. They can be updated later
			 * if application calls set_fmt to update pitch.
			 */
			data->m2m.out.fmt.pitch =
				data->m2m.out.fmt.width * 4U; /* 4 bytes per pixel for ABGR8888 */
			data->m2m.out.fmt.size = data->m2m.out.fmt.pitch * data->m2m.out.fmt.height;

			/*
			 * The input width/height is the same as output. PNG files do not have
			 * pitch and the size can be different for each file due to compression
			 * ratio, so will not set them.
			 */
			data->m2m.in.fmt.width = data->m2m.out.fmt.width;
			data->m2m.in.fmt.height = data->m2m.out.fmt.height;

			data->first_frame_rcv = true;
		}
	} else {
		/*
		 * Verify the output buffer size only if the first input frame
		 * has been received.
		 */
		if ((data->first_frame_rcv) && (vbuf->size < data->m2m.out.fmt.size)) {
			LOG_ERR("Output buffer size is insufficient");
			return -EINVAL;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	k_fifo_put(&common->fifo_in, vbuf);

	/*
	 * If the streaming is on but the decoder is not running due to lack of in/out buffer,
	 * start to decode new frame.
	 */
	if (!data->running && data->is_streaming) {
		mcux_pngdec_decode_one_frame(dev);
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mcux_pngdec_dequeue(const struct device *dev, struct video_buffer **vbuf,
			       k_timeout_t timeout)
{
	struct mcux_pngdec_data *data = dev->data;
	struct video_common_header *common =
		(*vbuf)->type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;

	*vbuf = k_fifo_get(&common->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int mcux_pngdec_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->min_vbuf_count = 1;

	if (caps->type == VIDEO_BUF_TYPE_OUTPUT) {
		caps->format_caps = mcux_pngdec_out_fmts;
	} else {
		caps->format_caps = mcux_pngdec_in_fmts;
	}

	return 0;
}

static DEVICE_API(video, mcux_pngdec_driver_api) = {
	/* mandatory callbacks */
	.set_format = mcux_pngdec_set_fmt,
	.get_format = mcux_pngdec_get_fmt,
	.set_stream = mcux_pngdec_set_stream, /* Start or stop streaming on the video device */
	.get_caps = mcux_pngdec_get_caps,
	/* optional callbacks */
	.enqueue = mcux_pngdec_enqueue, /* Enqueue a buffer in the driver's incoming queue */
	.dequeue = mcux_pngdec_dequeue, /* Dequeue a buffer from the driver's outgoing queue */
};

static void mcux_pngdec_isr(const struct device *dev)
{
	const struct mcux_pngdec_config *config = dev->config;
	struct mcux_pngdec_data *data = dev->data;
	struct video_buffer *current_in = k_fifo_get(&data->m2m.in.fifo_in, K_NO_WAIT);
	struct video_buffer *current_out = k_fifo_get(&data->m2m.out.fifo_in, K_NO_WAIT);
	uint32_t status_flags = PNGDEC_GetStatusFlags(config->base);

	if (status_flags & kPNGDEC_DecodePixelDoneFlag) {
		current_out->bytesused = data->m2m.out.fmt.size;
	}

	if (status_flags & kPNGDEC_ErrorFlags) {
		LOG_ERR("PNG decode error");
		current_out->bytesused = 0;
	}

	/* Clear status flags. */
	PNGDEC_ClearStatusFlags(config->base, status_flags);

	current_out->size = data->m2m.out.fmt.size;

	k_fifo_put(&data->m2m.in.fifo_out, current_in);
	k_fifo_put(&data->m2m.out.fifo_out, current_out);

	mcux_pngdec_decode_one_frame(dev);
}

static int mcux_pngdec_init(const struct device *dev)
{
	const struct mcux_pngdec_config *config = dev->config;
	struct mcux_pngdec_data *data = dev->data;
	pngdec_config_t init_config;

	k_mutex_init(&data->lock);
	k_fifo_init(&data->m2m.in.fifo_in);
	k_fifo_init(&data->m2m.in.fifo_out);
	k_fifo_init(&data->m2m.out.fifo_in);
	k_fifo_init(&data->m2m.out.fifo_out);

	/* Initialise default input / output formats */
	data->m2m.in.fmt.type = VIDEO_BUF_TYPE_INPUT;
	data->m2m.in.fmt.pixelformat = VIDEO_PIX_FMT_PNG;
	data->m2m.in.fmt.width = MCUX_PNGDEC_WIDTH_MIN;
	data->m2m.in.fmt.height = MCUX_PNGDEC_HEIGHT_MIN;

	data->m2m.out.fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	data->m2m.out.fmt.pixelformat = VIDEO_PIX_FMT_ABGR32;
	data->m2m.out.fmt.width = MCUX_PNGDEC_WIDTH_MIN;
	data->m2m.out.fmt.height = MCUX_PNGDEC_HEIGHT_MIN;
	data->m2m.out.fmt.pitch = MCUX_PNGDEC_WIDTH_MIN * 4U; /* 4 bytes per pixel for ABGR8888 */
	data->m2m.out.fmt.size = MCUX_PNGDEC_WIDTH_MIN * MCUX_PNGDEC_HEIGHT_MIN * 4U;

	/* Init PNG decoder module. */
	PNGDEC_GetDefaultConfig(&init_config);
	PNGDEC_Init(config->base, &init_config);

	PNGDEC_EnableInterrupts(config->base, kPNGDEC_DecodePixelDoneFlag | kPNGDEC_ErrorFlags);

	config->irq_config_func(dev);

	return 0;
}

#define MCUX_PNGDEC_INIT(n)                                                                        \
	static void mcux_pngdec_irq_config_##n(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_pngdec_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static const struct mcux_pngdec_config mcux_pngdec_config_##n = {                          \
		.base = (PNGDEC_Type *)DT_INST_REG_ADDR(n),                                        \
		.irq_config_func = mcux_pngdec_irq_config_##n,                                     \
	};                                                                                         \
	static struct mcux_pngdec_data mcux_pngdec_data_##n = {0};                                 \
	DEVICE_DT_INST_DEFINE(n, mcux_pngdec_init, NULL, &mcux_pngdec_data_##n,                    \
			      &mcux_pngdec_config_##n, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &mcux_pngdec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_PNGDEC_INIT)
