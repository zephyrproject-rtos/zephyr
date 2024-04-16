/*
 * Copyright (c) 2022, Basalte bv
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * "Bottom" of the SDL GPIO emulator.
 * When built with the native_simulator this will be built in the runner context,
 * that is, with the host C library, and with the host include paths.
 */

#ifndef DRIVERS_GPIO_GPIO_EMUL_SDL_BOTTOM_H
#define DRIVERS_GPIO_GPIO_EMUL_SDL_BOTTOM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note: None of these are public interfaces. But internal to the SDL GPIO emulator */

#define GPIOEMULSDL_SCANCODE_UNKNOWN 0

struct gpio_sdl_data {
	void *dev;
	int (*callback)(struct gpio_sdl_data *data);
	int event_scan_code;
	bool key_down;
};

void gpio_sdl_init_bottom(struct gpio_sdl_data *data);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_GPIO_GPIO_EMUL_SDL_BOTTOM_H */
