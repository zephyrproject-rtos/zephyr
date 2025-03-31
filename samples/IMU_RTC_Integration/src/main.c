/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * Connections:
 * 1)Display:
 * Display -> SPI1
 * Display D/C -> GPIO2
 * Display CS -> GPIO3
 * Display RST -> GPIO1
 * Display BL -> 3V3
 * 2)Touch :
 * Touch -> I2C0
 * Touch interrupt -> GPIO0
 * Touch RST -> 3V3
 * 3)Wakeup button : Push button to GPIO7
 * 4)Vibration Motor : +ve of motor to GPIO6
 * 5)IMU : I2C0
 * 6)RTC :I2C1
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
#include <zephyr/drivers/gpio.h>
#include "RTC.h"
#include "acc_gyro.h"
#include "cst816s.h"
#include<zephyr/devicetree.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(app);

#define MY_STACK_SIZE 2048
#define MY_PRIORITY 1
#define GPIO_INPUT 0

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
volatile Time time;
volatile Time p_time;
volatile Date date;
char num_str[5];
uint16_t steps;
const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
const struct device * dev1 = DEVICE_DT_GET(DT_NODELABEL(i2c1));//for RTC and touch
const struct device * dev2 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
struct gpio_dt_spec vPin;
uint8_t data_arr[8];
data_struct data_send;
K_MSGQ_DEFINE(touch_msg_q, sizeof(data_struct), 10, 1);
void num_to_string(char *str,uint16_t num,uint8_t padd);
void update_time_in_screen(lv_ui *ui,Time const *time,Date const *date);
void update_stepcount_in_screen(lv_ui *ui,uint32_t steps);
void vibration_motor();
typedef struct{
	char sender[15];
	char message[30];
}msg_data;
cst816s_t touch_dev_handle;
void touch_isr(void);
K_MSGQ_DEFINE(sms_msg_q, sizeof(msg_data), 5, 1);
void vibration_motor()
{
	gpio_pin_set_dt(&vPin,1);
	k_msleep(1000);
	gpio_pin_set_dt(&vPin,0);
}

uint8_t cmp_time(Time *time1,Time *time2)
{
	if(time1->minutes != time2->minutes)
		return 1;
	if(time1->hour != time2->hour)
		return 1;
	return 0;
	
}

void k_busy_wait_ms(uint32_t ms)
{
	for(uint32_t i = 0;i<ms;i++){
		k_busy_wait(1000);
	}
}


