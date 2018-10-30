/*
 * Copyright (c) 2018 Miras Absar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @author Miras Absar <miras@miras.io>
 * @brief The SEGDL API.
 *
 * The SEGDL (Simple/Shitty Embedded Graphic Display Library) API is a
 * collection of enums, lookup tables, structs, typedefs, and functions that
 * cover general display information and functionality.
 * It covers...
 *
 * - width
 * - height
 * - color space
 * - max brightness
 * - orientation
 * - drawing pixels
 * - drawing lines
 * - drawing frames
 * - setting brightness
 * - sleeping
 * - waking
 * - display specific information and functionality
 */

#ifndef SEGDL_H
#define SEGDL_H

#include <stdint.h>
#include <device.h>

/**
 * @brief An enum to represent color spaces.
 */
enum segdl_color_space {
	segdl_color_space_1,		/**< 1-bit color space. */
	segdl_color_space_rgb16,	/**< 16-bit RGB color space. */
	segdl_color_space_rgb18,	/**< 18-bit RGB color space. */
	segdl_color_space_bgr16,	/**< 16-bit BGR color space. */
	segdl_color_space_bgr18		/**< 18-bit BGR color space. */
};

/**
 * @brief A color space to bits required lookup table.
 */
static const uint8_t segdl_color_space__bits_req[] = {
	1,
	16,
	18,
	16,
	18
};

/**
 * @brief A color space to bits allocated lookup table.
 */
static const uint8_t segdl_color_space__bits_alloc[] = {
	8,
	16,
	32,
	16,
	32
};

/**
 * @brief A struct to represent a pixel.
 *
 * @param x is the pixel's x coordinate.
 * @param y is the pixel's y coordinate.
 * @param color is a valid pointer to the pixel's color.
 *
 *        For an n-bit display, @p color should be typeof the smallest unsigned
 *        integer needed to represent n bits, with the first n bit fields
 *        representing the color's data.
 *        For example...
 *
 *        - For a 1-bit display, @p color should be a @ref uint8_t with the
 *          first bit field representing the color's data
 *          (ie. 0b[dummy data: 7][color data: 1]).
 *
 *        - For a 16-bit display, @p color should be a @ref uint16_t with all
 *          bit fields representing the color's data
 *          (ie. 0b[color data: 16]).
 *
 *        - For an 18-bit display, @p color should be a @ref uint32_t with the
 *          first 18 bit fields representing the color's data
 *          (ie. 0b[dummy data: 14][color data: 18]).
 */
struct segdl_px {
	uint16_t x;
	uint16_t y;
	void *color;
};

/**
 * @brief A struct to represent a line.
 *
 * @param x is the line's starting x coordinate.
 * @param y is the line's starting y coordinate.
 * @param size is the line's size (width or height depending on orientation).
 * @param colors is a valid pointer to a 1D array of colors representing the
 *        line's pixels.
 *
 *        For an n-bit display, @p colors should an array of typeof the smallest
 *        unsigned integer needed to represent n bits, with color data packed as
 *        tighly as evenly possible in least significant order.
 *        For example...
 *
 *        - For a 1-bit display, @p colors should be an array of @ref uint8_t
 *          with all bit fields of a uint8_t representing 8 colors' data in
 *          least significant order
 *          (ie. 0b[color 0 data: 1][color 1 data: 1]...[color 7 data: 1]).
 *
 *        - For a 16-bit display, @p colors should be an array of @ref uint16_t
 *          with all bit fields of a uint16_t representing a color's data
 *          (ie. 0b[color data: 16]).
 *
 *        - For an 18-bit display, @p colors should be an array of @ref uint32_t
 *          with the first 18 bit fields of a uint32_t representing a color's
 *          data
 *          (ie. 0b[dummy data: 14][color data: 18]).
 *
 *        A color at index i...
 *
 *        - in a horizontal line, is drawn at the (x,y) coordinates
 *          ((@p x + i),@p y).
 *
 *        - in a vertical line, is drawn at the (x,y) coordinates
 *          (@p x,(@p y + i)).
 *
 *        The orientation of a line is determined by the function it's passed to
 *        (ie. @ref segdl_draw_fh_ln_t vs @ref segdl_draw_fv_ln_t).
 *
 * @param colors_len is the length of @p colors.
 */
