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

static int sdl_create_rounded_display_mask(uint16_t width, uint16_t height, uint32_t mask_color,
					   void **round_disp_mask, void *renderer)
{
	*round_disp_mask = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
					     SDL_TEXTUREACCESS_STREAMING, width, height);
	if (*round_disp_mask == NULL) {
		nsi_print_warning("Failed to create SDL mask texture: %s", SDL_GetError());
		return -1;
	}
	SDL_SetTextureBlendMode(*round_disp_mask, SDL_BLENDMODE_BLEND);

	void *mask_data;
	int mask_pitch;
	int err;

	err = SDL_LockTexture(*round_disp_mask, NULL, &mask_data, &mask_pitch);
	if (err != 0) {
		nsi_print_warning("Failed to lock mask texture: %d", err);
		return -1;
	}

	/* Create ellipse mask */
	float cx = width / 2.0f;
	float cy = height / 2.0f;
	float rx = width / 2.0f;
	float ry = height / 2.0f;

	for (int py = 0; py < height; py++) {
		uint32_t *row = (uint32_t *)((uint8_t *)mask_data + mask_pitch * py);

		for (int px = 0; px < width; px++) {
			/* Calculate normalized distance from center */
			float dx = (px - cx) / rx;
			float dy = (py - cy) / ry;
			float distance = dx * dx + dy * dy;

			/* Inside ellipse: transparent, outside: mask color with full opacity */
			if (distance <= 1.0f) {
				row[px] = 0x00000000; /* Transparent */
			} else {
				uint32_t r = (mask_color >> 16) & 0xff;
				uint32_t g = (mask_color >> 8) & 0xff;
				uint32_t b = mask_color & 0xff;

				row[px] = (0xFF << 24) | (r << 16) | (g << 8) | b;
			}
		}
	}
	SDL_UnlockTexture(*round_disp_mask);

	return 0;
}

