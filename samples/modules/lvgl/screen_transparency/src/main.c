/*
 * Copyright (c) 2024 Martin Stumpf <finomnis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>

#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

static void initialize_gui(void)
{
	lv_obj_t *label;

	/* Configure screen and background for transparency */
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, LV_PART_MAIN);

	/* Create a label, set its text and align it to the center */
	label = lv_label_create(lv_screen_active());
	lv_label_set_text(label, "Hello, world!");
	lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xff00ff), LV_PART_MAIN);
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

int main(void)
{
	int err;
	const struct device *display_dev;
	struct display_capabilities display_caps;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return -ENODEV;
	}

	display_get_capabilities(display_dev, &display_caps);
	if (!(display_caps.supported_pixel_formats & PIXEL_FORMAT_ARGB_8888)) {
		LOG_ERR("Display does not support ARGB8888 mode");
		return -ENOTSUP;
	}

	if (PIXEL_FORMAT_ARGB_8888 != display_caps.current_pixel_format) {
		err = display_set_pixel_format(display_dev, PIXEL_FORMAT_ARGB_8888);
		if (err != 0) {
			LOG_ERR("Failed to set ARGB8888 pixel format");
			return err;
		}
	}

	initialize_gui();

	lv_timer_handler();
	display_blanking_off(display_dev);

	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}
