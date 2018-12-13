/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>

#define PORT0  LED0_GPIO_CONTROLLER
#define PORT1  LED1_GPIO_CONTROLLER

#define LED0 LED0_GPIO_PIN
#define LED1 LED1_GPIO_PIN

#define SLEEP_TIME	500

void main(void)
{
	int cnt = 0;
	struct device *gpio0, *gpio1;

	gpio0 = device_get_binding(PORT0);
	gpio1 = device_get_binding(PORT1);

	gpio_pin_configure(gpio0, LED0, GPIO_DIR_OUT);
	gpio_pin_configure(gpio1, LED1, GPIO_DIR_OUT);

	while (1) {
		gpio_pin_write(gpio0, LED0, cnt % 2);
		gpio_pin_write(gpio1, LED1, (cnt + 1) % 2);
		k_sleep(SLEEP_TIME);
		cnt++;
	}
}
