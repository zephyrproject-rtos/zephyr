/*
 * Copyright (c) 2020 Jabil Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/kscan.h>
#include <logging/log.h>

#include <SDL.h>

LOG_MODULE_REGISTER(kscan, CONFIG_KSCAN_LOG_LEVEL);

struct sdl_data {
	const struct device *dev;
	kscan_callback_t callback;
	bool enabled;
};

static int sdl_filter(void *arg, SDL_Event *event)
{
	struct sdl_data *data = arg;
	uint32_t row = 0;
	uint32_t column = 0;
	bool pressed = 0;

	switch (event->type) {
	case SDL_MOUSEBUTTONDOWN: {
		pressed = 1;
		column = event->button.x;
		row = event->button.y;
	} break;
	case SDL_MOUSEBUTTONUP: {
		pressed = 0;
		column = event->button.x;
		row = event->button.y;
	} break;
	case SDL_MOUSEMOTION: {
		if (!event->motion.state)
			break;
		pressed = 1;
		column = event->button.x;
		row = event->button.y;
	} break;
	default:
		return 1;
	}

	if (data->enabled && data->callback) {
		data->callback(data->dev, row, column, pressed);
	}
	return 1;
}

static int sdl_configure(const struct device *dev, kscan_callback_t callback)
{
	struct sdl_data *data = dev->data;

	if (!callback) {
		LOG_ERR("Callback is null");
		return -EINVAL;
	}
	LOG_DBG("%s: set callback", dev->name);

	data->callback = callback;

	return 0;
}

static int sdl_enable_callback(const struct device *dev)
{
	struct sdl_data *data = dev->data;

	LOG_DBG("%s: enable cb", dev->name);
	data->enabled = true;
	return 0;
}

static int sdl_disable_callback(const struct device *dev)
{
	struct sdl_data *data = dev->data;

	LOG_DBG("%s: disable cb", dev->name);
	data->enabled = false;
	return 0;
}

static int sdl_init(const struct device *dev)
{
	struct sdl_data *data = dev->data;

	data->dev = dev;

	LOG_INF("Init '%s' device", dev->name);
	SDL_AddEventWatch(sdl_filter, data);

	return 0;
}


static const struct kscan_driver_api sdl_driver_api = {
	.config = sdl_configure,
	.enable_callback = sdl_enable_callback,
	.disable_callback = sdl_disable_callback,
};

static struct sdl_data sdl_data;

DEVICE_AND_API_INIT(sdl, CONFIG_SDL_POINTER_KSCAN_DEV_NAME, sdl_init,
		    &sdl_data, NULL,
		    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,
		    &sdl_driver_api);
