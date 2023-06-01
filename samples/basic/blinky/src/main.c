/*
 * Copyright (c) 2016-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* Toggle count */
#define GPIO_TOGGLE_COUNT 10

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;
	uint32_t count = GPIO_TOGGLE_COUNT;

	printk("GPIO blinky application started\n");
	if (!gpio_is_ready_dt(&led)) {
		printk("GPIO not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("GPIO pin configuration failed\n");
		return 0;
	}

	printk("GPIO toggle started for %d times\n", GPIO_TOGGLE_COUNT);
	while (count) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			printk("GPIO pin toggle failed\n");
			return 0;
		}
		k_msleep(SLEEP_TIME_MS);
		count--;
	}
	printk("GPIO blinky Application completed!!!\n");
	return 0;
}
