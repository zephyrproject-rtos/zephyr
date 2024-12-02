/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_VIDEO_CTRLS_H_
#define ZEPHYR_INCLUDE_VIDEO_CTRLS_H_

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

/*  Control flags */
#define VIDEO_CTRL_FLAG_READ_ONLY  0x0004
#define VIDEO_CTRL_FLAG_WRITE_ONLY 0x0040
#define VIDEO_CTRL_FLAG_VOLATILE   0x0080

/*  Control types */
enum video_ctrl_type {
	VIDEO_CTRL_TYPE_INTEGER = 1,
	VIDEO_CTRL_TYPE_BOOLEAN = 2,
	VIDEO_CTRL_TYPE_MENU = 3,
	VIDEO_CTRL_TYPE_BUTTON = 4,
	VIDEO_CTRL_TYPE_INTEGER64 = 5,
	VIDEO_CTRL_TYPE_CTRL_CLASS = 6,
	VIDEO_CTRL_TYPE_STRING = 7,
	VIDEO_CTRL_TYPE_BITMASK = 8,
	VIDEO_CTRL_TYPE_INTEGER_MENU = 9,
};

struct video_ctrl_handler {
	const struct device *dev;
	sys_slist_t list;
};

struct video_ctrl {
	struct video_ctrl_handler *handler;

	uint32_t id;
	enum video_ctrl_type type;
	unsigned long flags;
	int32_t min, max, def, val;
	uint32_t step;

	sys_snode_t node;
};

void video_ctrl_handler_init(struct video_ctrl_handler *hdl, const struct device *dev);

void video_ctrl_handler_free(struct video_ctrl_handler *hdl);

void video_ctrl_handler_add(struct video_ctrl_handler *hdl, struct video_ctrl_handler *added);

void video_ctrl_handler_remove(struct video_ctrl_handler *hdl, struct video_ctrl_handler *removed);

int video_ctrl_init(struct video_ctrl *ctrl, struct video_ctrl_handler *hdl, uint32_t id,
		    int32_t min, int32_t max, uint32_t step, int32_t def);

struct video_ctrl *video_ctrl_find(struct video_ctrl_handler *hdl, uint32_t id);

#endif /* ZEPHYR_INCLUDE_VIDEO_CTRLS_H_ */
