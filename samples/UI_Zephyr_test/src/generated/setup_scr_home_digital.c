/*
* Copyright 2024 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include<lvgl.h>
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

	//Write codes home_digital_arc_step
	ui->home_digital_arc_step = lv_arc_create(ui->home_digital);
	lv_arc_set_mode(ui->home_digital_arc_step, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui->home_digital_arc_step, 0, 100);
	lv_arc_set_bg_angles(ui->home_digital_arc_step, 60, 120);
	lv_arc_set_value(ui->home_digital_arc_step, 70);
	lv_arc_set_rotation(ui->home_digital_arc_step, 0);
	lv_obj_set_pos(ui->home_digital_arc_step, 28, 51);
	lv_obj_set_size(ui->home_digital_arc_step, 197, 183);

	//Write style for home_digital_arc_step, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home_digital_arc_step, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->home_digital_arc_step, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_width(ui->home_digital_arc_step, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->home_digital_arc_step, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->home_digital_arc_step, lv_color_hex(0x777777), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_arc_step, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_arc_step, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_arc_step, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_arc_step, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_arc_step, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_arc_step, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_digital_arc_step, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_arc_width(ui->home_digital_arc_step, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->home_digital_arc_step, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->home_digital_arc_step, lv_color_hex(0xFF4818), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for home_digital_arc_step, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home_digital_arc_step, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->home_digital_arc_step, 0, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes home_digital_arc_flash
	ui->home_digital_arc_flash = lv_arc_create(ui->home_digital);
	lv_arc_set_mode(ui->home_digital_arc_flash, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui->home_digital_arc_flash, 0, 100);
	lv_arc_set_bg_angles(ui->home_digital_arc_flash, 238, 300);
	lv_arc_set_value(ui->home_digital_arc_flash, 70);
	lv_arc_set_rotation(ui->home_digital_arc_flash, 0);
	lv_obj_set_pos(ui->home_digital_arc_flash, 28, 3);
	lv_obj_set_size(ui->home_digital_arc_flash, 200, 183);

	//Write style for home_digital_arc_flash, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home_digital_arc_flash, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->home_digital_arc_flash, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_width(ui->home_digital_arc_flash, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->home_digital_arc_flash, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->home_digital_arc_flash, lv_color_hex(0x777777), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_arc_flash, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_arc_flash, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_arc_flash, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_arc_flash, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_arc_flash, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_arc_flash, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for home_digital_arc_flash, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_arc_width(ui->home_digital_arc_flash, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->home_digital_arc_flash, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->home_digital_arc_flash, lv_color_hex(0x1000ff), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for home_digital_arc_flash, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->home_digital_arc_flash, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->home_digital_arc_flash, 0, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes home_digital_img_nxpLogo
	ui->home_digital_img_nxpLogo = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_nxpLogo, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_nxpLogo, &_img_nxp_alpha_39x12);
	lv_img_set_pivot(ui->home_digital_img_nxpLogo, 50,50);
	lv_img_set_angle(ui->home_digital_img_nxpLogo, 0);
	lv_obj_set_pos(ui->home_digital_img_nxpLogo, 248, 108);
	lv_obj_set_size(ui->home_digital_img_nxpLogo, 39, 12);

	//Write style for home_digital_img_nxpLogo, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_nxpLogo, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_stepIcon
	ui->home_digital_img_stepIcon = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_stepIcon, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_stepIcon, &_icn_step_red_alpha_10x12);
	lv_img_set_pivot(ui->home_digital_img_stepIcon, 50,50);
	lv_img_set_angle(ui->home_digital_img_stepIcon, 0);
	lv_obj_set_pos(ui->home_digital_img_stepIcon, 95, 206);
	lv_obj_set_size(ui->home_digital_img_stepIcon, 10, 12);

	//Write style for home_digital_img_stepIcon, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_stepIcon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_stepText
	ui->home_digital_label_stepText = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_stepText, "1526");
	lv_label_set_long_mode(ui->home_digital_label_stepText, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_stepText, 113, 206);
	lv_obj_set_size(ui->home_digital_label_stepText, 36, 12);

	//Write style for home_digital_label_stepText, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_digital_label_stepText, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_digital_label_stepText, &lv_font_montserratMedium_13, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->home_digital_label_stepText, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_digital_label_stepText, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_digital_label_stepText, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_label_stepText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_weather1Text
	ui->home_digital_label_weather1Text = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_weather1Text, "26°");
	lv_label_set_long_mode(ui->home_digital_label_weather1Text, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_weather1Text, 39, 151);
	lv_obj_set_size(ui->home_digital_label_weather1Text, 28, 12);

	//Write style for home_digital_label_weather1Text, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_digital_label_weather1Text, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_digital_label_weather1Text, &lv_font_montserratMedium_13, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->home_digital_label_weather1Text, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_digital_label_weather1Text, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_digital_label_weather1Text, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_digital_label_weather1Text, 253, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->home_digital_label_weather1Text, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->home_digital_label_weather1Text, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_label_weather1Text, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_week
	ui->home_digital_label_week = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_week, "WED");
	lv_label_set_long_mode(ui->home_digital_label_week, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_week, 20, 87);
	lv_obj_set_size(ui->home_digital_label_week, 41, 15);

	//Write style for home_digital_label_week, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_digital_label_week, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_digital_label_week, &lv_font_montserratMedium_13, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->home_digital_label_week, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_digital_label_week, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_digital_label_week, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_date
	ui->home_digital_label_date = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_date, "05/15");
	lv_label_set_long_mode(ui->home_digital_label_date, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_date, 20, 67);
	lv_obj_set_size(ui->home_digital_label_date, 45, 12);

	//Write style for home_digital_label_date, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_digital_label_date, lv_color_hex(0x777777), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_digital_label_date, &lv_font_montserratMedium_13, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->home_digital_label_date, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_digital_label_date, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_digital_label_date, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_flash
	ui->home_digital_img_flash = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_flash, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_flash, &_icn_flash_blue_alpha_10x12);
	lv_img_set_pivot(ui->home_digital_img_flash, 50,50);
	lv_img_set_angle(ui->home_digital_img_flash, 0);
	lv_obj_set_pos(ui->home_digital_img_flash, 99, 21);
	lv_obj_set_size(ui->home_digital_img_flash, 10, 12);

	//Write style for home_digital_img_flash, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_flash, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_flashText
	ui->home_digital_label_flashText = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_flashText, "86%");
	lv_label_set_long_mode(ui->home_digital_label_flashText, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_flashText, 113, 21);
	lv_obj_set_size(ui->home_digital_label_flashText, 36, 12);

	//Write style for home_digital_label_flashText, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->home_digital_label_flashText, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->home_digital_label_flashText, &lv_font_montserratMedium_13, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->home_digital_label_flashText, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->home_digital_label_flashText, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->home_digital_label_flashText, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->home_digital_label_flashText, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_dialDot
	ui->home_digital_img_dialDot = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_dialDot, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_dialDot, &_img_menu_alpha_6x71);
	lv_img_set_pivot(ui->home_digital_img_dialDot, 50,50);
	lv_img_set_angle(ui->home_digital_img_dialDot, 0);
	lv_obj_set_pos(ui->home_digital_img_dialDot, 223, 85);
	lv_obj_set_size(ui->home_digital_img_dialDot, 6, 71);

	//Write style for home_digital_img_dialDot, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_dialDot, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_label_min
	ui->home_digital_label_min = lv_label_create(ui->home_digital);
	lv_label_set_text(ui->home_digital_label_min, "32");
	lv_label_set_long_mode(ui->home_digital_label_min, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->home_digital_label_min, 85, 124);
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
	lv_obj_set_pos(ui->home_digital_label_hour, 81, 50);
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

	//Write codes home_digital_img_messageIcon
	ui->home_digital_img_messageIcon = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_messageIcon, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_messageIcon, &_icn_message_alpha_15x15);
	lv_img_set_pivot(ui->home_digital_img_messageIcon, 50,50);
	lv_img_set_angle(ui->home_digital_img_messageIcon, 0);
	lv_obj_set_pos(ui->home_digital_img_messageIcon, 187, 164);
	lv_obj_set_size(ui->home_digital_img_messageIcon, 15, 15);

	//Write style for home_digital_img_messageIcon, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_messageIcon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_sportIcon
	ui->home_digital_img_sportIcon = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_sportIcon, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_sportIcon, &_icn_sport_alpha_7x14);
	lv_img_set_pivot(ui->home_digital_img_sportIcon, 50,50);
	lv_img_set_angle(ui->home_digital_img_sportIcon, 0);
	lv_obj_set_pos(ui->home_digital_img_sportIcon, 194, 62);
	lv_obj_set_size(ui->home_digital_img_sportIcon, 7, 14);

	//Write style for home_digital_img_sportIcon, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_sportIcon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_weather1Icon
	ui->home_digital_img_weather1Icon = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_weather1Icon, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_weather1Icon, &_icn_weather_1_alpha_25x20);
	lv_img_set_pivot(ui->home_digital_img_weather1Icon, 50,50);
	lv_img_set_angle(ui->home_digital_img_weather1Icon, 0);
	lv_obj_set_pos(ui->home_digital_img_weather1Icon, 34, 127);
	lv_obj_set_size(ui->home_digital_img_weather1Icon, 25, 20);

	//Write style for home_digital_img_weather1Icon, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_recolor_opa(ui->home_digital_img_weather1Icon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor(ui->home_digital_img_weather1Icon, lv_color_hex(0x777777), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_opa(ui->home_digital_img_weather1Icon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_sportText
	ui->home_digital_img_sportText = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_sportText, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_sportText, &_text_sport_alpha_30x30);
	lv_img_set_pivot(ui->home_digital_img_sportText, 50,50);
	lv_img_set_angle(ui->home_digital_img_sportText, 30);
	lv_obj_set_pos(ui->home_digital_img_sportText, 196, 46);
	lv_obj_set_size(ui->home_digital_img_sportText, 30, 30);

	//Write style for home_digital_img_sportText, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_sportText, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_messageText
	ui->home_digital_img_messageText = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_messageText, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_messageText, &_text_message_alpha_42x42);
	lv_img_set_pivot(ui->home_digital_img_messageText, 50,50);
	lv_img_set_angle(ui->home_digital_img_messageText, -50);
	lv_obj_set_pos(ui->home_digital_img_messageText, 187, 157);
	lv_obj_set_size(ui->home_digital_img_messageText, 42, 42);

	//Write style for home_digital_img_messageText, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_messageText, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes home_digital_img_dot
	ui->home_digital_img_dot = lv_img_create(ui->home_digital);
	lv_obj_add_flag(ui->home_digital_img_dot, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->home_digital_img_dot, &_dot_alpha_4x4);
	lv_img_set_pivot(ui->home_digital_img_dot, 50,50);
	lv_img_set_angle(ui->home_digital_img_dot, 0);
	lv_obj_set_pos(ui->home_digital_img_dot, 223, 85);
	lv_obj_set_size(ui->home_digital_img_dot, 4, 4);

	//Write style for home_digital_img_dot, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->home_digital_img_dot, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//The custom code of home_digital.
	

	//Update current screen layout.
	lv_obj_update_layout(ui->home_digital);

	//Init events for screen.
	events_init_home_digital(ui);
}
