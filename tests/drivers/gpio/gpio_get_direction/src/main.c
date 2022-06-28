/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_gpio_get_direction.h"

static void test_disconnect(void)
{
	int rv;
	const struct device *port;
	gpio_pin_t pin = TEST_PIN;
	gpio_flags_t flags = GPIO_DISCONNECTED;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	rv = gpio_pin_configure(port, pin, flags);
	zassert_equal(0, rv, "gpio_pin_configure() failed: %d", rv);

	rv = gpio_pin_is_input(port, pin);
	if (rv == -ENOSYS) {
		ztest_test_skip();
	}

	zassert_equal(false, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(port, pin);
	zassert_equal(false, rv, "gpio_pin_is_output() failed: %d", rv);
}

static void test_input(void)
{
	int rv;
	const struct device *port;
	gpio_pin_t pin = TEST_PIN;
	gpio_flags_t flags = GPIO_INPUT;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	rv = gpio_pin_configure(port, pin, flags);
	zassert_equal(0, rv, "gpio_pin_configure() failed: %d", rv);

	rv = gpio_pin_is_input(port, pin);
	if (rv == -ENOSYS) {
		ztest_test_skip();
	}

	zassert_equal(true, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(port, pin);
	zassert_equal(false, rv, "gpio_pin_is_output() failed: %d", rv);
}

static void test_output(void)
{
	int rv;
	const struct device *port;
	gpio_pin_t pin = TEST_PIN;
	gpio_flags_t flags = GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	rv = gpio_pin_configure(port, pin, flags);
	zassert_equal(0, rv, "gpio_pin_configure() failed: %d", rv);

	rv = gpio_pin_is_input(port, pin);
	if (rv == -ENOSYS) {
		ztest_test_skip();
	}

	zassert_equal(false, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(port, pin);
	zassert_equal(true, rv, "gpio_pin_is_output() failed: %d", rv);
}

static void test_input_output(void)
{
	int rv;
	const struct device *port;
	gpio_pin_t pin = TEST_PIN;
	gpio_flags_t flags = GPIO_INPUT | GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	rv = gpio_pin_configure(port, pin, flags);
	zassert_equal(0, rv, "gpio_pin_configure() failed: %d", rv);

	rv = gpio_pin_is_input(port, pin);
	if (rv == -ENOSYS) {
		ztest_test_skip();
	}

	zassert_equal(true, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(port, pin);
	zassert_equal(true, rv, "gpio_pin_is_output() failed: %d", rv);
}

void test_main(void)
{
	ztest_test_suite(gpio_get_direction,
		ztest_unit_test(test_disconnect),
		ztest_unit_test(test_input),
		ztest_unit_test(test_output),
		ztest_unit_test(test_input_output));
	ztest_run_test_suite(gpio_get_direction);
}
