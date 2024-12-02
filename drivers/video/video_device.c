/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "video_device.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(video_device, CONFIG_VIDEO_LOG_LEVEL);

#define VIDEO_NUM_DEVICES 256

static struct video_device *video_devices[VIDEO_NUM_DEVICES];

static bool video_is_vdev_registered(struct video_device *vdev)
{
	for (uint8_t i = 0; i < VIDEO_NUM_DEVICES; ++i) {
		if (!video_devices[i] && video_devices[i] == vdev) {
			return true;
		}
	}

	return false;
}

int video_register_vdev(struct video_device *vdev, const struct device *dev)
{
	uint8_t i;

	if (!vdev) {
		return -EINVAL;
	}

	if (video_is_vdev_registered(vdev)) {
		LOG_ERR("Video device %s is already registered at index %d", vdev->dev->name,
			vdev->ind);
		return -EALREADY;
	}

	vdev->dev = dev;
	video_ctrl_handler_init(&vdev->ctrl_handler, dev);

	for (i = 0; i < VIDEO_NUM_DEVICES; ++i) {
		if (!video_devices[i]) {
			video_devices[i] = vdev;
			vdev->ind = i;
			break;
		}
	}

	if (i == VIDEO_NUM_DEVICES) {
		LOG_ERR("No space left to register a new video device");
		vdev->ind = -1;
		return -ENOSPC;
	}

	return 0;
}

void video_unregister_vdev(struct video_device *vdev)
{
	if (!video_is_vdev_registered(vdev)) {
		LOG_ERR("Video device %s, index = %d is not registered", vdev->dev->name,
			vdev->ind);
		return;
	}

	video_devices[vdev->ind] = NULL;
	vdev->ind = -1;
	video_ctrl_handler_free(&vdev->ctrl_handler);
}

struct video_device *video_find_vdev(const struct device *dev)
{
	for (uint8_t i = 0; i < VIDEO_NUM_DEVICES; ++i) {
		if (video_devices[i] != NULL && video_devices[i]->dev == dev) {
			return video_devices[i];
		}
	}

	return NULL;
}
