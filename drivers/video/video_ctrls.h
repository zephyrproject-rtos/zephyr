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
/** Control that affects other controls, e.g. the primary control of a cluster */
#define VIDEO_CTRL_FLAG_UPDATE     BIT(4)
/**
 * Control is executed immediately when set, and driver callback is invoked even if the value
 * hasn't changed
 */
#define VIDEO_CTRL_FLAG_EXECUTE_ON_WRITE BIT(5)

enum video_ctrl_type {
	/** Boolean type */
	VIDEO_CTRL_TYPE_BOOLEAN = 1,
	/** Integer type */
	VIDEO_CTRL_TYPE_INTEGER = 2,
	/** 64-bit integer type */
	VIDEO_CTRL_TYPE_INTEGER64 = 3,
	/** Menu string type, standard or driver-defined menu */
	VIDEO_CTRL_TYPE_MENU = 4,
	/** String type */
	VIDEO_CTRL_TYPE_STRING = 5,
	/** Menu integer type, standard or driver-defined menu */
	VIDEO_CTRL_TYPE_INTEGER_MENU = 6,
	/** Command-like type, triggers an action when written, without storing a value */
	VIDEO_CTRL_TYPE_BUTTON = 7,
};

struct video_device;

/**
 * @see video_control for the struct used in public API
 */
struct video_ctrl {
	/* Fields could be set directly by drivers if needed */
	uint32_t id;
	enum video_ctrl_type type;
	unsigned long flags;
	struct video_ctrl_range range;
	union {
		int32_t val;
		int64_t val64;
	};
	union {
		const char *const *menu;
		const int64_t *int_menu;
	};

	/* Fields should not be touched by drivers */
	const struct video_device *vdev;
	struct video_ctrl *cluster;
	uint8_t cluster_sz;
	bool is_auto;
	bool has_volatiles;
	sys_dnode_t node;
};

int video_init_ctrl(struct video_ctrl *ctrl, const struct device *const dev);

void video_cluster_ctrl(struct video_ctrl *ctrls, uint8_t sz);

void video_auto_cluster_ctrl(struct video_ctrl *ctrls, uint8_t sz, bool set_volatile);

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_VIDEO_CTRLS_H_ */
