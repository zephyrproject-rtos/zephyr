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
			    bool use_accelerator, void **window, const void *window_user_data,
			    const char *title, void **renderer, void **mutex, void **texture,
			    void **read_texture, void **background_texture,
			    uint32_t transparency_grid_color1, uint32_t transparency_grid_color2,
			    uint16_t transparency_grid_cell_size)
{
	/* clang-format off */
	*window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				   width * zoom_pct / 100,
				   height * zoom_pct / 100, SDL_WINDOW_SHOWN);
	/* clang-format on */
	if (*window == NULL) {
		nsi_print_warning("Failed to create SDL window %s: %s", title, SDL_GetError());
		return -1;
	}
	SDL_SetWindowData(*window, "zephyr_display", (void *)window_user_data);

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
	SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);

	*read_texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
					  SDL_TEXTUREACCESS_TARGET, width, height);
	if (*read_texture == NULL) {
		nsi_print_warning("Failed to create SDL texture for read: %s", SDL_GetError());
		return -1;
	}

	*background_texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
						SDL_TEXTUREACCESS_STREAMING, width, height);
	if (*background_texture == NULL) {
		nsi_print_warning("Failed to create SDL texture: %s", SDL_GetError());
		return -1;
	}

	void *background_data;
	int background_pitch;
	int err;

	err = SDL_LockTexture(*background_texture, NULL, &background_data, &background_pitch);
	if (err != 0) {
		nsi_print_warning("Failed to lock background texture: %d", err);
		return -1;
	}
	for (int y = 0; y < height; y++) {
		uint32_t *row = (uint32_t *)((uint8_t *)background_data + background_pitch * y);

		for (int x = 0; x < width; x++) {
			bool x_cell_even = ((x / transparency_grid_cell_size) % 2) == 0;
			bool y_cell_even = ((y / transparency_grid_cell_size) % 2) == 0;

			if (x_cell_even == y_cell_even) {
				row[x] = transparency_grid_color1 | 0xff000000;
			} else {
				row[x] = transparency_grid_color2 | 0xff000000;
			}
		}
	}
	SDL_UnlockTexture(*background_texture);

	SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(*renderer);
	SDL_RenderCopy(*renderer, *background_texture, NULL, NULL);
	SDL_RenderPresent(*renderer);

	return 0;
}

void sdl_display_write_bottom(const uint16_t height, const uint16_t width, const uint16_t x,
			      const uint16_t y, void *renderer, void *mutex, void *texture,
			      void *background_texture, uint8_t *buf, bool display_on,
			      bool frame_incomplete, uint32_t color_tint)
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

	if (display_on && !frame_incomplete) {
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, background_texture, NULL, NULL);
		SDL_SetTextureColorMod(texture,
				       (color_tint >> 16) & 0xff,
				       (color_tint >> 8) & 0xff,
				       color_tint & 0xff);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_SetTextureColorMod(texture, 255, 255, 255);
		SDL_RenderPresent(renderer);
	}

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
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_ARGB8888, buf, width * 4);

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(renderer, NULL);

	SDL_UnlockMutex(mutex);

	return err;
}

void sdl_display_blanking_off_bottom(void *renderer, void *texture, void *background_texture,
				     uint32_t color_tint)
{
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, background_texture, NULL, NULL);
	SDL_SetTextureColorMod(texture,
			       (color_tint >> 16) & 0xff,
			       (color_tint >> 8) & 0xff,
			       color_tint & 0xff);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_SetTextureColorMod(texture, 255, 255, 255);
	SDL_RenderPresent(renderer);
}

void sdl_display_blanking_on_bottom(void *renderer)
{
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void sdl_display_cleanup_bottom(void **window, void **renderer, void **mutex, void **texture,
				void **read_texture, void **background_texture)
{
	if (*background_texture != NULL) {
		SDL_DestroyTexture(*background_texture);
		*background_texture = NULL;
	}

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
