/*
 * Copyright (c) 2020 Jabil Inc.
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <SDL.h>
#include "input_sdl_touch_bottom.h"

static bool event_targets_display(SDL_Event *event, struct sdl_input_data *data)
{
	SDL_Window *window = NULL;
	const void *display_dev;

	if (!data->display_dev) {
		return true;
	}

	if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
		window = SDL_GetWindowFromID(event->button.windowID);
	} else if (event->type == SDL_MOUSEMOTION) {
		window = SDL_GetWindowFromID(event->motion.windowID);
	} else {
		return false;
	}

	if (!window) {
		return false;
	}

	/* Get the zephyr display associated with the window */
	display_dev = SDL_GetWindowData(window, "zephyr_display");

	return !display_dev || display_dev == data->display_dev;
}

static int sdl_filter(void *arg, SDL_Event *event)
{
	struct sdl_input_data *data = arg;

	if (!event_targets_display(event, data)) {
		return 1;
	}

	switch (event->type) {
	case SDL_MOUSEBUTTONUP:
		data->pressed = false;
		data->just_released = true;
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

	data->callback(data);

	return 1;
}

void sdl_input_init_bottom(struct sdl_input_data *data)
{
	SDL_AddEventWatch(sdl_filter, data);
}
