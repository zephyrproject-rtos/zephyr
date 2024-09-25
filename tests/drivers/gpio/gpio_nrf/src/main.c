/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
#define TEST_NODE DT_GPIO_CTLR(DT_ALIAS(led0), gpios)
#define TEST_PIN DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#else
#error Unsupported board
#endif

/*
 * Nordic Semiconductor specific pin drive configurations
 */
ZTEST(gpio_nrf, test_gpio_high_drive_strength)
{
	int err;
	const struct device *port;

	port = DEVICE_DT_GET(TEST_NODE);
	zassert_true(device_is_ready(port), "GPIO dev is not ready");

	err = gpio_pin_configure(port, TEST_PIN, GPIO_PUSH_PULL | NRF_GPIO_DRIVE_S0H1);
	zassert_equal(
		err, 0,
		"Failed to configure the pin as an P-P output with drive: NRF_GPIO_DRIVE_S0H1, err=%d",
		err);

	err = gpio_pin_configure(port, TEST_PIN, GPIO_PUSH_PULL | NRF_GPIO_DRIVE_H0S1);
	zassert_equal(
		err, 0,
		"Failed to configure the pin as an P-P output with drive: NRF_GPIO_DRIVE_H0S1, err=%d",
		err);

	err = gpio_pin_configure(port, TEST_PIN, GPIO_PUSH_PULL | NRF_GPIO_DRIVE_H0H1);
	zassert_equal(
		err, 0,
		"Failed to configure the pin as an P-P output with drive: NRF_GPIO_DRIVE_H0H1, err=%d",
		err);

	err = gpio_pin_configure(port, TEST_PIN, GPIO_OPEN_DRAIN | NRF_GPIO_DRIVE_H0S1);
	zassert_equal(
		err, 0,
		"Failed to configure the pin as an O-D output with drive: NRF_GPIO_DRIVE_H0S1, err=%d",
		err);

	err = gpio_pin_configure(port, TEST_PIN, GPIO_OPEN_SOURCE | NRF_GPIO_DRIVE_S0H1);
	zassert_equal(
		err, 0,
		"Failed to configure the pin as an O-S output with drive: NRF_GPIO_DRIVE_S0H1, err=%d",
		err);
}

ZTEST_SUITE(gpio_nrf, NULL, NULL, NULL, NULL, NULL);
