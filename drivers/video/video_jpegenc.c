/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Software baseline JPEG encoder based on the jpegenc module. Each input buffer is encoded
 * into one output buffer, in the thread calling video_enqueue().
 */

#define DT_DRV_COMPAT zephyr_video_jpegenc

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/controls.h>
#include <zephyr/video/video.h>

#include <zephyr_jpegenc.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_jpegenc, CONFIG_VIDEO_LOG_LEVEL);

#define DEFAULT_WIDTH   320
#define DEFAULT_HEIGHT  240
#define DEFAULT_FPS     30
#define DEFAULT_QUALITY 50
/* Room for the JPEG headers on top of the image data */
#define OUT_MARGIN      ZEPHYR_JPEGENC_MIN_OUTPUT_SIZE

struct jpegenc_data {
	struct k_mutex lock;
	struct video_format in_fmt;
	struct video_format out_fmt;
	struct video_frmival frmival;
	struct k_fifo in_fifo;
	struct k_fifo in_done;
	struct k_fifo out_fifo;
	struct k_fifo out_done;
	struct video_ctrl quality;
	struct zephyr_jpegenc enc;
};

#define JPEGENC_FORMAT_CAP(fmt)                                                                    \
	{                                                                                          \
		.pixelformat = fmt,                                                                \
		.width_min = 16,                                                                   \
		.width_max = 4096,                                                                 \
		.height_min = 16,                                                                  \
		.height_max = 4096,                                                                \
		.width_step = 16,                                                                  \
		.height_step = 16,                                                                 \
	}

static const struct video_format_cap in_fmts[] = {
	JPEGENC_FORMAT_CAP(VIDEO_PIX_FMT_NV12),
	JPEGENC_FORMAT_CAP(VIDEO_PIX_FMT_YUV420),
	JPEGENC_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),
	JPEGENC_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	JPEGENC_FORMAT_CAP(VIDEO_PIX_FMT_RGB565X),
	JPEGENC_FORMAT_CAP(VIDEO_PIX_FMT_GREY),
	{0},
};

static const struct video_format_cap out_fmts[] = {
	JPEGENC_FORMAT_CAP(VIDEO_PIX_FMT_JPEG),
	{0},
};

/* The encoder has four fixed quality levels */
static enum zephyr_jpegenc_quality jpegenc_quality(int32_t quality)
{
	if (quality >= 90) {
		return ZEPHYR_JPEGENC_QUALITY_BEST;
	} else if (quality >= 75) {
		return ZEPHYR_JPEGENC_QUALITY_HIGH;
	} else if (quality >= 50) {
		return ZEPHYR_JPEGENC_QUALITY_MEDIUM;
	}
	return ZEPHYR_JPEGENC_QUALITY_LOW;
}

static int jpegenc_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct jpegenc_data *data = dev->data;
	const struct video_format_cap *caps =
		(fmt->type == VIDEO_BUF_TYPE_INPUT) ? in_fmts : out_fmts;
	size_t idx;
	int ret;

	ret = video_format_caps_index(caps, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Unsupported format %s %ux%u", VIDEO_FOURCC_TO_STR(fmt->pixelformat),
			fmt->width, fmt->height);
		return -EINVAL;
	}

	if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
		switch (fmt->pixelformat) {
		case VIDEO_PIX_FMT_YUYV:
		case VIDEO_PIX_FMT_RGB565:
		case VIDEO_PIX_FMT_RGB565X:
			fmt->pitch = fmt->width * 2;
			fmt->size = fmt->pitch * fmt->height;
			break;
		case VIDEO_PIX_FMT_GREY:
			fmt->pitch = fmt->width;
			fmt->size = fmt->pitch * fmt->height;
			break;
		default:
			fmt->pitch = fmt->width;
			fmt->size = fmt->width * fmt->height * 3 / 2;
			break;
		}
	} else {
		/* Generous upper bound of a baseline JPEG image, plus room for the headers */
		fmt->pitch = 0;
		fmt->size = fmt->width * fmt->height * 2 + OUT_MARGIN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
		data->in_fmt = *fmt;
	} else {
		data->out_fmt = *fmt;
	}
	k_mutex_unlock(&data->lock);

	return 0;
}

