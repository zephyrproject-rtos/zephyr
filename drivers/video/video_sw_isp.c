/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/pixel/bayer.h>
#include <zephyr/pixel/formats.h>

LOG_MODULE_REGISTER(video_sw_isp, CONFIG_VIDEO_LOG_LEVEL);

#define WIDTH  CONFIG_VIDEO_SW_ISP_INPUT_WIDTH
#define HEIGHT CONFIG_VIDEO_SW_ISP_INPUT_HEIGHT

struct video_sw_isp_data {
	const struct device *dev;
	struct k_fifo fifo_input_in;
	struct k_fifo fifo_input_out;
	struct k_fifo fifo_output_in;
	struct k_fifo fifo_output_out;
	struct k_poll_signal *sig;
	struct video_format fmt_in;
	struct video_format fmt_out;
	struct pixel_stream *first_step;
	struct pixel_stream step_export;
	struct k_mutex mutex;
};

#define VIDEO_SW_ISP_FORMAT_CAP(pixfmt)                                                            \
	{                                                                                          \
		.pixelformat = (pixfmt),                                                           \
		.width_min = WIDTH,                                                                \
		.width_max = WIDTH,                                                                \
		.height_min = HEIGHT,                                                              \
		.height_max = HEIGHT,                                                              \
		.width_step = 0,                                                                   \
		.height_step = 0,                                                                  \
	}

static const struct video_format_cap fmts_in[] = {
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_RGB24),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_RGGB8),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_GRBG8),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_BGGR8),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_GBRG8),
	{0},
};

static const struct video_format_cap fmts_out[] = {
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_RGB24),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	VIDEO_SW_ISP_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),
	{0},
};

/* Bayer to RGB24 conversion */
static PIXEL_STREAM_RGGB8_TO_RGB24_3X3(step_rggb8_to_rgb24, WIDTH, HEIGHT);
static PIXEL_STREAM_GBRG8_TO_RGB24_3X3(step_gbrg8_to_rgb24, WIDTH, HEIGHT);
static PIXEL_STREAM_BGGR8_TO_RGB24_3X3(step_bggr8_to_rgb24, WIDTH, HEIGHT);
static PIXEL_STREAM_GRBG8_TO_RGB24_3X3(step_grbg8_to_rgb24, WIDTH, HEIGHT);

/* Pixel format conversion */
static PIXEL_STREAM_RGB24_TO_YUYV_BT709(step_rgb24_to_yuyv, WIDTH, HEIGHT);
static PIXEL_STREAM_YUYV_TO_RGB24_BT709(step_yuyv_to_rgb24, WIDTH, HEIGHT);
static PIXEL_STREAM_RGB565LE_TO_RGB24(step_rgb565le_to_rgb24, WIDTH, HEIGHT);
static PIXEL_STREAM_RGB24_TO_RGB565LE(step_rgb24_to_rgb565le, WIDTH, HEIGHT);

static void video_sw_isp_thread(void *p0, void *p1, void *p2)
{
	struct video_sw_isp_data *data = p0;
	const struct device *dev = data->dev;

	while (true) {
		struct video_buffer *input;
		struct video_buffer *output;
		struct video_format fmt_out = {0};

		input = k_fifo_get(&data->fifo_input_in, K_FOREVER);
		output = k_fifo_get(&data->fifo_output_in, K_FOREVER);

		/* Wait indefinitely if the stream is stopped */
		k_mutex_lock(&data->mutex, K_FOREVER);

		/* Do not block the thread calling video_set_stream(), check again next cycle */
		k_mutex_unlock(&data->mutex);

		video_get_format(dev, VIDEO_EP_OUT, &fmt_out);

		/* Load the video buffer into the last struct */
		ring_buf_init(&data->step_export.ring, output->size, output->buffer);
		data->step_export.height = fmt_out.height;
		data->step_export.pitch = fmt_out.pitch;
		data->step_export.run = NULL;
		pixel_stream_load(data->first_step, input->buffer, input->bytesused);
		output->bytesused = ring_buf_size_get(&data->step_export.ring);

		LOG_DBG("Converted a buffer from %p (%u bytes) to %p (%u bytes)", input->buffer,
			input->bytesused, output->buffer, output->bytesused);

		/* Move the buffer from submission to completion queue for the input endpoint */
		k_fifo_get(&data->fifo_input_in, K_NO_WAIT);
		k_fifo_put(&data->fifo_input_out, input);

		/* Move the buffer from submission to completion queue for the output endpoint */
		k_fifo_get(&data->fifo_output_in, K_NO_WAIT);
		k_fifo_put(&data->fifo_output_out, output);

		if (IS_ENABLED(CONFIG_POLL) && data->sig != NULL) {
			k_poll_signal_raise(data->sig, VIDEO_BUF_DONE);
		}
	}
}

