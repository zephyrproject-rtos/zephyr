/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_KERNEL_H_
#define ZEPHYR_INCLUDE_PIXEL_KERNEL_H_

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/pixel/stream.h>

/**
 * @brief Define a 3x3 identity kernel operation for RGB24 data.
 *
 * @param name The symbol of the @ref pixel_stream that will be defined.
 * @param width The total width of the input frame in number of pixels.
 * @param height The total height of the input frame in number of pixels.
 */
#define PIXEL_IDENTITY_RGB24STREAM_3X3(name, width, height)                                        \
	PIXEL_KERNEL_3X3_DEFINE(name, pixel_identity_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a 5x5 identity kernel operation for RGB24 data.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_IDENTITY_RGB24STREAM_5X5(name, width, height)                                        \
	PIXEL_KERNEL_5X5_DEFINE(name, pixel_identity_rgb24stream_5x5, (width), (height), 3)
/**
 * @brief Define a 3x3 sharpen kernel operation for RGB24 data.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_SHARPEN_RGB24STREAM_3X3(name, width, height)                                         \
	PIXEL_KERNEL_3X3_DEFINE(name, pixel_sharpen_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a 3x3 edge detection kernel operation for RGB24 data.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_EDGEDETECT_RGB24STREAM_3X3(name, width, height)                                      \
	PIXEL_KERNEL_3X3_DEFINE(name, pixel_edgedetect_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a 3x3 gaussian blur kernel operation for RGB24 data.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_GAUSSIANBLUR_RGB24STREAM_5X5(name, width, height)                                    \
	PIXEL_KERNEL_5X5_DEFINE(name, pixel_gaussianblur_rgb24stream_5x5, (width), (height), 3)
/**
 * @brief Define a 3x3 gaussian blur kernel operation for RGB24 data.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_GAUSSIANBLUR_RGB24STREAM_3X3(name, width, height)                                    \
	PIXEL_KERNEL_3X3_DEFINE(name, pixel_gaussianblur_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a 5x5 unsharp kernel operation for RGB24 data to sharpen it.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_UNSHARP_RGB24STREAM_5X5(name, width, height)                                         \
	PIXEL_KERNEL_5X5_DEFINE(name, pixel_unsharp_rgb24stream_5x5, (width), (height), 3)
/**
 * @brief Define a 3x3 median kernel operation for RGB24 data.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_MEDIAN_RGB24STREAM_3X3(name, width, height)                                          \
	PIXEL_KERNEL_3X3_DEFINE(name, pixel_median_rgb24stream_3x3, (width), (height), 3)
/**
 * @brief Define a 5x5 median kernel operation for RGB24 data.
 * @copydetails PIXEL_RGGB8STREAM_TO_RGB24STREAM_3X3()
 */
#define PIXEL_MEDIAN_RGB24STREAM_5X5(name, width, height)                                          \
	PIXEL_KERNEL_5X5_DEFINE(name, pixel_median_rgb24stream_5x5, (width), (height), 3)

/**
 * @brief Apply a 5x5 identity kernel to an RGB24 input window and produce one RGB24 line.
 *
 * @param in Array of input line buffers to convert.
 * @param out Pointer to the output line converted.
 * @param width Width of the input and output lines in pixels.
 */
void pixel_identity_rgb24line_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 identity kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24line_3x3()
 */
void pixel_identity_rgb24line_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 sharpen kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24line_3x3()
 */
void pixel_sharpen_rgb24line_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 edge detection kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24line_3x3()
 */
void pixel_edgedetect_rgb24line_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 gaussian blur kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24line_3x3()
 */
void pixel_gaussianblur_rgb24line_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 5x5 unsharp kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24line_3x3()
 */
void pixel_unsharp_rgb24line_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 median denoise kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24line_3x3()
 */
void pixel_median_rgb24line_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 5x5 median denoise kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24line_3x3()
 */
void pixel_median_rgb24line_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width);

/** @cond INTERNAL_HIDDEN */
#define PIXEL_KERNEL_5X5_DEFINE(name, fn, width, height, bytes_per_pixel)                          \
	PIXEL_STREAM_DEFINE((name), (fn), (width), (height), (width) * (bytes_per_pixel),          \
			    5 * (width) * (bytes_per_pixel))
#define PIXEL_KERNEL_3X3_DEFINE(name, fn, width, height, bytes_per_pixel)                          \
	PIXEL_STREAM_DEFINE((name), (fn), (width), (height), (width) * (bytes_per_pixel),          \
			    3 * (width) * (bytes_per_pixel))
void pixel_identity_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_identity_rgb24stream_5x5(struct pixel_stream *strm);
void pixel_sharpen_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_edgedetect_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_gaussianblur_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_gaussianblur_rgb24stream_5x5(struct pixel_stream *strm);
void pixel_unsharp_rgb24stream_5x5(struct pixel_stream *strm);
void pixel_median_rgb24stream_3x3(struct pixel_stream *strm);
void pixel_median_rgb24stream_5x5(struct pixel_stream *strm);
/** @endcond */

#endif /* ZEPHYR_INCLUDE_PIXEL_KERNEL_H_ */
