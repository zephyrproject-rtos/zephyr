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
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_home_digital(lv_ui *ui)
{
	//Write codes home_digital
	ui->home_digital = lv_obj_create(NULL);
	lv_obj_set_size(ui->home_digital, 240, 240);
	lv_obj_set_scrollbar_mode(ui->home_digital, LV_SCROLLBAR_MODE_OFF);

	//Write style for home_digital, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home_digital, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_img_src(ui->home_digital, &_img_bg_digital_240x240, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_img_opa(ui->home_digital, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_min
	ui->home_digital_label_min = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_min, "32");
	lv_label_set_long_mode(ui->home_digital_label_min, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_min, 78, 137);
	lv_obj_set_size(ui->home_digital_label_min, 93, 63);

	//Write style for home_digital_label_min, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_digital_label_min, lv_color_hex(0xFF4818), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_digital_label_min, &lv_font_montserratMedium_67, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->home_digital_label_min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_digital_label_min, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_digital_label_min, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_label_min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_hour
	ui->home_digital_label_hour = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_hour, "15");
	lv_label_set_long_mode(ui->home_digital_label_hour, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_hour, 85, 46);
	lv_obj_set_size(ui->home_digital_label_hour, 91, 74);

	//Write style for home_digital_label_hour, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_digital_label_hour, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_digital_label_hour, &lv_font_montserratMedium_73, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->home_digital_label_hour, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_digital_label_hour, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_digital_label_hour, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_label_hour, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//The custom code of home_digital.
	

	//Update current screen layout.
	lv_obj_update_layout(ui->home_digital);

	//Init events for screen.
	events_init_home_digital(ui);
}
