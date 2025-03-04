/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_BAYER_H
#define ZEPHYR_INCLUDE_PIXEL_BAYER_H

#include <stdint.h>

#include <zephyr/pixel/stream.h>

/**
 * Pixel streams definitions.
 * @{
 */

void pixel_rggb8stream_to_rgb24stream_3x3(struct pixel_stream *strm);

#define PIXEL_STREAM_RGGB8_TO_RGB24_3X3(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_rggb8stream_to_rgb24stream_3x3,                            \
			    (width), (height), (width) * 1, 3 * (width) * 1)

void pixel_grbg8stream_to_rgb24stream_3x3(struct pixel_stream *strm);

#define PIXEL_STREAM_GRBG8_TO_RGB24_3X3(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_grbg8stream_to_rgb24stream_3x3,                            \
			    (width), (height), (width) * 1, 3 * (width) * 1)

void pixel_bggr8stream_to_rgb24stream_3x3(struct pixel_stream *strm);

#define PIXEL_STREAM_BGGR8_TO_RGB24_3X3(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_bggr8stream_to_rgb24stream_3x3,                            \
			    (width), (height), (width) * 1, 3 * (width) * 1)

void pixel_gbrg8stream_to_rgb24stream_3x3(struct pixel_stream *strm);

#define PIXEL_STREAM_GBRG8_TO_RGB24_3X3(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_gbrg8stream_to_rgb24stream_3x3,                            \
			    (width), (height), (width) * 1, 3 * (width) * 1)

void pixel_rggb8stream_to_rgb24stream_2x2(struct pixel_stream *strm);

#define PIXEL_STREAM_RGGB8_TO_RGB24_2X2(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_rggb8stream_to_rgb24stream_2x2,                            \
			    (width), (height), (width) * 1, 2 * (width) * 1)

void pixel_gbrg8stream_to_rgb24stream_2x2(struct pixel_stream *strm);

#define PIXEL_STREAM_GBRG8_TO_RGB24_2X2(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_gbrg8stream_to_rgb24stream_2x2,                            \
			    (width), (height), (width) * 1, 2 * (width) * 1)

void pixel_bggr8stream_to_rgb24stream_2x2(struct pixel_stream *strm);

#define PIXEL_STREAM_BGGR8_TO_RGB24_2X2(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_bggr8stream_to_rgb24stream_2x2,                            \
			    (width), (height), (width) * 1, 2 * (width) * 1)

void pixel_grbg8stream_to_rgb24stream_2x2(struct pixel_stream *strm);

#define PIXEL_STREAM_GRBG8_TO_RGB24_2X2(name, width, height)                                       \
	PIXEL_STREAM_DEFINE(name, pixel_grbg8stream_to_rgb24stream_2x2,                            \
			    (width), (height), (width) * 1, 2 * (width) * 1)

/** @} */

/**
 * @brief Bayer to RGB24 conversion, 3x3 variant.
 *
 * @param i0 Buffer of the input row number 0 in bayer format (1 byte per pixel).
 * @param i1 Buffer of the input row number 1 in bayer format (1 byte per pixel).
 * @param i2 Buffer of the input row number 2 in bayer format (1 byte per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 * @{
 */
/** Variant for RGGB8 format */
void pixel_rggb8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, size_t width);
/** Variant for GRBG8 format */
void pixel_grbg8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, size_t width);
/** Variant for BGGR8 format */
void pixel_bggr8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, size_t width);
/** Variant for GBRG8 format */
void pixel_gbrg8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2,
				      uint8_t *rgb24, size_t width);
/** @} */

/**
 * @brief Bayer to RGB24 conversion, 2x2 variant.
 *
 * @param i0 Buffer of the input row number 0 in bayer format (1 byte per pixel).
 * @param i1 Buffer of the input row number 1 in bayer format (1 byte per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 * @{
 */
/** Variant for RGGB8 bayer format */
void pixel_rggb8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      size_t width);
/** Variant for GBRG8 bayer format */
void pixel_gbrg8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      size_t width);
/** Variant for BGGR8 bayer format */
void pixel_bggr8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      size_t width);
/** Variant for GRBG8 bayer format */
void pixel_grbg8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *rgb24,
				      size_t width);
/** @} */

