/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_board_if.h"
#include "soc.h"
#include <zephyr/arch/posix/posix_trace.h>
#include <zephyr/kernel.h>
#include "sdl_events_bottom.h"

static void sdl_handle_events(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		int rc = sdl_handle_pending_events();

		if (rc != 0) {
			posix_exit(0);
		}

		k_msleep(CONFIG_SDL_THREAD_INTERVAL);
	}
}

static void sdl_init(void)
{
	if (sdl_init_video() != 0) {
		posix_print_error_and_exit("Error on SDL_Init (%s)\n", sdl_get_error());
	}
}

static void sdl_cleanup(void)
{
	sdl_quit();
}

NATIVE_TASK(sdl_init, PRE_BOOT_2, 1);
NATIVE_TASK(sdl_cleanup, ON_EXIT, 2);

K_THREAD_DEFINE(sdl, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
		sdl_handle_events, NULL, NULL, NULL,
		CONFIG_SDL_THREAD_PRIORITY, K_ESSENTIAL, 0);
