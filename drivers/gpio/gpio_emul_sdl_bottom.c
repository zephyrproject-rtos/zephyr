/*
 * Copyright (c) 2022, Basalte bv
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <SDL.h>
#include "gpio_emul_sdl_bottom.h"

static int sdl_filter_bottom(void *arg, SDL_Event *event)
{
	struct gpio_sdl_data *data = arg;

	/* Only handle keyboard events */
	switch (event->type) {
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		break;
	default:
		return 1;
	}

	data->event_scan_code = event->key.keysym.scancode;
	data->key_down = event->type == SDL_KEYDOWN;

	return data->callback(arg);
}

void gpio_sdl_init_bottom(struct gpio_sdl_data *data)
{
	SDL_AddEventWatch(sdl_filter_bottom, (void *)data);
}
