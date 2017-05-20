/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <gpio.h>

#include <misc/printk.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME	1000

#ifndef CONFIG_BOARD_HEXIWEAR_K64
#error this is a board specific demo
#endif

void main(void)
{
	struct device *dev1;
	struct device *dev2;
	struct device *dev3;

	dev1 = device_get_binding(CLICK1_GPIO_NAME);
	dev2 = device_get_binding(CLICK2_GPIO_NAME);
	dev3 = device_get_binding(CLICK3_GPIO_NAME);

	if (dev1 == 0 || dev2 == 0 || dev3 == 0) {
		printk("unable to find devices\n");
		return;
	}

	gpio_pin_configure(dev1, CLICK1_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev1, CLICK1_GPIO_PIN, 0);

	gpio_pin_configure(dev2, CLICK2_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev2, CLICK2_GPIO_PIN, 0);

	gpio_pin_configure(dev3, CLICK3_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev3, CLICK3_GPIO_PIN, 0);

	while (1) {
		gpio_pin_write(dev1, CLICK1_GPIO_PIN, 1);
		k_sleep(SLEEP_TIME);
		gpio_pin_write(dev1, CLICK1_GPIO_PIN, 0);

		gpio_pin_write(dev2, CLICK2_GPIO_PIN, 1);
		k_sleep(SLEEP_TIME);
		gpio_pin_write(dev2, CLICK2_GPIO_PIN, 0);

		gpio_pin_write(dev3, CLICK3_GPIO_PIN, 1);
		k_sleep(SLEEP_TIME);
		gpio_pin_write(dev3, CLICK3_GPIO_PIN, 0);
	}
}
