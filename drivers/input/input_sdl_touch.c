/*
 * Copyright (c) 2020 Jabil Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_input_sdl_touch

#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include <SDL.h>

LOG_MODULE_REGISTER(sdl_input, CONFIG_INPUT_LOG_LEVEL);

struct sdl_data {
	const struct device *dev;
	int x;
	int y;
	bool pressed;
};

static int sdl_filter(void *arg, SDL_Event *event)
{
	struct sdl_data *data = arg;

	switch (event->type) {
	case SDL_MOUSEBUTTONUP:
		data->pressed = false;
		input_report_key(data->dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		break;
	case SDL_MOUSEBUTTONDOWN:
		data->pressed = true;
		break;
	case SDL_MOUSEMOTION:
		data->x = event->button.x;
		data->y = event->button.y;
		break;
	default:
		return 1;
	}

	if (data->pressed) {
		input_report_abs(data->dev, INPUT_ABS_X, data->x, false, K_FOREVER);
		input_report_abs(data->dev, INPUT_ABS_Y, data->y, false, K_FOREVER);
		input_report_key(data->dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	}

	return 1;
}

static int sdl_init(const struct device *dev)
{
	struct sdl_data *data = dev->data;

	data->dev = dev;

	LOG_INF("Init '%s' device", dev->name);
	SDL_AddEventWatch(sdl_filter, data);

	return 0;
}

static struct sdl_data sdl_data_0;

DEVICE_DT_INST_DEFINE(0, sdl_init, NULL, &sdl_data_0, NULL,
		    POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);
