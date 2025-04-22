/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_video_emul_rx

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "video_device.h"

LOG_MODULE_REGISTER(video_emul_rx, CONFIG_VIDEO_LOG_LEVEL);

struct emul_rx_config {
	const struct device *source_dev;
};

struct emul_rx_data {
	const struct device *dev;
	struct video_format fmt;
	struct k_work work;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
};

static int emul_rx_set_frmival(const struct device *dev, enum video_endpoint_id ep,
			       struct video_frmival *frmival)
{
	const struct emul_rx_config *cfg = dev->config;

	/* Input/output timing is driven by the source */
	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}
	return video_set_frmival(cfg->source_dev, VIDEO_EP_OUT, frmival);
}

static int emul_rx_get_frmival(const struct device *dev, enum video_endpoint_id ep,
			       struct video_frmival *frmival)
{
	const struct emul_rx_config *cfg = dev->config;

	/* Input/output timing is driven by the source */
	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}
	return video_get_frmival(cfg->source_dev, VIDEO_EP_OUT, frmival);
}

static int emul_rx_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
				struct video_frmival_enum *fie)
{
	const struct emul_rx_config *cfg = dev->config;

	/* Input/output timing is driven by the source */
	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}
	return video_enum_frmival(cfg->source_dev, VIDEO_EP_OUT, fie);
}

static int emul_rx_set_fmt(const struct device *const dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	const struct emul_rx_config *cfg = dev->config;
	struct emul_rx_data *data = dev->data;
	int ret;

	/* The same format is shared between input and output: data is just passed through */
	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	/* Propagate the format selection to the source */
	ret = video_set_format(cfg->source_dev, VIDEO_EP_OUT, fmt);
	if (ret < 0) {
		LOG_DBG("Failed to set %s format to %x %ux%u", cfg->source_dev->name,
			fmt->pixelformat, fmt->width, fmt->height);
		return -EINVAL;
	}

	/* Cache the format selected locally to use it for getting the size of the buffer  */
	data->fmt = *fmt;
	return 0;
}

static int emul_rx_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	struct emul_rx_data *data = dev->data;

	/* Input/output caps are the same as the source: data is just passed through */
	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}
	*fmt = data->fmt;
	return 0;
}

static int emul_rx_get_caps(const struct device *dev, enum video_endpoint_id ep,
			    struct video_caps *caps)
{
	const struct emul_rx_config *cfg = dev->config;

	/* Input/output caps are the same as the source: data is just passed through */
	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}
	return video_get_caps(cfg->source_dev, VIDEO_EP_OUT, caps);
}

static int emul_rx_set_stream(const struct device *dev, bool enable)
{
	const struct emul_rx_config *cfg = dev->config;

	/* A real hardware driver would first start / stop its own peripheral */
	return enable ? video_stream_start(cfg->source_dev) : video_stream_stop(cfg->source_dev);
}

static void emul_rx_worker(struct k_work *work)
{
	struct emul_rx_data *data = CONTAINER_OF(work, struct emul_rx_data, work);
	const struct device *dev = data->dev;
	const struct emul_rx_config *cfg = dev->config;
	struct video_format *fmt = &data->fmt;
	struct video_buffer *vbuf = vbuf;

	LOG_DBG("Queueing a frame of %u bytes in format %x %ux%u", fmt->pitch * fmt->height,
		fmt->pixelformat, fmt->width, fmt->height);

	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
		vbuf->bytesused = fmt->pitch * fmt->height;
		vbuf->line_offset = 0;

		LOG_DBG("Inserting %u bytes into buffer %p", vbuf->bytesused, vbuf->buffer);

		/* Simulate the MIPI/DVP hardware transferring image data line-by-line from the
		 * imager to the video buffer memory using DMA copying the data line-by-line over
		 * the whole frame. vbuf->size is already checked in emul_rx_enqueue().
		 */
		for (size_t i = 0; i + fmt->pitch <= vbuf->bytesused; i += fmt->pitch) {
			memcpy(vbuf->buffer + i, cfg->source_dev->data, fmt->pitch);
		}

		/* Once the buffer is completed, submit it to the video buffer */
		k_fifo_put(&data->fifo_out, vbuf);
	}
}

