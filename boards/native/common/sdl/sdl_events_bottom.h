/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARDS_POSIX_COMMON_SDL_SDL_EVENTS_BOTTOM_H
#define BOARDS_POSIX_COMMON_SDL_SDL_EVENTS_BOTTOM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Note: None of these functions are public interfaces. But internal to the SDL event handling */

int sdl_handle_pending_events(void);
int sdl_init_video(void);
void sdl_quit(void);
const char *sdl_get_error(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_COMMON_SDL_SDL_EVENTS_BOTTOM_H */
