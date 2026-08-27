/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_INPUT_DEVICE_H_
#define ZEPHYR_MODULES_LVGL_INPUT_DEVICE_H_

#include <zephyr/device.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_indev_t *lvgl_input_get_indev(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_INPUT_DEVICE_H_ */
