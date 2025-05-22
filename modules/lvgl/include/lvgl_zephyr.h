/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_ZEPHYR_H_
#define ZEPHYR_MODULES_LVGL_ZEPHYR_H_

#include <zephyr/kernel.h>

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_ZEPHYR_H_ */
