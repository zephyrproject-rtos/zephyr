/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_BAYER_H_
#define ZEPHYR_INCLUDE_PIXEL_BAYER_H_

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/pixel/operation.h>

/**
 * @brief Define a new bayer format conversion operation.
 *
 * @param _fn Function performing the operation.
 * @param _fourcc_in The input format for that operation.
 * @param _window_size The number of line of context needed for that debayer operation.
 */
#define PIXEL_DEFINE_BAYER_OPERATION(_fn, _fourcc_in, _window_size)                                \
	static const STRUCT_SECTION_ITERABLE_ALTERNATE(pixel_convert, pixel_operation,             \
						       _fn##_op) = {                               \
		.name = #_fn,                                                                      \
		.fourcc_in = _fourcc_in,                                                           \
		.fourcc_out = VIDEO_PIX_FMT_RGB24,                                                 \
		.window_size = _window_size,                                                       \
		.run = _fn,                                                                        \
	}

/**
 * @brief Convert a line from RGGB8 to RGB24 with 3x3 method
 *
 * @param i0 Buffer of the input row number 0 in bayer format (1 byte per pixel).
 * @param i1 Buffer of the input row number 1 in bayer format (1 byte per pixel).
 * @param i2 Buffer of the input row number 2 in bayer format (1 byte per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_line_rggb8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);
/**
 * @brief Convert a line from GRBG8 to RGB24 with 3x3 method
 * @copydetails pixel_line_rggb8_to_rgb24_3x3()
 */
void pixel_line_grbg8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);
/**
 * @brief Convert a line from BGGR8 to RGB24 with 3x3 method
 * @copydetails pixel_line_rggb8_to_rgb24_3x3()
 */
void pixel_line_bggr8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);
/**
 * @brief Convert a line from GBRG8 to RGB24 with 3x3 method
 * @copydetails pixel_line_rggb8_to_rgb24_3x3()
 */
void pixel_line_gbrg8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);

/**
 * @brief Convert a line from RGGB8 to RGB24 with 2x2 method
 *
 * @param i0 Buffer of the input row number 0 in bayer format (1 byte per pixel).
 * @param i1 Buffer of the input row number 1 in bayer format (1 byte per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_line_rggb8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);
/**
 * @brief Convert a line from GBRG8 to RGB24 with 2x2 method
 * @copydetails pixel_line_rggb8_to_rgb24_2x2()
 */
void pixel_line_gbrg8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);
/**
 * @brief Convert a line from BGGR8 to RGB24 with 2x2 method
 * @copydetails pixel_line_rggb8_to_rgb24_2x2()
 */
void pixel_line_bggr8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);
/**
 * @brief Convert a line from GRBG8 to RGB24 with 2x2 method
 * @copydetails pixel_line_rggb8_to_rgb24_2x2()
 */
void pixel_line_grbg8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);

#endif /* ZEPHYR_INCLUDE_PIXEL_BAYER_H_ */
