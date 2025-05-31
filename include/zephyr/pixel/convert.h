/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_CONVERT_H_
#define ZEPHYR_INCLUDE_PIXEL_CONVERT_H_

#include <stdlib.h>
#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pixel/image.h>

/**
 * @brief Define a new format conversion operation.
 *
 * @param _fn Function converting one input line.
 * @param _fourcc_in The input format for that operation.
 * @param _fourcc_out The Output format for that operation.
 */
#define PIXEL_DEFINE_CONVERT_OPERATION(_fn, _fourcc_in, _fourcc_out)                               \
	static const STRUCT_SECTION_ITERABLE_ALTERNATE(pixel_convert, pixel_operation,             \
						       _fn##_op) = {                               \
		.name = #_fn,                                                                      \
		.fourcc_in = _fourcc_in,                                                           \
		.fourcc_out = _fourcc_out,                                                         \
		.window_size = 1,                                                                  \
		.run = pixel_convert_op,                                                           \
		.arg = _fn,                                                                        \
	}
/**
 * @brief Helper to turn a format conversion function into an operation.
 *
 * The line conversion function is free to perform any processing on the input line.
 * The @c w argument is the width of both the source and destination buffers.
 *
 * The line conversion function is to be provided in @c op->arg
 *
 * @param op Current operation in progress.
 */
void pixel_convert_op(struct pixel_operation *op);

/**
 * @brief Get the luminance (luma channel) of an RGB24 pixel.
 *
 * @param rgb24 Pointer to an RGB24 pixel: red, green, blue channels.
 */
uint8_t pixel_rgb24_get_luma_bt709(const uint8_t rgb24[3]);

/**
 * @brief Convert a line of pixel data from RGB24 to RGB24 (null conversion).
 *
 * See @ref video_pixel_formats for the definition of the input and output formats.
 *
 * You only need to call this function to work directly on raw buffers.
 * See @ref pixel_image_convert for converting between formats.
 *
 * @param src Buffer of the input line in the format, @c XXX in @c pixel_line_XXX_to_YYY().
 * @param dst Buffer of the output line in the format, @c YYY in @c pixel_line_XXX_to_YYY().
 * @param width Width of the lines in number of pixels.
 */
void pixel_line_rgb24_to_rgb24(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB332 to RGB24.
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb332_to_rgb24(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to RGB332 little-endian.
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb24_to_rgb332(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB565 little-endian to RGB24.
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb565le_to_rgb24(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB565 big-endian to RGB24.
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb565be_to_rgb24(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to RGB565 little-endian.
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb24_to_rgb565le(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to RGB565 big-endian.
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb24_to_rgb565be(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUYV to RGB24 (BT.709 coefficients).
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_yuyv_to_rgb24_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to YUYV (BT.709 coefficients).
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb24_to_yuyv_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to YUV24 (BT.709 coefficients).
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb24_to_yuv24_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUV24 to RGB24 (BT.709 coefficients).
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_yuv24_to_rgb24_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUYV to YUV24
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_yuyv_to_yuv24(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUV24 to YUYV
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_yuv24_to_yuyv(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from Y8 to RGB24
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_y8_to_rgb24_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to Y8
 * @copydetails pixel_line_rgb24_to_rgb24()
 */
void pixel_line_rgb24_to_y8_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);

#endif /* ZEPHYR_INCLUDE_PIXEL_CONVERT_H_ */
