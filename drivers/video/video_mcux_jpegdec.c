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
#include <fsl_jpegdec.h>
#include "video_common.h"

#define DT_DRV_COMPAT nxp_jpegdec

LOG_MODULE_REGISTER(mcux_jpegdec, CONFIG_VIDEO_LOG_LEVEL);

#define MCUX_JPEGDEC_BUF_ALIGN 1024U

struct mcux_jpegdec_data {
	struct video_device vdev; /* keep first */
	struct k_sem decode_done;
	jpegdec_descpt_t decoder_despt;
	bool running;      /* Decoder is actively decoding. */
	bool first_frame_rcv; /* First incoming frame has been fed. */
	uint8_t format_idx;
};

struct mcux_jpegdec_config {
	JPEG_DECODER_Type base;
	void (*irq_config_func)(const struct device *dev);
};

#define MCUX_JPEGDEC_WIDTH_HEIGHT_MIN  64U
#define MCUX_JPEGDEC_WIDTH_HEIGHT_MAX  0x2000U
#define MCUX_JPEGDEC_WIDTH_HEIGHT_STEP 16U

#define MCUX_JPEGDEC_FORMAT_CAP(format)				\
	{.pixelformat = (format),				\
	 .width_min = (MCUX_JPEGDEC_WIDTH_HEIGHT_MIN),		\
	 .width_max = (MCUX_JPEGDEC_WIDTH_HEIGHT_MAX),		\
	 .height_min = (MCUX_JPEGDEC_WIDTH_HEIGHT_MIN),		\
	 .height_max = (MCUX_JPEGDEC_WIDTH_HEIGHT_MAX),		\
	 .width_step = (MCUX_JPEGDEC_WIDTH_HEIGHT_STEP),	\
	 .height_step = (MCUX_JPEGDEC_WIDTH_HEIGHT_STEP)}

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
};

static const struct pixel_map pixel_map_confs[] = {
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYUV420,
		.vid_pixel_format = VIDEO_PIX_FMT_NV12,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYUV422,
		.vid_pixel_format = VIDEO_PIX_FMT_YUYV,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatRGB,
		.vid_pixel_format = VIDEO_PIX_FMT_BGR24,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYUV444,
		.vid_pixel_format = VIDEO_PIX_FMT_YUV24,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatGray,
		.vid_pixel_format = VIDEO_PIX_FMT_GREY,
	},
	{
		.drv_pixel_format = kJPEGDEC_PixelFormatYCCK,
		.vid_pixel_format = VIDEO_PIX_FMT_ARGB32,
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

	LOG_ERR("Unknown pixel format 0x%x", pixel_format);
	return 0;
}

static void mcux_jpegdec_decode_one_frame(const struct device *dev)
{
	const struct mcux_jpegdec_config *config = dev->config;
	struct mcux_jpegdec_data *data = dev->data;
	struct video_buffer *current_in = NULL;
	struct video_buffer *current_out = NULL;

	if (!video_stream_busy(dev) ||
	    k_fifo_is_empty(in_fifo_in) || k_fifo_is_empty(out_fifo_in)) {
		/*
		 * Streaming was stopped, or nothing can be done if either input
		 * or output queue is empty.
		 */
		data->running = false;
		return;
	}

	/* Get the input buffer from the input queue. */
	current_in = k_fifo_peek_head(in_fifo_in);
	current_out = k_fifo_peek_head(out_fifo_in);

	data->decoder_despt.config.jpegBufAddr = (uint32_t)current_in->buffer;
	data->decoder_despt.config.jpegBufSize = ROUND_UP(current_in->size, MCUX_JPEGDEC_BUF_ALIGN);
	data->decoder_despt.config.outBufAddr0 = (uint32_t)current_out->buffer;
	data->decoder_despt.config.outBufPitch = data->vdev.out_fmt.pitch;

	/*
	 * For VIDEO_PIX_FMT_NV12, we need a second output buffer for the UV plane.
	 * Its address shall be strictly next to the end of Y plane.
	 */
	if (data->vdev.out_fmt.pixelformat == VIDEO_PIX_FMT_NV12) {
		data->decoder_despt.config.outBufAddr1 =
			data->decoder_despt.config.outBufAddr0 +
			data->vdev.out_fmt.pitch * data->vdev.out_fmt.height;
	}

	/* Start the decoding of next frame. */
	JPEGDEC_EnableSlotNextDescpt((JPEG_DECODER_Type *)&config->base, 0);

	data->running = true;
}

