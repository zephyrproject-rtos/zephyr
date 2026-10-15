/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_video_emul_rx

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_emul_rx, CONFIG_VIDEO_LOG_LEVEL);

struct emul_rx_data {
	struct video_device vdev; /* keep first */
	struct k_work work;
};

static int emul_rx_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct emul_rx_data *data = dev->data;

	return video_get_caps(data->source_dev, caps);
}

static int emul_rx_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct emul_rx_data *data = dev->data;

	/* A real hardware driver would first start / stop its own peripheral */
	if (enable) {
		return video_stream_start(data->vdev.source_dev, type)
	} else {
		struct k_work_sync sync;

		ret = video_stream_stop(data->vdev.source_dev, type);

		/* Cancel the jobs that were not running */
		k_work_cancel(&data->work);

		/* Flush the jobs that were still running */
		k_work_flush(&data->work, &sync);

		return ret;
	}
}

static void emul_rx_worker(struct k_work *work)
{
	struct emul_rx_data *data = CONTAINER_OF(work, struct emul_rx_data, work);
	const struct device *dev = data->vdev.dev;
	struct video_format *fmt = &data->vdev.out_fmt;
	struct video_buffer *vbuf = vbuf;

	LOG_DBG("Queueing a frame of %u bytes in format %x %ux%u", fmt->pitch * fmt->height,
		fmt->pixelformat, fmt->width, fmt->height);

	while ((vbuf = k_fifo_get(&data->vdev.out_buf_submitted, K_NO_WAIT)) != NULL) {
		vbuf->bytesused = fmt->pitch * fmt->height;
		vbuf->line_offset = 0;

		LOG_DBG("Inserting %u bytes into buffer %p", vbuf->bytesused, vbuf->buffer);

		/* Simulate the MIPI/DVP hardware transferring image data line-by-line from the
		 * imager to the video buffer memory using DMA copying the data line-by-line over
		 * the whole frame. vbuf->size is already checked in emul_rx_enqueue().
		 */
		for (size_t i = 0; i + fmt->pitch <= vbuf->bytesused; i += fmt->pitch) {
			memcpy(vbuf->buffer + i, data->vdev.source_dev->data, fmt->pitch);
		}

		/* Once the buffer is completed, submit it to the video buffer */
		video_buffer_done(dev, vbuf, 0);
	}
}

static int emul_rx_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct emul_rx_data *data = dev->data;
	struct video_format *fmt = &data->vdev.out_fmt;

	/* Submit the buffer for processing in the worker, where everything happens */
	k_fifo_put(&data->vdev.out_buf_submitted, vbuf);
	k_work_submit(&data->work);

	return 0;
}

static DEVICE_API(video, emul_rx_driver_api) = {
	.get_caps = emul_rx_get_caps,
	.set_stream = emul_rx_set_stream,
	.enqueue = emul_rx_enqueue,
};

int emul_rx_init(const struct device *dev)
{
	struct emul_rx_data *data = dev->data;
	int ret;

	if (!device_is_ready(data->vdev.source_dev)) {
		LOG_ERR("Source device %s is not ready", data->vdev.source_dev->name);
		return -ENODEV;
	}

	ret = video_get_format(data->vdev.source_dev, &data->vdev.out_fmt);
	if (ret < 0) {
		return ret;
	}

	ret = video_estimate_fmt_size(&data->vdev.out_fmt);
	if (ret < 0) {
		return ret;
	}

	k_work_init(&data->work, &emul_rx_worker);

	return 0;
}

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

#define EMUL_RX_DEFINE(n)                                                                          \
	static struct emul_rx_data emul_rx_data_##n = {                                            \
		.vdev = VIDEO_DEVICE_INIT(emul_rx_data_##n, DEVICE_DT_GET(n), SOURCE_DEV(n)),      \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &emul_rx_init, NULL, &emul_rx_data_##n, &emul_rx_cfg_##n,         \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &emul_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_RX_DEFINE)
