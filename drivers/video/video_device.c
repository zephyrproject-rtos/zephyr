/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>

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

struct rtio_iodev *video_find_iodev(const struct device *dev)
{
	struct video_interface *vi;

	if (!dev) {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(rtio_iodev, ri) {
		vi = ri->data;
		if (vi->dev == dev) {
			return ri;
		}
	}

	return NULL;
}

static void video_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct video_interface *vi = iodev_sqe->sqe.iodev->data;

	mpsc_push(vi->io_q, &iodev_sqe->q);
}

const struct rtio_iodev_api _video_iodev_api = {
	.submit = video_iodev_submit,
};
