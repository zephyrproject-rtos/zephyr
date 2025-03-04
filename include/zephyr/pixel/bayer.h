/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_BAYER_H_
#define ZEPHYR_INCLUDE_PIXEL_BAYER_H_

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/pixel/stream.h>

/**
 * @brief Convert a line from RGGB8 to RGB24 with 3x3 method
 *
 * @param i0 Buffer of the input row number 0 in bayer format (1 byte per pixel).
 * @param i1 Buffer of the input row number 1 in bayer format (1 byte per pixel).
 * @param i2 Buffer of the input row number 2 in bayer format (1 byte per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_rggb8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);
/**
 * @brief Convert a line from GRBG8 to RGB24 with 3x3 method
 * @copydetails pixel_rggb8line_to_rgb24line_3x3()
 */
void pixel_grbg8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);
/**
 * @brief Convert a line from BGGR8 to RGB24 with 3x3 method
 * @copydetails pixel_rggb8line_to_rgb24line_3x3()
 */
void pixel_bggr8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);
/**
 * @brief Convert a line from GBRG8 to RGB24 with 3x3 method
 * @copydetails pixel_rggb8line_to_rgb24line_3x3()
 */
void pixel_gbrg8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, uint16_t width);

/**
 * @brief Convert a line from RGGB8 to RGB24 with 2x2 method
 *
 * @param i0 Buffer of the input row number 0 in bayer format (1 byte per pixel).
 * @param i1 Buffer of the input row number 1 in bayer format (1 byte per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_rggb8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);
/**
 * @brief Convert a line from GBRG8 to RGB24 with 2x2 method
 * @copydetails pixel_rggb8line_to_rgb24line_2x2()
 */
void pixel_gbrg8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);
/**
 * @brief Convert a line from BGGR8 to RGB24 with 2x2 method
 * @copydetails pixel_rggb8line_to_rgb24line_2x2()
 */
void pixel_bggr8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);
/**
 * @brief Convert a line from GRBG8 to RGB24 with 2x2 method
 * @copydetails pixel_rggb8line_to_rgb24line_2x2()
 */
void pixel_grbg8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      uint16_t width);

/**
 * @brief Define a stream converter from RGGB8 to RGB24 with the 3x3 method
 *
 * @param name The symbol of the @ref pixel_stream that will be defined.
 * @param width The total width of the input frame in number of pixels.
 * @param height The total height of the input frame in number of pixels.
 */
#define PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_rggb8stream_to_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a stream converter from GRBG8 to RGB24 with the 3x3 method
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_GRBG8STREAM_TO_RGB24STREAM_3X3(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_grbg8stream_to_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a stream converter from BGGR8 to RGB24 with the 3x3 method
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_BGGR8STREAM_TO_RGB24STREAM_3X3(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_bggr8stream_to_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a stream converter from GBRG8 to RGB24 with the 3x3 method
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_GBRG8STREAM_TO_RGB24STREAM_3X3(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_gbrg8stream_to_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a stream converter from RGGB8 to RGB24 with the 2x2 method
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_RGGB8STREAM_TO_RGB24STREAM_2X2(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_rggb8stream_to_rgb24stream_2x2, (width), (height), 2)
/**
 * @brief Define a stream converter from GBRG8 to RGB24 with the 2x2 method
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_GBRG8STREAM_TO_RGB24STREAM_2X2(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_gbrg8stream_to_rgb24stream_2x2, (width), (height), 2)
/**
 * @brief Define a stream converter from BGGR8 to RGB24 with the 2x2 method
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_BGGR8STREAM_TO_RGB24STREAM_2X2(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_bggr8stream_to_rgb24stream_2x2, (width), (height), 2)
/**
 * @brief Define a stream converter from GRBG8 to RGB24 with the 2x2 method
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_GRBG8STREAM_TO_RGB24STREAM_2X2(name, width, height)                                  \
	PIXEL_BAYER_DEFINE(name, pixel_grbg8stream_to_rgb24stream_2x2, (width), (height), 2)

/** @cond INTERNAL_HIDDEN */
#define PIXEL_BAYER_DEFINE(name, fn, width, height, window_height)                                 \
	PIXEL_STREAM_DEFINE((name), (fn), (width), (height), (width), (window_height) * (width))
void pixel_rggb8stream_to_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_grbg8stream_to_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_bggr8stream_to_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_gbrg8stream_to_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_rggb8stream_to_rgb24stream_2x2(struct pixel_stream *strm);
void pixel_gbrg8stream_to_rgb24stream_2x2(struct pixel_stream *strm);
void pixel_bggr8stream_to_rgb24stream_2x2(struct pixel_stream *strm);
void pixel_grbg8stream_to_rgb24stream_2x2(struct pixel_stream *strm);
/** @endcond */

#endif /* ZEPHYR_INCLUDE_PIXEL_BAYER_H_ */
