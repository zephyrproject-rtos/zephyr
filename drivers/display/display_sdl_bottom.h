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

int sdl_display_init_bottom(uint16_t height, uint16_t width, uint16_t zoom_pct,
			    bool use_accelerator, void **window, void **renderer, void **mutex,
			    void **texture, void **read_texture);
void sdl_display_write_bottom(const uint16_t height, const uint16_t width, const uint16_t x,
			      const uint16_t y, void *renderer, void *mutex, void *texture,
			      uint8_t *buf, bool display_on, bool frame_incomplete);
int sdl_display_read_bottom(const uint16_t height, const uint16_t width, const uint16_t x,
			    const uint16_t y, void *renderer, void *buf, uint16_t pitch,
			    void *mutex, void *texture, void *read_texture);
void sdl_display_blanking_off_bottom(void *renderer, void *texture);
void sdl_display_blanking_on_bottom(void *renderer);
void sdl_display_cleanup_bottom(void **window, void **renderer, void **mutex, void **texture,
				void **read_texture);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_DISPLAY_DISPLAY_SDL_BOTTOM_H */
