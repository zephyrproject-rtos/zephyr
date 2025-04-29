/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_CTRLS_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_CTRLS_H_

#include <zephyr/device.h>
#include <zephyr/sys/dlist.h>

/** Control is read-only */
#define VIDEO_CTRL_FLAG_READ_ONLY  BIT(0)
/** Control is write-only */
#define VIDEO_CTRL_FLAG_WRITE_ONLY BIT(1)
/** Control that needs a freshly read as constanly updated by HW */
#define VIDEO_CTRL_FLAG_VOLATILE   BIT(2)
/** Control is inactive, e.g. manual controls of an autocluster in automatic mode */
#define VIDEO_CTRL_FLAG_INACTIVE   BIT(3)
/** Control that affects other controls, e.g. the master control of a cluster */
#define VIDEO_CTRL_FLAG_UPDATE     BIT(4)

enum video_ctrl_type {
	/** Boolean type */
	VIDEO_CTRL_TYPE_BOOLEAN = 1,
	/** Integer type */
	VIDEO_CTRL_TYPE_INTEGER = 2,
	/** 64-bit integer type */
	VIDEO_CTRL_TYPE_INTEGER64 = 3,
	/** Menu type, standard or driver-defined menu */
	VIDEO_CTRL_TYPE_MENU = 4,
	/** String type */
	VIDEO_CTRL_TYPE_STRING = 5,
};

struct video_device;

/**
 * @see video_control for the struct used in public API
 */
struct video_ctrl {
	/* Fields should not touched by drivers, used only for the 1st control of a cluster */
	struct video_ctrl *cluster;
	uint8_t cluster_sz;
	bool is_auto;
	bool has_volatiles;

	const struct video_device *vdev;
	uint32_t id;
	enum video_ctrl_type type;
	unsigned long flags;
	struct video_ctrl_range range;
	union {
		int32_t val;
		int64_t val64;
	};
	const char *const *menu;
	sys_dnode_t node;
};

int video_init_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
		    struct video_ctrl_range range);

int video_init_menu_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
			 uint8_t def, const char *const menu[]);

void video_cluster_ctrl(struct video_ctrl *ctrls, uint8_t sz);

void video_auto_cluster_ctrl(struct video_ctrl *ctrls, uint8_t sz, bool set_volatile);

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_CTRLS_H_ */
