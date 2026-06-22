/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup display_interface
 * @brief Palette definitions for ED2208-GCA (EL073TF1) Spectra 6 e-ink display
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_ED2208_GCA_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_ED2208_GCA_H_

/**
 * @defgroup ed2208_gca_display_interface ED2208-GCA (EL073TF1) display controller
 * @brief ED2208-GCA 6-color e-ink display controller device-specific API extension
 * @since 4.5.0
 * @version 0.1.0
 * @ingroup display_interface
 * @{
 */

#include <zephyr/drivers/display.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name ED2208-GCA (EL073TF1) Color Definitions
 * @{
 */
#define ED2208_GCA_COLOR_BLACK  0x00 /**< Black color index */
#define ED2208_GCA_COLOR_WHITE  0x01 /**< White color index */
#define ED2208_GCA_COLOR_YELLOW 0x02 /**< Yellow color index */
#define ED2208_GCA_COLOR_RED    0x03 /**< Red color index */
#define ED2208_GCA_COLOR_BLUE   0x05 /**< Blue color index */
#define ED2208_GCA_COLOR_GREEN  0x06 /**< Green color index */

#define ED2208_GCA_COLOR_PALETTE_SIZE 7    /**< Number of colors in palette, including empty slot */
/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_ED2208_GCA_H_ */