void num_to_string(char *str,uint16_t num,uint8_t padd)
{
    uint16_t digits = 1;
    uint16_t copy = num;
    if(padd){
        for(uint8_t i = 1;i<padd;i++)
        digits*=10;
    }
    else{
    while(copy){
        copy/=10;
        digits*=10;
    }
    digits/=10;
    }
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
	num_to_string(num_str,date->day,2);
	lv_label_set_text(ui->screen_1_date, num_str);
	num_to_string(num_str,date->month,2);
	lv_label_set_text(ui->screen_1_month, num_str);
	num_to_string(num_str,date->year,2);
	lv_label_set_text(ui->screen_1_year, num_str);
	num_to_string(num_str,time->hour,2);
	lv_label_set_text(ui->screen_1_hour, num_str);
	num_to_string(num_str,time->minutes,2);
	lv_label_set_text(ui->screen_1_mins, num_str);
}
void update_stepcount_in_screen(lv_ui *ui,uint32_t steps)
{
	num_to_string(num_str,steps,0);
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
	vPin.port = dev2;
	vPin.pin = 6;
    mpu9250_config(ACC_GYRO);
	// DS3231_getHours(dev,DS3231_ADDR);
	setup_ui(&guider_ui);
   	events_init(&guider_ui);
	//   DS3231_setSeconds(dev1,DS3231_ADDR,00);
    //   DS3231_setHours(dev1,DS3231_ADDR,2);
    //   DS3231_setMinutes(dev1,DS3231_ADDR,57);
	//   DS3231_setHourMode(dev1,DS3231_ADDR,CLOCK_H12);

	CST816S_init(&touch_dev_handle,dev2,dev1,5,6,data_arr);
	IRQ_CONNECT(32, 4, touch_isr, NULL, NULL);
	gpio_pin_interrupt_configure(dev2, 0, 1);
	irq_enable(32);
	gpio_pin_configure(dev2, 0, 0);
	plic_irq_enable(32);

	gpio_pin_configure(dev2,7,GPIO_INPUT);
	display_blanking_off(display_dev);
	while (1) {
		time.hour = DS3231_getHours(dev1,DS3231_ADDR);
		time.minutes = DS3231_getMinutes(dev1,DS3231_ADDR);
		printf("Time %d : %d\n",time.hour,time.minutes);
		date.day =02;
		date.month = 07;
		date.year =24;
		if(cmp_time(&time,&p_time)){
			vibration_motor();
			update_time_in_screen(&guider_ui,&time,&date);
			
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
		printf("AX = %f\t AY = %f\t AZ = %f\n", ax, ay, az);
		steps = readStepDetection(ax, ay, az);
		if(prev_step!=steps)
		{
			update_stepcount_in_screen(&guider_ui,steps);
			printf("Steps :%d\n",steps);
		}
		prev_step = steps;
		k_yield();
	}

}

void lcd_display_blank_task_handler(void){
	float ax, ay, az; // Variables to store accelerometer data (x, y, z)
    float gx, gy, gz; // Variables to store gyroscope data (x, y, z)
    float mx, my, mz; // Variables to store magnetometer data (x, y, z)
	uint16_t prev_step;
	uint8_t display_state = 1,buttonPressed=0;
	
	while(1){
	if(((gpio_pin_get(dev2,7))>>7)&1){
		display_state^=1;
		buttonPressed = 1;
	}
		if(display_state && buttonPressed){
		display_blanking_off(display_dev);
		buttonPressed = 0;
		k_msleep(150);
		}
		else if(buttonPressed){
		display_blanking_on(display_dev);
		buttonPressed = 0;
		k_msleep(150);
		}
		k_yield();
	}

}

void process_message_notification_handler(void)
{
	msg_data data_recv;
	while(1){
		k_msgq_get(&sms_msg_q, &data_recv, K_FOREVER);
		lv_label_set_text_fmt(guider_ui.screen_2_senderName, "" LV_SYMBOL_ENVELOPE " %s",data_recv.sender);
		lv_label_set_text(guider_ui.screen_2_messageBox, data_recv.message);
		lv_scr_load(guider_ui.screen_2);
		
		k_msleep(30000);
		lv_scr_load(guider_ui.screen_1);
		
	}
}
void gesture_control_handler(void)
{
	data_struct data_recv;
	while(1){
		k_msgq_get(&touch_msg_q, &data_recv, K_FOREVER);
		printf("Gesture ID:%d\n",data_recv.gestureID);
		if(data_recv.gestureID == SWIPE_DOWN){
				lv_scr_load(guider_ui.screen_2);
				printf("Completed screen 2\n");
		}
		else if(data_recv.gestureID == SWIPE_UP){
				lv_scr_load(guider_ui.screen_1);
				printf("Completed screen 1\n");
		}
	}
}
void screen_refresh_task_handler(void)
{
	while(1)
	{
		lv_task_handler();
		irq_enable(32);
		gpio_pin_configure(dev2, 0, 0);
		plic_irq_enable(32);
		k_yield();
	}
}

void touch_isr(void)
{
    printf("Touch detected\n");
    data_send = CST816S_read_touch(&touch_dev_handle);
	printf("Gesture ID ISR:%d\n",data_send.gestureID);
	if(data_send.gestureID !=0)
    k_msgq_put(&touch_msg_q, &data_send, K_NO_WAIT);
}
K_THREAD_DEFINE(gesture_control_task, MY_STACK_SIZE,
                gesture_control_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
K_THREAD_DEFINE(main_task, MY_STACK_SIZE,
                main_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
K_THREAD_DEFINE(step_count_task, MY_STACK_SIZE,
                step_count_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
K_THREAD_DEFINE(lcd_display_blank_task, MY_STACK_SIZE,
                lcd_display_blank_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
K_THREAD_DEFINE(process_message_notification, MY_STACK_SIZE,
                process_message_notification_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);

K_THREAD_DEFINE(screen_refresh_task, MY_STACK_SIZE,
                screen_refresh_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);