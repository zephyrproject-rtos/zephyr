/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * "Bottom" of the SDL display driver.
 * When built with the native_simulator this will be built in the runner context,
 * that is, with the host C library, and with the host include paths.
 */
#ifndef DRIVERS_DISPLAY_DISPLAY_SDL_BOTTOM_H
#define DRIVERS_DISPLAY_DISPLAY_SDL_BOTTOM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note: None of these functions are public interfaces. But internal to the SDL display driver */

struct sdl_display_init_params {
	uint16_t height;
	uint16_t width;
	uint16_t zoom_pct;
	bool use_accelerator;
	void **window;
	const void *window_user_data;
	const char *title;
	void **renderer;
	void **mutex;
	void **texture;
	void **read_texture;
	void **background_texture;
	uint32_t transparency_grid_color1;
	uint32_t transparency_grid_color2;
	uint16_t transparency_grid_cell_size;
	void **round_disp_mask;
	uint32_t mask_color;
};

struct sdl_display_write_params {
	uint16_t height;
	uint16_t width;
	uint16_t x;
	uint16_t y;
	void *renderer;
	void *mutex;
	void *texture;
	void *background_texture;
	uint8_t *buf;
	bool display_on;
	bool frame_incomplete;
	uint32_t color_tint;
	void *round_disp_mask;
};

struct sdl_display_read_params {
	uint16_t height;
	uint16_t width;
	uint16_t x;
	uint16_t y;
	void *renderer;
	void *buf;
	uint16_t pitch;
	void *mutex;
	void *texture;
	void *read_texture;
};

struct sdl_display_blanking_off_params {
	void *renderer;
	void *texture;
	void *background_texture;
	uint32_t color_tint;
	void *round_disp_mask;
};

struct sdl_display_cleanup_params {
	void **window;
	void **renderer;
	void **mutex;
	void **texture;
	void **read_texture;
	void **background_texture;
	void **round_disp_mask;
};

int sdl_display_init_bottom(struct sdl_display_init_params *params);
void sdl_display_write_bottom(const struct sdl_display_write_params *params);
int sdl_display_read_bottom(const struct sdl_display_read_params *params);
void sdl_display_blanking_off_bottom(const struct sdl_display_blanking_off_params *params);
void sdl_display_blanking_on_bottom(void *renderer);
void sdl_display_cleanup_bottom(const struct sdl_display_cleanup_params *params);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_DISPLAY_DISPLAY_SDL_BOTTOM_H */
