/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_led_pool.h"

static LED_POOL_DEFINE(led_pool);

static void set_led_blink(struct led_pool_data *led_pool)
{
	bool res = led_pool_set(led_pool, 1, LED_BLINK, K_MSEC(500), K_MSEC(500));

	printk("LED1 blink (500-500) [%u]\n", res);
}

static void set_led_on(struct led_pool_data *led_pool)
{
	bool res = led_pool_set(led_pool, 1, LED_ON);

	printk("LED1 on [%u]\n", res);
}

static void set_led_off(struct led_pool_data *led_pool)
{
	bool res = led_pool_set(led_pool, 1, LED_OFF);

	printk("LED1 off [%u]\n", res);
}

int main(void)
{
	if (!led_pool_init(&led_pool)) {
		printk("led_pool_init failed\n");
		return 0;
	}

	bool res = led_pool_set(&led_pool, 0, LED_BLINK, K_MSEC(100), K_MSEC(900));

	printk("LED0 blink (100-900) [%u]\n", res);

	void (*const led_states[]) (struct led_pool_data *led_pool) = {
		set_led_blink,
		set_led_on,
		set_led_off,
	};

	for (size_t i = 0;;) {
		k_ticks_t tick = sys_clock_tick_get();

		led_states[i++](&led_pool);
		if (i >= ARRAY_SIZE(led_states)) {
			i = 0;
		}
		k_sleep(K_TIMEOUT_ABS_TICKS(tick + K_MSEC(4000).ticks));
	}

	return 0;
}
