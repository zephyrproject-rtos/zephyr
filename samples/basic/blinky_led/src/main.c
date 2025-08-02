/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct led_dt_spec led = LED_DT_SPEC_GET(LED0_NODE);

int main(void)
{
	int ret;
	bool led_state = true;

	if (!led_is_ready_dt(&led)) {
		return 0;
	}

	while (1) {
		if (led_state) {
			ret = led_on_dt(&led);
		} else {
			ret = led_off_dt(&led);
		}
		if (ret < 0) {
			return 0;
		}

		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