struct segdl_ln {
	uint16_t x;
	uint16_t y;
	uint16_t size;
	void *colors;
	uint16_t colors_len;
};

/**
 * @brief A struct to represent a frame.
 *
 * @param x is the frame's starting x coordinate.
 * @param y is the frame's starting y coordinate.
 * @param width is the frame's width.
 * @param height is the frame's height.
 * @param colors is a valid pointer to a 2D array of colors representing the
 *        frame's pixels.
 *
 *        For an n-bit display, @p colors should an array of typeof the smallest
 *        unsigned integer needed to represent n bits, with color data packed as
 *        tighly as evenly possible in least significant order. For example...
 *
 *        - For a 1-bit display, @p colors should be an array of @ref uint8_t
 *          with all bit fields of a uint8_t representing 8 colors' data in
 *          least significant order
 *          (ie. 0b[color 0 data: 1][color 1 data: 1]...[color 7 data: 1]).
 *
 *        - For a 16-bit display, @p colors should be an array of @ref uint16_t
 *          with all bit fields of a uint16_t representing a color's data
 *          (ie. 0b[color data: 16]).
 *
 *        - For an 18-bit display, @p colors should be an array of @ref uint32_t
 *          with the first 18 bit fields of a uint32_t representing a color's
 *          data
 *          (ie. 0b[dummy data: 14][color data: 18]).
 *
 *        A color at indices (i,j) is drawn at the (x,y) coordinates
 *        ((@p x + i),(@p y + j)).
 *
 * @param colors_len is the length of @p colors.
 */
struct segdl_frame {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	void *colors;
	uint16_t colors_len;
};

/**
 * @brief A function to draw a pixel.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param px is a valid pointer to a pixel.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_px_t)(struct device *dev, struct segdl_px *px);

/**
 * @brief A function to draw pixels.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param pxs is a valid pointer to a 1D array of pixels.
 * @param pxs_len is the length of @p pxs.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_pxs_t)(struct device *dev, struct segdl_px *pxs,
				uint16_t pxs_len);

/**
 * @brief A function to draw a full-width, horizontal line.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param ln is a valid pointer to a line.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_fh_ln_t)(struct device *dev, struct segdl_ln *ln);

/**
 * @brief A function to draw a partial-width, horizontal line.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param ln is a valid pointer to a line.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_ph_ln_t)(struct device *dev, struct segdl_ln *ln);

/**
 * @brief A function to draw a full-height, vertical line.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param ln is a valid pointer to a line.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_fv_ln_t)(struct device *dev, struct segdl_ln *ln);

/**
 * @brief A function to draw a partial-height, vertical line.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param ln is a valid pointer to a line.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_pv_ln_t)(struct device *dev, struct segdl_ln *ln);

/**
 * @brief A function to draw full-width, horizontal lines.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param lns is a valid pointer to a 1D array of lines.
 * @param lns_len is the length of @p lns.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_fh_lns_t)(struct device *dev, struct segdl_ln *lns,
				   uint16_t lns_len);

/**
 * @brief A function to draw partial-width, horizontal lines.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param lns is a valid pointer to a 1D array of lines.
 * @param lns_len is the length of @p lns.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_ph_lns_t)(struct device *dev, struct segdl_ln *lns,
				   uint16_t lns_len);

/**
 * @brief A function to draw full-height, vertical lines.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param lns is a valid pointer to a 1D array of lines.
 * @param lns_len is the length of @p lns.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_fv_lns_t)(struct device *dev, struct segdl_ln *lns,
				   uint16_t lns_len);

/**
 * @brief A function to draw partial-height, vertical lines.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param lns is a valid pointer to a 1D array of lines.
 * @param lns_len is the length of @p lns.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_pv_lns_t)(struct device *dev, struct segdl_ln *lns,
				   uint16_t lns_len);

/**
 * @brief A function to draw a partial frame.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param frame is a valid pointer to a frame.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int(*segdl_draw_partial_frame_t)(struct device *dev,
					 struct segdl_frame *frame);

/**
 * @brief A function to draw partial frames.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param frames is a valid pointer to a 1D array of frames.
 * @param frames_len is the length of @p frames.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int(*segdl_draw_partial_frames_t)(struct device *dev,
					  struct segdl_frame *frames,
					  uint16_t frames_len);

/**
 * @brief A function to draw a full frame.
 *
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param frame is a valid pointer to a frame.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_draw_full_frame_t)(struct device *dev,
				       struct segdl_frame *frame);

/**
 * @brief A function to set the display brightness.
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_set_brightness_t)(struct device *dev, uint16_t brightness);

/**
 * @brief A function to put the display to sleep.
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*segdl_sleep_t)(struct device *dev);

/**
 * @brief A function to wake up the display.
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns 0 on success or a negative error number on error.
 */
