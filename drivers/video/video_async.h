/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_ASYNC_H_
#define ZEPHYR_INCLUDE_VIDEO_ASYNC_H_

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

struct video_device;

struct video_ctrl_handler;

struct video_notifier {
	struct video_device *vdev;
	const struct device *dev;
	struct video_ctrl_handler *ctrl_handler;
	struct video_notifier *parent;
	const struct device **children_devs;
	uint8_t children_num;
	sys_snode_t node;
};

void video_async_init(struct video_notifier *notifier, const struct device *dev,
		      struct video_device *vdev, struct video_ctrl_handler *hdl,
		      const struct device **children_devs, uint8_t children_num);

int video_async_register(struct video_notifier *notifier);

void video_async_unregister(struct video_notifier *notifier);

struct video_notifier *video_async_find_notifier(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_VIDEO_ASYNC_H_ */