static int mcux_jpegdec_set_fmt(const struct device *dev, struct video_format *fmt, size_t idx)
{
	struct mcux_jpegdec_data *data = dev->data;

	/*
	 * The input size and output format is obtained after parsing the first enqueued
	 * input frame buffer. So only validate the format after queueing the first frame,
	 * Note: It is assumed that all the following JPEG frames in the stream have the
	 * same format as the first one.
	 */
	fmt->size = 0;
	fmt->pitch = 0;
	if (data->first_frame_rcv) {
		struct video_format *current_format =
			(fmt->type == VIDEO_BUF_TYPE_INPUT) ? &(data->vdev.in_fmt) :
			&(data->vdev.out_fmt);

		if ((fmt->pixelformat != current_format->pixelformat) ||
			(fmt->width != current_format->width) ||
			(fmt->height != current_format->height) ||
			(fmt->pitch != current_format->pitch) ||
			(fmt->size != current_format->size)) {
			LOG_ERR("The input/output format is determined by the JPEG header"
				" and cannot be changed.");
			return -EINVAL;
		}
	}

	return 0;
}

static int mcux_jpegdec_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct mcux_jpegdec_data *data = dev->data;
	bool do_decode = false;

	ARG_UNUSED(type);

	if (enable) {
		mcux_jpegdec_decode_one_frame(dev);
	} else {
		/*
		 * Re-parse the header of the first frame fed after the
		 * stream is restarted, instead of assuming it still
		 * matches the format of the previous stream.
		 */
		data->first_frame_rcv = false;

		/*
		 * Let a frame that is already being decoded by the hardware finish
		 * before stopping, instead of tearing down the stream underneath it.
		 * mcux_jpegdec_decode_one_frame() will not start another frame once
		 * video_stream_busy() is false.
		 */
		while (data->running) {
			k_sem_take(&data->decode_done, K_FOREVER);
		}
	}

	return 0;
}

