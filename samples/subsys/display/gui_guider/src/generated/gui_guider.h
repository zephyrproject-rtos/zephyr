/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {

	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_slider_1;
} lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui *ui);

void ui_init_style(lv_style_t *style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t **new_scr, bool new_scr_del, bool *old_scr_del,
			   ui_setup_scr_t setup_scr, lv_screen_load_anim_t anim_type, uint32_t time,
			   uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void *var, uint32_t duration, int32_t delay, int32_t start_value,
		  int32_t end_value, lv_anim_path_cb_t path_cb, uint32_t repeat_cnt,
		  uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
		  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb,
		  lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void init_scr_del_flag(lv_ui *ui);

void setup_bottom_layer(void);

void setup_ui(lv_ui *ui);

extern lv_ui guider_ui;

void setup_scr_screen(lv_ui *ui);

#ifdef __cplusplus
}
#endif
#endif /* GUI_GUIDER_H */
