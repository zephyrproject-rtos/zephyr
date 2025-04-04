/*
* Copyright 2024 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include<lvgl.h>

#if LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif


static void home_digital_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_move_animation(guider_ui.home_digital_img_nxpLogo, 800, 0, 291, 175, &lv_anim_path_bounce, 0, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.home_digital_label_min, 800, 0, 145, 208, &lv_anim_path_bounce, 0, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.home_digital_label_hour, 800, 0, 143, 82, &lv_anim_path_bounce, 0, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
		switch(dir) {
			case LV_DIR_LEFT:
			{
				lv_indev_wait_release(lv_indev_get_act());
				ui_load_scr_animation(&guider_ui, &guider_ui.message_info, guider_ui.message_info_del, &guider_ui.home_digital_del, setup_scr_message_info, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, false, true);
				break;
			}
		}
		break;
	}
	default:
		break;
	}
}
void events_init_home_digital(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->home_digital, home_digital_event_handler, LV_EVENT_ALL, ui);
}
static void message_info_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_move_animation(guider_ui.message_info_app_icon, 800, 0, 166, 30, &lv_anim_path_bounce, 0, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.message_info_sender, 800, 0, 151, 107, &lv_anim_path_bounce, 0, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	default:
		break;
	}
}
static void message_info_left_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.home_digital, guider_ui.home_digital_del, &guider_ui.message_info_del, setup_scr_home_digital, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, false, true);
		break;
	}
	default:
		break;
	}
}
void events_init_message_info(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->message_info, message_info_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->message_info_left, message_info_left_event_handler, LV_EVENT_ALL, ui);
}
static void fitness_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_move_animation(guider_ui.fitness_title, 800, 0, 0, 0, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.fitness_calorie_arc, 500, 0, 7, 49, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.fitness_distance_arc, 500, 0, 32, 76, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.fitness_duration_arc, 500, 0, 57, 99, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.fitness_pluse_arc, 500, 0, 82, 124, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	default:
		break;
	}
}
void events_init_fitness(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->fitness, fitness_event_handler, LV_EVENT_ALL, ui);
}
static void info_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_move_animation(guider_ui.info_setting_info, 800, 0, 63, 83, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.info_title, 800, 0, 0, 0, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	default:
		break;
	}
}
void events_init_info(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->info, info_event_handler, LV_EVENT_ALL, ui);
}
static void blood_oxygen_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_move_animation(guider_ui.blood_oxygen_title, 800, 0, 0, 0, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.blood_oxygen_arc_blood, 800, 0, 11, 11, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.blood_oxygen_label_ago, 800, 0, 123, 260, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
		switch(dir) {
			case LV_DIR_LEFT:
			{
				lv_indev_wait_release(lv_indev_get_act());
				ui_load_scr_animation(&guider_ui, &guider_ui.ekg, guider_ui.ekg_del, &guider_ui.blood_oxygen_del, setup_scr_ekg, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, false, true);
				break;
			}
		}
		break;
	}
	default:
		break;
	}
}
void events_init_blood_oxygen(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->blood_oxygen, blood_oxygen_event_handler, LV_EVENT_ALL, ui);
}
static void ekg_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		lv_obj_clear_flag(guider_ui.ekg_cont_2, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(guider_ui.ekg_cont_2, LV_OBJ_FLAG_SCROLLABLE);
		ui_move_animation(guider_ui.ekg_cont_2, 700, 0, -127, 135, &lv_anim_path_linear, LV_ANIM_REPEAT_INFINITE, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.ekg_cont_1, 800, 0, 106, 294, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.ekg_title, 800, 0, 0, 0, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_img_zoom_animation(guider_ui.ekg_img_healthy, 500, 0, 372, &lv_anim_path_ease_in, LV_ANIM_REPEAT_INFINITE, 0, 500, 0, NULL, NULL, NULL);
		break;
	}
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
		switch(dir) {
			case LV_DIR_RIGHT:
			{
				lv_indev_wait_release(lv_indev_get_act());
				ui_load_scr_animation(&guider_ui, &guider_ui.blood_oxygen, guider_ui.blood_oxygen_del, &guider_ui.ekg_del, setup_scr_blood_oxygen, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, false, true);
				break;
			}
		}
		break;
	}
	default:
		break;
	}
}
void events_init_ekg(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->ekg, ekg_event_handler, LV_EVENT_ALL, ui);
}
static void step_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_move_animation(guider_ui.step_cont_1, 800, 0, 111, 260, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.step_title, 800, 0, 0, 0, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.step_icon_step, 800, 0, 182, 93, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		ui_move_animation(guider_ui.step_arc_step, 800, 0, 11, 11, &lv_anim_path_bounce, 1, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	default:
		break;
	}
}
void events_init_step(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->step, step_event_handler, LV_EVENT_ALL, ui);
}

void events_init(lv_ui *ui)
{

}
