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
#include <fsl_jpegdec.h>
#include "video_device.h"

#define DT_DRV_COMPAT nxp_jpegdec

LOG_MODULE_REGISTER(mcux_jpegdec, CONFIG_VIDEO_LOG_LEVEL);

struct video_common_header {
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
};

struct video_m2m_common {
	struct video_common_header in;  /* Incoming JPEG stream. */
	struct video_common_header out; /* Outgoing decoded image data. */
};

struct mcux_jpegdec_data {
	struct video_m2m_common m2m;
	struct k_mutex lock;
	bool is_streaming; /* Application has set the in/out stream on. */
	bool running;      /* Decoder is actively decoding. */
	jpegdec_descpt_t decoder_despt;
	bool first_frame_rcv; /* First incoming frame has been fed. */
	uint8_t format_idx;
};

struct mcux_jpegdec_config {
	JPEG_DECODER_Type base;
	void (*irq_config_func)(const struct device *dev);
};

#define MCUX_JPEGDEC_WIDTH_MIN  64U
#define MCUX_JPEGDEC_WIDTH_MAX  0x2000U
#define MCUX_JPEGDEC_HEIGHT_MIN 64U
#define MCUX_JPEGDEC_HEIGHT_MAX 0x2000U
#define MCUX_JPEGDEC_WIDTH_STEP 16U

#define MCUX_JPEGDEC_FORMAT_CAP(format)                                                            \
	{.pixelformat = (format),                                                                  \
	 .width_min = (MCUX_JPEGDEC_WIDTH_MIN),                                                    \
	 .width_max = (MCUX_JPEGDEC_WIDTH_MAX),                                                    \
	 .height_min = (MCUX_JPEGDEC_HEIGHT_MIN),                                                  \
	 .height_max = (MCUX_JPEGDEC_HEIGHT_MAX),                                                  \
	 .width_step = (MCUX_JPEGDEC_WIDTH_STEP),                                                  \
	 .height_step = (MCUX_JPEGDEC_WIDTH_STEP)}

static const struct video_format_cap mcux_jpegdec_out_all_fmts[] = {
	MCUX_JPEGDEC_FORMAT_CAP(
		VIDEO_PIX_FMT_NV12), /* YUV420, Y at first plane, UV at second plane */
	MCUX_JPEGDEC_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),   /* Packed YUV422 format */
	MCUX_JPEGDEC_FORMAT_CAP(VIDEO_PIX_FMT_BGR24),  /* 24-bit BGR color format */
	MCUX_JPEGDEC_FORMAT_CAP(VIDEO_PIX_FMT_YUV24),  /* 24-bit 1 planar YUV format */
	MCUX_JPEGDEC_FORMAT_CAP(VIDEO_PIX_FMT_GREY),   /* 8-bit grayscale */
	MCUX_JPEGDEC_FORMAT_CAP(VIDEO_PIX_FMT_ARGB32), /* YCCK or any format with 4 component */
};

/* Empty output format array - dynamically populated based on JPEG header parsing. */
static struct video_format_cap mcux_jpegdec_out_fmts[2] = {0};

/* JPEG decoder only support JPEG input. */
static const struct video_format_cap mcux_jpegdec_in_fmts[2] = {
	MCUX_JPEGDEC_FORMAT_CAP(VIDEO_PIX_FMT_JPEG), /* JPEG compressed image format */
	{0}};

struct pixel_map {
	jpegdec_pixel_format_t drv_pixel_format;
	uint32_t vid_pixel_format;
	uint8_t bytes_per_pixel;
};

static const struct pixel_map pixel_map_confs[] = {
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYUV420,
		.vid_pixel_format = VIDEO_PIX_FMT_NV12,
		.bytes_per_pixel = 1,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYUV422,
		.vid_pixel_format = VIDEO_PIX_FMT_YUYV,
		.bytes_per_pixel = 2,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatRGB,
		.vid_pixel_format = VIDEO_PIX_FMT_BGR24,
		.bytes_per_pixel = 3,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYUV444,
		.vid_pixel_format = VIDEO_PIX_FMT_YUV24,
		.bytes_per_pixel = 3,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatGray,
		.vid_pixel_format = VIDEO_PIX_FMT_GREY,
		.bytes_per_pixel = 1,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYCCK,
		.vid_pixel_format = VIDEO_PIX_FMT_ARGB32,
		.bytes_per_pixel = 4,
	},
};

