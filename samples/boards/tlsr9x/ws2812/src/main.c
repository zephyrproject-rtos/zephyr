/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_ws2812.h"

static WS2812_LED_DEFINE(led_strip);

int main(void)
{
	if (!ws2812_led_init(&led_strip)) {
		printk("ws2812_led_init failed\n");
		return 0;
	}

	printk("Application started with %u RGB LEDs\n", led_strip.led_len);

	for (size_t id = 0;;) {
		/* Increase LED brightness */
		for (size_t permille = 0; permille <= PERMILLE_MAX; permille += 10) {
			if (!ws2812_led_set(&led_strip, id, WS2812_LED_FIXED, permille)) {
				printk("LED%u failed\n", id);
			}
			k_msleep(20);
		}
		/* Switch off LED */
		if (!ws2812_led_set(&led_strip, id, WS2812_LED_OFF)) {
			printk("LED%u failed\n", id);
		}
		id++;
		if (id >= led_strip.led_len * 3) {
			id = 0;
		}
	}

	return 0;
}
