/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup display_interface
 * @brief Devicetree pixel format identifiers for display panels.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_

/**
 * @defgroup display_panel_dt Display panel devicetree bindings
 * @ingroup display_interface
 * @brief Bitmasks for panel pixel formats in devicetree.
 *
 * Each @c PANEL_PIXEL_FORMAT_* value uses the same bit position as the matching @c PIXEL_FORMAT_*
 * entry in @ref display_pixel_format.
 *
 * These macros exist because C enumerators cannot be used in devicetree source files.
 *
 * @see display_pixel_format
 * @{
 */

#define PANEL_PIXEL_FORMAT_RGB_888   (0x1 << 0)  /**< 24-bit RGB (8 bits per component) */
#define PANEL_PIXEL_FORMAT_MONO01    (0x1 << 1)  /**< 1 bpp monochrome (0=black, 1=white) */
#define PANEL_PIXEL_FORMAT_MONO10    (0x1 << 2)  /**< 1 bpp monochrome (1=black, 0=white) */
#define PANEL_PIXEL_FORMAT_ARGB_8888 (0x1 << 3)  /**< 32-bit ARGB (8 bits per component) */
#define PANEL_PIXEL_FORMAT_RGB_565   (0x1 << 4)  /**< 16-bit RGB 5:6:5 */
#define PANEL_PIXEL_FORMAT_RGB_565X  (0x1 << 5)  /**< Byte-swapped 16-bit RGB 5:6:5 */
#define PANEL_PIXEL_FORMAT_L_8       (0x1 << 6)  /**< 8-bit luminance/grayscale */
#define PANEL_PIXEL_FORMAT_AL_88     (0x1 << 7)  /**< 16-bit luminance and alpha (8 bits each) */
#define PANEL_PIXEL_FORMAT_XRGB_8888 (0x1 << 8)  /**< 32-bit XRGB (8 bits per component) */
#define PANEL_PIXEL_FORMAT_BGR_888   (0x1 << 9)  /**< 24-bit BGR (8 bits per component) */
#define PANEL_PIXEL_FORMAT_ABGR_8888 (0x1 << 10) /**< 32-bit ABGR (8 bits per component) */
#define PANEL_PIXEL_FORMAT_RGBA_8888 (0x1 << 11) /**< 32-bit RGBA (8 bits per component) */
#define PANEL_PIXEL_FORMAT_BGRA_8888 (0x1 << 12) /**< 32-bit BGRA (8 bits per component) */
#define PANEL_PIXEL_FORMAT_I_4       (0x1 << 13) /**< 4-bit indexed color */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_ */
