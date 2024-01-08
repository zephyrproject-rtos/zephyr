/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include "lvgl_display.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lvgl, CONFIG_LV_Z_LOG_LEVEL);

static lv_indev_drv_t indev_drv;
#define KSCAN_NODE DT_CHOSEN(zephyr_keyboard_scan)

K_MSGQ_DEFINE(kscan_msgq, sizeof(lv_indev_data_t), CONFIG_LV_Z_POINTER_KSCAN_MSGQ_COUNT, 4);

static void lvgl_pointer_kscan_callback(const struct device *dev, uint32_t row, uint32_t col,
					bool pressed)
{
	lv_indev_data_t data = {
		.point.x = col,
		.point.y = row,
		.state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL,
	};

	if (k_msgq_put(&kscan_msgq, &data, K_NO_WAIT) != 0) {
		LOG_DBG("Could not put input data into queue");
	}
}

static void lvgl_pointer_kscan_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	lv_disp_t *disp;
	struct lvgl_disp_data *disp_data;
	struct display_capabilities *cap;
	lv_indev_data_t curr;

	static lv_indev_data_t prev = {
		.point.x = 0,
		.point.y = 0,
		.state = LV_INDEV_STATE_REL,
	};

	if (k_msgq_get(&kscan_msgq, &curr, K_NO_WAIT) != 0) {
		goto set_and_release;
	}

	prev = curr;

	disp = lv_disp_get_default();
	disp_data = disp->driver->user_data;
	cap = &disp_data->cap;

	/* adjust kscan coordinates */
	if (IS_ENABLED(CONFIG_LV_Z_POINTER_KSCAN_SWAP_XY)) {
		lv_coord_t x;

		x = prev.point.x;
		prev.point.x = prev.point.y;
		prev.point.y = x;
	}

	if (IS_ENABLED(CONFIG_LV_Z_POINTER_KSCAN_INVERT_X)) {
		if (cap->current_orientation == DISPLAY_ORIENTATION_NORMAL ||
		    cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			prev.point.x = cap->x_resolution - prev.point.x;
		} else {
			prev.point.x = cap->y_resolution - prev.point.x;
		}
	}

	if (IS_ENABLED(CONFIG_LV_Z_POINTER_KSCAN_INVERT_Y)) {
		if (cap->current_orientation == DISPLAY_ORIENTATION_NORMAL ||
		    cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			prev.point.y = cap->y_resolution - prev.point.y;
		} else {
			prev.point.y = cap->x_resolution - prev.point.y;
		}
	}

	/* rotate touch point to match display rotation */
	if (cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		lv_coord_t x;

		x = prev.point.x;
		prev.point.x = prev.point.y;
		prev.point.y = cap->y_resolution - x;
	} else if (cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		prev.point.x = cap->x_resolution - prev.point.x;
		prev.point.y = cap->y_resolution - prev.point.y;
	} else if (cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		lv_coord_t x;

		x = prev.point.x;
		prev.point.x = cap->x_resolution - prev.point.y;
		prev.point.y = x;
	}

	/* filter readings within display */
	if (prev.point.x <= 0) {
		prev.point.x = 0;
	} else if (prev.point.x >= cap->x_resolution) {
		prev.point.x = cap->x_resolution - 1;
	}

	if (prev.point.y <= 0) {
		prev.point.y = 0;
	} else if (prev.point.y >= cap->y_resolution) {
		prev.point.y = cap->y_resolution - 1;
	}

set_and_release:
	*data = prev;

	data->continue_reading = k_msgq_num_used_get(&kscan_msgq) > 0;
}

static int lvgl_kscan_pointer_init(void)
{
	const struct device *kscan_dev = DEVICE_DT_GET(KSCAN_NODE);

	if (!device_is_ready(kscan_dev)) {
		LOG_ERR("Keyboard scan device not ready.");
		return -ENODEV;
	}

	if (kscan_config(kscan_dev, lvgl_pointer_kscan_callback) < 0) {
		LOG_ERR("Could not configure keyboard scan device.");
		return -ENODEV;
	}

	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = lvgl_pointer_kscan_read;

	if (lv_indev_drv_register(&indev_drv) == NULL) {
		LOG_ERR("Failed to register input device.");
		return -EPERM;
	}

	kscan_enable_callback(kscan_dev);

	return 0;
}

SYS_INIT(lvgl_kscan_pointer_init, APPLICATION, CONFIG_INPUT_INIT_PRIORITY);
