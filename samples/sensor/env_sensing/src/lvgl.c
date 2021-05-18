/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include "resources.h"
#include <drivers/sensor.h>

extern const struct device *display;
extern const struct device *sensor_dev;

#include <logging/log.h>
LOG_MODULE_DECLARE(main);

lv_obj_t *temp_value_label;
lv_obj_t *humid_value_label;
lv_obj_t *pres_value_label;
lv_obj_t *LogOutput;

lv_obj_t *city_label;
lv_obj_t *rain_meter;
lv_obj_t *rain_static;
lv_obj_t *temp_label;
lv_obj_t *pressure_label;
lv_obj_t *humidity_label;

void init_display(void)
{
	lv_obj_t *img1;
	const char *location;
	static lv_style_t large_style;

	display = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display == NULL) {
		LOG_ERR("Display device not found.");
		return;
	}

	lv_theme_t *th =
		lv_theme_material_init(LV_COLOR_BLACK, LV_COLOR_WHITE, LV_THEME_MATERIAL_FLAG_DARK,
				       &lv_font_montserrat_14, &lv_font_montserrat_16,
				       &lv_font_montserrat_18, &lv_font_montserrat_22);

	lv_theme_set_act(th);
	img1 = lv_img_create(lv_scr_act(), NULL);
	city_label = lv_label_create(lv_scr_act(), NULL);
	rain_meter = lv_linemeter_create(lv_scr_act(), NULL);
	rain_static = lv_label_create(lv_scr_act(), NULL);
	temp_label = lv_label_create(lv_scr_act(), NULL);
	pressure_label = lv_label_create(lv_scr_act(), NULL);
	humidity_label = lv_label_create(lv_scr_act(), NULL);
	temp_value_label = lv_label_create(lv_scr_act(), NULL);
	humid_value_label = lv_label_create(lv_scr_act(), NULL);
	pres_value_label = lv_label_create(lv_scr_act(), NULL);


	lv_style_init(&large_style);
	lv_style_set_text_font(&large_style, LV_STATE_DEFAULT, lv_theme_get_font_title());

	lv_img_set_src(img1, &cloudy);
	lv_obj_set_x(img1, 20);
	lv_obj_set_y(img1, 120);
	lv_obj_set_style_local_image_recolor_opa(img1, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, 255);
	lv_obj_set_style_local_image_recolor(img1, LV_IMG_PART_MAIN, LV_STATE_DEFAULT,
					     LV_COLOR_WHITE);

	lv_obj_add_style(city_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(city_label, 115);
	lv_obj_set_y(city_label, 10);
	lv_obj_set_height(city_label, 20);
	lv_obj_set_width(city_label, 50);
	lv_label_set_text(city_label, location);

	lv_obj_set_x(rain_meter, 15);
	lv_obj_set_y(rain_meter, 10);
	lv_obj_set_width(rain_meter, 75);
	lv_obj_set_height(rain_meter, 75);
	lv_linemeter_set_range(rain_meter, 0, 100);

	lv_obj_set_x(rain_static, 25);
	lv_obj_set_y(rain_static, 90);
	lv_obj_set_width(rain_static, 60);
	lv_obj_set_height(rain_static, 40);
	lv_label_set_text(rain_static, "% Rain");

	lv_obj_set_x(temp_label, 110);
	lv_obj_set_y(temp_label, 50);
	lv_label_set_text(temp_label, "Temp (C°)");

	lv_obj_set_x(pressure_label, 110);
	lv_obj_set_y(pressure_label, 100);
	lv_label_set_text(pressure_label, "Pressure (kPa)");

	lv_obj_set_x(humidity_label, 110);
	lv_obj_set_y(humidity_label, 150);
	lv_label_set_text(humidity_label, "Humidity (%)");

	lv_obj_add_style(temp_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(temp_value_label, 240);
	lv_obj_set_y(temp_value_label, 50);

	lv_obj_add_style(pres_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(pres_value_label, 240);
	lv_obj_set_y(pres_value_label, 100);

	lv_obj_add_style(humid_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(humid_value_label, 240);
	lv_obj_set_y(humid_value_label, 150);

	LogOutput = lv_label_create(lv_scr_act(), NULL);
	lv_label_set_long_mode(LogOutput, LV_LABEL_LONG_CROP);
	lv_label_set_recolor(LogOutput, true);
	lv_label_set_align(LogOutput, LV_LABEL_ALIGN_LEFT);
	lv_obj_set_width(LogOutput, 280);
	lv_obj_align(LogOutput, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 200);
	lv_obj_set_style_local_bg_opa(LogOutput, LV_PAGE_PART_BG, LV_STATE_DEFAULT, 64);

	lv_task_handler();
	display_blanking_off(display);
}

void reset_display(void)
{
}

void update_display(struct sensor_value temp, struct sensor_value hum, struct sensor_value press,
		    struct sensor_value gas)
{
	display_set_brightness(display, 0);

	lv_label_set_text_fmt(temp_value_label, "%.1f", sensor_value_to_double(&temp));
#if CONFIG_HAS_PRESSURE
	lv_label_set_text_fmt(pres_value_label, "%.1f", sensor_value_to_double(&press));
#else
	lv_label_set_text(pres_value_label, "N/A");
#endif
	lv_label_set_text_fmt(humid_value_label, "%.1f", sensor_value_to_double(&hum));

	lv_label_set_text_fmt(LogOutput, "Powered By: %s", sensor_dev->name);
	lv_label_set_text_fmt(city_label, "%s", "Montreal");

	lv_task_handler();
}
