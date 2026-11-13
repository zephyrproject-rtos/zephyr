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
	void **compose_texture;
	/* One texture per plane; entry 0 is the one *texture names */
	void **plane_textures;
	uint8_t num_planes;
	uint32_t transparency_grid_color1;
	uint32_t transparency_grid_color2;
	uint16_t transparency_grid_cell_size;
	void **round_disp_mask;
	uint32_t mask_color;
	double angle;
};

/* One plane as the renderer sees it */
struct sdl_display_plane_params {
	void *texture;
	/* Packed BGRA32 pixels, width x height, uploaded when update is set */
	const uint8_t *buf;
	bool update;
	bool enabled;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	/* Blend with the pixel alpha; otherwise the pixels are copied opaque */
	bool blend;
	uint8_t alpha;
};

/* Everything a full redraw of the window needs */
struct sdl_display_render_params {
	void *renderer;
	void *mutex;
	void *background_texture;
	void *compose_texture;
	struct sdl_display_plane_params *planes;
	uint8_t num_planes;
	uint32_t color_tint;
	void *round_disp_mask;
	double angle;
};

struct sdl_display_write_params {
	uint16_t height;
	uint16_t width;
	uint16_t x;
	uint16_t y;
	void *texture;
	uint8_t *buf;
	bool display_on;
	bool frame_incomplete;
	const struct sdl_display_render_params *render;
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
	double angle;
};

struct sdl_display_cleanup_params {
	void **window;
	void **renderer;
	void **mutex;
	void **texture;
	void **read_texture;
	void **background_texture;
	void **compose_texture;
	void **plane_textures;
	uint8_t num_planes;
	void **round_disp_mask;
};

int sdl_display_init_bottom(struct sdl_display_init_params *params);
void sdl_display_write_bottom(const struct sdl_display_write_params *params);
int sdl_display_read_bottom(const struct sdl_display_read_params *params);
void sdl_display_commit_bottom(const struct sdl_display_render_params *params, bool display_on);
void sdl_display_blanking_off_bottom(const struct sdl_display_render_params *params);
void sdl_display_blanking_on_bottom(void *renderer);
void sdl_display_cleanup_bottom(const struct sdl_display_cleanup_params *params);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_DISPLAY_DISPLAY_SDL_BOTTOM_H */
