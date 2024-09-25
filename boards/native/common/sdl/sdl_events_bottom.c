/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * "Bottom" of the SDL event handler for the POSIX architecture.
 * When built with the native_simulator this will be built in the runner context,
 * that is, with the host C library, and with the host include paths.
 *
 * Therefore it cannot include Zephyr headers
 */

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

/*
 * Handle all pending display events
 * Return 1 if the window was closed, 0 otherwise.
 */
int sdl_handle_pending_events(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_WINDOWEVENT:
			sdl_handle_window_event(&event);
			break;
		case SDL_QUIT:
			return 1;
		default:
			break;
		}
	}
	return 0;
}

/*
 * Initialize the SDL library
 *
 * Returns 0 on success, something else on failure.
 */
int sdl_init_video(void)
{
	return SDL_Init(SDL_INIT_VIDEO);
}

/*
 * Trampoline to SDL_GetError
 */
const char *sdl_get_error(void)
{
	return SDL_GetError();
}

/*
 * Trampoline to SDL_Quit()
 */
void sdl_quit(void)
{
	SDL_Quit();
}
