/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_video_sw_pipeline

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/pixel/stream.h>

LOG_MODULE_REGISTER(video_sw_pipeline, CONFIG_VIDEO_LOG_LEVEL);

struct video_sw_pipeline_data {
	struct k_fifo fifo_input_in;
	struct k_fifo fifo_input_out;
	struct k_fifo fifo_output_in;
	struct k_fifo fifo_output_out;
	struct k_poll_signal *sig;
	struct pixel_stream step_root;
	struct pixel_stream step_export;
	struct k_mutex mutex;
	struct video_format_cap fmts_in[2];
	struct video_format_cap fmts_out[2];
	void (*load)(struct pixel_stream *s, const uint8_t *b, size_t n);
	const struct device *source_dev;
};

static void video_sw_pipeline_thread(void *p0, void *p1, void *p2)
{
	const struct device *dev = p0;
	struct video_sw_pipeline_data *data = dev->data;

	while (true) {
		struct video_buffer *ibuf;
		struct video_buffer *obuf;

		ibuf = k_fifo_get(&data->fifo_input_in, K_FOREVER);
		obuf = k_fifo_get(&data->fifo_output_in, K_FOREVER);

		/* Wait until the stream is started and unlock to not block the API calls */
		k_mutex_lock(&data->mutex, K_FOREVER);
		k_mutex_unlock(&data->mutex);

		/* Load the output video buffer and run the stream */
		ring_buf_init(&data->step_export.ring, obuf->size, obuf->buffer);
		data->load(data->step_root.next, ibuf->buffer, ibuf->bytesused);

		/* Store the obuf in the obuf buffer */
		obuf->timestamp = k_uptime_get_32();
		obuf->bytesused = ring_buf_size_get(&data->step_export.ring);
		obuf->line_offset = 0;

		/* Move the buffers from submission to completion queue for both input/output */
		k_fifo_get(&data->fifo_input_in, K_NO_WAIT);
		k_fifo_put(&data->fifo_input_out, ibuf);
		k_fifo_get(&data->fifo_output_in, K_NO_WAIT);
		k_fifo_put(&data->fifo_output_out, obuf);

		if (IS_ENABLED(CONFIG_POLL) && data->sig != NULL) {
			k_poll_signal_raise(data->sig, VIDEO_BUF_DONE);
		}
	}
}

static int video_sw_pipeline_get_caps(const struct device *dev, enum video_endpoint_id ep,
				      struct video_caps *caps)
{
	struct video_sw_pipeline_data *data = dev->data;
	const struct video_format_cap *fmts = (ep == VIDEO_EP_IN) ? data->fmts_in : data->fmts_out;

