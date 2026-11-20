/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

#ifdef __GNUC__
#define FAST_GPIO_FLATTEN __attribute__((flatten))
#else
#define FAST_GPIO_FLATTEN
#endif

/* Inline the whole GPIO call stack into the loop body, overriding the
 * inliner's cost heuristics: the port ops are address-taken by the api
 * vtable, which makes the compiler reluctant to also inline the
 * devirtualized direct calls.
 */
FAST_GPIO_FLATTEN
int main(void)
{

	(void)gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	printk("fast_gpio: toggling %s pin %u\n", led.port->name, led.pin);

	/* Sixteen toggles back to back, then a pause. Unrolling amortizes
	 * the loop branch so the steady state gap between edges is the
	 * sustained bus write throughput to the GPIO block, and the pause
	 * makes each burst stand alone on a scope: trigger on the first
	 * edge after the quiet gap.
	 */
	while (1) {
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);
		(void)gpio_pin_toggle_dt(&led);

		k_busy_wait(10);
	}

	return 0;
}