static int jpegenc_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct jpegenc_data *data = dev->data;

	*fmt = (fmt->type == VIDEO_BUF_TYPE_INPUT) ? data->in_fmt : data->out_fmt;

	return 0;
}

static int jpegenc_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = (caps->type == VIDEO_BUF_TYPE_INPUT) ? in_fmts : out_fmts;
	caps->min_vbuf_count = 1;

	return 0;
}

static int jpegenc_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct jpegenc_data *data = dev->data;

	if (frmival->numerator == 0 || frmival->denominator == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->frmival = *frmival;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int jpegenc_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct jpegenc_data *data = dev->data;

	*frmival = data->frmival;

	return 0;
}

/* Encode one input buffer into one output buffer; called with the lock held */
static int jpegenc_encode(struct jpegenc_data *data, struct video_buffer *in,
			  struct video_buffer *out)
{
	const struct video_format *fmt = &data->in_fmt;
	uint32_t w = fmt->width;
	uint32_t h = fmt->height;
	enum zephyr_jpegenc_format format;
	enum zephyr_jpegenc_subsampling subsampling = ZEPHYR_JPEGENC_SUBSAMPLING_420;
	size_t len;
	int ret;

	if (w != data->out_fmt.width || h != data->out_fmt.height) {
		LOG_ERR("Input %ux%u and output %ux%u sizes differ", w, h, data->out_fmt.width,
			data->out_fmt.height);
		return -EINVAL;
	}

	if (in->bytesused < fmt->size) {
		LOG_ERR("Input buffer has %u bytes, expected %u", in->bytesused, fmt->size);
		return -EINVAL;
	}

	switch (fmt->pixelformat) {
	case VIDEO_PIX_FMT_NV12:
		format = ZEPHYR_JPEGENC_NV12;
		break;
	case VIDEO_PIX_FMT_YUV420:
		format = ZEPHYR_JPEGENC_YUV420P;
		break;
	case VIDEO_PIX_FMT_YUYV:
		format = ZEPHYR_JPEGENC_YUYV;
		break;
	case VIDEO_PIX_FMT_RGB565:
		format = ZEPHYR_JPEGENC_RGB565_LE;
		break;
	case VIDEO_PIX_FMT_RGB565X:
		format = ZEPHYR_JPEGENC_RGB565_BE;
		break;
	case VIDEO_PIX_FMT_GREY:
		format = ZEPHYR_JPEGENC_GRAY8;
		subsampling = ZEPHYR_JPEGENC_SUBSAMPLING_444;
		break;
	default:
		return -ENOTSUP;
	}

	if (out->size < OUT_MARGIN * 2) {
		return -ENOSPC;
	}

	ret = zephyr_jpegenc_begin(&data->enc, out->buffer, out->size, w, h, format, subsampling,
				   jpegenc_quality(data->quality.val));
	if (ret == 0) {
		switch (fmt->pixelformat) {
		case VIDEO_PIX_FMT_NV12:
			ret = zephyr_jpegenc_add_frame_nv12(&data->enc, in->buffer, w,
							    in->buffer + w * h, w);
			break;
		case VIDEO_PIX_FMT_YUV420:
			ret = zephyr_jpegenc_add_frame_yuv420p(
				&data->enc, in->buffer, w, in->buffer + w * h, w / 2,
				in->buffer + w * h + w * h / 4, w / 2);
			break;
		default:
			ret = zephyr_jpegenc_add_frame(&data->enc, in->buffer, fmt->pitch);
			break;
		}
	}
	if (ret == 0) {
		/* Writes the end of image marker and returns the image size */
		ret = zephyr_jpegenc_end(&data->enc, &len);
	}
	if (ret < 0) {
		LOG_ERR("Encoding failed: %d", ret);
		return ret;
	}

	out->bytesused = len;
	out->timestamp = in->timestamp;

