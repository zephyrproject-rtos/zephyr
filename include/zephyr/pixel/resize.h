/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_RESIZE_H_
#define ZEPHYR_INCLUDE_PIXEL_RESIZE_H_

#include <stdint.h>

#include <zephyr/pixel/stream.h>

/**
 * @brief Resize an RGB24 line by subsampling the pixels horizontally.
 *
 * @param src_buf Input buffer to resize
 * @param src_width Number of pixels to resize to a different format.
 * @param dst_buf Output buffer in which the stretched/compressed is stored.
 * @param dst_width Number of pixels in the output buffer.
 */
void pixel_subsample_rgb24line(const uint8_t *src_buf, size_t src_width, uint8_t *dst_buf,
			       size_t dst_width);
/**
 * @brief Resize an RGB565BE/RGB565LE line by subsampling the pixels horizontally.
 * @copydetails pixel_subsample_rgb24line()
 */
void pixel_subsample_rgb565line(const uint8_t *src_buf, size_t src_width, uint8_t *dst_buf,
				size_t dst_width);

/**
 * @brief Resize an RGB24 frame by subsampling the pixels horizontally/vertically.
 *
 * @param src_buf Input buffer to resize
 * @param src_width Width of the input in number of pixels.
 * @param src_height Height of the input in number of pixels.
 * @param dst_buf Output buffer in which the stretched/compressed is stored.
 * @param dst_width Width of the output in number of pixels.
 * @param dst_height Height of the output in number of pixels.
 */
void pixel_subsample_rgb24frame(const uint8_t *src_buf, size_t src_width, size_t src_height,
				uint8_t *dst_buf, size_t dst_width, size_t dst_height);
/**
 * @brief Resize an RGB565BE/RGB565LE frame by subsampling the pixels horizontally/vertically.
 * @copydetails pixel_subsample_rgb24frame()
 */
void pixel_subsample_rgb565frame(const uint8_t *src_buf, size_t src_width, size_t src_height,
				 uint8_t *dst_buf, size_t dst_width, size_t dst_height);

/**
 * @brief Resize an RGB24 stream by subsampling the pixels vertically/horizontally.
 *
 * @note the "width" and "height" macro parameters are for the input like any stream element.
 *       The output size is configured implicitly by connecting this block to another one of a
 *       different size.
 *
 * @param name The symbol of the @ref pixel_stream that will be defined.
 * @param width The total width of the input frame in number of pixels.
 * @param height The total height of the input frame in number of pixels.
 */
#define PIXEL_SUBSAMPLE_RGB24STREAM(name, width, height)                                           \
	PIXEL_RESIZE_DEFINE(name, pixel_subsample_rgb24stream, (width), (height), 3)
/**
 * @brief Resize an RGB565BE/RGB565LE stream by subsampling the pixels vertically/horizontally.
 * @copydetails PIXEL_SUBSAMPLE_RGB24STREAM()
 */
#define PIXEL_SUBSAMPLE_RGB565STREAM(name, width, height)                                          \
	PIXEL_RESIZE_DEFINE(name, pixel_subsample_rgb565stream, (width), (height), 2)

/** @cond INTERNAL_HIDDEN */
#define PIXEL_RESIZE_DEFINE(name, fn, width, height, bytes_per_pixel)                              \
	PIXEL_STREAM_DEFINE(name, (fn), (width), (height),                                         \
			    (width) * (bytes_per_pixel), 1 * (width) * (bytes_per_pixel))
void pixel_subsample_rgb24stream(struct pixel_stream *strm);
void pixel_subsample_rgb565stream(struct pixel_stream *strm);
/** @endcond */

#endif /* ZEPHYR_INCLUDE_PIXEL_RESIZE_H_ */
