/*
 * Copyright (c) 2026 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup display_interface
 * @brief Public API for AC057TC1 7-color e-ink display
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_AC057TC1_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_AC057TC1_H_

/**
 * @brief AC057TC1 7-color e-ink display controller device-specific API extension
 * @defgroup ac057tc1_display_interface AC057TC1 display controller
 * @since 4.4.0
 * @version 0.1.0
 * @ingroup display_interface
 * @{
 */

#include <zephyr/drivers/display.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AC057TC1 private pixel format
 *
 * 4 bits per pixel, 2 pixels per byte (high nibble first).
 * Supports 7 colors indexed 0-6 (3-bit color palette).
 */
#define PIXEL_FORMAT_L_4 (PIXEL_FORMAT_PRIV_START << 0)

/**
 * @brief Bits per pixel for AC057TC1 format
 */
#define AC057TC1_BITS_PER_PIXEL 4

/**
 * @name AC057TC1 Color Definitions
 * @{
 */
#define AC057TC1_COLOR_BLACK  0x00 /**< Black color index */
#define AC057TC1_COLOR_WHITE  0x01 /**< White color index */
#define AC057TC1_COLOR_GREEN  0x02 /**< Green color index */
#define AC057TC1_COLOR_BLUE   0x03 /**< Blue color index */
#define AC057TC1_COLOR_RED    0x04 /**< Red color index */
#define AC057TC1_COLOR_YELLOW 0x05 /**< Yellow color index */
#define AC057TC1_COLOR_ORANGE 0x06 /**< Orange color index */
/** @} */

/**
 * @brief Pack two AC057TC1 pixels into a single byte
 *
 * @param pixel1 First pixel (high nibble, left pixel)
 * @param pixel2 Second pixel (low nibble, right pixel)
 * @return Packed byte containing both pixels
 */
#define AC057TC1_PACK_PIXELS(pixel1, pixel2) (((pixel1) << 4) | ((pixel2) & 0x0F))

/**
 * @brief Calculate buffer size needed for AC057TC1 framebuffer
 *
 * @param width Display width in pixels
 * @param height Display height in pixels
 * @return Size in bytes needed for framebuffer
 */
#define AC057TC1_BUFFER_SIZE(width, height) (((width) * (height)) / 2)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_AC057TC1_H_ */
