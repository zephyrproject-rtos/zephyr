/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_lvgl_button_input

#include "lvgl_common_input.h"
#include "lvgl_button_input.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(lvgl, CONFIG_LV_Z_LOG_LEVEL);

struct lvgl_button_input_config {
	struct lvgl_common_input_config common_config; /* Needs to be first member */
	const uint16_t *input_codes;
	uint8_t num_codes;
	const int32_t *coordinates;
};

static void lvgl_button_process_event(struct input_event *evt, void *user_data)
{
	const struct device *dev = user_data;
	struct lvgl_common_input_data *data = dev->data;
	const struct lvgl_button_input_config *cfg = dev->config;
	uint8_t i;

	for (i = 0; i < cfg->num_codes; i++) {
		if (evt->code == cfg->input_codes[i]) {
			break;
		}
	}

	if (i == cfg->num_codes) {
		LOG_DBG("Ignored input event: %u", evt->code);
		return;
	}

	data->pending_event.btn_id = i;
	data->pending_event.state = evt->value ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

	if (k_msgq_put(cfg->common_config.event_msgq, &data->pending_event, K_NO_WAIT) != 0) {
		LOG_WRN("Could not put input data into queue");
	}
}

int lvgl_button_input_init(const struct device *dev)
{
	struct lvgl_common_input_data *data = dev->data;
	const struct lvgl_button_input_config *cfg = dev->config;
	int ret;

	ret = lvgl_input_register_driver(LV_INDEV_TYPE_BUTTON, dev);
	if (ret < 0) {
		return ret;
	}

	lv_indev_set_button_points(data->indev, (const lv_point_t *)cfg->coordinates);

	return ret;
}

#define ASSERT_PROPERTIES(inst)                                                                    \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, input_codes) * 2 ==                                    \
			     DT_INST_PROP_LEN(inst, coordinates),                                  \
		     "Property coordinates must contain one coordinate per input-code.")

#define LVGL_BUTTON_INPUT_DEFINE(inst)                                                             \
	ASSERT_PROPERTIES(inst);                                                                   \
	LVGL_INPUT_INST_DEFINE(inst, button, CONFIG_LV_Z_BUTTON_INPUT_MSGQ_COUNT,                  \
			       lvgl_button_process_event);                                         \
	static const uint16_t lvgl_button_input_codes_##inst[] = DT_INST_PROP(inst, input_codes);  \
	static const int32_t lvgl_button_coordinates_##inst[] = DT_INST_PROP(inst, coordinates);   \
	static const struct lvgl_button_input_config lvgl_button_input_config_##inst = {           \
		.common_config.event_msgq = &LVGL_INPUT_EVENT_MSGQ(inst, button),                  \
		.input_codes = lvgl_button_input_codes_##inst,                                     \
		.num_codes = DT_INST_PROP_LEN(inst, input_codes),                                  \
		.coordinates = lvgl_button_coordinates_##inst,                                     \
	};                                                                                         \
	static struct lvgl_common_input_data lvgl_common_input_data_##inst;                        \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &lvgl_common_input_data_##inst,                    \
			      &lvgl_button_input_config_##inst, POST_KERNEL,                       \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(LVGL_BUTTON_INPUT_DEFINE)
