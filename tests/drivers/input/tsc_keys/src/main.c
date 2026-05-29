/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

extern const struct gpio_dt_spec signal_mock;

static void *stm32_tsc_setup(void)
{
	zexpect_ok(gpio_pin_configure_dt(&signal_mock, GPIO_OUTPUT_INACTIVE),
		   "Failed to configure signal_mock pin");

	return NULL;
}

ZTEST_SUITE(stm32_tsc, NULL, stm32_tsc_setup, NULL, NULL, NULL);
