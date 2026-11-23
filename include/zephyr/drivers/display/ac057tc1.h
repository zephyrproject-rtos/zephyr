/*
 * Copyright (c) 2026 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup display_interface
 * @brief Palette definitions for AC057TC1 7-color e-ink display
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
 * @name AC057TC1 Color Definitions
 * @{
 *
 * The display supports 7 colors, mapped via 3-bit indices (0-6).
 * The color palette in display_capabilities provides the ARGB8888 representation
 * of these colors for UI rendering (e.g., with LVGL).
 *
 * The controller uses the index values directly when rendering; conversion from
 * ARGB8888 to the hardware's color representation is application-specific.
 */
#define AC057TC1_COLOR_BLACK        0x00 /**< Black color index */
#define AC057TC1_COLOR_WHITE        0x01 /**< White color index */
#define AC057TC1_COLOR_GREEN        0x02 /**< Green color index */
#define AC057TC1_COLOR_BLUE         0x03 /**< Blue color index */
#define AC057TC1_COLOR_RED          0x04 /**< Red color index */
#define AC057TC1_COLOR_YELLOW       0x05 /**< Yellow color index */
#define AC057TC1_COLOR_ORANGE       0x06 /**< Orange color index */

#define AC057TC1_COLOR_PALETTE_SIZE 7    /**< Number of colors in palette */
/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_AC057TC1_H_ */
