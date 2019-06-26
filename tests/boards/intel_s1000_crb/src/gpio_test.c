/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Sample app to utilize GPIO on Intel S1000 CRB.
 *
 * Intel S1000 CRB
 * ---------------
 *
 * The gpio_dw driver is being used.
 *
 * This sample app toggles GPIO_23. It also waits for
 * GPIO_24 to go high and display a message.
 *
 * If GPIOs 23 and 24 are connected together, the GPIO should
 * triggers every 1 second. And you should see this repeatedly
 * on console:
 * "
 *     Reading GPIO_24 = 0
 *     GPIO_24 triggered
 *     Reading GPIO_24 = 1
 * "
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <device.h>
#include <drivers/gpio.h>

#define GPIO_OUT_PIN            23
#define GPIO_INT_PIN            24
#define GPIO_NAME               "GPIO_"
#define GPIO_DRV_NAME           CONFIG_GPIO_DW_0_NAME

/* size of stack area used by each thread */
#define STACKSIZE               1024

/* scheduling priority used by each thread */
#define PRIORITY                7

/* delay between greetings (in ms) */
#define SLEEPTIME               500

extern struct k_sem thread_sem;

void gpio_test_callback(struct device *port,
		   struct gpio_callback *cb, u32_t pins)
{
	printk(GPIO_NAME "%d triggered\n", GPIO_INT_PIN);
}

static struct gpio_callback gpio_cb;

void setup_gpio(struct device *gpio_dev)
{
	int ret;

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_dev, GPIO_OUT_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
	}

	/* Setup GPIO input, and triggers on rising edge. */
	ret = gpio_pin_configure(gpio_dev, GPIO_INT_PIN,
				 (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH |
				  GPIO_INT_DEBOUNCE));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_INT_PIN);
	}

	gpio_init_callback(&gpio_cb, gpio_test_callback, BIT(GPIO_INT_PIN));

	ret = gpio_add_callback(gpio_dev, &gpio_cb);
	if (ret) {
		printk("Cannot setup callback!\n");
	}

	ret = gpio_pin_enable_callback(gpio_dev, GPIO_INT_PIN);
	if (ret) {
		printk("Error enabling callback!\n");
	}

	/* Disable the GPIO interrupt. It is enabled by default */
	/* irq_disable(DT_GPIO_DW_0_IRQ); */
}

/* gpio_thread is a static thread that is spawned automatically */
void gpio_thread(void *dummy1, void *dummy2, void *dummy3)
{
	struct device *gpio_dev;
	int ret;
	int toggle = 1;
	u32_t read_val = 0U;

	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		printk("Cannot find %s!\n", GPIO_DRV_NAME);
		return;
	}

	setup_gpio(gpio_dev);

	while (1) {
		/* take semaphore */
		k_sem_take(&thread_sem, K_FOREVER);

		if (toggle) {
			toggle = 0;
		} else {
			toggle = 1;
		}

		ret = gpio_pin_write(gpio_dev, GPIO_OUT_PIN, toggle);
		if (ret) {
			printk("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
		}

		gpio_pin_read(gpio_dev, GPIO_INT_PIN, &read_val);
		printk("Reading "GPIO_NAME"%d = %d\n", GPIO_INT_PIN, read_val);

		/* let other threads have a turn */
		k_sem_give(&thread_sem);

		/* wait a while */
		k_sleep(SLEEPTIME);
	}
}

K_THREAD_DEFINE(gpio_thread_id, STACKSIZE, gpio_thread, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
