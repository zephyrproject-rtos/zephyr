/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"

void ui_init_style(lv_style_t *style)
{
	if (style->prop_cnt > 1) {
		lv_style_reset(style);
	} else {
		lv_style_init(style);
	}
}

void ui_load_scr_animation(lv_ui *ui, lv_obj_t **new_scr, bool new_scr_del, bool *old_scr_del,
			   ui_setup_scr_t setup_scr, lv_screen_load_anim_t anim_type, uint32_t time,
			   uint32_t delay, bool is_clean, bool auto_del)
{
	lv_obj_t *act_scr = lv_screen_active();

	if (auto_del && is_clean) {
		lv_obj_clean(act_scr);
	}
	if (new_scr_del) {
		setup_scr(ui);
	}
	lv_screen_load_anim(*new_scr, anim_type, time, delay, auto_del);
	*old_scr_del = auto_del;
}

void ui_animation(void *var, uint32_t duration, int32_t delay, int32_t start_value,
		  int32_t end_value, lv_anim_path_cb_t path_cb, uint32_t repeat_cnt,
		  uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
		  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb,
		  lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);

	lv_anim_set_var(&anim, var);
	lv_anim_set_exec_cb(&anim, exec_cb);
	lv_anim_set_values(&anim, start_value, end_value);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);
	if (start_cb) {
		lv_anim_set_start_cb(&anim, start_cb);
	}
	if (ready_cb) {
		lv_anim_set_completed_cb(&anim, ready_cb);
	}
	if (deleted_cb) {
		lv_anim_set_deleted_cb(&anim, deleted_cb);
	}
	lv_anim_start(&anim);
}

void init_scr_del_flag(lv_ui *ui)
{

	ui->screen_del = true;
}

void setup_bottom_layer(void)
{
	lv_theme_apply(lv_layer_bottom());
}

void setup_ui(lv_ui *ui)
{
	setup_bottom_layer();
	init_scr_del_flag(ui);
	setup_scr_screen(ui);
	lv_screen_load(ui->screen);
}
