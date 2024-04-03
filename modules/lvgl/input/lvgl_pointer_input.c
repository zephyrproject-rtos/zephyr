/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_lvgl_pointer_input

#include "lvgl_common_input.h"
#include "lvgl_pointer_input.h"

#include <lvgl_display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(lvgl, CONFIG_LV_Z_LOG_LEVEL);

struct lvgl_pointer_input_config {
	struct lvgl_common_input_config common_config; /* Needs to be first member */
	bool swap_xy;
	bool invert_x;
	bool invert_y;
};

struct lvgl_pointer_input_data {
	struct lvgl_common_input_data common_data;
	uint32_t point_x;
	uint32_t point_y;
};

static void lvgl_pointer_process_event(struct input_event *evt, void *user_data)
{
	const struct device *dev = user_data;
	const struct lvgl_pointer_input_config *cfg = dev->config;
	struct lvgl_pointer_input_data *data = dev->data;
	lv_display_t *disp = lv_display_get_default();
	struct lvgl_disp_data *disp_data = (struct lvgl_disp_data *)lv_display_get_user_data(disp);
	struct display_capabilities *cap = &disp_data->cap;
	lv_point_t *point = &data->common_data.pending_event.point;

	switch (evt->code) {
	case INPUT_ABS_X:
		if (cfg->swap_xy) {
			data->point_y = evt->value;
		} else {
			data->point_x = evt->value;
		}
		break;
	case INPUT_ABS_Y:
		if (cfg->swap_xy) {
			data->point_x = evt->value;
		} else {
			data->point_y = evt->value;
		}
		break;
	case INPUT_BTN_TOUCH:
		data->common_data.pending_event.state =
			evt->value ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
		break;
	}

	if (!evt->sync) {
		return;
	}

	lv_point_t tmp_point = {
		.x = data->point_x,
		.y = data->point_y,
	};

	if (cfg->invert_x) {
		if (cap->current_orientation == DISPLAY_ORIENTATION_NORMAL ||
		    cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			tmp_point.x = cap->x_resolution - tmp_point.x;
		} else {
			tmp_point.x = cap->y_resolution - tmp_point.x;
		}
	}

	if (cfg->invert_y) {
		if (cap->current_orientation == DISPLAY_ORIENTATION_NORMAL ||
		    cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			tmp_point.y = cap->y_resolution - tmp_point.y;
		} else {
			tmp_point.y = cap->x_resolution - tmp_point.y;
		}
	}

	/* rotate touch point to match display rotation */
	switch (cap->current_orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		point->x = tmp_point.x;
		point->y = tmp_point.y;
		break;
	case DISPLAY_ORIENTATION_ROTATED_90:
		point->x = tmp_point.y;
		point->y = cap->y_resolution - tmp_point.x;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		point->x = cap->x_resolution - tmp_point.x;
		point->y = cap->y_resolution - tmp_point.y;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		point->x = cap->x_resolution - tmp_point.y;
		point->y = tmp_point.x;
		break;
	default:
		LOG_ERR("Invalid display orientation");
		break;
	}

	/* filter readings within display */
	if (point->x <= 0) {
		point->x = 0;
	} else if (point->x >= cap->x_resolution) {
		point->x = cap->x_resolution - 1;
	}

	if (point->y <= 0) {
		point->y = 0;
	} else if (point->y >= cap->y_resolution) {
		point->y = cap->y_resolution - 1;
	}

	if (k_msgq_put(cfg->common_config.event_msgq, &data->common_data.pending_event,
		       K_NO_WAIT) != 0) {
		LOG_WRN("Could not put input data into queue");
	}
}

int lvgl_pointer_input_init(const struct device *dev)
{
	return lvgl_input_register_driver(LV_INDEV_TYPE_POINTER, dev);
}

#define LVGL_POINTER_INPUT_DEFINE(inst)                                                            \
	LVGL_INPUT_DEFINE(inst, pointer, CONFIG_LV_Z_POINTER_INPUT_MSGQ_COUNT,                     \
			  lvgl_pointer_process_event);                                             \
	static const struct lvgl_pointer_input_config lvgl_pointer_input_config_##inst = {         \
		.common_config.event_msgq = &LVGL_INPUT_EVENT_MSGQ(inst, pointer),                 \
		.swap_xy = DT_INST_PROP(inst, swap_xy),                                            \
		.invert_x = DT_INST_PROP(inst, invert_x),                                          \
		.invert_y = DT_INST_PROP(inst, invert_y),                                          \
	};                                                                                         \
	static struct lvgl_pointer_input_data lvgl_pointer_input_data_##inst;                      \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &lvgl_pointer_input_data_##inst,                   \
			      &lvgl_pointer_input_config_##inst, POST_KERNEL,                      \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(LVGL_POINTER_INPUT_DEFINE)