static int video_sw_isp_get_caps(const struct device *dev, enum video_endpoint_id ep,
				 struct video_caps *caps)
{
	caps->min_vbuf_count = 0;
	caps->min_line_count = 1;
	caps->max_line_count = LINE_COUNT_HEIGHT;

	switch (ep) {
	case VIDEO_EP_IN:
		caps->format_caps = fmts_in;
		return 0;
	case VIDEO_EP_OUT:
		caps->format_caps = fmts_out;
		return 0;
	default:
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}
}

static int video_sw_isp_reconfigure(const struct device *dev)
{
	struct video_sw_isp_data *data = dev->data;
	struct pixel_stream root = {0};
	struct pixel_stream *strm = &root;
	struct video_format fmt_in = {0};
	struct video_format fmt_out = {0};
	uint32_t pixfmt;
	int ret = 0;

	video_get_format(dev, VIDEO_EP_IN, &fmt_in);
	video_get_format(dev, VIDEO_EP_OUT, &fmt_out);
	pixfmt = fmt_in.pixelformat;

	/* Entering the Unpacking stage */

	switch (pixfmt) {
	case VIDEO_PIX_FMT_RGB565:
		strm = strm->next = &step_rgb565le_to_rgb24;
		pixfmt = VIDEO_PIX_FMT_RGB24;
		break;
	}

	/* Entering the debayer stage */

	switch (pixfmt) {
	case VIDEO_PIX_FMT_RGGB8:
		strm = strm->next = &step_rggb8_to_rgb24;
		pixfmt = VIDEO_PIX_FMT_RGB24;
		break;
	case VIDEO_PIX_FMT_GRBG8:
		strm = strm->next = &step_grbg8_to_rgb24;
		pixfmt = VIDEO_PIX_FMT_RGB24;
		break;
	case VIDEO_PIX_FMT_BGGR8:
		strm = strm->next = &step_bggr8_to_rgb24;
		pixfmt = VIDEO_PIX_FMT_RGB24;
		break;
	case VIDEO_PIX_FMT_GBRG8:
		strm = strm->next = &step_gbrg8_to_rgb24;
		pixfmt = VIDEO_PIX_FMT_RGB24;
		break;
	}

	/* Entering the Output stage */

	switch (fmt_out.pixelformat) {
	case VIDEO_PIX_FMT_RGB565:
		switch (pixfmt) {
		case VIDEO_PIX_FMT_RGB565:
			break;
		case VIDEO_PIX_FMT_RGB24:
			strm = strm->next = &step_rgb24_to_rgb565le;
			break;
		case VIDEO_PIX_FMT_YUYV:
			strm = strm->next = &step_yuyv_to_rgb24;
			strm = strm->next = &step_rgb24_to_rgb565le;
			break;
		default:
			LOG_ERR("Cannot convert '%c%c%c%c' to RGB565LE", pixfmt >> 24, pixfmt >> 16,
				pixfmt >> 8, pixfmt);
			ret = -ENOTSUP;
			goto end;
		}
		pixfmt = VIDEO_PIX_FMT_RGB565;
		break;
	case VIDEO_PIX_FMT_YUYV:
		switch (pixfmt) {
		case VIDEO_PIX_FMT_YUYV:
			break;
		case VIDEO_PIX_FMT_RGB24:
			strm = strm->next = &step_rgb24_to_yuyv;
			break;
		default:
			LOG_ERR("Cannot convert '%c%c%c%c' to YUYV", pixfmt >> 24, pixfmt >> 16,
				pixfmt >> 8, pixfmt);
			ret = -ENOTSUP;
			goto end;
		}
		break;
	case VIDEO_PIX_FMT_RGB24:
		switch (pixfmt) {
		case VIDEO_PIX_FMT_RGB24:
			break;
		case VIDEO_PIX_FMT_YUYV:
			strm = strm->next = &step_yuyv_to_rgb24;
			break;
		default:
			LOG_ERR("Cannot convert '%c%c%c%c' to RGB24", pixfmt >> 24, pixfmt >> 16,
				pixfmt >> 8, pixfmt);
			ret = -ENOTSUP;
			goto end;
		}
		break;
	default:
		LOG_ERR("Unknown output format '%c%c%c%c'", fmt_out.pixelformat >> 24,
			fmt_out.pixelformat >> 16, fmt_out.pixelformat >> 8, fmt_out.pixelformat);
		ret = -ENOTSUP;
		goto end;
	}

	/* Entering the Export stage */

	strm = strm->next = &data->step_export;
	strm->name = "[export video_sw_isp " STRINGIFY(WIDTH) "x" STRINGIFY(HEIGHT) "]";

end:
	data->first_step = root.next;

	LOG_INF("Current topology:");
	for (strm = data->first_step; strm != NULL; strm = strm->next) {
		LOG_INF("- %s, pitch %u", strm->name, strm->pitch);
	}

	return ret;
}

