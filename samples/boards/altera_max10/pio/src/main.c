/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <misc/printk.h>
#include <misc/util.h>

/* GPIO driver name */
#define GPIO_DRV_NAME	CONFIG_GPIO_ALTERA_NIOS2_OUTPUT_DEV_NAME

/* PIO pins[0:3] are wired to LED's */
#define LED_PINS_WIRED		4
#define LED_PIN_MASK		0xf

void main(void)
{
	struct device *gpio_dev;
	int i, ret;

	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		printk("Cannot find %s!\n", GPIO_DRV_NAME);
		return;
	}

	/* Setup GPIO output */
	ret = gpio_port_configure(gpio_dev, GPIO_DIR_OUT);
	if (ret) {
		printk("Error configuring GPIO PORT\n");
	}

	/*
	 * Note: the LED's are connected in inverse logic
	 * to the Nios-II PIO core.
	 */
	printk("Turning off all LEDs\n");
	ret = gpio_port_write(gpio_dev, LED_PIN_MASK);
	if (ret) {
		printk("Error set GPIO port\n");
	}
	k_sleep(MSEC_PER_SEC * 5);

	for (i = 0; i < LED_PINS_WIRED; i++) {
		printk("Turn On LED[%d]\n", i);
		ret = gpio_port_write(gpio_dev, LED_PIN_MASK & ~(1 << i));
		if (ret) {
			printk("Error in seting GPIO port\n");
		}

		k_sleep(MSEC_PER_SEC * 5);
	}

	printk("Turning on all LEDs\n");
	ret = gpio_port_write(gpio_dev, 0x0);
	if (ret) {
		printk("Error set GPIO port\n");
	}
}
