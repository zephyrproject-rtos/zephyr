/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_DEVICE_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_DEVICE_H_

#include <zephyr/device.h>
#include <zephyr/sys/dlist.h>

struct video_device {
	const struct device *dev;
	const struct device *src_dev;
	sys_dlist_t ctrls;
};

#define VIDEO_DEVICE_DEFINE(name, device, source)                                                  \
	static STRUCT_SECTION_ITERABLE(video_device, name) = {                                     \
		.dev = device,                                                                     \
		.src_dev = source,                                                                 \
		.ctrls = SYS_DLIST_STATIC_INIT(&name.ctrls),                                       \
	}

struct video_device *video_find_vdev(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_DEVICE_H_ */
