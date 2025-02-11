/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_lvgl_keypad_input

#include "lvgl_common_input.h"
#include "lvgl_keypad_input.h"
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(lvgl, CONFIG_LV_Z_LOG_LEVEL);

struct lvgl_keypad_input_config {
	struct lvgl_common_input_config common_config; /* Needs to be first member */
	const uint16_t *input_codes;
	const uint16_t *lvgl_codes;
	uint8_t num_codes;
};

static void lvgl_keypad_process_event(const struct device *dev, struct input_event *evt)
{
	struct lvgl_common_input_data *data = dev->data;
	const struct lvgl_keypad_input_config *cfg = dev->config;
	uint8_t i;

	for (i = 0; i < cfg->num_codes; i++) {
		if (evt->code == cfg->input_codes[i]) {
			data->pending_event.key = cfg->lvgl_codes[i];
			break;
		}
	}

	if (i == cfg->num_codes) {
		LOG_WRN("Ignored input event: %u", evt->code);
		return;
	}

	data->pending_event.state = evt->value ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
	if (k_msgq_put(cfg->common_config.event_msgq, &data->pending_event, K_NO_WAIT) != 0) {
		LOG_WRN("Could not put input data into keypad queue");
	}
}

int lvgl_keypad_input_init(const struct device *dev)
{
	return lvgl_input_register_driver(LV_INDEV_TYPE_KEYPAD, dev);
}

#define ASSERT_PROPERTIES(inst)                                                                    \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, input_codes) == DT_INST_PROP_LEN(inst, lvgl_codes),    \
		     "Property input-codes must have the same length as lvgl-codes.");

#define LVGL_KEYPAD_INPUT_DEFINE(inst)                                                             \
	ASSERT_PROPERTIES(inst);                                                                   \
	LVGL_INPUT_DEFINE(inst, keypad, CONFIG_LV_Z_KEYPAD_INPUT_MSGQ_COUNT,                       \
			  lvgl_keypad_process_event);                                              \
	static const uint16_t lvgl_keypad_input_codes_##inst[] = DT_INST_PROP(inst, input_codes);  \
	static const uint16_t lvgl_keypad_lvgl_codes_##inst[] = DT_INST_PROP(inst, lvgl_codes);    \
	static const struct lvgl_keypad_input_config lvgl_keypad_input_config_##inst = {           \
		.common_config.event_msgq = &LVGL_INPUT_EVENT_MSGQ(inst, keypad),                  \
		.input_codes = lvgl_keypad_input_codes_##inst,                                     \
		.lvgl_codes = lvgl_keypad_lvgl_codes_##inst,                                       \
		.num_codes = DT_INST_PROP_LEN(inst, input_codes),                                  \
	};                                                                                         \
	static struct lvgl_common_input_data lvgl_common_input_data_##inst;                        \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &lvgl_common_input_data_##inst,                    \
			      &lvgl_keypad_input_config_##inst, POST_KERNEL,                       \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(LVGL_KEYPAD_INPUT_DEFINE)
