/*
* Copyright 2024 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include<lvgl.h>

typedef struct
{
  
	lv_obj_t *home_digital;
	bool home_digital_del;
	lv_obj_t *home_digital_arc_step;
	lv_obj_t *home_digital_arc_flash;
	lv_obj_t *home_digital_img_nxpLogo;
	lv_obj_t *home_digital_img_stepIcon;
	lv_obj_t *home_digital_label_stepText;
	lv_obj_t *home_digital_label_weather1Text;
	lv_obj_t *home_digital_label_week;
	lv_obj_t *home_digital_label_date;
	lv_obj_t *home_digital_img_flash;
	lv_obj_t *home_digital_label_flashText;
	lv_obj_t *home_digital_img_dialDot;
	lv_obj_t *home_digital_label_min;
	lv_obj_t *home_digital_label_hour;
	lv_obj_t *home_digital_img_messageIcon;
	lv_obj_t *home_digital_img_sportIcon;
	lv_obj_t *home_digital_img_weather1Icon;
	lv_obj_t *home_digital_img_sportText;
	lv_obj_t *home_digital_img_messageText;
	lv_obj_t *home_digital_img_dot;
	lv_obj_t *message_info;
	bool message_info_del;
	lv_obj_t *message_info_app_icon;
	lv_obj_t *message_info_sender;
	lv_obj_t *message_info_message_text;
	lv_obj_t *message_info_left;
	lv_obj_t *message_info_right;
	lv_obj_t *message_info_time_text;
	lv_obj_t *message_info_ok_btn;
	lv_obj_t *message_info_home_text;
	lv_obj_t *fitness;
	bool fitness_del;
	lv_obj_t *fitness_home;
	lv_obj_t *fitness_start_icon;
	lv_obj_t *fitness_title;
	lv_obj_t *fitness_calorie_arc;
	lv_obj_t *fitness_distance_arc;
	lv_obj_t *fitness_duration_arc;
	lv_obj_t *fitness_pluse_arc;
	lv_obj_t *fitness_pluse_icon;
	lv_obj_t *fitness_pulse_text;
	lv_obj_t *fitness_pulse_value;
	lv_obj_t *fitness_duration_icon;
	lv_obj_t *fitness_distance_value;
	lv_obj_t *fitness_duration_text;
	lv_obj_t *fitness_distance_icon;
	lv_obj_t *fitness_duration_value;
	lv_obj_t *fitness_distance_text;
	lv_obj_t *fitness_calorie_icon;
	lv_obj_t *fitness_calorie_value;
	lv_obj_t *fitness_calorie_text;
	lv_obj_t *fitness_img_menu;
	lv_obj_t *fitness_img_dot;
	lv_obj_t *info;
	bool info_del;
	lv_obj_t *info_title;
	lv_obj_t *info_setting_info;
	lv_obj_t *info_label_11;
	lv_obj_t *info_label_10;
	lv_obj_t *info_label_9;
	lv_obj_t *info_label_8;
	lv_obj_t *info_label_7;
	lv_obj_t *info_label_6;
	lv_obj_t *info_label_5;
	lv_obj_t *info_label_4;
	lv_obj_t *info_label_3;
	lv_obj_t *info_label_2;
	lv_obj_t *info_label_1;
	lv_obj_t *info_right;
	lv_obj_t *info_left;
	lv_obj_t *info_home;
	lv_obj_t *blood_oxygen;
	bool blood_oxygen_del;
	lv_obj_t *blood_oxygen_arc_blood;
	lv_obj_t *blood_oxygen_home;
	lv_obj_t *blood_oxygen_title;
	lv_obj_t *blood_oxygen_label_blood;
	lv_obj_t *blood_oxygen_label_ago;
	lv_obj_t *blood_oxygen_water_icon;
	lv_obj_t *blood_oxygen_label_percentage;
	lv_obj_t *blood_oxygen_img_menu;
	lv_obj_t *blood_oxygen_img_dot;
	lv_obj_t *ekg;
	bool ekg_del;
	lv_obj_t *ekg_home;
	lv_obj_t *ekg_title;
	lv_obj_t *ekg_cont_1;
	lv_obj_t *ekg_label_bmp;
	lv_obj_t *ekg_label_pulse;
	lv_obj_t *ekg_img_healthy;
	lv_obj_t *ekg_cont_2;
	lv_obj_t *ekg_img_2;
	lv_obj_t *ekg_img_3;
	lv_obj_t *ekg_img_4;
	lv_obj_t *ekg_img_5;
	lv_obj_t *ekg_label_RR_text;
	lv_obj_t *ekg_label_count_text;
	lv_obj_t *ekg_label_RR_interval;
	lv_obj_t *ekg_label_count;
	lv_obj_t *step;
	bool step_del;
	lv_obj_t *step_arc_step;
	lv_obj_t *step_home;
	lv_obj_t *step_label_current_steps;
	lv_obj_t *step_left;
	lv_obj_t *step_right;
	lv_obj_t *step_title;
	lv_obj_t *step_cont_1;
	lv_obj_t *step_label_goals_steps;
	lv_obj_t *step_label_goal;
	lv_obj_t *step_icon_step;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_move_animation(void * var, int32_t duration, int32_t delay, int32_t x_end, int32_t y_end, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void ui_scale_animation(void * var, int32_t duration, int32_t delay, int32_t width, int32_t height, lv_anim_path_cb_t path_cb,
                        uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                        lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void ui_img_zoom_animation(void * var, int32_t duration, int32_t delay, int32_t zoom, lv_anim_path_cb_t path_cb,
                           uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                           lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void ui_img_rotate_animation(void * var, int32_t duration, int32_t delay, lv_coord_t x, lv_coord_t y, int32_t rotate,
                   lv_anim_path_cb_t path_cb, uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time,
                   uint32_t playback_delay, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);

void init_scr_del_flag(lv_ui *ui);

void setup_ui(lv_ui *ui);


extern lv_ui guider_ui;


void setup_scr_home_digital(lv_ui *ui);
void setup_scr_message_info(lv_ui *ui);
void setup_scr_fitness(lv_ui *ui);
void setup_scr_info(lv_ui *ui);
void setup_scr_blood_oxygen(lv_ui *ui);
void setup_scr_ekg(lv_ui *ui);
void setup_scr_step(lv_ui *ui);

LV_IMG_DECLARE(_img_bg_digital_240x240);
LV_IMG_DECLARE(_img_nxp_alpha_39x12);
LV_IMG_DECLARE(_icn_step_red_alpha_10x12);
LV_IMG_DECLARE(_icn_flash_blue_alpha_10x12);
LV_IMG_DECLARE(_img_menu_alpha_6x71);
LV_IMG_DECLARE(_icn_message_alpha_15x15);
LV_IMG_DECLARE(_icn_sport_alpha_7x14);
LV_IMG_DECLARE(_icn_weather_1_alpha_25x20);
LV_IMG_DECLARE(_text_sport_alpha_30x30);
LV_IMG_DECLARE(_text_message_alpha_42x42);
LV_IMG_DECLARE(_dot_alpha_4x4);

LV_IMG_DECLARE(_img_bg_2_240x240);
LV_IMG_DECLARE(_icn_whatsapp_alpha_36x36);
LV_IMG_DECLARE(_img_arrow_left_alpha_5x19);
LV_IMG_DECLARE(_img_arrow_right_alpha_5x19);
LV_IMG_DECLARE(_text_home_alpha_22x29);

LV_IMG_DECLARE(_img_bg_2_240x240);
LV_IMG_DECLARE(_text_home_alpha_22x29);
LV_IMG_DECLARE(_text_start_stop_alpha_26x34);

LV_IMG_DECLARE(_img_header_bg_240x45);
LV_IMG_DECLARE(_icn_small_pulse_alpha_15x13);
LV_IMG_DECLARE(_icn_small_time_alpha_15x15);
LV_IMG_DECLARE(_icn_small_pos_alpha_11x15);
LV_IMG_DECLARE(_icn_small_burn_alpha_12x14);
LV_IMG_DECLARE(_img_menu_alpha_6x61);
LV_IMG_DECLARE(_dot_alpha_4x4);

LV_IMG_DECLARE(_img_bg_2_240x240);

LV_IMG_DECLARE(_img_header_bg_240x45);
LV_IMG_DECLARE(_img_arrow_right_alpha_5x19);
LV_IMG_DECLARE(_img_arrow_left_alpha_5x19);
LV_IMG_DECLARE(_text_home_alpha_22x29);

LV_IMG_DECLARE(_img_bg_2_240x240);
LV_IMG_DECLARE(_text_home_alpha_22x29);

LV_IMG_DECLARE(_img_header_bg_240x45);
LV_IMG_DECLARE(_icn_water_alpha_19x26);
LV_IMG_DECLARE(_img_menu_alpha_6x61);
LV_IMG_DECLARE(_dot_alpha_4x4);

LV_IMG_DECLARE(_img_bg_health_240x240);
LV_IMG_DECLARE(_text_home_alpha_22x29);

LV_IMG_DECLARE(_img_header_bg_240x45);
LV_IMG_DECLARE(_img_heart_alpha_31x26);
LV_IMG_DECLARE(_img_ekg_alpha_78x69);
LV_IMG_DECLARE(_img_ekg_alpha_78x69);
LV_IMG_DECLARE(_img_ekg_alpha_78x69);
LV_IMG_DECLARE(_img_ekg_alpha_78x69);

LV_IMG_DECLARE(_img_bg_2_240x240);
LV_IMG_DECLARE(_text_home_alpha_22x29);
LV_IMG_DECLARE(_img_arrow_left_alpha_5x19);
LV_IMG_DECLARE(_img_arrow_right_alpha_5x19);

LV_IMG_DECLARE(_img_header_bg_240x45);
LV_IMG_DECLARE(_img_step_small_alpha_17x27);

LV_FONT_DECLARE(lv_font_montserratMedium_13)
LV_FONT_DECLARE(lv_font_montserratMedium_10)
LV_FONT_DECLARE(lv_font_montserratMedium_67)
LV_FONT_DECLARE(lv_font_montserratMedium_73)
LV_FONT_DECLARE(lv_font_montserratMedium_15)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_montserratMedium_14)
LV_FONT_DECLARE(lv_font_montserratMedium_18)
LV_FONT_DECLARE(lv_font_montserratMedium_61)


#ifdef __cplusplus
}
#endif
#endif
