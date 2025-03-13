/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_ZEPHYR_H_
#define ZEPHYR_MODULES_LVGL_ZEPHYR_H_

#include <zephyr/kernel.h>

#if DT_ZEPHYR_DISPLAYS_COUNT == 0
#error Could not find "zephyr,display" chosen property, or node with compatible "zephyr,displays"
#endif /* DT_ZEPHYR_DISPLAYS_COUNT == 0 */

#define LV_DISPLAY_IDX_MACRO(i, _) _##i

#define LV_DISPLAYS_IDX_LIST LISTIFY(DT_ZEPHYR_DISPLAYS_COUNT, LV_DISPLAY_IDX_MACRO, (,))

#define LVGL_DISPLAY_OBJ(n) lvgl_display[n]

#ifdef __cplusplus
extern "C" {
#endif

extern lv_display_t *lvgl_display[];

/**
 * @brief Initialize the LittlevGL library
 *
 * This function initializes the LittlevGL library and setups the display and input devices.
 * If `LV_Z_AUTO_INIT` is disabled it must be called before any other LittlevGL function.
 *
 * @return 0 on success, negative errno code on failure
 */
int lvgl_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_ZEPHYR_H_ */
