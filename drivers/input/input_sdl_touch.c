/*
 * Copyright (c) 2020 Jabil Inc.
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_input_sdl_touch

#include <stdbool.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include "input_sdl_touch_bottom.h"

LOG_MODULE_REGISTER(sdl_input, CONFIG_INPUT_LOG_LEVEL);

static void sdl_input_callback(struct sdl_input_data *data)
{
	if (data->just_released == true) {
		input_report_key(data->dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		data->just_released = false;
	}

	if (data->pressed) {
		input_report_abs(data->dev, INPUT_ABS_X, data->x, false, K_FOREVER);
		input_report_abs(data->dev, INPUT_ABS_Y, data->y, false, K_FOREVER);
		input_report_key(data->dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	}
}

static int sdl_init(const struct device *dev)
{
	struct sdl_input_data *data = dev->data;

	LOG_INF("Init '%s' device", dev->name);

	data->dev = dev;
	data->callback = sdl_input_callback;
	sdl_input_init_bottom(data);

	return 0;
}

#define INPUT_SDL_TOUCH_DEFINE(inst)                                                               \
	static struct sdl_input_data sdl_data_##inst = {                                           \
		.display_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, display)),              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sdl_init, NULL, &sdl_data_##inst, NULL, POST_KERNEL,           \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_SDL_TOUCH_DEFINE)
