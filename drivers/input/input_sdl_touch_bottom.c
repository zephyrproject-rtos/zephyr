/*
 * Copyright (c) 2020 Jabil Inc.
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <SDL.h>
#include "input_sdl_touch_bottom.h"

static int sdl_filter(void *arg, SDL_Event *event)
{
	struct sdl_input_data *data = arg;

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
