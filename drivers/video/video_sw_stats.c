/*
 * Copyright (c) 2025, tinyVision.ai Inc.
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sw_stats

#include <zephyr/kernel.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pixel/bayer.h>
#include <zephyr/pixel/stats.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(video_sw_stats, CONFIG_VIDEO_LOG_LEVEL);

#define NUM_SAMPLES CONFIG_VIDEO_SW_STATS_NUM_SAMPLES

struct video_sw_stats_data {
	const struct device *dev;
	struct video_format fmt;
	struct video_buffer *vbuf;
	uint16_t frame_counter;
};

#define VIDEO_SW_STATS_FORMAT_CAP(pixfmt)                                                          \
	{                                                                                          \
		.pixelformat = pixfmt,                                                             \
		.width_min = 2,                                                                    \
		.width_max = UINT16_MAX,                                                           \
		.height_min = 2,                                                                   \
		.height_max = UINT16_MAX,                                                          \
		.width_step = 2,                                                                   \
		.height_step = 2,                                                                  \
	}

static const struct video_format_cap fmts[] = {
	VIDEO_SW_STATS_FORMAT_CAP(VIDEO_PIX_FMT_RGB24),
	VIDEO_SW_STATS_FORMAT_CAP(VIDEO_PIX_FMT_RGGB8),
	VIDEO_SW_STATS_FORMAT_CAP(VIDEO_PIX_FMT_GRBG8),
	VIDEO_SW_STATS_FORMAT_CAP(VIDEO_PIX_FMT_BGGR8),
	VIDEO_SW_STATS_FORMAT_CAP(VIDEO_PIX_FMT_GBRG8),
	{0},
};

static int video_sw_stats_set_fmt(const struct device *dev, enum video_endpoint_id ep,
				  struct video_format *fmt)
{
	struct video_sw_stats_data *data = dev->data;
	int i;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	for (i = 0; fmts[i].pixelformat != 0; ++i) {
		if (fmt->pixelformat == fmts[i].pixelformat &&
		    IN_RANGE(fmt->width, fmts[i].width_min, fmts[i].width_max) &&
		    IN_RANGE(fmt->height, fmts[i].height_min, fmts[i].height_max)) {
			break;
		}
	}

	if (fmts[i].pixelformat == 0) {
		LOG_ERR("Unsupported pixel format or resolution");
		return -ENOTSUP;
	}

	data->fmt = *fmt;

	return 0;
}

static int video_sw_stats_get_fmt(const struct device *dev, enum video_endpoint_id ep,
				  struct video_format *fmt)
{
	struct video_sw_stats_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*fmt = data->fmt;

	return 0;
}

static int video_sw_stats_set_stream(const struct device *dev, bool enable)
{
	return 0;
}

static void video_sw_stats_histogram_y(const struct device *const dev, struct video_buffer *vbuf,
				       struct video_stats *stats)
{
	struct video_sw_stats_data *data = dev->data;
	struct video_stats_histogram *hist = (void *)stats;

	LOG_DBG("%u buckets submitted, using %u bits per channel",
		hist->num_buckets, LOG2(hist->num_buckets));

	memset(hist->buckets, 0x00, hist->num_buckets * sizeof(uint16_t));

	__ASSERT(hist->num_buckets % 3 == 0, "Each of R, G, B channel should have the same size.");

	/* Adjust the size to the lower power of two, as required by the pixel library */
	hist->num_buckets = 1 << LOG2(hist->num_buckets);
	hist->num_values = NUM_SAMPLES;
	stats->flags = VIDEO_STATS_HISTOGRAM_Y;

	LOG_DBG("Using %u buckets", hist->num_buckets);

	switch (data->fmt.pixelformat) {
	case VIDEO_PIX_FMT_RGB24:
		pixel_rgb24frame_to_y8hist(vbuf->buffer, vbuf->bytesused, hist->buckets,
					   hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_RGGB8:
		pixel_rggb8frame_to_y8hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					   hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_GBRG8:
		pixel_gbrg8frame_to_y8hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					   hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_BGGR8:
		pixel_bggr8frame_to_y8hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					   hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_GRBG8:
		pixel_grbg8frame_to_y8hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					   hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static void video_sw_stats_histogram_rgb(const struct device *const dev, struct video_buffer *vbuf,
					 struct video_stats *stats)
{
	struct video_sw_stats_data *data = dev->data;
	struct video_stats_histogram *hist = (void *)stats;

	LOG_DBG("%u buckets submitted, using %u bits per channel",
		hist->num_buckets, LOG2(hist->num_buckets / 3));

	stats->flags = VIDEO_STATS_HISTOGRAM_RGB;

	__ASSERT(hist->num_buckets % 3 == 0, "Each of R, G, B channel should have the same size.");

	/* Adjust the size to the lower power of two, as required by the pixel library */
	hist->num_buckets = (1 << LOG2(hist->num_buckets / 3)) * 3;
	hist->num_values = NUM_SAMPLES;

	memset(hist->buckets, 0x00, hist->num_buckets * sizeof(uint16_t));

	switch (data->fmt.pixelformat) {
	case VIDEO_PIX_FMT_RGB24:
		pixel_rgb24frame_to_rgb24hist(vbuf->buffer, vbuf->bytesused, hist->buckets,
					      hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_RGGB8:
		pixel_rggb8frame_to_rgb24hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					      hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_GBRG8:
		pixel_gbrg8frame_to_rgb24hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					      hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_BGGR8:
		pixel_bggr8frame_to_rgb24hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					      hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_GRBG8:
		pixel_grbg8frame_to_rgb24hist(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					      hist->buckets, hist->num_buckets, NUM_SAMPLES);
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static void video_sw_stats_channels(const struct device *const dev, struct video_buffer *vbuf,
				    struct video_stats *stats)
{
	struct video_sw_stats_data *data = dev->data;
	struct video_stats_channels *chan = (void *)stats;

	stats->flags = VIDEO_STATS_CHANNELS_RGB;

	chan->rgb[0] = chan->rgb[1] = chan->rgb[2] = chan->y = 0x00;

	switch (data->fmt.pixelformat) {
	case VIDEO_PIX_FMT_RGB24:
		pixel_rgb24frame_to_rgb24avg(vbuf->buffer, vbuf->bytesused, chan->rgb, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_RGGB8:
		pixel_rggb8frame_to_rgb24avg(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					     chan->rgb, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_GBRG8:
		pixel_gbrg8frame_to_rgb24avg(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					     chan->rgb, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_BGGR8:
		pixel_bggr8frame_to_rgb24avg(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					     chan->rgb, NUM_SAMPLES);
		break;
	case VIDEO_PIX_FMT_GRBG8:
		pixel_grbg8frame_to_rgb24avg(vbuf->buffer, vbuf->bytesused, data->fmt.width,
					     chan->rgb, NUM_SAMPLES);
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static int video_sw_stats_enqueue(const struct device *dev, enum video_endpoint_id ep,
				  struct video_buffer *vbuf)
{
	struct video_sw_stats_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	if (data->vbuf != NULL) {
		LOG_ERR("Buffer already loaded: %p, dequeue it first", data->vbuf);
		return -ENOTSUP;
	}

	if (vbuf->bytesused == 0 || vbuf->size == 0) {
		LOG_ERR("The input buffer is empty");
		return -EINVAL;
	}


	data->frame_counter++;
	data->vbuf = vbuf;

	return 0;
}

static int video_sw_stats_get_stats(const struct device *dev, enum video_endpoint_id ep,
				    struct video_stats *stats)
{
	struct video_sw_stats_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -ENOTSUP;
	}

	/* Wait that a frame is enqueued so that statistics can be performed on it */
	if (data->vbuf == NULL) {
		LOG_DBG("No frame is currently loaded, cannot perform statistics");
		return -EAGAIN;
	}

	LOG_DBG("Getting statistics out of %p (%u bytes)", data->vbuf->buffer,
		data->vbuf->bytesused);

	if (stats->flags & VIDEO_STATS_CHANNELS) {
		video_sw_stats_channels(dev, data->vbuf, stats);
	} else if (stats->flags & VIDEO_STATS_HISTOGRAM_Y) {
		video_sw_stats_histogram_y(dev, data->vbuf, stats);
	} else if (stats->flags & VIDEO_STATS_HISTOGRAM_RGB) {
		video_sw_stats_histogram_rgb(dev, data->vbuf, stats);
	}

	stats->frame_counter = data->frame_counter;

	return 0;
}

static int video_sw_stats_dequeue(const struct device *dev, enum video_endpoint_id ep,
				  struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct video_sw_stats_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*vbuf = data->vbuf;
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	data->vbuf = NULL;

	return 0;
}

static int video_sw_stats_get_caps(const struct device *dev, enum video_endpoint_id ep,
				   struct video_caps *caps)
{
	caps->format_caps = fmts;
	caps->min_vbuf_count = 0;

	/* SW stats processes full frames */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	return 0;
}

static DEVICE_API(video, video_sw_stats_driver_api) = {
	.set_format = video_sw_stats_set_fmt,
	.get_format = video_sw_stats_get_fmt,
	.set_stream = video_sw_stats_set_stream,
	.enqueue = video_sw_stats_enqueue,
	.dequeue = video_sw_stats_dequeue,
	.get_caps = video_sw_stats_get_caps,
	.get_stats = video_sw_stats_get_stats,
};

static struct video_sw_stats_data video_sw_stats_data_0 = {
	.fmt.width = 320,
	.fmt.height = 160,
	.fmt.pitch = 320 * 2,
	.fmt.pixelformat = VIDEO_PIX_FMT_RGB565,
};

static int video_sw_stats_init(const struct device *dev)
{
	struct video_sw_stats_data *data = dev->data;

	data->dev = dev;

	return 0;
}

DEVICE_DEFINE(video_sw_stats, "VIDEO_SW_STATS", &video_sw_stats_init, NULL, &video_sw_stats_data_0,
	      NULL, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &video_sw_stats_driver_api);
