/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include<zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include"cst816s.h"
#include "../generated/gui_guider.h"
#include "../generated/events_init.h"
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);
K_MSGQ_DEFINE(touch_msg_q, sizeof(data_struct), 10, 1);
data_struct data_send;
void touch_isr(void);
lv_ui guider_ui;
data_struct data;
uint8_t data_arr[8];
const struct device *dev =  DEVICE_DT_GET(DT_NODELABEL(i2c1));
const struct device *dev1 =  DEVICE_DT_GET(DT_NODELABEL(gpio0));
bool is_first = true;
cst816s_t touch_dev_handle;
// int main(void)
// {
// 	const struct device *display_dev;

// 	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
// 	if (!device_is_ready(display_dev)) {
// 		LOG_ERR("Device not ready, aborting test");
// 		return 0;
// 	}

// 	setup_ui(&guider_ui);
//    	events_init(&guider_ui);

// 	// lv_task_handler();
// 	display_blanking_off(display_dev);
// 		lv_scr_load(guider_ui.screen_1);
// 		lv_task_handler();
// 	while (1) {
// 		// lv_scr_load(guider_ui.screen_2);
// 		// lv_task_handler();
// 	}
// }






void thread1(void)
{
	data_struct data_recv;
	const struct device *display_dev;
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
	}
	setup_ui(&guider_ui);
   	events_init(&guider_ui);
	// lv_task_handler();
	display_blanking_off(display_dev);
	lv_scr_load(guider_ui.screen_1);
	lv_task_handler();
	CST816S_init(&touch_dev_handle,dev1,dev,5,6,data_arr);
	IRQ_CONNECT(32, 4, touch_isr, NULL, NULL);
	gpio_pin_interrupt_configure(dev1, 0, 1);
// CST816S_begin(&touch_dev_handle);//
// /* Enabling intterupts*/
while (1)
{
	irq_enable(32);
	gpio_pin_configure(dev1, 0, 0);
	plic_irq_enable(32);
		printf("Running main thread\n");
		k_msgq_get(&touch_msg_q, &data_recv, K_FOREVER);
		printk("Gesture ID:%d\n",data_recv.gestureID);
		if(data_recv.gestureID == SWIPE_DOWN){
				lv_scr_load(guider_ui.screen_2);
				printf("Completed screen 2\n");
		}
		else if(data_recv.gestureID == SWIPE_UP){
				lv_scr_load(guider_ui.screen_1);
				printf("Completed screen 1\n");
		}
		lv_task_handler();
	}
}

void touch_isr(void)
{
    printf("Touch detected\n");
    data_send = CST816S_read_touch(&touch_dev_handle);
	if(data_send.gestureID !=0)
    k_msgq_put(&touch_msg_q, &data_send, K_NO_WAIT);
}






K_THREAD_DEFINE(thread1_id, 1024, thread1, NULL, NULL, NULL,
		7, 0, 0);
