/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_

/**
 * @brief LCD Interface
 * @defgroup lcd_interface LCD Interface
 * @ingroup display_interface
 * @{
 */

/**
 * @brief Display pixel formats
 *
 * Display pixel format enumeration.
 *
 * These defines must match those present in the display_pixel_format enum.
 * They are required because the enum cannot be reused within devicetree,
 * since C enums are not valid devicetree syntax.
 */

#define PANEL_PIXEL_FORMAT_RGB_888   (0x1 << 0)
#define PANEL_PIXEL_FORMAT_MONO01    (0x1 << 1) /* 0=Black 1=White */
#define PANEL_PIXEL_FORMAT_MONO10    (0x1 << 2) /* 1=Black 0=White */
#define PANEL_PIXEL_FORMAT_ARGB_8888 (0x1 << 3)
#define PANEL_PIXEL_FORMAT_RGB_565   (0x1 << 4)
#define PANEL_PIXEL_FORMAT_RGB_565X  (0x1 << 5)
#define PANEL_PIXEL_FORMAT_L_8       (0x1 << 6)
#define PANEL_PIXEL_FORMAT_AL_88     (0x1 << 7)
#define PANEL_PIXEL_FORMAT_XRGB_8888 (0x1 << 8)  /**< 32-bit XRGB */
#define PANEL_PIXEL_FORMAT_BGR_888   (0x1 << 9)  /**< 24-bit BGR */
#define PANEL_PIXEL_FORMAT_ABGR_8888 (0x1 << 10) /**< 32-bit ABGR */
#define PANEL_PIXEL_FORMAT_RGBA_8888 (0x1 << 11) /**< 32-bit RGBA */
#define PANEL_PIXEL_FORMAT_BGRA_8888 (0x1 << 12) /**< 32-bit BGRA */

/**
 * @brief Pack color components into a 32-bit ARGB8888 value.
 *
 * This macro is intended for use in devicetree files to specify color values.
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 */
#define ARGB8888(r, g, b, a) (((b) << 0) | ((g) << 8) | ((r) << 16) | ((a) << 24))

/**
 * @brief Panel color palette null value
 *
 * This value is used to fill empty entries in case of
 * non-contiguous color palette entries.
 */
#define PANEL_COLOR_PALETTE_NULL 0

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_ */
