/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"

void setup_scr_screen(lv_ui *ui)
{
	/* Write codes screen */
	ui->screen = lv_screen_active();
	lv_obj_set_scrollbar_mode(ui->screen, LV_SCROLLBAR_MODE_OFF);

	/* Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT. */
	lv_obj_set_style_bg_opa(ui->screen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	/* Write codes screen_slider_1 */
	ui->screen_slider_1 = lv_slider_create(ui->screen);
	lv_obj_set_pos(ui->screen_slider_1, 10, 30);
	lv_obj_set_size(ui->screen_slider_1, 160, 8);
	lv_slider_set_range(ui->screen_slider_1, 0, 100);
	lv_slider_set_mode(ui->screen_slider_1, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui->screen_slider_1, 50, LV_ANIM_OFF);

	/* Write style for screen_slider_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT. */
	lv_obj_set_style_bg_opa(ui->screen_slider_1, 60, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_slider_1, lv_color_hex(0x2195f6),
				  LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_slider_1, LV_GRAD_DIR_NONE,
				     LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_slider_1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ui->screen_slider_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_slider_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	/* Write style for screen_slider_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT. */
	lv_obj_set_style_bg_opa(ui->screen_slider_1, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_slider_1, lv_color_hex(0x2195f6),
				  LV_PART_INDICATOR | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_slider_1, LV_GRAD_DIR_NONE,
				     LV_PART_INDICATOR | LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_slider_1, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);

	/* Write style for screen_slider_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT. */
	lv_obj_set_style_bg_opa(ui->screen_slider_1, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_slider_1, lv_color_hex(0x2195f6),
				  LV_PART_KNOB | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_slider_1, LV_GRAD_DIR_NONE,
				     LV_PART_KNOB | LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_slider_1, 8, LV_PART_KNOB | LV_STATE_DEFAULT);

	/* The custom code of screen. */

	/* Update current screen layout. */
	lv_obj_update_layout(ui->screen);
}
