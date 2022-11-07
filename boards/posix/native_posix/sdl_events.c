/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_board_if.h"
#include "soc.h"
#include <zephyr/arch/posix/posix_trace.h>
#include <zephyr/kernel.h>

#include <SDL.h>

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

static void sdl_handle_events(void *p1, void *p2, void *p3)
{
	SDL_Event event;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
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

		k_msleep(CONFIG_SDL_THREAD_INTERVAL);
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

K_THREAD_DEFINE(sdl, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
		sdl_handle_events, NULL, NULL, NULL,
		CONFIG_SDL_THREAD_PRIORITY, K_ESSENTIAL, 0);