static int emul_rx_enqueue(const struct device *dev, enum video_endpoint_id ep,
			   struct video_buffer *vbuf)
{
	struct emul_rx_data *data = dev->data;
	struct video_format *fmt = &data->fmt;

	/* Can only enqueue a buffer to get data out, data input is from hardware */
	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	if (vbuf->size < fmt->pitch * fmt->height) {
		LOG_ERR("Buffer too small for a full frame");
		return -ENOMEM;
	}

	/* The buffer has not been filled yet: flag as emtpy */
	vbuf->bytesused = 0;

	/* Submit the buffer for processing in the worker, where everything happens */
	k_fifo_put(&data->fifo_in, vbuf);
	k_work_submit(&data->work);

	return 0;
}

static int emul_rx_dequeue(const struct device *dev, enum video_endpoint_id ep,
			   struct video_buffer **vbufp, k_timeout_t timeout)
{
	struct emul_rx_data *data = dev->data;

	/* Can only dequeue a buffer to get data out, data input is from hardware */
	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	/* All the processing is expected to happen in the worker */
	*vbufp = k_fifo_get(&data->fifo_out, timeout);
	if (*vbufp == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int emul_rx_flush(const struct device *dev, enum video_endpoint_id ep, bool cancel)
{
	struct emul_rx_data *data = dev->data;
	struct k_work_sync sync;

	/* Can only flush the buffer going out, data input is from hardware */
	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	if (cancel) {
		struct video_buffer *vbuf;

		/* Cancel the jobs that were not running */
		k_work_cancel(&data->work);

		/* Flush the jobs that were still running */
		k_work_flush(&data->work, &sync);

		/* Empty all the cancelled items */
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
			k_fifo_put(&data->fifo_out, vbuf);
		}
	} else {
		/* Process all the remaining items from the queue */
		k_work_flush(&data->work, &sync);
	}

	return 0;
}

static DEVICE_API(video, emul_rx_driver_api) = {
	.set_frmival = emul_rx_set_frmival,
	.get_frmival = emul_rx_get_frmival,
	.enum_frmival = emul_rx_enum_frmival,
	.set_format = emul_rx_set_fmt,
	.get_format = emul_rx_get_fmt,
	.get_caps = emul_rx_get_caps,
	.set_stream = emul_rx_set_stream,
	.enqueue = emul_rx_enqueue,
	.dequeue = emul_rx_dequeue,
	.flush = emul_rx_flush,
};

int emul_rx_init(const struct device *dev)
{
	struct emul_rx_data *data = dev->data;
	const struct emul_rx_config *cfg = dev->config;
	int ret;

	data->dev = dev;

	if (!device_is_ready(cfg->source_dev)) {
		LOG_ERR("Source device %s is not ready", cfg->source_dev->name);
		return -ENODEV;
	}

	ret = video_get_format(cfg->source_dev, VIDEO_EP_OUT, &data->fmt);
	if (ret < 0) {
		return ret;
	}

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_work_init(&data->work, &emul_rx_worker);

	return 0;
}

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

#define EMUL_RX_DEFINE(n)                                                                          \
	static const struct emul_rx_config emul_rx_cfg_##n = {                                     \
		.source_dev = SOURCE_DEV(n),                                                       \
	};                                                                                         \
                                                                                                   \
	static struct emul_rx_data emul_rx_data_##n = {                                            \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &emul_rx_init, NULL, &emul_rx_data_##n, &emul_rx_cfg_##n,         \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &emul_rx_driver_api);       \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(emul_rx_##n, DEVICE_DT_INST_GET(n), SOURCE_DEV(n));

DT_INST_FOREACH_STATUS_OKAY(EMUL_RX_DEFINE)
