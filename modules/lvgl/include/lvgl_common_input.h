/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_LVGL_COMMON_INPUT_H_
#define ZEPHYR_MODULES_LVGL_LVGL_COMMON_INPUT_H_

#include <lvgl.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lvgl_common_input_config {
	struct k_msgq *event_msgq;
};

struct lvgl_common_input_data {
	lv_indev_drv_t indev_drv;
	lv_indev_t *indev;
	lv_indev_data_t pending_event;
	lv_indev_data_t previous_event;
};

int lvgl_input_register_driver(lv_indev_type_t indev_type, const struct device *dev);
int lvgl_init_input_devices(void);

#define LVGL_INPUT_EVENT_MSGQ(inst, type) lvgl_input_msgq_##type##_##inst
#define LVGL_INPUT_DEVICE(inst)           DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, input))

#define LVGL_COORD_VALID(coord) IN_RANGE(coord, LV_COORD_MIN, LV_COORD_MAX)
#define LVGL_KEY_VALID(key)     IN_RANGE(key, 0, UINT8_MAX)

#define LVGL_INPUT_DEFINE(inst, type, msgq_size, process_evt_cb)                                   \
	INPUT_CALLBACK_DEFINE(LVGL_INPUT_DEVICE(inst), process_evt_cb,                             \
			      (void *)DEVICE_DT_INST_GET(inst));                                   \
	K_MSGQ_DEFINE(lvgl_input_msgq_##type##_##inst, sizeof(lv_indev_data_t), msgq_size, 4)

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_MODULES_LVGL_LVGL_COMMON_INPUT_H_ */
