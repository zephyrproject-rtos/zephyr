/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* A separate translation unit using the generic GPIO API, to show the
 * driver keeps working normally for code outside the fused translation
 * unit: these calls dispatch through the device api vtable like any
 * other driver consumer.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led_other = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

void consumer_blink(void)
{
	(void)gpio_pin_configure_dt(&led_other, GPIO_OUTPUT_INACTIVE);

	for (int i = 0; i < 10; i++) {
		(void)gpio_pin_toggle_dt(&led_other);
	}
}