typedef int(*segdl_wake_t)(struct device *dev);

/**
 * @brief A struct to represent a SEGDL API implementation.
 *
 * @param width is the display's width.
 * @param height is the display's height.
 * @param color_space is the display's color space.
 * @param max_brightness is the display's max brightness.
 * @param orientation is the display's orientation in degrees.
 *
 * @param supports_draw_px is whether the display supports @ref segdl_draw_px_t.
 * @param supports_draw_pxs is whether the display supports
 *        @ref segdl_draw_pxs_t.
 *
 *
 *
 * @param supports_draw_fh_ln is whether the display supports
 *        @ref segdl_draw_fh_ln_t.
 *
 * @param supports_draw_ph_ln is whether the display supports
 *        @ref segdl_draw_ph_ln_t.
 *
 * @param supports_draw_fv_ln is whether the display supports
 *        @ref segdl_draw_fv_ln_t.
 *
 * @param supports_draw_pv_ln is whether the display supports
 *        @ref segdl_draw_pv_ln_t.
 *
 *
 *
 * @param supports_draw_fh_lns is whether the display supports
 *        @ref segdl_draw_fh_lns_t.
 *
 * @param supports_draw_ph_lns is whether the display supports
 *        @ref segdl_draw_ph_lns_t.
 *
 * @param supports_draw_fv_lns is whether the display supports
 *        @ref segdl_draw_fv_lns_t.
 *
 * @param supports_draw_pv_lns is whether the display supports
 *        @ref segdl_draw_pv_lns_t.
 *
 *
 *
 * @param supports_draw_partial_frame is whether the display supports
 *        @ref segdl_draw_partial_frame_t.
 *
 * @param supports_draw_partial_frames is whether the display supports
 *        @ref segdl_draw_partial_frames_t.
 *
 * @param supports_draw_full_frame is whether the display supports
 *        @ref segdl_draw_full_frame_t.
 *
 *
 *
 * @param supports_set_brightness is whether the display supports
 *        @ref segdl_set_brightness_t.
 *
 * @param supports_sleep_wake is whether the display supports
 *        @ref segdl_sleep_t and @ref segdl_wake_t.
 *
 *
 *
 * @param has_extra_api is whether the display has an extra API.
 *
 *
 *
 * @param draw_px is the display's implementation of @ref segdl_draw_px_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_px_t.
 *
 * @param draw_pxs is the display's implementation of @ref segdl_draw_pxs_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_pxs_t.
 *
 *
 *
 * @param draw_fh_ln is the display's implementation of @ref segdl_draw_fh_ln_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_fh_ln_t.
 *
 * @param draw_ph_ln is the display's implementation of @ref segdl_draw_ph_ln_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_ph_ln_t.
 *
 * @param draw_fv_ln is the display's implementation of @ref segdl_draw_fv_ln_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_fv_ln_t.
 *
 * @param draw_pv_ln is the display's implementation of @ref segdl_draw_pv_ln_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_pv_ln_t.
 *
 *
 *
 * @param draw_fh_lns is the display's implementation of
 *        @ref segdl_draw_fh_lns_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_fh_lns_t.
 *
 * @param draw_ph_lns is the display's implementation of
 *        @ref segdl_draw_ph_lns_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_ph_lns_t.
 *
 * @param draw_fv_lns is the display's implementation of
 *        @ref segdl_draw_fv_lns_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_fv_lns_t.
 *
 * @param draw_pv_lns is the display's implementation of
 *        @ref segdl_draw_pv_lns_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_pv_lns_t.
 *
 *
 *
 * @param draw_partial_frame is the display's implementation of
 *        @ref segdl_draw_partial_frame_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_partial_frame_t.
 *
 * @param draw_partial_frames is the display's implementation of
 *        @ref segdl_draw_partial_frames_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_partial_frames_t.
 *
 * @param draw_full_frame is the display's implementation of
 *        @ref segdl_draw_full_frame_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_draw_full_frame_t.
 *
 *
 *
 * @param set_brightness is the display's implementation of
 *        @ref segdl_set_brightness_t.
 *        This should be NULL if the display doesn't support
 *        @ref segdl_set_brightness_t.
 *
 * @param sleep is the display's implementation of @ref segdl_sleep_t.
 *        This should be NULL if the display doesn't support @ref segdl_sleep_t.
 *
 * @param wake is the display's implementation of @ref segdl_wake_t.
 *        This should be NULL if the display doesn't support @ref segdl_wake_t.
 *
 *
 *
 * @param extra_api is a valid pointer to the display's extra API.
 *        This should be NULL if the display doesn't have an extra API.
 */
