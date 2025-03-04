/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_PRINT_H_
#define ZEPHYR_INCLUDE_PIXEL_PRINT_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/shell/shell.h>

/**
 * @brief Show a frame in the RGB24 format using 256COLOR terminal escape codes
 *
 * The 256COLOR variants use 256COLOR 8-bit RGB format, lower quality, more compact and supported.
 * The TRUECOLOR variants use true 24-bit RGB24 colors, available on a wide range of terminals.
 *
 * @param buf Input buffer to display in the terminal.
 * @param size Size of the input buffer in bytes.
 * @param width Number of pixel of the input buffer in width
 * @param height Max number of rows to print
 */
void pixel_print_rgb24frame_256color(const uint8_t *buf, size_t size, uint16_t width,
				     uint16_t height);
/**
 * @brief Show a frame in the RGB24 format using TRUECOLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_rgb24frame_truecolor(const uint8_t *buf, size_t size, uint16_t width,
				      uint16_t height);
/**
 * @brief Show a frame in the RGB565 little-endian format using 256COLOR terminal escape codes
 */
void pixel_print_rgb565leframe_256color(const uint8_t *buf, size_t size, uint16_t width,
					uint16_t height);
/**
 * @brief Show a frame in the RGB565 little-endian format using TRUECOLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_rgb565leframe_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					 uint16_t height);
/**
 * @brief Show a frame in the RGB565 big-endian format using 256COLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_rgb565beframe_256color(const uint8_t *buf, size_t size, uint16_t width,
					uint16_t height);
/**
 * @brief Show a frame in the RGB565 big-endian format using TRUECOLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_rgb565beframe_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					 uint16_t height);
/**
 * @brief Show a frame in the RGB332 format using 256COLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_rgb332frame_256color(const uint8_t *buf, size_t size, uint16_t width,
				      uint16_t height);
/**
 * @brief Show a frame in the RGB332 format using TRUECOLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_rgb332frame_truecolor(const uint8_t *buf, size_t size, uint16_t width,
				       uint16_t height);
/**
 * @brief Show a frame in the YUYV (BT.709) format using 256COLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_yuyvframe_bt709_256color(const uint8_t *buf, size_t size, uint16_t width,
					  uint16_t height);
/**
 * @brief Show a frame in the YUYV (BT.709) format using TRUECOLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_yuyvframe_bt709_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					   uint16_t height);
/**
 * @brief Show a frame in the YUV24 (BT.709) format using 256COLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_yuv24frame_bt709_256color(const uint8_t *buf, size_t size, uint16_t width,
					   uint16_t height);
/**
 * @brief Show a frame in the YUV24 (BT.709) format using TRUECOLOR terminal escape codes
 * @copydetails pixel_print_rgb24frame_256color()
 */
void pixel_print_yuv24frame_bt709_truecolor(const uint8_t *buf, size_t size, uint16_t width,
					    uint16_t height);

/**
 * @brief Hexdump a frame with pixels in the RAW8 format
 *
 * The 256COLOR variants use 256COLOR 8-bit RGB format, lower quality, more compact and supported.
 * The TRUECOLOR variants use true 24-bit RGB24 colors, available on a wide range of terminals.
 *
 * @param buf Input buffer to display in the terminal.
 * @param size Size of the input buffer in bytes.
 * @param width Number of pixel of the input buffer in width
 * @param height Max number of rows to print
 */
void pixel_print_raw8frame_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/**
 * @brief Hexdump a frame with pixels in the RGB24 format
 * @copydetails pixel_print_raw8frame_hex()
 */
void pixel_print_rgb24frame_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/**
 * @brief Hexdump a frame with pixels in the RGB565 format
 * @copydetails pixel_print_raw8frame_hex()
 */
void pixel_print_rgb565frame_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/**
 * @brief Hexdump a frame with pixels in the YUYV format
 * @copydetails pixel_print_raw8frame_hex()
 */
void pixel_print_yuyvframe_hex(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);

/**
 * @brief Printing RGB histograms to the terminal for debug and quick insights purpose.
 *
 * @param rgb24hist Buffer storing 3 histograms one after the other, for the R, G, B channels.
 * @param size Total number of buckets in total contained within @p rgb24hist all channels included.
 * @param height Desired height of the chart in pixels.
 */
void pixel_print_rgb24hist(const uint16_t *rgb24hist, size_t size, uint16_t height);

/**
 * @brief Printing Y histograms to the terminal for debug and quick insights purpose.
 *
 * @param y8hist Buffer storing the histogram for the Y (luma) channel.
 * @param size Total number of buckets in total contained within @p hist.
 * @param height Desired height of the chart in pixels.
 */
void pixel_print_y8hist(const uint16_t *y8hist, size_t size, uint16_t height);

/**
 * @brief Set the shell instance to use when printing via the shell back-end.
 *
 * @see CONFIG_PIXEL_PRINT
 *
 * @param sh Shell instance set as a global variable.
 */
void pixel_print_set_shell(struct shell *sh);

#endif /* ZEPHYR_INCLUDE_PIXEL_PRINT_H_ */