static int mcux_jpegdec_get_conf(uint32_t pixel_format, bool is_driver)
{
	for (size_t i = 0; i < ARRAY_SIZE(pixel_map_confs); i++) {
		if (is_driver) {
			if (pixel_map_confs[i].drv_pixel_format ==
			    (jpegdec_pixel_format_t)pixel_format) {
				return i;
			}
		} else {
			if (pixel_map_confs[i].vid_pixel_format == pixel_format) {
				return i;
			}
		}
	}

	return -1;
}

static void mcux_jpegdec_decode_one_frame(const struct device *dev)
{
	const struct mcux_jpegdec_config *config = dev->config;
	struct mcux_jpegdec_data *data = dev->data;
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

	data->decoder_despt.config.jpegBufAddr = (uint32_t)current_in->buffer;
	data->decoder_despt.config.jpegBufSize = ROUND_UP(current_in->size, 1024);
	data->decoder_despt.config.outBufAddr0 = (uint32_t)current_out->buffer;
	data->decoder_despt.config.outBufPitch = data->m2m.out.fmt.pitch;

	/*
	 * For VIDEO_PIX_FMT_NV12, we need a second output buffer for the UV plane.
	 * Its address shall be strictly next to the end of Y plane.
	 */
	if (data->m2m.out.fmt.pixelformat == VIDEO_PIX_FMT_NV12) {
		data->decoder_despt.config.outBufAddr1 =
			data->decoder_despt.config.outBufAddr0 +
			data->m2m.out.fmt.pitch * data->m2m.out.fmt.height;
	}

	/* Start the decoding of next frame. */
	JPEGDEC_EnableSlotNextDescpt((JPEG_DECODER_Type *)&config->base, 0);

	data->running = true;
}

static int mcux_jpegdec_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct mcux_jpegdec_data *data = dev->data;

	*fmt = fmt->type == VIDEO_BUF_TYPE_INPUT ? data->m2m.in.fmt : data->m2m.out.fmt;

	return 0;
}

static int mcux_jpegdec_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct mcux_jpegdec_data *data = dev->data;
	bool is_input = (fmt->type == VIDEO_BUF_TYPE_INPUT) ? true : false;
	struct video_format *current_format =
		(fmt->type == VIDEO_BUF_TYPE_INPUT) ? &(data->m2m.in.fmt) : &(data->m2m.in.fmt);
	int conf_idx = 0;
	int ret = 0;

	/* Check whether the pixel format is supported by the driver. */
	if (!is_input) {
		conf_idx = mcux_jpegdec_get_conf(fmt->pixelformat, false);
	}

	if ((conf_idx < 0) || (is_input && (fmt->pixelformat != VIDEO_PIX_FMT_JPEG))) {
		LOG_ERR("Unsupported pixel format: %d", fmt->pixelformat);
		return -EINVAL;
	}

	/*
	 * The input pixel format can only be VIDEO_PIX_FMT_JPEG, and the output
	 * format cannot be configured since the decoder does not have CSC. Which means
	 * if the JPEG file is encoded in YUV420_2p(NV12) then the output can only be
	 * the same.
	 * The input size and output format is obtained after parsing the first enqueued
	 * input frame buffer. So only valdate the input size and output format if
	 * the first frame received yet, otherwise update the format directly.
	 * Note: It is assumed that all the following JPEG frames in the stream have the
	 * same format as the first one.
	 */
	if (data->first_frame_rcv) {
		/*
		 * Validate the settings. Only ned to check the input width/height since output is
		 * the same.
		 */
		if ((fmt->width != data->m2m.in.fmt.width) ||
		    (fmt->height != data->m2m.in.fmt.height)) {
			LOG_ERR("The input/output width/height are fixed and cannot be "
				"configured.");
			return -EINVAL;
		}

		if (!is_input) {
			if (fmt->pixelformat != data->m2m.out.fmt.pixelformat) {
				LOG_ERR("JPEG decoder does not support CSC, the output format can "
					"not be changed.");
				return -EINVAL;
			}

			if (fmt->pitch < data->m2m.out.fmt.pitch) {
				LOG_ERR("The pitch cannot be smaller than the line size.");
				return -EINVAL;
			}

			/* Update pitch value. */
			data->m2m.out.fmt.pitch = fmt->pitch;
			data->m2m.out.fmt.size = fmt->pitch * fmt->height;
			if (data->m2m.out.fmt.pixelformat == VIDEO_PIX_FMT_NV12) {
				data->m2m.out.fmt.size = fmt->pitch * fmt->height * 2U;
			} else {
				data->m2m.out.fmt.size = fmt->pitch * fmt->height;
			}
		}
	} else {
		current_format->pixelformat = fmt->pixelformat;
		current_format->width = fmt->width;
		current_format->height = fmt->height;
		/* Input JPEG does not have pitch, no need to check it. */
		if (!is_input) {
			if (fmt->pitch == 0) {
				current_format->pitch =
					fmt->width * pixel_map_confs[conf_idx].bytes_per_pixel;
			} else if (fmt->pitch <
				   (fmt->width * pixel_map_confs[conf_idx].bytes_per_pixel)) {
				LOG_ERR("The pitch cannot be smaller than the line size.");
				return -EINVAL;
			}
			current_format->pitch = fmt->pitch;
		}
		current_format->size = fmt->pitch * fmt->height;
	}

	return ret;
}