struct segdl_api {
	uint16_t width;
	uint16_t height;
	enum segdl_color_space color_space;
	uint16_t max_brightness;
	uint16_t orientation;

	bool supports_draw_px;
	bool supports_draw_pxs;

	bool supports_draw_fh_ln;
	bool supports_draw_ph_ln;
	bool supports_draw_fv_ln;
	bool supports_draw_pv_ln;

	bool supports_draw_fh_lns;
	bool supports_draw_ph_lns;
	bool supports_draw_fv_lns;
	bool supports_draw_pv_lns;

	bool supports_draw_partial_frame;
	bool supports_draw_partial_frames;
	bool supports_draw_full_frame;

	bool supports_set_brightness;
	bool supports_sleep_wake;

	bool has_extra_api;

	segdl_draw_px_t draw_px;
	segdl_draw_pxs_t draw_pxs;

	segdl_draw_fh_ln_t draw_fh_ln;
	segdl_draw_ph_ln_t draw_ph_ln;
	segdl_draw_fv_ln_t draw_fv_ln;
	segdl_draw_pv_ln_t draw_pv_ln;

	segdl_draw_fh_lns_t draw_fh_lns;
	segdl_draw_ph_lns_t draw_ph_lns;
	segdl_draw_fv_lns_t draw_fv_lns;
	segdl_draw_pv_lns_t draw_pv_lns;

	segdl_draw_partial_frame_t draw_partial_frame;
	segdl_draw_partial_frames_t draw_partial_frames;
	segdl_draw_full_frame_t draw_full_frame;

	segdl_set_brightness_t set_brightness;
	segdl_sleep_t sleep;
	segdl_wake_t wake;

	void *extra_api;
};

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns the display's width.
 */