static int video_sw_isp_set_fmt(const struct device *dev, enum video_endpoint_id ep,
				struct video_format *fmt)
{
	struct video_sw_isp_data *data = dev->data;
	const struct video_format_cap *cap;
	size_t i;
	int ret;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	LOG_DBG("Asking for format '%c%c%c%c' %ux%u", fmt->pixelformat >> 24,
		fmt->pixelformat >> 16, fmt->pixelformat >> 8, fmt->pixelformat, fmt->width,
		fmt->height);

	cap = (ep == VIDEO_EP_IN) ? &fmts_in[0] : &fmts_out[0];

	for (i = 0; cap[i].pixelformat != 0; i++) {
		if (fmt->pixelformat != cap[i].pixelformat &&
		    IN_RANGE(fmt->width, cap[i].width_min, cap[i].width_max) &&
		    IN_RANGE(fmt->height, cap[i].height_min, cap[i].height_max)) {
			break;
		}
	}

	if (cap[i].pixelformat == 0) {
		LOG_ERR("Trying to set an unsupported format '%c%c%c%c'", cap[i].pixelformat >> 24,
			cap[i].pixelformat >> 16, cap[i].pixelformat >> 8, cap[i].pixelformat);
		return -ENOTSUP;
	}

	memcpy((ep == VIDEO_EP_IN) ? &data->fmt_in : &data->fmt_out, fmt, sizeof(*fmt));

	ret = video_sw_isp_reconfigure(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reconfigure the isp");
		return ret;
	}

	return 0;
}

static int video_sw_isp_get_fmt(const struct device *dev, enum video_endpoint_id ep,
				struct video_format *fmt)
{
	struct video_sw_isp_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	memcpy(fmt, (ep == VIDEO_EP_IN) ? &data->fmt_in : &data->fmt_out, sizeof(*fmt));

	return 0;
}

static int video_sw_isp_flush(const struct device *dev, enum video_endpoint_id ep, bool cancel)
{
	struct video_sw_isp_data *data = dev->data;
	struct video_buffer *vbuf;

	if (cancel) {
		/* Skip all the buffers of the input endpointo, unprocessed */
		while ((vbuf = k_fifo_get(&data->fifo_input_in, K_NO_WAIT)) != NULL) {
			k_fifo_put(&data->fifo_input_out, vbuf);
			if (IS_ENABLED(CONFIG_POLL) && data->sig) {
				k_poll_signal_raise(data->sig, VIDEO_BUF_ABORTED);
			}
		}
		/* Skip all the buffers of the output endpoint, unprocessed */
		while ((vbuf = k_fifo_get(&data->fifo_output_in, K_NO_WAIT)) != NULL) {
			k_fifo_put(&data->fifo_output_out, vbuf);
			if (IS_ENABLED(CONFIG_POLL) && data->sig) {
				k_poll_signal_raise(data->sig, VIDEO_BUF_ABORTED);
			}
		}
	} else {
		/* Wait for all buffer to be processed on the input endpointo */
		while (!k_fifo_is_empty(&data->fifo_input_in)) {
			k_sleep(K_MSEC(1));
		}
		/* Wait for all buffer to be processed on the output endpointo */
		while (!k_fifo_is_empty(&data->fifo_output_in)) {
			k_sleep(K_MSEC(1));
		}
	}

	return 0;
}

