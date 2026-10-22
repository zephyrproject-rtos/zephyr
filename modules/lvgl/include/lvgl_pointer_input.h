/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_LVGL_POINTER_INPUT_H_
#define ZEPHYR_MODULES_LVGL_LVGL_POINTER_INPUT_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

int lvgl_pointer_input_init(const struct device *dev);

#ifdef CONFIG_LV_Z_POINTER_FROM_CHOSEN_TOUCH

const struct device *lvgl_pointer_from_chosen_get(void);

#endif /* CONFIG_LV_Z_POINTER_FROM_CHOSEN_TOUCH */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_LVGL_POINTER_INPUT_H_ */
