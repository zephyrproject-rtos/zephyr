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
 * @brief Print a buffer using higher quality TRUECOLOR terminal escape codes.
 *
 * @param buf Imagme buffer to display in the terminal.
 * @param size Size of the buffer in bytes.
 * @param width Number of pixel of the input buffer in width
 * @param height Max number of rows to print
 * @param fourcc Format of the buffer to print
 */
void pixel_print_buffer_truecolor(const uint8_t *buf, size_t size, uint16_t width,
				  uint16_t height, uint32_t fourcc);
/**
 * @brief Print a buffer using higher speed 256COLOR terminal escape codes.
 * @copydetails pixel_print_buffer_truecolor()
 */
void pixel_print_buffer_256color(const uint8_t *buf, size_t size, uint16_t width,
				 uint16_t height, uint32_t fourcc);

/**
 * @brief Hexdump a buffer in the RAW8 format
 *
 * @param buf Input buffer to display in the terminal.
 * @param size Size of the input buffer in bytes.
 * @param width Number of pixel of the input buffer in width
 * @param height Max number of rows to print
 */
void pixel_hexdump_raw8(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/**
 * @brief Hexdump a buffer in the RGB24 format
 * @copydetails pixel_hexdump_raw8()
 */
void pixel_hexdump_rgb24(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/**
 * @brief Hexdump a buffer in the RGB565 format
 * @copydetails pixel_hexdump_raw8()
 */
void pixel_hexdump_rgb565(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);
/**
 * @brief Hexdump a buffer in the YUYV format
 * @copydetails pixel_hexdump_raw8()
 */
void pixel_hexdump_yuyv(const uint8_t *buf, size_t size, uint16_t width, uint16_t height);

/**
 * @brief Printing RGB histograms to the terminal.
 *
 * @param rgb24hist Buffer storing 3 histograms one after the other, for the R, G, B channels.
 * @param size Total number of buckets in total contained within @p rgb24hist all channels included.
 * @param height Desired height of the chart in pixels.
 */
void pixel_print_rgb24hist(const uint16_t *rgb24hist, size_t size, uint16_t height);

/**
 * @brief Printing Y histograms to the terminal.
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
