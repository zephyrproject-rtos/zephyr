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
#include "lvgl_pointer_input.h"
#include "lvgl_button_input.h"
#include "lvgl_encoder_input.h"
#include "lvgl_keypad_input.h"

LOG_MODULE_DECLARE(lvgl, CONFIG_LV_Z_LOG_LEVEL);

lv_indev_t *lvgl_input_get_indev(const struct device *dev)
{
	struct lvgl_common_input_data *common_data = dev->data;

	return common_data->indev;
}

static void lvgl_input_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	const struct device *dev = drv->user_data;
	const struct lvgl_common_input_config *cfg = dev->config;
	struct lvgl_common_input_data *common_data = dev->data;

	if (k_msgq_get(cfg->event_msgq, data, K_NO_WAIT) != 0) {
		memcpy(data, &common_data->previous_event, sizeof(lv_indev_data_t));
		if (drv->type == LV_INDEV_TYPE_ENCODER) {
			data->enc_diff = 0; /* For encoders, clear last movement */
		}
		data->continue_reading = false;
		return;
	}

	memcpy(&common_data->previous_event, data, sizeof(lv_indev_data_t));
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

#define LV_DEV_INIT(node_id, init_fn)                                                              \
	do {                                                                                       \
		int ret = init_fn(DEVICE_DT_GET(node_id));                                         \
		if (ret) {                                                                         \
			return ret;                                                                \
		}                                                                                  \
	} while (0)

int lvgl_init_input_devices(void)
{
#ifdef CONFIG_LV_Z_POINTER_INPUT
	DT_FOREACH_STATUS_OKAY_VARGS(zephyr_lvgl_pointer_input, LV_DEV_INIT,
				     lvgl_pointer_input_init);
#endif /* CONFIG_LV_Z_POINTER_INPUT */

#ifdef CONFIG_LV_Z_BUTTON_INPUT
	DT_FOREACH_STATUS_OKAY_VARGS(zephyr_lvgl_button_input, LV_DEV_INIT, lvgl_button_input_init);
#endif /* CONFIG_LV_Z_BUTTON_INPUT */

#ifdef CONFIG_LV_Z_ENCODER_INPUT
	DT_FOREACH_STATUS_OKAY_VARGS(zephyr_lvgl_encoder_input, LV_DEV_INIT,
				     lvgl_encoder_input_init);
#endif /* CONFIG_LV_Z_ENCODER_INPUT */

#ifdef CONFIG_LV_Z_KEYPAD_INPUT
	DT_FOREACH_STATUS_OKAY_VARGS(zephyr_lvgl_keypad_input, LV_DEV_INIT, lvgl_keypad_input_init);
#endif /* CONFIG_LV_Z_KEYPAD_INPUT */

	return 0;
}
