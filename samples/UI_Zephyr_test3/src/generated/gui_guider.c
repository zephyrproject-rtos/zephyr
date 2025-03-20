/*
* Copyright 2024 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#if LV_USE_FREEMASTER
#include "gg_external_data.h"
#endif

void ui_init_style(lv_style_t * style)
{
	if (style->prop_cnt > 1)
		lv_style_reset(style);
	else
		lv_style_init(style);
}

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del)
{
	lv_obj_t * act_scr = lv_scr_act();

#if LV_USE_FREEMASTER
	if(auto_del) {
		gg_edata_task_clear(act_scr);
	}
#endif
	if (auto_del && is_clean) {
		lv_obj_clean(act_scr);
	}
	if (new_scr_del) {
		setup_scr(ui);
	}
	lv_scr_load_anim(*new_scr, anim_type, time, delay, auto_del);
	*old_scr_del = auto_del;
}

void ui_move_animation(void * var, int32_t duration, int32_t delay, int32_t x_end, int32_t y_end, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);

	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
	lv_anim_set_values(&anim, lv_obj_get_x(var), x_end);
	lv_anim_start(&anim);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
	lv_anim_set_values(&anim, lv_obj_get_y(var), y_end);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void ui_scale_animation(void * var, int32_t duration, int32_t delay, int32_t width, int32_t height, lv_anim_path_cb_t path_cb,
                        uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                        lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);

	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_width);
	lv_anim_set_values(&anim, lv_obj_get_width(var), width);
	lv_anim_start(&anim);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_height);
	lv_anim_set_values(&anim, lv_obj_get_height(var), height);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void ui_img_zoom_animation(void * var, int32_t duration, int32_t delay, int32_t zoom, lv_anim_path_cb_t path_cb,
                           uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                           lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_img_set_zoom);
	lv_anim_set_values(&anim, lv_img_get_zoom(var), zoom);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void ui_img_rotate_animation(void * var, int32_t duration, int32_t delay, lv_coord_t x, lv_coord_t y, int32_t rotate,
                   lv_anim_path_cb_t path_cb, uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                   lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_img_set_pivot(var, x, y);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_img_set_angle);
	lv_anim_set_values(&anim, 0, rotate*10);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void init_scr_del_flag(lv_ui *ui)
{
  
	ui->screen_del = true;
}

void setup_ui(lv_ui *ui)
{
	init_scr_del_flag(ui);
	setup_scr_screen(ui);
	lv_scr_load(ui->screen);
}
