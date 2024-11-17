/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

const struct device *tsc_device_get(void);
const struct gpio_dt_spec *tsc_signal_mock_get(void);

static void *stm32_tsc_setup(void)
{
	const struct device *dev = tsc_device_get();

	zassert_true(device_is_ready(dev), "STM32 TSC device is not ready");

	zexpect_ok(gpio_pin_configure_dt(tsc_signal_mock_get(), GPIO_OUTPUT_INACTIVE),
		   "Failed to configure signal_mock pin");

	return NULL;
}

ZTEST_SUITE(stm32_tsc, NULL, stm32_tsc_setup, NULL, NULL, NULL);
