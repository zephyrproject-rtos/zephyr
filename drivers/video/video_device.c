/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>

#include "video_device.h"

struct video_device *video_find_vdev(const struct device *dev)
{
	if (!dev) {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(video_device, vdev) {
		if (vdev->dev == dev) {
			return vdev;
		}
	}

	return NULL;
}

const struct device *video_get_dev(uint8_t i)
{
	const struct video_mdev *vmd = NULL;

	STRUCT_SECTION_GET(video_mdev, i, &vmd);

	return vmd->dev;
}

uint8_t video_get_devs_num(void)
{
	uint8_t count = 0;

	STRUCT_SECTION_COUNT(video_mdev, &count);

	return count;
}