static int mcux_jpegdec_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	int ret = 0;
	struct mcux_jpegdec_data *data = dev->data;
	struct k_fifo *fifo = vbuf->type == VIDEO_BUF_TYPE_INPUT
		? &data->vdev.in_buf_submitted : &data->vdev.out_buf_submitted;
	bool do_decode = false;

	if (((uint32_t)vbuf->buffer & 0xFU) != 0U) {
		LOG_ERR("Buffer address must be 16-byte aligned");
		return -EINVAL;
	}

	/*
	 * The decoder cannot perform CSC, which means if the JPEG file is encoded in
	 * YUV420_2p(NV12) then the output can only be the same.
	 * So the output format and capability can only be got after the first frame
	 * of JPEG is fed to the decoder, and it is assumed that all the following
	 * JPEG frames in the stream have the same format as the first one, so the
	 * parsing is done only once.
	 */
	if (vbuf->type == VIDEO_BUF_TYPE_INPUT) {
		/*
		 * Check if it is the first frame, if so parse the JPEG header
		 * and obtain the frame info.
		 */
		if (!data->first_frame_rcv) {
			/* Feed the input buffer to the decoder for header parsing. */
			data->decoder_despt.config.jpegBufAddr = (uint32_t)vbuf->buffer;

			/* Input buffer size must be 1024-byte aligned, round up */
			data->decoder_despt.config.jpegBufSize =
				ROUND_UP(vbuf->size, MCUX_JPEGDEC_BUF_ALIGN);

			if (JPEGDEC_ParseHeader(&data->decoder_despt.config) ==
				kStatus_JPEGDEC_NotSupported) {
				LOG_ERR("JPEG format not supported");
				return -ENOTSUP;
			}

			/* Set output pixel format based on parsed JPEG header */
			data->format_idx = mcux_jpegdec_get_conf(
				data->decoder_despt.config.pixelFormat, true);
			data->vdev.out_fmt.pixelformat =
				pixel_map_confs[data->format_idx].vid_pixel_format;
			data->vdev.out_fmt.width = data->decoder_despt.config.width;
			data->vdev.out_fmt.height = data->decoder_despt.config.height;

			/* Calculate the default pitch and buffer size. */
			if (data->vdev.out_fmt.pixelformat == VIDEO_PIX_FMT_NV12) {
				/*
				 * NV12 is a 2-plane format. The Y plane pitch is
				 * 1 byte per pixel. video_bits_per_pixel() returns
				 * 12 bpp (sum of both planes) which cannot be used
				 * here for pitch calculation.
				 */
				data->vdev.out_fmt.pitch = 1U * data->vdev.out_fmt.width;
			} else {
				data->vdev.out_fmt.pitch =
					video_bits_per_pixel(data->vdev.out_fmt.pixelformat)
					/ 8U * data->vdev.out_fmt.width;
			}
			/*
			 * For NV12 2-plane format, the Y plane pitch is 1 byte per
			 * pixel and the UV plane (interleaved, half height) uses the
			 * same pitch. The JPEGDEC hardware addresses the UV plane at
			 * outBufAddr0 + pitch*height, so the total buffer must span
			 * two full pitch*height regions: Y plane (pitch*height) + UV
			 * plane slot (pitch*height, of which only the top half
			 * contains valid UV data). video_bits_per_pixel(NV12) = 12
			 * bpp gives a smaller 1.5x factor and cannot be used here.
			 */
			if (data->vdev.out_fmt.pixelformat == VIDEO_PIX_FMT_NV12) {
				data->vdev.out_fmt.size = data->vdev.out_fmt.pitch *
					data->vdev.out_fmt.height * 2U;
			} else {
				data->vdev.out_fmt.size =
					data->vdev.out_fmt.pitch * data->vdev.out_fmt.height;
			}
			/* outBufPitch is the pitch of the Y (Luma) plane. For NV12, the UV
			 * plane uses the same pitch value; the hardware places the UV data
			 * immediately after the Y plane at outBufAddr0 +
			 * outBufPitch * height.
			 */
			data->decoder_despt.config.outBufPitch = data->vdev.out_fmt.pitch;

			/*
			 * The input width/height is the same as output. JPEG files
			 * do not have pitch and the size can be different for each
			 * file due to compression ratio, so will not set them.
			 */
			data->vdev.in_fmt.width = data->vdev.out_fmt.width;
			data->vdev.in_fmt.height = data->vdev.out_fmt.height;

			data->first_frame_rcv = true;
		}
	}

	k_fifo_put(fifo, vbuf);

	/*
	 * If the streaming is on but the decoder is not running due to
	 * lack of in/out buffer, start to decode new frame.
	 */
	if (!data->running && video_stream_busy(&data->vdev)) {
		mcux_jpegdec_decode_one_frame(dev);
	}

	return ret;
}

static int mcux_jpegdec_dequeue(const struct device *dev, struct video_buffer **vbuf,
				k_timeout_t timeout)
{
	struct mcux_jpegdec_data *data = dev->data;
	struct k_fifo *fifo = vbuf->type == VIDEO_BUF_TYPE_INPUT
		? &data->vdev.in_buf_completed : &data->vdev.out_buf__completed;

	*vbuf = k_fifo_get(fifo, timeout);
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
	caps->buf_align = 16U;

	return 0;
}