static inline uint16_t segdl_get_width(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->width;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param width is the display's width.
 */
static inline void segdl_set_width(struct device *dev, uint16_t width)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	api->width = width;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns the display's height.
 */
static inline uint16_t segdl_get_height(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->height;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param height is the display's height.
 */
static inline void segdl_set_height(struct device *dev, uint16_t height)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	api->height = height;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns the display's color space.
 */
static inline enum segdl_color_space segdl_get_color_space(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->color_space;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param color_space is the display's color space.
 */
static inline void segdl_set_color_space(struct device *dev,
					 enum segdl_color_space color_space)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	api->color_space = color_space;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns the display's max brightness.
 */
static inline uint16_t segdl_get_max_brightness(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->max_brightness;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns the display's orientation.
 */
static inline uint16_t segdl_get_orientation(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->orientation;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @param orientation is the display's orientation.
 */
static inline void segdl_set_orientation(struct device *dev,
					 uint16_t orientation)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	api->orientation = orientation;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_px_t.
 */
static inline bool segdl_supports_draw_px(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_px;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_pxs_t.
 */
static inline bool segdl_supports_draw_pxs(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_pxs;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_fh_ln_t.
 */
static inline bool segdl_supports_draw_fh_ln(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_fh_ln;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_ph_ln_t.
 */
static inline bool segdl_supports_draw_ph_ln(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_ph_ln;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_fv_ln_t.
 */
static inline bool segdl_supports_draw_fv_ln(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_fv_ln;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_pv_ln_t.
 */
static inline bool segdl_supports_draw_pv_ln(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_pv_ln;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_fh_lns_t.
 */
static inline bool segdl_supports_draw_fh_lns(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_fh_lns;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_ph_lns_t.
 */
static inline bool segdl_supports_draw_ph_lns(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_ph_lns;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_fv_lns_t.
 */
static inline bool segdl_supports_draw_fv_lns(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_fv_lns;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_pv_lns_t.
 */
static inline bool segdl_supports_draw_pv_lns(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_pv_lns;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_partial_frame_t.
 */
static inline bool segdl_supports_draw_partial_frame(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_partial_frame;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_partial_frames_t.
 */
static inline bool segdl_supports_draw_partial_frames(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_partial_frames;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_draw_full_frame_t.
 */
static inline bool segdl_supports_draw_full_frame(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_draw_full_frame;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_set_brightness_t.
 */
static inline bool segdl_supports_set_brightness(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_set_brightness;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display supports @ref segdl_sleep_t and segdl_wake_t.
 */
static inline bool segdl_supports_supports_sleep_wake(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->supports_sleep_wake;
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns whether the display has an extra API.
 */
static inline bool segdl_has_extra_api(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->has_extra_api;
}

/**
 * @see segdl_draw_px_t
 */
static inline int segdl_draw_px(struct device *dev, struct segdl_px *px)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_px(dev, px);
}

/**
 * @see segdl_draw_pxs_t
 */
static inline int segdl_draw_pxs(struct device *dev, struct segdl_px *pxs,
				 uint16_t pxs_len)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_pxs(dev, pxs, pxs_len);
}

/**
 * @see segdl_draw_fh_ln_t
 */
static inline int segdl_draw_fh_ln(struct device *dev, struct segdl_ln *ln)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_fh_ln(dev, ln);
}

/**
 * @see segdl_draw_ph_ln_t
 */
static inline int segdl_draw_ph_ln(struct device *dev, struct segdl_ln *ln)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_ph_ln(dev, ln);
}

/**
 * @see segdl_draw_fv_ln_t
 */
static inline int segdl_draw_fv_ln(struct device *dev, struct segdl_ln *ln)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_fv_ln(dev, ln);
}

/**
 * @see segdl_draw_pv_ln_t
 */
static inline int segdl_draw_pv_ln(struct device *dev, struct segdl_ln *ln)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_pv_ln(dev, ln);
}

/**
 * @see segdl_draw_fh_lns_t
 */
static inline int segdl_draw_fh_lns(struct device *dev, struct segdl_ln *lns,
				    uint16_t lns_len)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_fh_lns(dev, lns, lns_len);
}

/**
 * @see segdl_draw_ph_lns_t
 */
static inline int segdl_draw_ph_lns(struct device *dev, struct segdl_ln *lns,
				    uint16_t lns_len)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_ph_lns(dev, lns, lns_len);
}

/**
 * @see segdl_draw_fv_lns_t
 */
static inline int segdl_draw_fv_lns(struct device *dev, struct segdl_ln *lns,
				    uint16_t lns_len)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_fv_lns(dev, lns, lns_len);
}

/**
 * @see segdl_draw_pv_lns_t
 */
static inline int segdl_draw_pv_lns(struct device *dev, struct segdl_ln *lns,
				    uint16_t lns_len)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_pv_lns(dev, lns, lns_len);
}

/**
 * @see segdl_draw_partial_frame_t
 */
static inline int segdl_draw_partial_frame(struct device *dev,
					   struct segdl_frame *frame)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_partial_frame(dev, frame);
}

/**
 * @see segdl_draw_partial_frames_t
 */
static inline int segdl_draw_partial_frames(struct device *dev,
					    struct segdl_frame *frames,
					    uint16_t frames_len)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_partial_frames(dev, frames, frames_len);
}

/**
 * @see segdl_draw_full_frame_t
 */
static inline int segdl_draw_full_frame(struct device* dev,
					struct segdl_frame *frame)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->draw_full_frame(dev, frame);
}

/**
 * @see segdl_set_brightness_t
 */
static inline int segdl_set_brightness(struct device *dev, uint16_t brightness)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->set_brightness(dev, brightness);
}

/**
 * @see segdl_sleep_t
 */
static inline int segdl_sleep(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->sleep(dev);
}

/**
 * @see segdl_wake_t
 */
static inline int segdl_wake(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->wake(dev);
}

/**
 * @param dev is a valid pointer to a device that implements the SEGDL API.
 * @returns a pointer to the display's extra API.
 */
static inline void *segdl_extra_api(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	return api->extra_api;
}

#endif
