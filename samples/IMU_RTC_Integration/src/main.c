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
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
#include "RTC.h"
#include "acc_gyro.h"
LOG_MODULE_REGISTER(app);

#define MY_STACK_SIZE 2048
#define MY_PRIORITY 1


lv_ui guider_ui;
typedef struct{
	uint8_t day;
	uint8_t month;
	uint8_t year;
}Date;

typedef struct{
	uint8_t hour;
	uint8_t minutes;
}Time;

void num_to_string(char *str,uint16_t num);
void update_time_in_screen(lv_ui *ui,Time const *time,Date const *date);
void update_stepcount_in_screen(lv_ui *ui,uint32_t steps);

uint8_t cmp_time(Time *time1,Time *time2)
{
	if(time1->minutes != time2->minutes)
		return 1;
	if(time1->hour != time2->hour)
		return 1;
	return 0;
	
}

volatile Time time;
volatile Time p_time;
volatile Date date;
char num_str[5];
uint16_t steps;
const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
const struct device * dev1 = DEVICE_DT_GET(DT_NODELABEL(i2c1));
void num_to_string(char *str,uint16_t num)
{
    uint16_t digits = 1;
    uint16_t copy = num;
    while(copy){
        copy/=10;
        digits*=10;
    }
    digits/=10;
    uint8_t i = 0;
    while(digits)
    {
        str[i++] = (num / digits) + '0';
        num = num % digits;
        digits/=10;
    }
    str[i] = '\0';
}


void update_time_in_screen(lv_ui *ui,Time const *time,Date const *date)
{
	// num_to_string(num_str,date->day);
	// lv_label_set_text(ui->screen_1_date, num_str);
	// num_to_string(num_str,date->month);
	// lv_label_set_text(ui->screen_1_month, num_str);
	// num_to_string(num_str,date->year);
	// lv_label_set_text(ui->screen_1_year, num_str);
	num_to_string(num_str,time->hour);
	lv_label_set_text(ui->screen_1_hour, num_str);
	num_to_string(num_str,time->minutes);
	lv_label_set_text(ui->screen_1_mins, num_str);
}
void update_stepcount_in_screen(lv_ui *ui,uint32_t steps)
{
	num_to_string(num_str,steps);
	lv_label_set_text(ui->screen_1_step_count, num_str);
}
void main_task_handler(void)
{

	const struct device *display_dev;
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}
    // configure the mpu9250 for accelerometer and gyroscope
    mpu9250_config(ACC_GYRO);
	// DS3231_getHours(dev,DS3231_ADDR);
	setup_ui(&guider_ui);
   	events_init(&guider_ui);
	  DS3231_setSeconds(dev1,DS3231_ADDR,00);
      DS3231_setHours(dev1,DS3231_ADDR,4);
      DS3231_setMinutes(dev1,DS3231_ADDR,33);
	  DS3231_setHourMode(dev1,DS3231_ADDR,CLOCK_H12);
	display_blanking_off(display_dev);
	while (1) {
		time.hour = DS3231_getHours(dev1,DS3231_ADDR);
		time.minutes = DS3231_getMinutes(dev1,DS3231_ADDR);
		printf("Time %d : %d\n",time.hour,time.minutes);
		date.day =26;
		date.month = 6;
		date.year =24;
		if(cmp_time(&time,&p_time)){
			update_time_in_screen(&guider_ui,&time,&date);
			lv_task_handler();
		}
		p_time = time;
		k_msleep(60000);
	}

}

void step_count_task_handler(void){
	float ax, ay, az; // Variables to store accelerometer data (x, y, z)
    float gx, gy, gz; // Variables to store gyroscope data (x, y, z)
    float mx, my, mz; // Variables to store magnetometer data (x, y, z)
	uint16_t prev_step;
	while(1){
		accel_xyz(ACC_GYRO, &ax, &ay, &az);
		// printf("AX = %f\t AY = %f\t AZ = %f\n", ax, ay, az);
		steps = readStepDetection(ax, ay, az);
		if(prev_step!=steps)
		{
			update_stepcount_in_screen(&guider_ui,steps);
			printf("Steps :%d\n",steps);
			lv_task_handler();
		}
		prev_step = steps;
		k_yield();
	}

}
K_THREAD_DEFINE(main_task, MY_STACK_SIZE,
                main_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
K_THREAD_DEFINE(step_count_task, MY_STACK_SIZE,
                step_count_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);