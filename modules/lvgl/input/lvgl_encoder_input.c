/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_lvgl_encoder_input

#include "lvgl_common_input.h"
#include "lvgl_encoder_input.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(lvgl, CONFIG_LV_Z_LOG_LEVEL);

struct lvgl_encoder_input_config {
	struct lvgl_common_input_config common_config; /* Needs to be first member */
	int rotation_input_code;
	int button_input_code;
};

static void lvgl_encoder_process_event(const struct device *dev, struct input_event *evt)
{
	struct lvgl_common_input_data *data = dev->data;
	const struct lvgl_encoder_input_config *cfg = dev->config;

	if (evt->code == cfg->rotation_input_code) {
		data->pending_event.enc_diff = evt->value;
	} else if (evt->code == cfg->button_input_code) {
		data->pending_event.state = evt->value ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
		data->pending_event.enc_diff = 0;
		data->pending_event.key = LV_KEY_ENTER;
	} else {
		LOG_DBG("Ignored input event: %u", evt->code);
		return;
	}

	if (k_msgq_put(cfg->common_config.event_msgq, &data->pending_event, K_NO_WAIT) != 0) {
		LOG_WRN("Could not put input data into queue");
	}
}

int lvgl_encoder_input_init(const struct device *dev)
{
	return lvgl_input_register_driver(LV_INDEV_TYPE_ENCODER, dev);
}

#define BUTTON_CODE(inst)   DT_INST_PROP_OR(inst, button_input_code, -1)
#define ROTATION_CODE(inst) DT_INST_PROP(inst, rotation_input_code)

#define ASSERT_PROPERTIES(inst)                                                                    \
	BUILD_ASSERT(IN_RANGE(ROTATION_CODE(inst), 0, 65536),                                      \
		     "Property rotation-input-code needs to be between 0 and 65536.");             \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(inst, button_input_code) ||                            \
			     IN_RANGE(BUTTON_CODE(inst), 0, 65536),                                \
		     "Property button-input-code needs to be between 0 and 65536.");               \
	BUILD_ASSERT(ROTATION_CODE(inst) != BUTTON_CODE(inst),                                     \
		     "Property rotation-input-code and button-input-code should not be equal.")

#define LVGL_ENCODER_INPUT_DEFINE(inst)                                                            \
	ASSERT_PROPERTIES(inst);                                                                   \
	LVGL_INPUT_DEFINE(inst, encoder, CONFIG_LV_Z_ENCODER_INPUT_MSGQ_COUNT,                     \
			  lvgl_encoder_process_event);                                             \
	static const struct lvgl_encoder_input_config lvgl_encoder_input_config_##inst = {         \
		.common_config.event_msgq = &LVGL_INPUT_EVENT_MSGQ(inst, encoder),                 \
		.rotation_input_code = ROTATION_CODE(inst),                                        \
		.button_input_code = BUTTON_CODE(inst),                                            \
	};                                                                                         \
	static struct lvgl_common_input_data lvgl_common_input_data_##inst;                        \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &lvgl_common_input_data_##inst,                    \
			      &lvgl_encoder_input_config_##inst, POST_KERNEL,                      \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(LVGL_ENCODER_INPUT_DEFINE)
