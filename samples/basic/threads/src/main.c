/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <misc/printk.h>
#include <board.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* Change this if you have an LED connected to a custom port */
#define PORT0	LED0_GPIO_PORT
#define PORT1	LED1_GPIO_PORT

/* Change this if you have an LED connected to a custom pin */
#define LED0	LED0_GPIO_PIN
#define LED1	LED1_GPIO_PIN


void blink1(void)
{
	int cnt = 0;
	struct device *gpioa;

	gpioa = device_get_binding(PORT0);
	gpio_pin_configure(gpioa, LED0, GPIO_DIR_OUT);
	while (1) {
		gpio_pin_write(gpioa, LED0, (cnt + 1) % 2);
		k_sleep(100);
		cnt++;
	}
}

void blink2(void)
{
	int cnt = 0;
	struct device *gpiod;

	gpiod = device_get_binding(PORT1);
	gpio_pin_configure(gpiod, LED1, GPIO_DIR_OUT);
	while (1) {
		gpio_pin_write(gpiod, LED1, cnt % 2);
		k_sleep(1000);
		cnt++;
	}
}

void uart_out(void)
{
	int cnt = 1;

	while (1) {
		printk("Toggle USR1 LED: Counter = %d\n", cnt);
		if (cnt >= 10) {
			printk("Toggle USR2 LED: Counter = %d\n", cnt);
			cnt = 0;
		}
		k_sleep(100);
		cnt++;
	}
}

K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(blink2_id, STACKSIZE, blink2, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
