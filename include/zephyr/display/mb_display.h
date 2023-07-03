/** @file
 *  @brief BBC micro:bit display APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DISPLAY_MB_DISPLAY_H_
#define ZEPHYR_INCLUDE_DISPLAY_MB_DISPLAY_H_

/**
 * @brief BBC micro:bit display APIs
 * @defgroup mb_display BBC micro:bit display APIs
 * @ingroup third_party
 * @{
 */

#include <stdio.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

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
 * @brief Display mode.
 *
 * First 16 bits are reserved for modes, last 16 for flags.
 */
enum mb_display_mode {
	/** Default mode ("single" for images, "scroll" for text). */
	MB_DISPLAY_MODE_DEFAULT,

	/** Display images sequentially, one at a time. */
	MB_DISPLAY_MODE_SINGLE,

	/** Display images by scrolling. */
	MB_DISPLAY_MODE_SCROLL,

	/* Display flags, i.e. modifiers to the chosen mode */

	/** Loop back to the beginning when reaching the last image. */
	MB_DISPLAY_FLAG_LOOP        = BIT(16),
};

/**
 *
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
 * @brief Display one or more images on the BBC micro:bit LED display.
 *
 * This function takes an array of one or more images and renders them
 * sequentially on the micro:bit display. The call is asynchronous, i.e.
 * the processing of the display happens in the background. If there is
 * another image being displayed it will be canceled and the new one takes
 * over.
 *
 * @param disp      Display object.
 * @param mode      One of the MB_DISPLAY_MODE_* options.
 * @param duration  Duration how long to show each image (in milliseconds), or
 *                  @ref SYS_FOREVER_MS.
 * @param img       Array of image bitmaps (struct mb_image objects).
 * @param img_count Number of images in 'img' array.
 */
void mb_display_image(struct mb_display *disp, uint32_t mode, int32_t duration,
		      const struct mb_image *img, uint8_t img_count);

/**
 * @brief Print a string of characters on the BBC micro:bit LED display.
 *
 * This function takes a printf-style format string and outputs it in a
 * scrolling fashion to the display.
 *
 * The call is asynchronous, i.e. the processing of the display happens in
 * the background. If there is another image or string being displayed it
 * will be canceled and the new one takes over.
 *
 * @param disp     Display object.
 * @param mode     One of the MB_DISPLAY_MODE_* options.
 * @param duration Duration how long to show each character (in milliseconds),
 *                 or @ref SYS_FOREVER_MS.
 * @param fmt      printf-style format string
 * @param ...      Optional list of format arguments.
 */
__printf_like(4, 5) void mb_display_print(struct mb_display *disp,
					  uint32_t mode, int32_t duration,
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

#endif /* ZEPHYR_INCLUDE_DISPLAY_MB_DISPLAY_H_ */