static int mcux_jpegdec_transform_cap(const struct device *const dev,
				const struct video_format_cap *const cap,
				struct video_format_cap *const res_cap,
				enum video_buf_type direction, uint16_t ind)
{
	struct mcux_jpegdec_data *data = dev->data;

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

static DEVICE_API(video, mcux_jpegdec_driver_api) = {
	.set_format = mcux_jpegdec_set_fmt,
	.set_stream = mcux_jpegdec_set_stream,
	.get_caps = mcux_jpegdec_get_caps,
	.transform_cap = mcux_jpegdec_transform_cap,
	.enqueue = mcux_jpegdec_enqueue,
	.dequeue = mcux_jpegdec_dequeue,
};

static int mcux_jpegdec_init(const struct device *dev)
{
	const struct mcux_jpegdec_config *config = dev->config;
	struct mcux_jpegdec_data *data = dev->data;
	jpegdec_config_t init_config;

	k_sem_init(&data->decode_done, 0, 1);

	/* Initialise default input / output formats */
	data->vdev.in_fmt.type = VIDEO_BUF_TYPE_INPUT;
	data->vdev.in_fmt.pixelformat = VIDEO_PIX_FMT_JPEG; /* Can only be JPEG format for input */
	data->vdev.in_fmt.width = MCUX_JPEGDEC_WIDTH_HEIGHT_MIN;
	data->vdev.in_fmt.height = MCUX_JPEGDEC_WIDTH_HEIGHT_MIN;

	/*
	 * The output format and size are determined by the JPEG header parsing,
	 * so just set a default value here.
	 */
	data->vdev.out_fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	data->vdev.out_fmt.pixelformat = VIDEO_PIX_FMT_NV12;
	data->vdev.out_fmt.width = MCUX_JPEGDEC_WIDTH_HEIGHT_MIN;
	data->vdev.out_fmt.height = MCUX_JPEGDEC_WIDTH_HEIGHT_MIN;
	data->vdev.out_fmt.size = MCUX_JPEGDEC_WIDTH_HEIGHT_MIN * MCUX_JPEGDEC_WIDTH_HEIGHT_MIN * 2U;

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
	struct video_buffer *current_in = k_fifo_get(&data->vdev.in_buf_submitted, K_NO_WAIT);
	struct video_buffer *current_out = k_fifo_get(&data->vdev.out_buf_submitted, K_NO_WAIT);
	uint32_t flag = JPEGDEC_GetStatusFlags((JPEG_DECODER_Type *)&config->base, 0);

	/* Decode complete. */
	if ((flag & kJPEGDEC_DecodeCompleteFlag) != 0U) {
		current_out->bytesused = data->vdev.out_fmt.size;
	}

	/* Error occur, this output buffer is broken, no valid data can be used. */
	if ((flag & kJPEGDEC_ErrorFlags) != 0U) {
		if (flag & kJPEGDEC_DecodeErrorFlag) {
			LOG_ERR("JPEG decode error: decode error");
		}
		if (flag & kJPEGDEC_DescptReadErrorFlag) {
			LOG_ERR("JPEG decode error: descriptor read error");
		}
		if (flag & kJPEGDEC_BitReadErrorFlag) {
			LOG_ERR("JPEG decode error: bitstream read error");
		}
		if (flag & kJPEGDEC_PixelWriteErrorFlag) {
			LOG_ERR("JPEG decode error: pixel write error");
		}
		current_out->bytesused = 0;
	}

	JPEGDEC_ClearStatusFlags((JPEG_DECODER_Type *)&config->base, 0, flag);

	current_out->size = data->vdev.out_fmt.size;

	k_fifo_put(&data->vdev.in_buf_completed, current_in);
	k_fifo_put(&data->vdev.out_buf_completed, current_out);

	k_sem_give(&data->decode_done);

	mcux_jpegdec_decode_one_frame(dev);
}

#define MCUX_JPEGDEC_INIT(n)									\
	static void mcux_jpegdec_irq_config_##n(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_jpegdec_isr,	\
			DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_INST_IRQN(n));							\
	}											\
												\
	static struct mcux_jpegdec_data mcux_jpegdec_data_##n = {				\
		.vdev = VIDEO_DEVICE_INIT(mcux_jpegdec_data_##n, DEVICE_DT_GET(n), NULL),	\
		.decoder_despt.config.clearStreamBuf = true,					\
		.decoder_despt.config.autoStart = true,						\
	};											\
												\
	static const struct mcux_jpegdec_config mcux_jpegdec_config_##n = {			\
		.base =	{									\
			.wrapper = (JPGDECWRP_Type *)DT_INST_REG_ADDR_BY_IDX(n, 0),		\
			.core = (JPEGDEC_Type *)DT_INST_REG_ADDR_BY_IDX(n, 1),			\
		},										\
		.irq_config_func = mcux_jpegdec_irq_config_##n,					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, &mcux_jpegdec_init, NULL, &mcux_jpegdec_data_##n,		\
		&mcux_jpegdec_config_##n, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
		&mcux_jpegdec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_JPEGDEC_INIT)
