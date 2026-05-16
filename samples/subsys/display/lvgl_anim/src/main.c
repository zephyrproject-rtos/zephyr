/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generated at build time using video_to_lvgl_frames.py */
#include "img/lvgl_anim_frames.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lvgl_anim);

static void frame_set_cb(void *var, int frame_idx)
{
	lv_image_set_src((lv_obj_t *)var, lvgl_anim_frames[(uint32_t)frame_idx]);
}

int main(void)
{
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	static lv_obj_t *g_img;
	static lv_anim_t g_anim;

	if (!device_is_ready(display_dev)) {
		return -ENODEV;
	}

	display_blanking_off(display_dev);

	g_img = lv_image_create(lv_screen_active());
	lv_image_set_src(g_img, lvgl_anim_frames[0]);

	lv_anim_init(&g_anim);
	lv_anim_set_var(&g_anim, g_img);
	lv_anim_set_exec_cb(&g_anim, frame_set_cb);
	lv_anim_set_values(&g_anim, 0, LVGL_ANIMATION_FRAME_COUNT - 1);
	lv_anim_set_duration(&g_anim, LVGL_ANIMATION_DURATION_MS);
	lv_anim_set_repeat_count(&g_anim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_path_cb(&g_anim, lv_anim_path_step);
	lv_anim_start(&g_anim);

	while (1) {
		lv_timer_handler();
		k_msleep(LVGL_ANIMATION_DURATION_MS / LVGL_ANIMATION_FRAME_COUNT);
	}

	return 0;
}
