/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * "Bottom" of the SDL input driver.
 * When built with the native_simulator this will be built in the runner context,
 * that is, with the host C library, and with the host include paths.
 */

#ifndef DRIVERS_INPUT_INPUT_SDL_TOUCH_BOTTOM_H
#define DRIVERS_INPUT_INPUT_SDL_TOUCH_BOTTOM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note: None of these are public interfaces. But internal to the SDL input driver */

struct sdl_input_data {
	const void *dev; /* device structure pointer */
	void (*callback)(struct sdl_input_data *data);
	int x;
	int y;
	bool pressed;
	bool just_released;
};

void sdl_input_init_bottom(struct sdl_input_data *data);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INPUT_INPUT_SDL_TOUCH_BOTTOM_H */
