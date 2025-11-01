/*
 * Copyright 2023 NXP
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
 * since enum definitions are not supported by devicetree tooling.
 */

#define PANEL_PIXEL_FORMAT_RGB_888   (0x1 << 0)
#define PANEL_PIXEL_FORMAT_MONO01    (0x1 << 1) /* 0=Black 1=White */
#define PANEL_PIXEL_FORMAT_MONO10    (0x1 << 2) /* 1=Black 0=White */
#define PANEL_PIXEL_FORMAT_ARGB_8888 (0x1 << 3)
#define PANEL_PIXEL_FORMAT_RGB_565   (0x1 << 4)
#define PANEL_PIXEL_FORMAT_BGR_565   (0x1 << 5)
#define PANEL_PIXEL_FORMAT_L_8       (0x1 << 6)
#define PANEL_PIXEL_FORMAT_AL_88     (0x1 << 7)
#define PANEL_PIXEL_FORMAT_XRGB_8888 (0x1 << 8)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_ */