int sdl_display_init_bottom(struct sdl_display_init_params *params)
{
	/* clang-format off */
	*params->window = SDL_CreateWindow(params->title, SDL_WINDOWPOS_UNDEFINED,
				   SDL_WINDOWPOS_UNDEFINED,
				   params->width * params->zoom_pct / 100,
				   params->height * params->zoom_pct / 100, SDL_WINDOW_SHOWN);
	/* clang-format on */
	if (*params->window == NULL) {
		nsi_print_warning("Failed to create SDL window %s: %s", params->title,
				  SDL_GetError());
		return -1;
	}
	SDL_SetWindowData(*params->window, "zephyr_display", (void *)params->window_user_data);

	if (params->use_accelerator) {
		*params->renderer =
			SDL_CreateRenderer(*params->window, -1, SDL_RENDERER_ACCELERATED);
	} else {
		*params->renderer = SDL_CreateRenderer(*params->window, -1, SDL_RENDERER_SOFTWARE);
	}

	if (*params->renderer == NULL) {
		nsi_print_warning("Failed to create SDL renderer: %s",
				SDL_GetError());
		return -1;
	}

	*params->mutex = SDL_CreateMutex();
	if (*params->mutex == NULL) {
		nsi_print_warning("Failed to create SDL mutex: %s", SDL_GetError());
		return -1;
	}

	SDL_RenderSetLogicalSize(*params->renderer, params->width, params->height);

	*params->texture =
		SDL_CreateTexture(*params->renderer, SDL_PIXELFORMAT_ARGB8888,
				  SDL_TEXTUREACCESS_STATIC, params->width, params->height);
	if (*params->texture == NULL) {
		nsi_print_warning("Failed to create SDL texture: %s", SDL_GetError());
		return -1;
	}
	SDL_SetTextureBlendMode(*params->texture, SDL_BLENDMODE_BLEND);

	*params->read_texture =
		SDL_CreateTexture(*params->renderer, SDL_PIXELFORMAT_ARGB8888,
				  SDL_TEXTUREACCESS_TARGET, params->width, params->height);
	if (*params->read_texture == NULL) {
		nsi_print_warning("Failed to create SDL texture for read: %s", SDL_GetError());
		return -1;
	}

	*params->background_texture =
		SDL_CreateTexture(*params->renderer, SDL_PIXELFORMAT_ARGB8888,
				  SDL_TEXTUREACCESS_STREAMING, params->width, params->height);
	if (*params->background_texture == NULL) {
		nsi_print_warning("Failed to create SDL texture: %s", SDL_GetError());
		return -1;
	}

	void *background_data;
	int background_pitch;
	int err;

	err = SDL_LockTexture(*params->background_texture, NULL, &background_data,
			      &background_pitch);
	if (err != 0) {
		nsi_print_warning("Failed to lock background texture: %d", err);
		return -1;
	}
	for (int y = 0; y < params->height; y++) {
		uint32_t *row = (uint32_t *)((uint8_t *)background_data + background_pitch * y);

		for (int x = 0; x < params->width; x++) {
			bool x_cell_even = ((x / params->transparency_grid_cell_size) % 2) == 0;
			bool y_cell_even = ((y / params->transparency_grid_cell_size) % 2) == 0;

			if (x_cell_even == y_cell_even) {
				row[x] = params->transparency_grid_color1 | 0xff000000;
			} else {
				row[x] = params->transparency_grid_color2 | 0xff000000;
			}
		}
	}
	SDL_UnlockTexture(*params->background_texture);

	/* Create ellipse mask texture if rounded mask is enabled */
	if (params->round_disp_mask != NULL) {
		err = sdl_create_rounded_display_mask(params->width, params->height,
						      params->mask_color, params->round_disp_mask,
						      *params->renderer);
		if (err != 0) {
			nsi_print_warning("Failed to create rounded display mask");
			return -1;
		}
	}

	SDL_SetRenderDrawColor(*params->renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(*params->renderer);
	SDL_RenderCopy(*params->renderer, *params->background_texture, NULL, NULL);
	SDL_RenderPresent(*params->renderer);

	return 0;
}

void sdl_display_write_bottom(const struct sdl_display_write_params *params)
{
	SDL_Rect rect;
	int err;

	rect.x = params->x;
	rect.y = params->y;
	rect.w = params->width;
	rect.h = params->height;

	err = SDL_TryLockMutex(params->mutex);
	if (err) {
		nsi_print_warning("Failed to lock SDL mutex: %s", SDL_GetError());
		return;
	}

	SDL_UpdateTexture(params->texture, &rect, params->buf, 4 * rect.w);

	if (params->display_on && !params->frame_incomplete) {
		SDL_RenderClear(params->renderer);
		SDL_RenderCopy(params->renderer, params->background_texture, NULL, NULL);
		SDL_SetTextureColorMod(params->texture,
				       (params->color_tint >> 16) & 0xff,
				       (params->color_tint >> 8) & 0xff,
				       params->color_tint & 0xff);
		SDL_RenderCopy(params->renderer, params->texture, NULL, NULL);
		SDL_SetTextureColorMod(params->texture, 255, 255, 255);

		/* Apply ellipse mask if enabled */
		if (params->round_disp_mask != NULL) {
			SDL_SetRenderDrawBlendMode(params->renderer, SDL_BLENDMODE_MOD);
			SDL_RenderCopy(params->renderer, params->round_disp_mask, NULL, NULL);
			SDL_SetRenderDrawBlendMode(params->renderer, SDL_BLENDMODE_BLEND);
		}

		SDL_RenderPresent(params->renderer);
	}

	SDL_UnlockMutex(params->mutex);
}

int sdl_display_read_bottom(const struct sdl_display_read_params *params)
{
	SDL_Rect rect;
	int err;

	rect.x = params->x;
	rect.y = params->y;
	rect.w = params->width;
	rect.h = params->height;

	err = SDL_TryLockMutex(params->mutex);
	if (err) {
		nsi_print_warning("Failed to lock SDL mutex: %s", SDL_GetError());
		return -1;
	}

	SDL_SetRenderTarget(params->renderer, params->read_texture);
	SDL_SetTextureBlendMode(params->texture, SDL_BLENDMODE_NONE);

	SDL_RenderClear(params->renderer);
	SDL_RenderCopy(params->renderer, params->texture, NULL, NULL);
	SDL_RenderReadPixels(params->renderer, &rect, SDL_PIXELFORMAT_ARGB8888, params->buf,
			     params->width * 4);

	SDL_SetTextureBlendMode(params->texture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(params->renderer, NULL);

	SDL_UnlockMutex(params->mutex);

	return err;
}

void sdl_display_blanking_off_bottom(const struct sdl_display_blanking_off_params *params)
{
	SDL_RenderClear(params->renderer);
	SDL_RenderCopy(params->renderer, params->background_texture, NULL, NULL);
	SDL_SetTextureColorMod(params->texture, (params->color_tint >> 16) & 0xff,
			       (params->color_tint >> 8) & 0xff, params->color_tint & 0xff);
	SDL_RenderCopy(params->renderer, params->texture, NULL, NULL);
	SDL_SetTextureColorMod(params->texture, 255, 255, 255);

	/* Apply ellipse mask if enabled */
	if (params->round_disp_mask != NULL) {
		SDL_SetRenderDrawBlendMode(params->renderer, SDL_BLENDMODE_MOD);
		SDL_RenderCopy(params->renderer, params->round_disp_mask, NULL, NULL);
		SDL_SetRenderDrawBlendMode(params->renderer, SDL_BLENDMODE_BLEND);
	}

	SDL_RenderPresent(params->renderer);
}

void sdl_display_blanking_on_bottom(void *renderer)
{
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void sdl_display_cleanup_bottom(const struct sdl_display_cleanup_params *params)
{
	if (*params->round_disp_mask != NULL) {
		SDL_DestroyTexture(*params->round_disp_mask);
		*params->round_disp_mask = NULL;
	}

	if (*params->background_texture != NULL) {
		SDL_DestroyTexture(*params->background_texture);
		*params->background_texture = NULL;
	}

	if (*params->read_texture != NULL) {
		SDL_DestroyTexture(*params->read_texture);
		*params->read_texture = NULL;
	}

	if (*params->texture != NULL) {
		SDL_DestroyTexture(*params->texture);
		*params->texture = NULL;
	}

	if (*params->mutex != NULL) {
		SDL_DestroyMutex(*params->mutex);
		*params->mutex = NULL;
	}

	if (*params->renderer != NULL) {
		SDL_DestroyRenderer(*params->renderer);
		*params->renderer = NULL;
	}

	if (*params->window != NULL) {
		SDL_DestroyWindow(*params->window);
		*params->window = NULL;
	}
}
