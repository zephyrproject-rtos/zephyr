/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <led.h>

#define LED_DEV_NAME	LED0_LABEL
#define LED		LED0_GPIO_PIN

/* 1000 msec = 1 sec */
#define SLEEP_TIME 	1000

void main(void)
{
	struct device *dev;

	dev = device_get_binding(LED_DEV_NAME);

	while (1) {
		/* Set pin to HIGH/LOW every 1 second */
		led_on(dev, LED);
		k_sleep(SLEEP_TIME);

		led_off(dev, LED);
		k_sleep(SLEEP_TIME);
	}
}
