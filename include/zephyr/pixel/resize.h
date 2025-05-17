/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_RESIZE_H_
#define ZEPHYR_INCLUDE_PIXEL_RESIZE_H_

#include <stdint.h>

#include <zephyr/pixel/operation.h>

/**
 * @brief Define a format conversion operation.
 *
 * Invoking this macro suffices for @ref pixel_image_convert() to include the extra format.
 *
 * @param _fn Function converting one input line.
 * @param _fourcc The pixel format of the data resized.
 */
#define PIXEL_DEFINE_RESIZE_OPERATION(_fn, _fourcc)                                                \
	static const STRUCT_SECTION_ITERABLE_ALTERNATE(pixel_resize, pixel_operation,              \
						       pixel_resize_op##_fourcc) = {               \
		.name = #_fn,                                                                      \
		.fourcc_in = _fourcc,                                                              \
		.fourcc_out = _fourcc,                                                             \
		.window_size = 1,                                                                  \
		.run = _fn,                                                                        \
	}

/**
 * @brief Resize an 24-bit per pixel frame by subsampling the pixels horizontally/vertically.
 *
 * @param src_buf Input buffer to resize
 * @param src_width Width of the input in number of pixels.
 * @param src_height Height of the input in number of pixels.
 * @param dst_buf Output buffer in which the stretched/compressed is stored.
 * @param dst_width Width of the output in number of pixels.
 * @param dst_height Height of the output in number of pixels.
 */
void pixel_resize_frame_raw24(const uint8_t *src_buf, size_t src_width, size_t src_height,
			      uint8_t *dst_buf, size_t dst_width, size_t dst_height);
/**
 * @brief Resize an 16-bit per pixel frame by subsampling the pixels horizontally/vertically.
 * @copydetails pixel_resize_frame_raw24()
 */
void pixel_resize_frame_raw16(const uint8_t *src_buf, size_t src_width, size_t src_height,
			      uint8_t *dst_buf, size_t dst_width, size_t dst_height);
/**
 * @brief Resize an 8-bit per pixel frame by subsampling the pixels horizontally/vertically.
 * @copydetails pixel_resize_frame_raw24()
 */
void pixel_resize_frame_raw8(const uint8_t *src_buf, size_t src_width, size_t src_height,
			      uint8_t *dst_buf, size_t dst_width, size_t dst_height);

#endif /* ZEPHYR_INCLUDE_PIXEL_RESIZE_H_ */