	LOG_DBG("%ux%u %s frame: %zu bytes", w, h, VIDEO_FOURCC_TO_STR(fmt->pixelformat), len);

	return 0;
}

/* Called with the lock held, once both an input and an output buffer are queued */
static int jpegenc_process(struct jpegenc_data *data)
{
	struct video_buffer *in;
	struct video_buffer *out;
	int ret;

	if (k_fifo_is_empty(&data->in_fifo) || k_fifo_is_empty(&data->out_fifo)) {
		return 0;
	}

	in = k_fifo_get(&data->in_fifo, K_NO_WAIT);
	out = k_fifo_get(&data->out_fifo, K_NO_WAIT);

	ret = jpegenc_encode(data, in, out);
	if (ret < 0) {
		out->bytesused = 0;
	}

	k_fifo_put(&data->in_done, in);
	k_fifo_put(&data->out_done, out);

	return ret;
}

static int jpegenc_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct jpegenc_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	k_fifo_put((vbuf->type == VIDEO_BUF_TYPE_INPUT) ? &data->in_fifo : &data->out_fifo, vbuf);
	ret = jpegenc_process(data);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int jpegenc_dequeue(const struct device *dev, struct video_buffer **vbuf,
			   k_timeout_t timeout)
{
	struct jpegenc_data *data = dev->data;

	*vbuf = k_fifo_get(((*vbuf)->type == VIDEO_BUF_TYPE_INPUT) ? &data->in_done
								   : &data->out_done,
			   timeout);

	return (*vbuf == NULL) ? -EAGAIN : 0;
}

static int jpegenc_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	ARG_UNUSED(type);

	/* Each image is encoded independently: there is no stream state */
	return 0;
}

static int jpegenc_set_ctrl(const struct device *dev, uint32_t id)
{
	ARG_UNUSED(dev);

	/* The quality applies from the next image */
	return (id == VIDEO_CID_JPEG_COMPRESSION_QUALITY) ? 0 : -ENOTSUP;
}

static DEVICE_API(video, jpegenc_driver_api) = {
	.set_format = jpegenc_set_fmt,
	.get_format = jpegenc_get_fmt,
	.get_caps = jpegenc_get_caps,
	.set_frmival = jpegenc_set_frmival,
	.get_frmival = jpegenc_get_frmival,
	.set_stream = jpegenc_set_stream,
	.enqueue = jpegenc_enqueue,
	.dequeue = jpegenc_dequeue,
	.set_ctrl = jpegenc_set_ctrl,
};

static int jpegenc_init(const struct device *dev)
{
	struct jpegenc_data *data = dev->data;
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_INPUT,
		.pixelformat = VIDEO_PIX_FMT_NV12,
		.width = DEFAULT_WIDTH,
		.height = DEFAULT_HEIGHT,
	};
	int ret;

	k_mutex_init(&data->lock);
	k_fifo_init(&data->in_fifo);
	k_fifo_init(&data->in_done);
	k_fifo_init(&data->out_fifo);
	k_fifo_init(&data->out_done);

	data->frmival = (struct video_frmival){.numerator = 1, .denominator = DEFAULT_FPS};

	/* Same range and default as the other in-tree JPEG encoder */
	ret = video_init_ctrl(
		&data->quality, dev, VIDEO_CID_JPEG_COMPRESSION_QUALITY,
		(struct video_ctrl_range){.min = 5, .max = 100, .step = 1, .def = DEFAULT_QUALITY});
	if (ret < 0) {
		return ret;
	}

	ret = jpegenc_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	fmt.pixelformat = VIDEO_PIX_FMT_JPEG;

	return jpegenc_set_fmt(dev, &fmt);
}

#define JPEGENC_DEFINE(n)                                                                          \
	static struct jpegenc_data jpegenc_data_##n;                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, jpegenc_init, NULL, &jpegenc_data_##n, NULL, POST_KERNEL,         \
			      CONFIG_VIDEO_INIT_PRIORITY, &jpegenc_driver_api);                    \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(jpegenc_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(JPEGENC_DEFINE)
