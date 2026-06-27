/*
 * Copyright (c) 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup lvgl_keypad_dt
 * @brief Devicetree definitions for LVGL keypad input keys.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_LVGL_LVGL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_LVGL_LVGL_H_

/**
 * @defgroup lvgl_keypad_dt LVGL keypad keys
 * @ingroup input_interface
 * @brief Predefined LVGL keys for use in Devicetree.
 *
 * Used in the @c lvgl-codes property of a <tt>zephyr,lvgl-keypad-input</tt> node to map input event
 * codes to LVGL keys. Values mirror <tt>enum _lv_key_t</tt> from LVGL's @c lv_group.h.
 *
 * Example devicetree usage:
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/input/input-event-codes.h>
 * #include <zephyr/dt-bindings/lvgl/lvgl.h>
 *
 * keypad {
 *         compatible = "zephyr,lvgl-keypad-input";
 *         input = <&buttons>;
 *         input-codes = <INPUT_KEY_1 INPUT_KEY_2>;
 *         lvgl-codes = <LV_KEY_NEXT LV_KEY_PREV>;
 * };
 * @endcode
 *
 * @see https://lvgl.io/docs/open/main-modules/indev/keypad#keys
 * @{
 */

#define LV_KEY_UP        17  /**< Move up or increase value. */
#define LV_KEY_DOWN      18  /**< Move down or decrease value. */
#define LV_KEY_RIGHT     19  /**< Move right or increase value. */
#define LV_KEY_LEFT      20  /**< Move left or decrease value. */
#define LV_KEY_ESC       27  /**< Close or exit (e.g. leave edit mode). */
#define LV_KEY_DEL       127 /**< Delete. */
#define LV_KEY_BACKSPACE 8   /**< Delete the character to the left. */
#define LV_KEY_ENTER     10  /**< Confirm, select, or toggle edit mode. */
#define LV_KEY_NEXT      9   /**< Focus the next object in the group. */
#define LV_KEY_PREV      11  /**< Focus the previous object in the group. */
#define LV_KEY_HOME      2   /**< Go to the beginning/top. */
#define LV_KEY_END       3   /**< Go to the end/bottom. */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_LVGL_LVGL_H_ */
