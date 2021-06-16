/** @file
 *  @brief BBC micro:bit legacy display APIs.
 *
 * This file is here for compatibility with the replacement
 * mb_display_v2.h for old projects. See mb_display_v2.h
 * for type and function definitions.
 *
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DISPLAY_MB_DISPLAY_H_
#define ZEPHYR_INCLUDE_DISPLAY_MB_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mb_display_v2.h"

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
