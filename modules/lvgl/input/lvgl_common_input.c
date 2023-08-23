/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "lvgl_common_input.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl_input_device.h>

LOG_MODULE_DECLARE(lvgl);

lv_indev_t *lvgl_input_get_indev(const struct device *dev)
{
	struct lvgl_common_input_data *common_data = dev->data;

	return common_data->indev;
}

static void lvgl_input_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	const struct device *dev = drv->user_data;
	const struct lvgl_common_input_config *cfg = dev->config;

	k_msgq_get(cfg->event_msgq, data, K_NO_WAIT);
	data->continue_reading = k_msgq_num_used_get(cfg->event_msgq) > 0;
}

int lvgl_input_register_driver(lv_indev_type_t indev_type, const struct device *dev)
{
	/* Currently no indev binding has its dedicated data
	 * if that ever changes ensure that `lvgl_common_input_data`
	 * remains the first member
	 */
	struct lvgl_common_input_data *common_data = dev->data;

	if (common_data == NULL) {
		return -EINVAL;
	}

	lv_indev_drv_init(&common_data->indev_drv);
	common_data->indev_drv.type = indev_type;
	common_data->indev_drv.read_cb = lvgl_input_read_cb;
	common_data->indev_drv.user_data = (void *)dev;
	common_data->indev = lv_indev_drv_register(&common_data->indev_drv);

	if (common_data->indev == NULL) {
		return -EINVAL;
	}

	return 0;
}
