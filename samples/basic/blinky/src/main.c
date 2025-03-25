/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

/* Check if LED1_NODE is defined in cases such as with matrix displays. */
#define LED1_IS_DEFINED DT_NODE_EXISTS(LED1_NODE)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Extended support for matrix displays. */
#if LED1_IS_DEFINED
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#endif

int main(void)
{
	int ret;
	bool led_state = true;

	if (!gpio_is_ready_dt(&led0)) {
		return 0;
	}
	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

#if LED1_IS_DEFINED
	if (!gpio_is_ready_dt(&led1)) {
		return 0;
	}
	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
#endif

	while (1) {
		ret = gpio_pin_toggle_dt(&led0);
		if (ret < 0) {
			return 0;
		}
#if LED1_IS_DEFINED
		ret = gpio_pin_toggle_dt(&led1);
		if (ret < 0) {
			return 0;
		}
#endif

		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
