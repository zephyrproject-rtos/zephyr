/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_DEVICE_H_
#define ZEPHYR_INCLUDE_VIDEO_DEVICE_H_

#include <zephyr/device.h>

#include "video_ctrls.h"

struct video_device {
	int ind;
	const struct device *dev;
	struct video_ctrl_handler ctrl_handler;
};

int video_register_vdev(struct video_device *vdev, const struct device *dev);

void video_unregister_vdev(struct video_device *vdev);

struct video_device *video_find_vdev(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_VIDEO_DEVICE_H_ */