	__ASSERT(data->fmts_in[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");
	__ASSERT(data->fmts_out[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	caps->min_vbuf_count = 0;
	caps->min_line_count = 1;
	caps->max_line_count = LINE_COUNT_HEIGHT;
	caps->format_caps = fmts;

	return 0;
}

void video_sw_pipeline_set_source(const struct device *dev, const struct device *source_dev)
{
	struct video_sw_pipeline_data *data = dev->data;

	data->source_dev = source_dev;
}

void video_sw_pipeline_set_loader(const struct device *dev,
				  void (*fn)(struct pixel_stream *s, const uint8_t *b, size_t n))
{
	struct video_sw_pipeline_data *data = dev->data;

	data->load = fn;
}

void video_sw_pipeline_set_pipeline(const struct device *dev, struct pixel_stream *strm,
				    uint32_t pixfmt_in, uint16_t width_in, uint16_t height_in,
				    uint32_t pixfmt_out, uint16_t width_out, uint16_t height_out)
{
	struct video_sw_pipeline_data *data = dev->data;
	struct pixel_stream *step = &data->step_root;

	/* Set the driver input/output format according to the stream input/output format */
	data->fmts_in[0].pixelformat = pixfmt_in;
	data->fmts_in[0].width_min = data->fmts_in[0].width_max = width_in;
	data->fmts_in[0].height_min = data->fmts_in[0].height_max = height_in;
	data->fmts_out[0].pixelformat = pixfmt_out;
	data->fmts_out[0].width_min = data->fmts_out[0].width_max = width_out;
	data->fmts_out[0].height_min = data->fmts_out[0].height_max = height_out;

	/* Find the first and last steps of the stream */
	data->step_root.next = strm;
	while (step->next != NULL) {
		step = step->next;
	}
	step = step->next = &data->step_export;

	/* Add an extra export step */
	data->step_export.name = "[export video_sw_pipeline]";
	data->step_export.width = width_out;
	data->step_export.height = height_out;
	data->step_export.pitch = width_out * video_bits_per_pixel(pixfmt_out) / BITS_PER_BYTE;
	data->step_export.run = NULL;
}

static int video_sw_pipeline_set_fmt(const struct device *dev, enum video_endpoint_id ep,
				     struct video_format *fmt)
{
	struct video_sw_pipeline_data *data = dev->data;
	const struct video_format_cap *fmts = (ep == VIDEO_EP_IN) ? data->fmts_in : data->fmts_out;
	struct video_format fmt_out = {
		.width = data->fmts_in[0].width_min,
		.height = data->fmts_in[0].height_min,
		.pixelformat = data->fmts_in[0].pixelformat,
		.pitch = data->fmts_in[0].width_min *
			 video_bits_per_pixel(data->fmts_in[0].pixelformat) / BITS_PER_BYTE,
	};

	__ASSERT(data->fmts_in[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");
	__ASSERT(data->fmts_out[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	if (fmt->pixelformat != fmts[0].pixelformat || fmt->width != fmts[0].width_min ||
	    fmt->height != fmts[0].height_min) {
		LOG_ERR("Format requested different from format %ux%u '%s'",
			fmt->width, fmt->height, VIDEO_FOURCC_TO_STR(fmt->pixelformat));
		return -EINVAL;
	}

	/* Apply the format configured by video_sw_pipeline_set_pipeline() to the source device */
	return video_set_format(data->source_dev, VIDEO_EP_OUT, &fmt_out);
}

static int video_sw_pipeline_get_fmt(const struct device *dev, enum video_endpoint_id ep,
				     struct video_format *fmt)
{
	struct video_sw_pipeline_data *data = dev->data;
	const struct video_format_cap *fmts = (ep == VIDEO_EP_IN) ? data->fmts_in : data->fmts_out;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	fmt->pixelformat = fmts[0].pixelformat;
	fmt->width = fmts[0].width_min;
	fmt->height = fmts[0].height_min;
	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	return 0;
}

static int video_sw_pipeline_flush(const struct device *dev, enum video_endpoint_id ep, bool cancel)
{
	struct video_sw_pipeline_data *data = dev->data;
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

static int video_sw_pipeline_enqueue(const struct device *dev, enum video_endpoint_id ep,
				     struct video_buffer *vbuf)
{
	struct video_sw_pipeline_data *data = dev->data;
	struct k_fifo *fifo = (ep == VIDEO_EP_IN) ? &data->fifo_input_in : &data->fifo_output_in;

	__ASSERT(data->fmts_in[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");
	__ASSERT(data->fmts_out[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	k_fifo_put(fifo, vbuf);

	return 0;
}

static int video_sw_pipeline_dequeue(const struct device *dev, enum video_endpoint_id ep,
				     struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct video_sw_pipeline_data *data = dev->data;
	struct k_fifo *fifo = (ep == VIDEO_EP_IN) ? &data->fifo_input_out : &data->fifo_output_out;

	__ASSERT(data->fmts_in[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");
	__ASSERT(data->fmts_out[0].pixelformat != 0, "Call video_sw_pipeline_set_stream() first");

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		LOG_ERR("Need to specify input or output endpoint.");
		return -EINVAL;
	}

	*vbuf = k_fifo_get(fifo, timeout);
	if (*vbuf == NULL) {
		LOG_DBG("Failed to dequeue %s buffer", ep == VIDEO_EP_IN ? "input" : "output");
		return -EAGAIN;
	}

	return 0;
}

static int video_sw_pipeline_set_stream(const struct device *dev, bool enable)
{
	struct video_sw_pipeline_data *data = dev->data;

	if (enable) {
		/* Release the stream processing thread */
		k_mutex_unlock(&data->mutex);
	} else {
		/* This will stop the stream thread without blocking this thread for long */
		k_mutex_lock(&data->mutex, K_FOREVER);
	}

	return 0;
}

static int video_sw_pipeline_set_frmival(const struct device *dev, enum video_endpoint_id ep,
					 struct video_frmival *frmival)
{
	struct video_sw_pipeline_data *data = dev->data;

	__ASSERT(data->source_dev != NULL, "Call video_sw_pipeline_set_source() first");

	return video_set_frmival(data->source_dev, ep, frmival);
}

static int video_sw_pipeline_get_frmival(const struct device *dev, enum video_endpoint_id ep,
					 struct video_frmival *frmival)
{
	struct video_sw_pipeline_data *data = dev->data;

	__ASSERT(data->source_dev != NULL, "Call video_sw_pipeline_set_source() first");

	return video_get_frmival(data->source_dev, ep, frmival);
}

static int video_sw_pipeline_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
					  struct video_frmival_enum *fie)
{
	struct video_sw_pipeline_data *data = dev->data;

	__ASSERT(data->source_dev != NULL, "Call video_sw_pipeline_set_source() first");

	return video_enum_frmival(data->source_dev, ep, fie);
}

#ifdef CONFIG_POLL
static int video_sw_pipeline_set_signal(const struct device *dev, enum video_endpoint_id ep,
					struct k_poll_signal *sig)
{
	struct video_sw_pipeline_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	data->sig = sig;

	return 0;
}
#endif

static DEVICE_API(video, video_sw_pipeline_driver_api) = {
	.set_format = video_sw_pipeline_set_fmt,
	.get_format = video_sw_pipeline_get_fmt,
	.flush = video_sw_pipeline_flush,
	.enqueue = video_sw_pipeline_enqueue,
	.dequeue = video_sw_pipeline_dequeue,
	.get_caps = video_sw_pipeline_get_caps,
	.set_stream = video_sw_pipeline_set_stream,
	.set_frmival = video_sw_pipeline_set_frmival,
	.get_frmival = video_sw_pipeline_get_frmival,
	.enum_frmival = video_sw_pipeline_enum_frmival,
#ifdef CONFIG_POLL
	.set_signal = video_sw_pipeline_set_signal,
#endif
};

static int video_sw_pipeline_init(const struct device *dev)
{
	struct video_sw_pipeline_data *data = dev->data;

	k_fifo_init(&data->fifo_input_in);
	k_fifo_init(&data->fifo_input_out);
	k_fifo_init(&data->fifo_output_in);
	k_fifo_init(&data->fifo_output_out);

	LOG_DBG("Initial format %ux%u", data->fmts_in[0].width_min, data->fmts_in[0].height_min);

	return 0;
}

#define VIDEO_SW_GENERATOR_DEFINE(n)                                                               \
	static struct video_sw_pipeline_data video_sw_pipeline_data_##n = {                        \
		.load = &pixel_stream_load,                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &video_sw_pipeline_init, NULL, &video_sw_pipeline_data_##n, NULL, \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,                             \
			      &video_sw_pipeline_driver_api);                                      \
                                                                                                   \
	K_THREAD_DEFINE(video_sw_pipeline, 1024, video_sw_pipeline_thread, DEVICE_DT_INST_GET(n),  \
			NULL, NULL, CONFIG_VIDEO_SW_PIPELINE_THREAD_PRIORITY, 0, 0);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_SW_GENERATOR_DEFINE)