static int mcux_jpegdec_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct mcux_jpegdec_data *data = dev->data;
	int ret = 0;

	ARG_UNUSED(type);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (enable == data->is_streaming) {
		goto out;
	}

	data->is_streaming = enable;

	/* Start decoding if streaming is on otherwise stop the decoder. */
	if (enable) {
		mcux_jpegdec_decode_one_frame(dev);
	} else {
		data->running = false;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mcux_jpegdec_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct mcux_jpegdec_data *data = dev->data;
	bool is_input = (vbuf->type == VIDEO_BUF_TYPE_INPUT) ? true : false;
	struct video_common_header *common =
		vbuf->type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;

	if (vbuf->buffer == NULL) {
		LOG_ERR("Buffer pointer is NULL");
		return -EINVAL;
	}

	if (((uint32_t)vbuf->buffer & 0xFU) != 0U) {
		LOG_ERR("Buffer address must be 16-byte aligned");
		return -EINVAL;
	}

	/*
	 * The decoder cannot perform CSC, which means if the JPEG file
	 * is encoded in YUV420_2p(NV12) then the output can only be
	 * the same.
	 * So the output format and capability can only be got after the
	 * first frame of JPEG is fed to the decoder, and it is assumed
	 * that all the following JPEG frames in the stream have the
	 * same format as the first one, so the parsing is done only once.
	 */
	if (is_input) {
		if (vbuf->size == 0U) {
			LOG_ERR("Input buffer size must be non-zero");
			return -EINVAL;
		}

		/*
		 * Check if it is the first frame, if so parse the JPEG header
		 * and obtain the frame into.
		 */
		if (!data->first_frame_rcv) {
			/* Feed the input buffer to the decoder for header parsing. */
			data->decoder_despt.config.jpegBufAddr = (uint32_t)vbuf->buffer;

			/* Input buffer size must be 1024-byte aligned, round up */
			data->decoder_despt.config.jpegBufSize = ROUND_UP(vbuf->size, 1024);

			if (JPEGDEC_ParseHeader(&data->decoder_despt.config) ==
			    kStatus_JPEGDEC_NotSupported) {
				LOG_ERR("JPEG format not supported");
				return -ENOTSUP;
			}

			/* Set output pixel format, width and height based on parsed JPEG header */
			data->format_idx =
				mcux_jpegdec_get_conf(data->decoder_despt.config.pixelFormat, true);
			data->m2m.out.fmt.pixelformat =
				pixel_map_confs[data->format_idx].vid_pixel_format;
			data->m2m.out.fmt.width = data->decoder_despt.config.width;
			data->m2m.out.fmt.height = data->decoder_despt.config.height;
			/*
			 * Calculate the default pitch and buffer size. They can be updated later
			 * if application calls set_fmt to update pitch.
			 */
			data->m2m.out.fmt.pitch =
				pixel_map_confs[data->format_idx].bytes_per_pixel *
				data->m2m.out.fmt.width;
			/*
			 * For NV12 2-planner format, the Y planner is 1BPP and UV planner is
			 * 0.5BPP. But the same pitch configuration applies to the 2 planners, so
			 * double the buffer size.
			 */
			if (data->m2m.out.fmt.pixelformat == VIDEO_PIX_FMT_NV12) {
				data->m2m.out.fmt.size =
					data->m2m.out.fmt.pitch * data->m2m.out.fmt.height * 2U;
			} else {
				data->m2m.out.fmt.size =
					data->m2m.out.fmt.pitch * data->m2m.out.fmt.height;
			}
			data->decoder_despt.config.outBufPitch = data->m2m.out.fmt.pitch;

			/*
			 * The input width/height is the same as output. JPEG files do not have
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
		mcux_jpegdec_decode_one_frame(dev);
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mcux_jpegdec_dequeue(const struct device *dev, struct video_buffer **vbuf,
				k_timeout_t timeout)
{
	struct mcux_jpegdec_data *data = dev->data;
	struct video_common_header *common =
		(*vbuf)->type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;

	*vbuf = k_fifo_get(&common->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int mcux_jpegdec_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct mcux_jpegdec_data *data = dev->data;

	caps->min_vbuf_count = 1;

	if (caps->type == VIDEO_BUF_TYPE_OUTPUT) {
		if (data->first_frame_rcv) {
			/* Assign the format capability based on parsed JPEG header. */
			mcux_jpegdec_out_fmts[0] = mcux_jpegdec_out_all_fmts[data->format_idx];
		} else {
			LOG_WRN("Output format can only be obtained after first input buffer "
				"is received, use VIDEO_PIX_FMT_NV12 as default since it is mostly "
				"used by JPEG.");
			mcux_jpegdec_out_fmts[0] = mcux_jpegdec_out_all_fmts[0];
		}
		caps->format_caps = mcux_jpegdec_out_fmts;
	} else {
		caps->format_caps = mcux_jpegdec_in_fmts;
	}

	return 0;
}

