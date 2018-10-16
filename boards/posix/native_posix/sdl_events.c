/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <SDL.h>
#include "posix_board_if.h"
#include "posix_trace.h"
#include "posix_arch_internal.h"
#include "soc.h"
#include "hw_models_top.h"

u64_t sdl_event_timer;

static void sdl_handle_window_event(const SDL_Event *event)
{
	SDL_Window *window;
	SDL_Renderer *renderer;

	switch (event->window.event) {
	case SDL_WINDOWEVENT_EXPOSED:

		window = SDL_GetWindowFromID(event->window.windowID);
		if (window == NULL) {
			return;
		}

		renderer = SDL_GetRenderer(window);
		if (renderer == NULL) {
			return;
		}
		SDL_RenderPresent(renderer);
		break;
	default:
		break;
	}
}

void sdl_handle_events(void)
{
	SDL_Event event;

	sdl_event_timer = hwm_get_time() + 10000;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_WINDOWEVENT:
			sdl_handle_window_event(&event);
			break;
		case SDL_QUIT:
			posix_exit(0);
			break;
		default:
			break;
		}
	}

}

static void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		posix_print_error_and_exit("Error on SDL_Init (%s)\n",
				SDL_GetError());
	}
}

static void sdl_cleanup(void)
{
	SDL_Quit();
}

NATIVE_TASK(sdl_init, PRE_BOOT_2, 1);
NATIVE_TASK(sdl_cleanup, ON_EXIT, 2);
