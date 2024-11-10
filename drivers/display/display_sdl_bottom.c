/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_sdl_bottom.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <SDL.h>
#include "nsi_tracing.h"

int sdl_display_init_bottom(uint16_t height, uint16_t width, uint16_t zoom_pct,
			    bool use_accelerator, void **window, void **renderer, void **mutex,
			    void **texture, void **read_texture)
{
	*window = SDL_CreateWindow("Zephyr Display", SDL_WINDOWPOS_UNDEFINED,
				   SDL_WINDOWPOS_UNDEFINED, width * zoom_pct / 100,
				   height * zoom_pct / 100, SDL_WINDOW_SHOWN);
	if (*window == NULL) {
		nsi_print_warning("Failed to create SDL window: %s", SDL_GetError());
		return -1;
	}

	if (use_accelerator) {
		*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
	} else {
		*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_SOFTWARE);
	}

	if (*renderer == NULL) {
		nsi_print_warning("Failed to create SDL renderer: %s",
				SDL_GetError());
		return -1;
	}

	*mutex = SDL_CreateMutex();
	if (*mutex == NULL) {
		nsi_print_warning("Failed to create SDL mutex: %s", SDL_GetError());
		return -1;
	}

	SDL_RenderSetLogicalSize(*renderer, width, height);

	*texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
				     SDL_TEXTUREACCESS_STATIC, width, height);
	if (*texture == NULL) {
		nsi_print_warning("Failed to create SDL texture: %s", SDL_GetError());
		return -1;
	}

	*read_texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
					  SDL_TEXTUREACCESS_TARGET, width, height);
	if (*read_texture == NULL) {
		nsi_print_warning("Failed to create SDL texture for read: %s", SDL_GetError());
		return -1;
	}

	SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(*renderer);
	SDL_RenderPresent(*renderer);

	return 0;
}

void sdl_display_write_bottom(const uint16_t height, const uint16_t width, const uint16_t x,
			      const uint16_t y, void *renderer, void *mutex, void *texture,
			      uint8_t *buf)
{
	SDL_Rect rect;
	int err;

	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;

	err = SDL_TryLockMutex(mutex);
	if (err) {
		nsi_print_warning("Failed to lock SDL mutex: %s", SDL_GetError());
		return;
	}

	SDL_UpdateTexture(texture, &rect, buf, 4 * rect.w);

	SDL_UnlockMutex(mutex);
}

int sdl_display_read_bottom(const uint16_t height, const uint16_t width,
			    const uint16_t x, const uint16_t y,
			    void *renderer, void *buf, uint16_t pitch,
			    void *mutex, void *texture, void *read_texture)
{
	SDL_Rect rect;
	int err;

	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;

	err = SDL_TryLockMutex(mutex);
	if (err) {
		nsi_print_warning("Failed to lock SDL mutex: %s", SDL_GetError());
		return -1;
	}

	SDL_SetRenderTarget(renderer, read_texture);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_ARGB8888, buf, width * 4);

	SDL_SetRenderTarget(renderer, NULL);

	SDL_UnlockMutex(mutex);

	return err;
}

void sdl_display_show_bottom(void *renderer, void *texture, void *mutex, bool display_on)
{
	int err = 0;

	if (display_on) {
		err = SDL_TryLockMutex(mutex);
		if (err) {
			nsi_print_warning("Failed to lock SDL mutex: %s", SDL_GetError());
			return;
		}

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		SDL_UnlockMutex(mutex);
	}
}

void sdl_display_blanking_off_bottom(void *renderer, void *texture, void *mutex)
{
	sdl_display_show_bottom(renderer, texture, mutex, true);
}

void sdl_display_blanking_on_bottom(void *renderer, void *mutex)
{
	int err;

	err = SDL_TryLockMutex(mutex);
	if (err) {
		nsi_print_warning("Failed to lock SDL mutex: %s", SDL_GetError());
		return;
	}

	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	SDL_UnlockMutex(mutex);
}

void sdl_display_cleanup_bottom(void **window, void **renderer, void **mutex, void **texture,
				void **read_texture)
{
	if (*read_texture != NULL) {
		SDL_DestroyTexture(*read_texture);
		*read_texture = NULL;
	}

	if (*texture != NULL) {
		SDL_DestroyTexture(*texture);
		*texture = NULL;
	}

	if (*mutex != NULL) {
		SDL_DestroyMutex(*mutex);
		*mutex = NULL;
	}

	if (*renderer != NULL) {
		SDL_DestroyRenderer(*renderer);
		*renderer = NULL;
	}

	if (*window != NULL) {
		SDL_DestroyWindow(*window);
		*window = NULL;
	}
}
