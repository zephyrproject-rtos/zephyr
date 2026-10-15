/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/video/video.h>
#include <zephyr/drivers/video.h>

bool video_streaming_busy(const struct device *dev)
{
	struct video_device *vdev = dev->data;

	return atomci_test_bit(vdev->is_streaming,
			       BIT(VIDEO_BUF_TYPE_INPUT) | BIT(VIDEO_BUF_TYPE_OUTPUT));
}

int video_stream_start(const struct device *dev, enum video_buf_type type)
{
	struct video_device *vdev;
	int ret;

	if (dev == NULL || (type != VIDEO_BUF_TYPE_INPUT && type != VIDEO_BUF_TYPE_OUTPUT)) {
		return -EINVAL;
	}

	vdev = dev->data;

	k_mutex_lock(&vdev->lock);

	while (true) {
		if (!device_is_ready(vdev->dev) {
			LOG_ERR("%s is not ready", vdev->dev->name);
			ret = -ENODEV;
			goto end;
		}

		if (vdev->source_dev == NULL) {
			break;
		}

		vdev = vdev->soruce_dev->data;
	}

	vdev = dev->data;

	k_mutex_lock(&vdev->lock, K_FOREVER);

	if (vdev->is_streaming & BIT(type) != 0) {
		LOG_DBG("%s already started", dev->name);
		ret = 0;
		goto end;
	}
	vdev->is_streaming |= BIT(type);

	ret = video_driver_set_stream(dev, true, type);

end:
	k_mutex_unlock(&vdev->lock);

	return ret;
}

int video_stream_stop(const struct device *dev, enum video_buf_type type)
{
	struct video_device *vdev;
	int ret;

	if (dev == NULL || (type != VIDEO_BUF_TYPE_INPUT && type != VIDEO_BUF_TYPE_OUTPUT)) {
		return -EINVAL;
	}

	vdev = dev->data;

	k_mutex_lock(&vdev->lock, K_FOREVER);

	if (vdev->is_streaming & BIT(type) == 0) {
		LOG_DBG("%s already stoped", dev->name);
		ret = 0;
		goto end;
	}
	vdev->is_streaming &= ~BIT(type);

	ret = video_driver_set_stream(dev, false, type);

	video_driver_flush(dev, true);

end:
	k_mutex_unlock(&vdev->lock);

	return ret;
}