static DEVICE_API(video, mcux_jpegdec_driver_api) = {
	/* mandatory callbacks */
	.set_format = mcux_jpegdec_set_fmt,
	.get_format = mcux_jpegdec_get_fmt,
	.set_stream = mcux_jpegdec_set_stream, /* Start or stop streaming on the video device */
	.get_caps = mcux_jpegdec_get_caps,
	/* optional callbacks */
	.enqueue = mcux_jpegdec_enqueue, /* Enqueue a buffer in the driver’s incoming queue */
	.dequeue = mcux_jpegdec_dequeue, /* Dequeue a buffer from the driver’s outgoing queue */
};

static int mcux_jpegdec_init(const struct device *dev)
{
	const struct mcux_jpegdec_config *config = dev->config;
	struct mcux_jpegdec_data *data = dev->data;
	jpegdec_config_t init_config;

	k_mutex_init(&data->lock);
	k_fifo_init(&data->m2m.in.fifo_in);
	k_fifo_init(&data->m2m.in.fifo_out);
	k_fifo_init(&data->m2m.out.fifo_in);
	k_fifo_init(&data->m2m.out.fifo_out);

	/* Initialise default input / output formats */
	data->m2m.in.fmt.type = VIDEO_BUF_TYPE_INPUT;
	data->m2m.in.fmt.pixelformat = VIDEO_PIX_FMT_JPEG;
	data->m2m.in.fmt.width = MCUX_JPEGDEC_WIDTH_MIN;
	data->m2m.in.fmt.height = MCUX_JPEGDEC_HEIGHT_MIN;

	data->m2m.out.fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	data->m2m.out.fmt.pixelformat = VIDEO_PIX_FMT_NV12;
	data->m2m.out.fmt.width = MCUX_JPEGDEC_WIDTH_MIN;
	data->m2m.out.fmt.height = MCUX_JPEGDEC_HEIGHT_MIN;
	data->m2m.out.fmt.size = MCUX_JPEGDEC_WIDTH_MIN * MCUX_JPEGDEC_HEIGHT_MIN * 2U;

	/* Init JPEG decoder module. */
	JPEGDEC_GetDefaultConfig(&init_config);
	init_config.slots = kJPEGDEC_Slot0; /* Enable only one slot. */
	JPEGDEC_Init((JPEG_DECODER_Type *)&config->base, &init_config);

	JPEGDEC_EnableInterrupts((JPEG_DECODER_Type *)&config->base, 0U,
				 kJPEGDEC_DecodeCompleteFlag | kJPEGDEC_ErrorFlags);

	/* Link the descriptor to itself so no need to set the descriptor address every time. */
	data->decoder_despt.nextDescptAddr = (uint32_t)&(data->decoder_despt);

	/* Assign this descriptor to slot 0. */
	JPEGDEC_SetSlotNextDescpt((JPEG_DECODER_Type *)&config->base, 0, &(data->decoder_despt));

	/* Run IRQ init */
	config->irq_config_func(dev);

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

static void mcux_jpegdec_isr(const struct device *dev)
{
	const struct mcux_jpegdec_config *config = dev->config;
	struct mcux_jpegdec_data *data = dev->data;
	struct video_buffer *current_in = k_fifo_get(&data->m2m.in.fifo_in, K_NO_WAIT);
	struct video_buffer *current_out = k_fifo_get(&data->m2m.out.fifo_in, K_NO_WAIT);
	uint32_t flag = JPEGDEC_GetStatusFlags((JPEG_DECODER_Type *)&config->base, 0);

	/* Decode complete. */
	if ((flag & kJPEGDEC_DecodeCompleteFlag) != 0U) {
		current_out->bytesused = data->m2m.out.fmt.size;
	}

	/* Error occur, this output buffer is broken, no valid data can be used. */
	if ((flag & kJPEGDEC_ErrorFlags) != 0U) {
		LOG_ERR("JPEG decode error");
		current_out->bytesused = 0;
	}

	JPEGDEC_ClearStatusFlags((JPEG_DECODER_Type *)&config->base, 0, flag);

	current_out->size = data->m2m.out.fmt.size;

	k_fifo_put(&data->m2m.in.fifo_out, current_in);
	k_fifo_put(&data->m2m.out.fifo_out, current_out);

	mcux_jpegdec_decode_one_frame(dev);
}

#define MCUX_JPEGDEC_INIT(n)                                                                       \
	static void mcux_jpegdec_irq_config_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_jpegdec_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct mcux_jpegdec_data mcux_jpegdec_data_##n = {                                  \
		.decoder_despt.config.clearStreamBuf = true,                                       \
		.decoder_despt.config.autoStart = true,                                            \
	};                                                                                         \
	static const struct mcux_jpegdec_config mcux_jpegdec_config_##n = {                        \
		.base =                                                                            \
			{                                                                          \
				.wrapper = (JPGDECWRP_Type *)DT_INST_REG_ADDR_BY_IDX(n, 0),        \
				.core = (JPEGDEC_Type *)DT_INST_REG_ADDR_BY_IDX(n, 1),             \
			},                                                                         \
		.irq_config_func = mcux_jpegdec_irq_config_##n,                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &mcux_jpegdec_init, NULL, &mcux_jpegdec_data_##n,                 \
			      &mcux_jpegdec_config_##n, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,   \
			      &mcux_jpegdec_driver_api);                                           \
	VIDEO_DEVICE_DEFINE(jpegdec_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(MCUX_JPEGDEC_INIT)
