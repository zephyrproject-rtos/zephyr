/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/device_runtime.h>

/* define SLEEP_TIME_MS higher than <st,counter-value> in ms */
#if DT_PROP(DT_NODELABEL(stm32_lp_tick_source), st_counter_value)
#define SLEEP_TIME_MS   (DT_PROP(DT_NODELABEL(stm32_lp_tick_source), st_counter_value) * 1400)
#else
#define SLEEP_TIME_MS   2000
#endif

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	bool led_is_on = true;

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));

	printk("Device ready\n");

	while (true) {
		pm_device_runtime_get(led.port);
		if (led_is_on) {
			gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		} else {
			gpio_pin_configure_dt(&led, GPIO_DISCONNECTED);
		}
		pm_device_runtime_put(led.port);
		k_msleep(SLEEP_TIME_MS);
		led_is_on = !led_is_on;
	}
	return 0;
}
