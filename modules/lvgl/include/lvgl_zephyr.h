/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * Copyright (c) 2025 Abderrahmane JARMOUNI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_ZEPHYR_H_
#define ZEPHYR_MODULES_LVGL_ZEPHYR_H_

#include <zephyr/kernel.h>

#if DT_ZEPHYR_DISPLAYS_COUNT == 0
#error Could not find "zephyr,display" chosen property, or "zephyr,displays" compatible node in DT
#endif /* DT_ZEPHYR_DISPLAYS_COUNT == 0 */

#define LV_DISPLAY_IDX_MACRO(i, _) _##i

#define LV_DISPLAYS_IDX_LIST LISTIFY(DT_ZEPHYR_DISPLAYS_COUNT, LV_DISPLAY_IDX_MACRO, (,))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LittlevGL library
 *
 * This function initializes the LittlevGL library and setups the display and input devices.
 * If `LV_Z_AUTO_INIT` is disabled it must be called before any other LittlevGL function.
 *
 * @return 0 on success, negative errno code on failure
 */
int lvgl_init(void);

#ifdef CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE
/**
 * @brief Gets the instance of LVGL's workqueue
 *
 * This function returns the workqueue instance used
 * by LVGL core, allowing user to submit their own
 * work items to it.
 *
 * @return pointer to LVGL's workqueue instance
 */
struct k_work_q *lvgl_get_workqueue(void);

#endif /* CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_ZEPHYR_H_ */
