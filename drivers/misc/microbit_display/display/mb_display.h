/** SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Peter Niebert <peter.niebert@univ-amu.fr>
 *
 * @brief BBC micro:bit legacy display APIs.
 *
 * This file is here for compatibility with the replacement
 * microbit_display.h for old projects. See microbit_display.h
 * for type and function definitions.
 *
 */


#ifndef ZEPHYR_INCLUDE_DISPLAY_MB_DISPLAY_H_
#define ZEPHYR_INCLUDE_DISPLAY_MB_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "microbit_display.h"

struct mb_display;

/**
 * @brief Get a pointer to the BBC micro:bit display object.
 *
 * @return Pointer to display object.
 */
inline struct mb_display *mb_display_get(void)
{
	return NULL;
}

#define mb_display_image(disp, mode, duration, img, img_count) \
	{ \
		(void) disp; \
		mb_display_image_v2(mode, duration, img, img_count); \
	}


#define mb_display_print(disp, mode, duration, fmt, ...) \
	{ \
	 (void) disp; \
	 mb_display_print_v2(mode, duration, fmt __VA_OPT__(,) __VA_ARGS__); \
	}

#define mb_display_stop(disp) \
	{ \
		(void) disp; \
		mb_display_stop_v2(); \
	}

#ifdef __cplusplus
}
#endif
#endif
