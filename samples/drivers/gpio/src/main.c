/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver sample
 *
 * This sample toggles LED1 and waits for an interrupt on BUTTON1.
 * Note that an internal pull-up is set on BUTTON1 as the button
 * only drives low when pressed.
 */
#include <zephyr.h>

#include <sys/printk.h>

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

#if defined(DT_ALIAS_SW0_GPIOS_CONTROLLER) && defined(DT_ALIAS_LED0_GPIOS_CONTROLLER)
#define GPIO_OUT_DRV_NAME DT_ALIAS_LED0_GPIOS_CONTROLLER
#define GPIO_OUT_PIN  DT_ALIAS_LED0_GPIOS_PIN
#define GPIO_IN_DRV_NAME DT_ALIAS_SW0_GPIOS_CONTROLLER
#define GPIO_INT_PIN  DT_ALIAS_SW0_GPIOS_PIN
#else
#error Change the pins based on your configuration. This sample \
	defaults to built-in buttons and LEDs
#endif

void gpio_callback(struct device *port,
		   struct gpio_callback *cb, u32_t pins)
{
	printk("Pin %s.%d triggered\n", port->config->name, pins);
}

static struct gpio_callback gpio_cb;

void main(void)
{
	struct device *gpio_out_dev, *gpio_in_dev;
	int ret;
	int toggle = 1;

	gpio_out_dev = device_get_binding(GPIO_OUT_DRV_NAME);
	if (!gpio_out_dev) {
		printk("Cannot find %s!\n", GPIO_OUT_DRV_NAME);
		return;
	}

	gpio_in_dev = device_get_binding(GPIO_IN_DRV_NAME);
	if (!gpio_in_dev) {
		printk("Cannot find %s!\n", GPIO_IN_DRV_NAME);
		return;
	}

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_out_dev, GPIO_OUT_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring pin %s.%d!\n",
		       gpio_out_dev->config->name,
		       GPIO_OUT_PIN);
	}

	/* Setup GPIO input, and triggers on rising edge. */
#ifdef CONFIG_SOC_CC2650
	ret = gpio_pin_configure(gpio_in_dev, GPIO_INT_PIN,
				 (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH |
				  GPIO_INT_DEBOUNCE | GPIO_PUD_PULL_UP));
#else
	ret = gpio_pin_configure(gpio_in_dev, GPIO_INT_PIN,
				 (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH |
				  GPIO_INT_DEBOUNCE));
#endif
	if (ret) {
		printk("Error configuring pin %s.%d!\n",
		       gpio_in_dev->config->name,
		       GPIO_INT_PIN);
	}

	gpio_init_callback(&gpio_cb, gpio_callback, BIT(GPIO_INT_PIN));

	ret = gpio_add_callback(gpio_in_dev, &gpio_cb);
	if (ret) {
		printk("Cannot setup callback!\n");
	}

	ret = gpio_pin_enable_callback(gpio_in_dev, GPIO_INT_PIN);
	if (ret) {
		printk("Error enabling callback!\n");
	}

	while (1) {
		printk("Toggling pin %s.%d\n",
		       gpio_out_dev->config->name,
		       GPIO_OUT_PIN);

		ret = gpio_pin_write(gpio_out_dev, GPIO_OUT_PIN, toggle);
		if (ret) {
			printk("Error set pin %s.%d!\n",
			       gpio_out_dev->config->name,
			       GPIO_OUT_PIN);
		}

		if (toggle) {
			toggle = 0;
		} else {
			toggle = 1;
		}

		k_sleep(K_SECONDS(2));
	}
}
