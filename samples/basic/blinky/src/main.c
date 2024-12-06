/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led[4] = {
						GPIO_DT_SPEC_GET(LED0_NODE, gpios),
						GPIO_DT_SPEC_GET(LED1_NODE, gpios),
						GPIO_DT_SPEC_GET(LED2_NODE, gpios),
						GPIO_DT_SPEC_GET(LED3_NODE, gpios)
					};

int main(void)
{
	int ret;
	bool led_state = true;

	for (int i = 0; i < ARRAY_SIZE(led); i++) {
		if (!gpio_is_ready_dt(&led[i])) {
			return 0;
		}

		ret = gpio_pin_configure_dt(&led[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return 0;
		}
	}

	while (1) {
		for (int i = 0; i < ARRAY_SIZE(led); i++) {
			ret = gpio_pin_toggle_dt(&led[i]);
			if (ret < 0) {
				return 0;
			}

			led_state = !led_state;
			printf("LED state: %s\n", led_state ? "ON" : "OFF");
			k_msleep(SLEEP_TIME_MS);
		}
	}
	return 0;
}
