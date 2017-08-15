/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <board.h>

#define SLEEP_TIME	500

void main(void)
{
	int cnt = 0;
	struct device *gpioa;
	struct device *gpiob;
	
	gpioa = device_get_binding(LED0_GPIO_PORT);
	gpiob = device_get_binding(LED1_GPIO_PORT);
	
	gpio_pin_configure(gpioa, LED0_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(gpiob, LED1_GPIO_PIN, GPIO_DIR_OUT);

	while (1) {
		gpio_pin_write(gpioa, LED0_GPIO_PIN, cnt % 2);
		gpio_pin_write(gpiob, LED1_GPIO_PIN, (cnt + 1) % 2);
		k_sleep(SLEEP_TIME);
		cnt++;
	}
}
