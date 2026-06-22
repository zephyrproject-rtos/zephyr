/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

/* size of stack area used by each thread */
#define STACKSIZE (2048)

/* scheduling priority used by each thread */
#define PRIORITY 7

void test_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	printk("Interrupt detected!\n");
}

void gpio_sample(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio_emul));

	if (!device_is_ready(dev)) {
		printk("%s: device not ready.\n", dev->name);
		return;
	}

	/* Configure pin 0 as output and toggle */
	gpio_pin_configure(dev, 0, GPIO_OUTPUT);
	gpio_pin_set(dev, 0, 1);
	gpio_pin_set(dev, 0, 0);

	/* Configure pin 1 as input */
	gpio_pin_configure(dev, 1, GPIO_INPUT);

	/* Read pin 1 */
	gpio_emul_input_set(dev, 1, 1);
	gpio_pin_get(dev, 1);
	gpio_emul_input_set(dev, 1, 0);
	gpio_pin_get(dev, 1);

	/* Setup pin 1 for interrupt */
	gpio_pin_interrupt_configure(dev, 1, GPIO_INT_EDGE_RISING);
	static struct gpio_callback gpio_cb;

	gpio_init_callback(&gpio_cb, test_handler, BIT(1));
	gpio_add_callback(dev, &gpio_cb);

	/* Trigger interrupt */
	gpio_emul_input_set(dev, 1, 1);

	/* Remove interrupt */
	gpio_remove_callback(dev, &gpio_cb);
}

static void gpio_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	gpio_sample();
}

K_THREAD_DEFINE(thread_gpio, STACKSIZE, gpio_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
