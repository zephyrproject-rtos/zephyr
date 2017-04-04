/** @file
 *  @brief BBC micro:bit display APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MB_DISPLAY_H
#define __MB_DISPLAY_H

/**
 * @brief BBC micro:bit display APIs
 * @defgroup mb_display BBC micro:bit display APIs
 * @{
 */

#include <stdio.h>
#include <stdint.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Representation of a BBC micro:bit display image.
 *
 * This struct should normally not be used directly, rather created
 * using the MB_IMAGE() macro.
 */
struct mb_image {
	union {
		struct {
			uint8_t c1:1,
				c2:1,
				c3:1,
				c4:1,
				c5:1;
		} r[5];
		uint8_t row[5];
	};
};

/**
 * @def MB_IMAGE
 * @brief Generate an image object from a given array rows/columns.
 *
 * This helper takes an array of 5 rows, each consisting of 5 0/1 values which
 * correspond to the columns of that row. The value 0 means the pixel is
 * disabled whereas a 1 means the pixel is enabled.
 *
 * The pixels go from left to right and top to bottom, i.e. top-left corner
 * is the first row's first value, top-right is the first rows last value,
 * and bottom-right corner is the last value of the last (5th) row. As an
 * example, the following would create a smiley face image:
 *
 * <pre>
 * static const struct mb_image smiley = MB_IMAGE({ 0, 1, 0, 1, 0 },
 *						  { 0, 1, 0, 1, 0 },
 *						  { 0, 0, 0, 0, 0 },
 *						  { 1, 0, 0, 0, 1 },
 *						  { 0, 1, 1, 1, 0 });
 * </pre>
 *
 * @param _rows Each of the 5 rows represented as a 5-value column array.
 *
 * @return Image bitmap that can be passed e.g. to mb_display_image().
 */
#define MB_IMAGE(_rows...) { .r = { _rows } }

/**
 * @brief Opaque struct representing the BBC micro:bit display.
 *
 * For more information see the following links:
 *
 * https://www.microbit.co.uk/device/screen
 *
 * https://lancaster-university.github.io/microbit-docs/ubit/display/
 */
struct mb_display;

/**
 * @brief Get a pointer to the BBC micro:bit display object.
 *
 * @return Pointer to display object.
 */
struct mb_display *mb_display_get(void);

/**
 * @brief Display an image on the BBC micro:bit LED display.
 *
 * @param disp     Display object.
 * @param img      Bitmap of pixels.
 * @param duration Duration how long to show the image (in milliseconds).
 */
void mb_display_image(struct mb_display *disp, const struct mb_image *img,
		      int32_t duration);

/**
 * @brief Display a character on the BBC micro:bit LED display.
 *
 * @param disp     Display object.
 * @param chr      Character to display.
 * @param duration Duration how long to show the character (in milliseconds).
 */
void mb_display_char(struct mb_display *disp, char chr, int32_t duration);

/**
 * @brief Display a string of characters on the BBC micro:bit LED display.
 *
 * This function takes a printf-style format string and outputs it one
 * character at a time to the display. For scrolling-based string output
 * see the mb_display_print() API.
 *
 * @param disp     Display object.
 * @param duration Duration how long to show each character (in milliseconds).
 * @param fmt      printf-style format string.
 * @param ...      Optional list of format arguments.
 */
__printf_like(3, 4) void mb_display_string(struct mb_display *disp,
					   int32_t duration,
					   const char *fmt, ...);

/**
 * @brief Print a string of characters on the BBC micro:bit LED display.
 *
 * This function takes a printf-style format string and outputs it in a
 * scrolling fashion to the display. For character-by-character output
 * instead of scrolling, see the mb_display_string() API.
 *
 * @param disp     Display object.
 * @param fmt      printf-style format string
 * @param ...      Optional list of format arguments.
 */
__printf_like(2, 3) void mb_display_print(struct mb_display *disp,
					  const char *fmt, ...);

/**
 * @brief Stop the ongoing display of an image.
 *
 * @param disp    Display object.
 */
void mb_display_stop(struct mb_display *disp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __MB_DISPLAY_H */
