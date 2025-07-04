/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_DEVICE_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_DEVICE_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/dlist.h>

struct video_device {
	const struct device *dev;
	const struct device *src_dev;
	sys_dlist_t ctrls;
};

struct video_interface {
	const struct device *dev;
	struct mpsc *io_q;
};

extern const struct rtio_iodev_api _video_iodev_api;

#define VIDEO_DEVICE_DEFINE(name, device, source)                                                  \
	static STRUCT_SECTION_ITERABLE(video_device, name) = {                                     \
		.dev = device,                                                                     \
		.src_dev = source,                                                                 \
		.ctrls = SYS_DLIST_STATIC_INIT(&name.ctrls),                                       \
	}

#define VIDEO_INTERFACE_DEFINE(name, device, io_queue)                                             \
	static STRUCT_SECTION_ITERABLE(video_interface, name) = {                                  \
		.dev = device,                                                                     \
		.io_q = io_queue,                                                                  \
	};                                                                                         \
	RTIO_IODEV_DEFINE(name##_iodev, &_video_iodev_api, &name)

struct video_device *video_find_vdev(const struct device *dev);
struct rtio_iodev *video_find_iodev(const struct device *dev);

struct video_buffer *video_get_buf_sqe(struct mpsc *io_q);

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_DEVICE_H_ */
