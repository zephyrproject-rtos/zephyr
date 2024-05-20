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

static void lvgl_pointer_process_event(const struct device *dev, struct input_event *evt)
{
	const struct lvgl_pointer_input_config *cfg = dev->config;
	struct lvgl_pointer_input_data *data = dev->data;
	lv_disp_t *disp = lv_disp_get_default();
	struct lvgl_disp_data *disp_data = disp->driver->user_data;
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
			evt->value ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
		break;
	}

	if (!evt->sync) {
		return;
	}

	point->x = data->point_x;
	point->y = data->point_y;

	if (cfg->invert_x) {
		if (cap->current_orientation == DISPLAY_ORIENTATION_NORMAL ||
		    cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			point->x = cap->x_resolution - data->point_x;
		} else {
			point->x = cap->y_resolution - data->point_x;
		}
	}

	if (cfg->invert_y) {
		if (cap->current_orientation == DISPLAY_ORIENTATION_NORMAL ||
		    cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			point->y = cap->y_resolution - data->point_y;
		} else {
			point->y = cap->x_resolution - data->point_y;
		}
	}

	/* rotate touch point to match display rotation */
	if (cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		point->x = data->point_y;
		point->y = cap->y_resolution - data->point_x;
	} else if (cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		point->x = cap->x_resolution - data->point_x;
		point->y = cap->y_resolution - data->point_y;
	} else if (cap->current_orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		point->x = cap->x_resolution - data->point_y;
		point->y = data->point_x;
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