static int video_sw_isp_enqueue(const struct device *dev, enum video_endpoint_id ep,
				struct video_buffer *vbuf)
{
	struct video_sw_isp_data *data = dev->data;

	switch (ep) {
	case VIDEO_EP_IN:
		k_fifo_put(&data->fifo_input_in, vbuf);
		break;
	case VIDEO_EP_OUT:
		k_fifo_put(&data->fifo_output_in, vbuf);
		break;
	default:
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	return 0;
}

static int video_sw_isp_dequeue(const struct device *dev, enum video_endpoint_id ep,
				struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct video_sw_isp_data *data = dev->data;

	switch (ep) {
	case VIDEO_EP_IN:
		*vbuf = k_fifo_get(&data->fifo_input_out, timeout);
		break;
	case VIDEO_EP_OUT:
		*vbuf = k_fifo_get(&data->fifo_output_out, timeout);
		break;
	default:
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	if (*vbuf == NULL) {
		LOG_DBG("Failed to dequeue %s buffer", ep == VIDEO_EP_IN ? "input" : "output");
		return -EAGAIN;
	}

	return 0;
}

static int video_sw_isp_set_stream(const struct device *dev, bool enable)
{
	struct video_sw_isp_data *data = dev->data;

	if (enable) {
		/* Release the stream processing thread */
		k_mutex_unlock(&data->mutex);
	} else {
		/* This will stop the stream thread without blocking this thread for long */
		k_mutex_lock(&data->mutex, K_FOREVER);
	}

	return 0;
}

#ifdef CONFIG_POLL
static int video_sw_isp_set_signal(const struct device *dev, enum video_endpoint_id ep,
				   struct k_poll_signal *sig)
{
	struct video_sw_isp_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	data->sig = sig;

	return 0;
}
#endif

static DEVICE_API(video, video_sw_isp_driver_api) = {
	.set_format = video_sw_isp_set_fmt,
	.get_format = video_sw_isp_get_fmt,
	.flush = video_sw_isp_flush,
	.enqueue = video_sw_isp_enqueue,
	.dequeue = video_sw_isp_dequeue,
	.get_caps = video_sw_isp_get_caps,
	.set_stream = video_sw_isp_set_stream,
#ifdef CONFIG_POLL
	.set_signal = video_sw_isp_set_signal,
#endif
};

static int video_sw_isp_init(const struct device *dev)
{
	struct video_sw_isp_data *data = dev->data;
	struct video_format fmt_in = {
		.pixelformat = fmts_in[0].pixelformat,
		.width = WIDTH,
		.height = HEIGHT,
		.pitch = WIDTH * video_bits_per_pixel(fmts_in[0].pixelformat) / BITS_PER_BYTE,
	};
	struct video_format fmt_out = {
		.pixelformat = fmts_out[0].pixelformat,
		.width = WIDTH,
		.height = HEIGHT,
		.pitch = WIDTH * video_bits_per_pixel(fmts_out[0].pixelformat) / BITS_PER_BYTE,
	};
	int ret;

	data->dev = dev;
	k_fifo_init(&data->fifo_input_in);
	k_fifo_init(&data->fifo_input_out);
	k_fifo_init(&data->fifo_output_in);
	k_fifo_init(&data->fifo_output_out);

	ret = video_sw_isp_set_fmt(dev, VIDEO_EP_IN, &fmt_in);
	if (ret != 0) {
		return ret;
	}

	ret = video_sw_isp_set_fmt(dev, VIDEO_EP_OUT, &fmt_out);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

struct video_sw_isp_data video_sw_isp_data_0 = {
	.fmt_in.pixelformat = VIDEO_PIX_FMT_RGB24,
	.fmt_out.pixelformat = VIDEO_PIX_FMT_RGB24,
};

DEVICE_DEFINE(video_sw_isp, "VIDEO_SW_ISP", &video_sw_isp_init, NULL, &video_sw_isp_data_0, NULL,
	      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &video_sw_isp_driver_api);

K_THREAD_DEFINE(video_sw_isp, CONFIG_VIDEO_SW_ISP_STACK_SIZE, video_sw_isp_thread,
		&video_sw_isp_data_0, NULL, NULL, CONFIG_VIDEO_SW_ISP_THREAD_PRIORITY, 0, 0);
