/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

void main(void)
{
	u32_t count = 0U;
	char count_str[11] = {0};
	struct device *display_dev;
	lv_obj_t *hello_world_label;
	lv_obj_t *count_label;

	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL) {
		LOG_ERR("device not found.  Aborting test.");
		return;
	}

	hello_world_label = lv_label_create(lv_scr_act(), NULL);
	lv_label_set_text(hello_world_label, "Hello world!");
	lv_obj_align(hello_world_label, NULL, LV_ALIGN_CENTER, 0, 0);

	count_label = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(count_label, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

	/*Create styles for the keyboard*/
	static lv_style_t rel_style, pr_style;

	lv_style_copy(&rel_style, &lv_style_btn_rel);
	rel_style.body.radius = 0;

	lv_style_copy(&pr_style, &lv_style_btn_pr);
	pr_style.body.radius = 0;

	/*Create a keyboard and apply the styles*/
	lv_obj_t *kb = lv_kb_create(lv_scr_act(), NULL);

	lv_kb_set_cursor_manage(kb, true);
	lv_kb_set_style(kb, LV_KB_STYLE_BG, &lv_style_transp_tight);
	lv_kb_set_style(kb, LV_KB_STYLE_BTN_REL, &rel_style);
	lv_kb_set_style(kb, LV_KB_STYLE_BTN_PR, &pr_style);

	/*Create a text area. The keyboard will write here*/
	lv_obj_t *ta = lv_ta_create(lv_scr_act(), NULL);

	lv_obj_align(ta, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
	lv_ta_set_text(ta, "");

	/*Assign the text area to the keyboard*/
	lv_kb_set_ta(kb, ta);

	display_blanking_off(display_dev);

	while (1) {
		if ((count % 100) == 0U) {
			sprintf(count_str, "%d", count/100U);
			lv_label_set_text(count_label, count_str);
		}
		lv_task_handler();
		k_sleep(10);
		++count;
	}
}
