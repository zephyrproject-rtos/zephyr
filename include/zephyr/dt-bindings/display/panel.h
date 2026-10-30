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

#include <zephyr/media/formats.h>

/**
 * @brief Compatibility header for panel pixel formats.
 */
#define PANEL_PIXEL_FORMAT_RGB_888   PIXFMT_BGR24
#define PANEL_PIXEL_FORMAT_MONO01    PIXFMT_MONO01
#define PANEL_PIXEL_FORMAT_MONO10    PIXFMT_MONO10
#define PANEL_PIXEL_FORMAT_ARGB_8888 PIXFMT_BGRA32
#define PANEL_PIXEL_FORMAT_RGB_565   PIXFMT_RGB565
#define PANEL_PIXEL_FORMAT_RGB_565X  PIXFMT_RGB565X
#define PANEL_PIXEL_FORMAT_L_8       PIXFMT_GREY
#define PANEL_PIXEL_FORMAT_AL_88     PIXFMT_AL88
#define PANEL_PIXEL_FORMAT_XRGB_8888 PIXFMT_BGRX32
#define PANEL_PIXEL_FORMAT_BGR_888   PIXFMT_RGB24
#define PANEL_PIXEL_FORMAT_ABGR_8888 PIXFMT_RGBA32
#define PANEL_PIXEL_FORMAT_RGBA_8888 PIXFMT_ABGR32
#define PANEL_PIXEL_FORMAT_BGRA_8888 PIXFMT_ARGB32
#define PANEL_PIXEL_FORMAT_I_4       PIXFMT_I4
#define PANEL_PIXEL_FORMAT_L_4       PIXFMT_L4
#define PANEL_PIXEL_FORMAT_YUYV      PIXFMT_YUYV

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
