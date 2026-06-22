/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 * Copyright 2025 Abderrahmane JARMOUNI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "lvgl_common_input.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>
#include <lvgl_display.h>
#include <zephyr/logging/log.h>
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

static void lvgl_input_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	const struct device *dev = lv_indev_get_user_data(indev);
	const struct lvgl_common_input_config *cfg = dev->config;
	struct lvgl_common_input_data *common_data = dev->data;

	if (k_msgq_get(cfg->event_msgq, data, K_NO_WAIT) != 0) {
		memcpy(data, &common_data->previous_event, sizeof(lv_indev_data_t));
		if (lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
			data->enc_diff = 0; /* For encoders, clear last movement */
		}
		data->continue_reading = false;
		return;
	}

	memcpy(&common_data->previous_event, data, sizeof(lv_indev_data_t));
	data->continue_reading = k_msgq_num_used_get(cfg->event_msgq) > 0;
}

static lv_display_t *lvgl_input_get_display(const struct device *dev)
{
	const struct lvgl_common_input_config *cfg = dev->config;
	const struct device *disp_dev = cfg->display_dev;
	struct lvgl_disp_data *lv_disp_data;
	lv_display_t *lv_disp = NULL;

	if (disp_dev == NULL) {
		LOG_DBG("No display phandle is passed in DT, defaulting to LV Default Display");
		return lv_display_get_default();
	}

	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		lv_disp = lv_display_get_next(lv_disp);
		if (lv_disp == NULL) {
			LOG_ERR("Could not find LV display objects of all Zephyr displays");
			break;
		}
		lv_disp_data = (struct lvgl_disp_data *)lv_display_get_user_data(lv_disp);
		if (disp_dev == lv_disp_data->display_dev) {
			return lv_disp;
		}
	}

	LOG_ERR("LV display corresponding to display device %s not found", disp_dev->name);
	__ASSERT_NO_MSG(false);

	return NULL;
}

int lvgl_input_register_driver(lv_indev_type_t indev_type, const struct device *dev)
{
	/* Ensure that `lvgl_common_input_data` remains the first member
	 * of indev dedicated data
	 */
	struct lvgl_common_input_data *common_data = dev->data;
	lv_display_t *disp;

	if (common_data == NULL) {
		return -EINVAL;
	}

	common_data->indev = lv_indev_create();

	if (common_data->indev == NULL) {
		return -EINVAL;
	}

	lv_indev_set_type(common_data->indev, indev_type);
	lv_indev_set_read_cb(common_data->indev, lvgl_input_read_cb);
	lv_indev_set_user_data(common_data->indev, (void *)dev);

	disp = lvgl_input_get_display(dev);
	lv_indev_set_display(common_data->indev, disp);

	return 0;
}

#define LV_DEV_INIT(node_id, init_fn)                                                              \
	do {                                                                                       \
		int ret = init_fn(DEVICE_DT_GET(node_id));                                         \
		if (ret) {                                                                         \
			return ret;                                                                \
		}                                                                                  \
	} while (0);

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

#ifdef CONFIG_LV_Z_POINTER_FROM_CHOSEN_TOUCH
	lvgl_pointer_input_init(lvgl_pointer_from_chosen_get());
#endif

	return 0;
}
