/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

typedef struct
{
  
	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_label_1;
}lv_ui;

lv_ui guider_ui;

int main(void)
{
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

  lv_init();
  // lv_group_set_default(lv_group_create());
  // lv_disp_t * disp = lv_win_create(lv_scr_act(), 240);
  // lv_disp_set_default(disp);
    /*Change the active screen's background color*/
    printk("Code started!!");
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xffffff), LV_PART_MAIN);
    /*Create a white label, set its text and align it to the center*/
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello world");
    lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
      while(1) {
      lv_timer_handler();
      printk("Running lvgl\n");
  }

  return 0;
}
