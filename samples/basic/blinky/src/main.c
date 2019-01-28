/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>

#define LED_PORT  LED0_GPIO_CONTROLLER
#define LED_PIN   LED0_GPIO_PIN
#define LED_FLAGS LED0_GPIO_FLAGS

/* 1000 msec = 1 sec */
#define SLEEP_TIME 	1000

void main(void)
{
	int cnt = 0;
	struct device *dev;

	dev = device_get_binding(LED_PORT);
	/* Set LED pin as output */
	gpio_pin_configure(dev, LED_PIN, GPIO_DIR_OUT | LED_FLAGS);

	while (1) {
		/* Set pin to HIGH/LOW every 1 second */
		gpio_pin_write(dev, LED_PIN, cnt % 2);
		cnt++;
		k_sleep(SLEEP_TIME);
	}
}
