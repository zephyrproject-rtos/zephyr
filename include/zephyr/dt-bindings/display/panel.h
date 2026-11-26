/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup display_interface
 * @brief Devicetree helpers for display panels.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_PANEL_H_

/**
 * @defgroup display_panel_dt Display panel devicetree bindings
 * @ingroup display_interface
 * @brief Helper macros for display panels in devicetree.
 *
 * @{
 */

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