/**
 * @brief Perform a RGGB8 to RGB24 conversion, 3x3 variant.
 *
 * @param rgr0 Row 0 with 3 bytes of RGGB8 data
 * @param gbg1 Row 1 with 3 bytes of RGGB8 data
 * @param rgr2 Row 2 with 3 bytes of RGGB8 data
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_rggb8_to_rgb24_3x3(const uint8_t rgr0[3], const uint8_t gbg1[3],
					    const uint8_t rgr2[3], uint8_t rgb24[3])
{
	rgb24[0] = ((uint16_t)rgr0[0] + rgr0[2] + rgr2[0] + rgr2[2]) / 4;
	rgb24[1] = ((uint16_t)rgr0[1] + gbg1[2] + gbg1[0] + rgr2[1]) / 4;
	rgb24[2] = gbg1[1];
}

/**
 * @brief Perform a BGGR8 to RGB24 conversion, 3x3 variant.
 *
 * @param bgb0 Row 0 with 3 bytes of BGGR8 data
 * @param grg1 Row 1 with 3 bytes of BGGR8 data
 * @param bgb2 Row 2 with 3 bytes of BGGR8 data
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_bggr8_to_rgb24_3x3(const uint8_t bgb0[3], const uint8_t grg1[3],
					    const uint8_t bgb2[3], uint8_t rgb24[3])
{
	rgb24[0] = grg1[1];
	rgb24[1] = ((uint16_t)bgb0[1] + grg1[2] + grg1[0] + bgb2[1]) / 4;
	rgb24[2] = ((uint16_t)bgb0[0] + bgb0[2] + bgb2[0] + bgb2[2]) / 4;
}

/**
 * @brief Perform a GRBG8 to RGB24 conversion, 3x3 variant.
 *
 * @param grg0 Row 0 with 3 bytes of GRBG8 data
 * @param bgb1 Row 1 with 3 bytes of GRBG8 data
 * @param grg2 Row 2 with 3 bytes of GRBG8 data
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_grbg8_to_rgb24_3x3(const uint8_t grg0[3], const uint8_t bgb1[3],
					    const uint8_t grg2[3], uint8_t rgb24[3])
{
	rgb24[0] = ((uint16_t)grg0[1] + grg2[1]) / 2;
	rgb24[1] = bgb1[1];
	rgb24[2] = ((uint16_t)bgb1[0] + bgb1[2]) / 2;
}

/**
 * @brief Perform a GBRG8 to RGB24 conversion, 3x3 variant.
 *
 * @param gbg0 Row 0 with 3 bytes of GBRG8 data
 * @param rgr1 Row 1 with 3 bytes of GBRG8 data
 * @param gbg2 Row 2 with 3 bytes of GBRG8 data
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_gbrg8_to_rgb24_3x3(const uint8_t gbg0[3], const uint8_t rgr1[3],
					    const uint8_t gbg2[3], uint8_t rgb24[3])
{
	rgb24[0] = ((uint16_t)rgr1[0] + rgr1[2]) / 2;
	rgb24[1] = rgr1[1];
	rgb24[2] = ((uint16_t)gbg0[1] + gbg2[1]) / 2;
}

/**
 * @brief Perform a RGGB8 to RGB24 conversion, 2x2 variant.
 *
 * @param r0 Red value from the bayer pattern.
 * @param g0 Green value from the bayer pattern.
 * @param g1 Green value from the bayer pattern.
 * @param b0 Blue value from the bayer pattern.
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_rggb8_to_rgb24_2x2(uint8_t r0, uint8_t g0, uint8_t g1, uint8_t b0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

/**
 * @brief Perform a GBRG8 to RGB24 conversion, 2x2 variant.
 *
 * @param g1 Green value from the bayer pattern.
 * @param b0 Blue value from the bayer pattern.
 * @param r0 Red value from the bayer pattern.
 * @param g0 Green value from the bayer pattern.
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_gbrg8_to_rgb24_2x2(uint8_t g1, uint8_t b0, uint8_t r0, uint8_t g0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

/**
 * @brief Perform a BGGR8 to RGB24 conversion, 2x2 variant.
 *
 * @param b0 Blue value from the bayer pattern.
 * @param g0 Green value from the bayer pattern.
 * @param g1 Green value from the bayer pattern.
 * @param r0 Red value from the bayer pattern.
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_bggr8_to_rgb24_2x2(uint8_t b0, uint8_t g0, uint8_t g1, uint8_t r0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

/**
 * @brief Perform a GRBG8 to RGB24 conversion, 2x2 variant.
 *
 * @param g1 Green value from the bayer pattern.
 * @param b0 Blue value from the bayer pattern.
 * @param r0 Red value from the bayer pattern.
 * @param g0 Green value from the bayer pattern.
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_grbg8_to_rgb24_2x2(uint8_t g1, uint8_t r0, uint8_t b0, uint8_t g0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

#endif /* ZEPHYR_INCLUDE_PIXEL_BAYER_H */
