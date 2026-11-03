/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/video/video.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(video_stream, CONFIG_VIDEO_LOG_LEVEL);

bool video_streaming_busy(const struct device *dev)
{
	struct video_device *vdev = dev->data;

	return atomic_test_bit(&vdev->is_streaming,
			       BIT(VIDEO_BUF_TYPE_INPUT) | BIT(VIDEO_BUF_TYPE_OUTPUT));
}

int video_stream_start(const struct device *dev, enum video_buf_type type)
{
	struct video_device *vdev;

	if (dev == NULL || (type != VIDEO_BUF_TYPE_INPUT && type != VIDEO_BUF_TYPE_OUTPUT)) {
		LOG_ERR("Invalid parameters (%p, %d)", (void *)dev, type);
		return -EINVAL;
	}

	vdev = dev->data;

	while (true) {
		if (!device_is_ready(vdev->dev)) {
			LOG_ERR("%s is not ready", vdev->dev->name);
			return -ENODEV;
		}

		if (vdev->source_dev == NULL) {
			break;
		}

		vdev = vdev->source_dev->data;
	}

	vdev = dev->data;

	if ((vdev->is_streaming & BIT(type)) != 0) {
		LOG_DBG("%s already started", dev->name);
		return 0;
	}
	vdev->is_streaming |= BIT(type);

	return video_driver_set_stream(dev, true, type);
}

int video_stream_stop(const struct device *dev, enum video_buf_type type)
{
	struct video_buffer *vbuf;
	struct video_device *vdev;
	struct k_fifo *fifo_submitted;
	struct k_fifo *fifo_completed;
	int ret;

	if (dev == NULL || (type != VIDEO_BUF_TYPE_INPUT && type != VIDEO_BUF_TYPE_OUTPUT)) {
		LOG_ERR("Invalid parameters (%p, %d)", (void *)dev, type);
		return -EINVAL;
	}

	vdev = dev->data;

	switch (type) {
	case VIDEO_BUF_TYPE_INPUT:
		fifo_completed = &vdev->in_buf_completed;
		fifo_submitted = &vdev->in_buf_completed;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		fifo_completed = &vdev->out_buf_completed;
		fifo_submitted = &vdev->out_buf_submitted;
		break;
	default:
		LOG_ERR("Invalid buffer type %u requested for %s", type, dev->name);
		return -EINVAL;
	}

	k_mutex_lock(&vdev->lock, K_FOREVER);

	if ((vdev->is_streaming & BIT(type)) == 0) {
		LOG_DBG("%s already stoped", dev->name);
		return 0;
	}
	vdev->is_streaming &= ~BIT(type);

	ret = video_driver_set_stream(dev, false, type);
	if (ret < 0) {
		LOG_ERR("Failed to stop %s for buffer types %u", dev->name, type);
		return ret;
	}

	while (!k_fifo_is_empty(fifo_submitted)) {
		vbuf = k_fifo_get(fifo_submitted, K_NO_WAIT);
		if (vbuf != NULL) {
			k_fifo_put(fifo_completed, vbuf);

			if (IS_ENABLED(CONFIG_POLL) && vdev->signal != NULL) {
				k_poll_signal_raise(vdev->signal, VIDEO_BUF_ABORTED);
			}
		}
	}

	return ret;
}
