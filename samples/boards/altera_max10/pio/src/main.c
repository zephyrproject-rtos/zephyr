/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <sys/util.h>

/* GPIO driver name */
#define GPIO_DRV_NAME	CONFIG_GPIO_ALTERA_NIOS2_OUTPUT_DEV_NAME

/* PIO pins[0:3] are wired to LED's */
#define LED_PINS_WIRED		4

static int write_all_leds(struct device *gpio_dev, u32_t value)
{
	int ret;

	for (int i = 0; i < LED_PINS_WIRED; i++) {
		ret = gpio_pin_write(gpio_dev, i, value);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

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
	for (i = 0; i < LED_PINS_WIRED; i++) {
		ret = gpio_pin_configure(gpio_dev, i, GPIO_DIR_OUT);
		if (ret) {
			printk("Error configuring GPIO PORT\n");
			return;
		}
	}

	/*
	 * Note: the LED's are connected in inverse logic
	 * to the Nios-II PIO core.
	 */
	printk("Turning off all LEDs\n");
	ret = write_all_leds(gpio_dev, 1);
	if (ret) {
		printk("Error set GPIO port\n");
	}
	k_sleep(MSEC_PER_SEC * 5U);

	for (i = 0; i < LED_PINS_WIRED; i++) {
		printk("Turn On LED[%d]\n", i);
		ret = gpio_pin_write(gpio_dev, i, 0);
		if (ret) {
			printk("Error writing led pin\n");
		}

		k_sleep(MSEC_PER_SEC * 5U);
		ret = gpio_pin_write(gpio_dev, i, 1);
		if (ret) {
			printk("Error writing led pin\n");
		}
	}

	printk("Turning on all LEDs\n");
	ret = write_all_leds(gpio_dev, 0);
	if (ret) {
		printk("Error set GPIO port\n");
	}
}
