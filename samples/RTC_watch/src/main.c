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

#include "../generated/gui_guider.h"
#include "../generated/events_init.h"
#include <zephyr/drivers/i2c.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
#include"RTC.h"
LOG_MODULE_REGISTER(app);

lv_ui guider_ui;

// int main(void)
// {
// 	const struct device *display_dev;
// 	uint8_t hr,min;
// 	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
//     const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
//     //   DS3231_setSeconds(dev,DS3231_ADDR,00);
//     //   DS3231_setHours(dev,DS3231_ADDR,11);
//     //   DS3231_setMinutes(dev,DS3231_ADDR,45);
// 	//   DS3231_setHourMode(dev,DS3231_ADDR,CLOCK_H12);
// 	if (!device_is_ready(display_dev)) {
// 		LOG_ERR("Device not ready, aborting test");
// 		return 0;
// 	}

// 	printk("Lvgl has started\n");
// 	//setup_ui(&guider_ui,hr,min);
//    	// events_init(&guider_ui);

// 	// lv_task_handler();
// 	display_blanking_off(display_dev);
// 	while (1) {
// 		hr=DS3231_getHours(dev,DS3231_ADDR);
//      	min=DS3231_getMinutes(dev,DS3231_ADDR);
// 		setup_ui(&guider_ui,hr,min);
// 		lv_task_handler();
// 		printk("Lvgl is running\n");
// 		// k_sleep(K_MSEC(10));
// 	}
// }
uint8_t hr,min;
void main_task_handler(void)
{
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
 
    //   DS3231_setSeconds(dev,DS3231_ADDR,00);
    //   DS3231_setHours(dev,DS3231_ADDR,11);
    //   DS3231_setMinutes(dev,DS3231_ADDR,45);
	//   DS3231_setHourMode(dev,DS3231_ADDR,CLOCK_H12);
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	printk("Lvgl has started\n");
	//setup_ui(&guider_ui,hr,min);
   	// events_init(&guider_ui);

	// lv_task_handler();
	display_blanking_off(display_dev);
	while (1) {
		setup_ui(&guider_ui,hr,min);
		lv_task_handler();
		printk("Lvgl is running\n");
		// k_sleep(K_MSEC(10));
		k_yield();
	}
}
void rtc_task_handler(void){
   const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
   while(1){
		hr=DS3231_getHours(dev,DS3231_ADDR);
     	min=DS3231_getMinutes(dev,DS3231_ADDR);
		k_yield();
   }
}


#define MY_STACK_SIZE 2048
#define MY_PRIORITY 1
K_THREAD_DEFINE(display_task, MY_STACK_SIZE,
                main_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
K_THREAD_DEFINE(rtc_task, MY_STACK_SIZE,
                rtc_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
