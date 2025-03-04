/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_PRINT_H
#define ZEPHYR_INCLUDE_PIXEL_PRINT_H

#include <stddef.h>
#include <stdint.h>

/**
 * Printing images to the terminal for debug and quick insights purpose.
 *
 * The 256COLOR variants use 256COLOR 8-bit RGB format, lower quality, more compact and supported.
 * The TRUECOLOR variants use true 24-bit RGB24 colors, available on a wide range of terminals.
 *
 * @param buf Input buffer to display in the terminal.
 * @param size Size of the input buffer in bytes.
 * @param width Number of pixel of the input buffer in width
 * @param height Max number of rows to print
 * @{
 */
/** Variant for RGB24 input using 256COLOR escape codes. */
void pixel_print_rgb24frame_256color(const uint8_t *buf, size_t size, uint16_t width,
				     uint16_t height);
/** Variant for RGB24 input using TRUECOLOR escape codes. */
void pixel_print_rgb24frame_truecolor(const uint8_t *buf, size_t size, uint16_t width,
				      uint16_t height);
/** Variant for RGB565LE input using 256COLOR escape codes. */
void pixel_print_rgb565leframe_256color(const uint8_t *buf, size_t size, uint16_t width,
					uint16_t height);
/** Variant for RGB565LE input using TRUECOLOR escape codes. */
void pixel_print_rgb565leframe_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					 uint16_t height);
/** Variant for RGB565BE input using 256COLOR escape codes. */
void pixel_print_rgb565beframe_256color(const uint8_t *buf, size_t size, uint16_t width,
					uint16_t height);
/** Variant for RGB565BE input using TRUECOLOR escape codes. */
void pixel_print_rgb565beframe_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					 uint16_t height);
/** Variant for YUYV input with VT601 profile using 256COLOR escape codes. */
void pixel_print_yuyvframe_bt601_256color(const uint8_t *buf, size_t size, uint16_t width,
					  uint16_t height);
/** Variant for YUYV input with VT601 profile using TRUECOLOR escape codes. */
void pixel_print_yuyvframe_bt601_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					   uint16_t height);
/** Variant for YUYV input with VT709 profile using 256COLOR escape codes. */
void pixel_print_yuyvframe_bt709_256color(const uint8_t *buf, size_t size, uint16_t width,
					  uint16_t height);
/** Variant for YUYV input with VT709 profile using TRUECOLOR escape codes. */
void pixel_print_yuyvframe_bt709_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					   uint16_t height);
/** @} */

/**
 * Printing images to the terminal for debug and quick insights purpose.
 *
 * The 256COLOR variants use 256COLOR 8-bit RGB format, lower quality, more compact and supported.
 * The TRUECOLOR variants use true 24-bit RGB24 colors, available on a wide range of terminals.
 *
 * @param buf Input buffer to display in the terminal.
 * @param size Size of the input buffer in bytes.
 * @param width Number of pixel of the input buffer in width
 * @param height Max number of rows to print
 * @{
 */
/* RAW8 pixel format variant */
void pixel_print_raw8frame_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/* RGB24 pixel format variant */
void pixel_print_rgb24frame_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/* RGB565 pixel format variant */
void pixel_print_rgb565frame_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/* YUYV pixel format variant */
void pixel_print_yuyvframe_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/** @} */

/**
 * Printing RGB histograms to the terminal for debug and quick insights purpose.
 *
 * @param rgb24hist Buffer storing 3 histograms one after the other, for the R, G, B channels.
 * @param size Total number of buckets in total contained within @p rgb24hist all channels included.
 * @param height Desired height of the chart in pixels.
 */
void pixel_print_rgb24hist(const uint16_t *rgb24hist, size_t size, uint16_t height);

/**
 * Printing Y histograms to the terminal for debug and quick insights purpose.
 *
 * @param y8hist Buffer storing the histogram for the Y (luma) channel.
 * @param size Total number of buckets in total contained within @p hist.
 * @param height Desired height of the chart in pixels.
 */
void pixel_print_y8hist(const uint16_t *y8hist, size_t size, uint16_t height);

#endif /* ZEPHYR_INCLUDE_PIXEL_PRINT_H */
