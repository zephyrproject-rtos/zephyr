/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_CTRLS_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_CTRLS_H_

#include <zephyr/device.h>
#include <zephyr/sys/dlist.h>

/*  Control flags */
#define VIDEO_CTRL_FLAG_READ_ONLY  BIT(0)
#define VIDEO_CTRL_FLAG_WRITE_ONLY BIT(1)
#define VIDEO_CTRL_FLAG_VOLATILE   BIT(2)
#define VIDEO_CTRL_FLAG_INACTIVE   BIT(3)
#define VIDEO_CTRL_FLAG_UPDATE     BIT(4)

/*  Control types */
enum video_ctrl_type {
	VIDEO_CTRL_TYPE_BOOLEAN = 1,
	VIDEO_CTRL_TYPE_INTEGER = 2,
	VIDEO_CTRL_TYPE_INTEGER64 = 3,
	VIDEO_CTRL_TYPE_MENU = 4,
	VIDEO_CTRL_TYPE_INTEGER_MENU = 5,
	VIDEO_CTRL_TYPE_STRING = 6,
};

struct video_device;

struct video_ctrl {
	const struct video_device *vdev;
	uint32_t id;
	enum video_ctrl_type type;
	unsigned long flags;
	struct video_ctrl_range range;
	union {
		int32_t val;
		int64_t val64;
	};
	sys_dnode_t node;
};

int video_init_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
		    struct video_ctrl_range range);

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_CTRLS_H_ */
