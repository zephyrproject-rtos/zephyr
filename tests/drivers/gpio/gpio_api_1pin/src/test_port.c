/*
 * Copyright (c) 2019 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_gpio_api
 * @{
 * @defgroup t_gpio_api_port test_gpio_api_port
 * @brief TestPurpose: verify all gpio port functions using single pin
 *        configured as input/output.
 * @}
 */

#include <limits.h>
#include <sys/util.h>
#include "test_gpio_api.h"

#define TEST_GPIO_PORT_VALUE_MAX         ((1LLU << GPIO_MAX_PINS_PER_PORT) - 1)

static void port_get_raw_and_verify(struct device *port,
				    gpio_port_value_t val_expected, int idx)
{
	gpio_port_value_t val_actual;

	zassert_equal(gpio_port_get_raw(port, &val_actual), 0,
		     "Test point %d: failed to get physical port value", idx);
	zassert_equal(val_expected, val_actual,
		      "Test point %d: invalid physical port get value", idx);
}

static void port_get_and_verify(struct device *port,
				gpio_port_value_t val_expected, int idx)
{
	gpio_port_value_t val_actual;

	zassert_equal(gpio_port_get(port, &val_actual), 0,
		     "Test point %d: failed to get logical port value", idx);
	zassert_equal(val_expected, val_actual,
		      "Test point %d: invalid logical port get value", idx);
}

static void port_set_masked_raw_and_verify(struct device *port,
		gpio_port_pins_t mask, gpio_port_value_t value, int idx)
{
	zassert_equal(gpio_port_set_masked_raw(port, mask, value), 0,
		      "Test point %d: failed to set physical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void port_set_masked_and_verify(struct device *port,
		gpio_port_pins_t mask, gpio_port_value_t value, int idx)
{
	zassert_equal(gpio_port_set_masked(port, mask, value), 0,
		      "Test point %d: failed to set logical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void port_set_bits_raw_and_verify(struct device *port,
		gpio_port_pins_t pins, int idx)
{
	zassert_equal(gpio_port_set_bits_raw(port, pins), 0,
		      "Test point %d: failed to set physical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void port_set_bits_and_verify(struct device *port,
		gpio_port_pins_t pins, int idx)
{
	zassert_equal(gpio_port_set_bits(port, pins), 0,
		      "Test point %d: failed to set logical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void port_clear_bits_raw_and_verify(struct device *port,
		gpio_port_pins_t pins, int idx)
{
	zassert_equal(gpio_port_clear_bits_raw(port, pins), 0,
		      "Test point %d: failed to set physical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void port_clear_bits_and_verify(struct device *port,
		gpio_port_pins_t pins, int idx)
{
	zassert_equal(gpio_port_clear_bits(port, pins), 0,
		      "Test point %d: failed to set logical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void port_set_clr_bits_raw(struct device *port,
		gpio_port_pins_t set_pins, gpio_port_pins_t clear_pins, int idx)
{
	zassert_equal(gpio_port_set_clr_bits_raw(port, set_pins, clear_pins), 0,
		      "Test point %d: failed to set physical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void port_set_clr_bits(struct device *port,
		gpio_port_pins_t set_pins, gpio_port_pins_t clear_pins, int idx)
{
	zassert_equal(gpio_port_set_clr_bits(port, set_pins, clear_pins), 0,
		      "Test point %d: failed to set logical port value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

/** @brief Verify gpio_port_toggle_bits function.
 *
 * - Verify that gpio_port_toggle_bits function changes pin state from active to
 *   inactive and vice versa.
 */
void test_gpio_port_toggle(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	port_set_bits_raw_and_verify(port, 1 << TEST_PIN, 0);

	zassert_equal(gpio_port_get(port, &val_expected), 0,
		      "Failed to get logical port value");

	for (int i = 0; i < 5; i++) {
		ret = gpio_port_toggle_bits(port, 1 << TEST_PIN);
		zassert_equal(ret, 0, "Failed to toggle pin value");
		k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);

		val_expected ^= 1 << TEST_PIN;

		port_get_raw_and_verify(port, val_expected, i);
	}
}

void test_gpio_port_set_masked_get_raw(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	const gpio_port_value_t test_vector[] = {
		0xEE11EE11,
		0x11EE11EE,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
		0x00000000,
		0x55555555,
		0xAAAAAAAA,
		0x00000000,
		0x00000000,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get_raw(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get physical port value");

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_masked_raw_and_verify(port, 1 << TEST_PIN, test_vector[i], i);

		val_expected &= ~(1 << TEST_PIN);
		val_expected |= test_vector[i] & (1 << TEST_PIN);

		port_get_raw_and_verify(port, val_expected, i);
	}
}

void test_gpio_port_set_masked_get(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	const gpio_port_value_t test_vector[] = {
		0xEE11EE11,
		0x11EE11EE,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
		0x00000000,
		0x55555555,
		0xAAAAAAAA,
		0x00000000,
		0x00000000,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get logical port value");

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_masked_and_verify(port, 1 << TEST_PIN, test_vector[i], i);

		val_expected &= ~(1 << TEST_PIN);
		val_expected |= test_vector[i] & (1 << TEST_PIN);

		port_get_and_verify(port, val_expected, i);
	}
}

void test_gpio_port_set_masked_get_active_high(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	const gpio_port_value_t test_vector[] = {
		0xCC33CC33,
		0x33CC33CC,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
		0x00000000,
		0x00000000,
		0x55555555,
		0x00000000,
		0xAAAAAAAA,
		0x00000000,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT |
				 GPIO_ACTIVE_HIGH);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get logical port value");

	TC_PRINT("Step 1: Set logical, get logical and physical port value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_masked_and_verify(port, 1 << TEST_PIN, test_vector[i], i);

		val_expected &= ~(1 << TEST_PIN);
		val_expected |= test_vector[i] & (1 << TEST_PIN);

		port_get_and_verify(port, val_expected, i);
		port_get_raw_and_verify(port, val_expected, i);
	}

	TC_PRINT("Step 2: Set physical, get logical and physical port value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_masked_raw_and_verify(port, 1 << TEST_PIN, test_vector[i], i);

		val_expected &= ~(1 << TEST_PIN);
		val_expected |= test_vector[i] & (1 << TEST_PIN);

		port_get_and_verify(port, val_expected, i);
		port_get_raw_and_verify(port, val_expected, i);
	}
}

void test_gpio_port_set_masked_get_active_low(void)
{
	struct device *port;
	gpio_port_value_t val_expected, val_raw_expected;
	int ret;

	const gpio_port_value_t test_vector[] = {
		0xCC33CC33,
		0x33CC33CC,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
		0x00000000,
		0x00000000,
		0x55555555,
		0x00000000,
		0xAAAAAAAA,
		0x00000000,
		TEST_GPIO_PORT_VALUE_MAX,
		0x00000000,
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT |
				 GPIO_ACTIVE_LOW);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get_raw(port, &val_raw_expected);
	zassert_equal(ret, 0, "Failed to get physical port value");
	ret = gpio_port_get(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get logical port value");

	TC_PRINT("Step 1: Set logical, get logical and physical port value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_masked_and_verify(port, 1 << TEST_PIN, test_vector[i], i);

		val_raw_expected &= ~(1 << TEST_PIN);
		val_raw_expected |= (test_vector[i] & (1 << TEST_PIN)) ^ (1 << TEST_PIN);
		val_expected &= ~(1 << TEST_PIN);
		val_expected |= test_vector[i] & (1 << TEST_PIN);

		port_get_and_verify(port, val_expected, i);
		port_get_raw_and_verify(port, val_raw_expected, i);
	}

	TC_PRINT("Step 2: Set physical, get logical and physical port value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_masked_raw_and_verify(port, 1 << TEST_PIN, test_vector[i], i);

		val_raw_expected &= ~(1 << TEST_PIN);
		val_raw_expected |= test_vector[i] & (1 << TEST_PIN);
		val_expected &= ~(1 << TEST_PIN);
		val_expected |= (test_vector[i] & (1 << TEST_PIN)) ^ (1 << TEST_PIN);

		port_get_and_verify(port, val_expected, i);
		port_get_raw_and_verify(port, val_raw_expected, i);
	}
}

void test_gpio_port_set_bits_clear_bits_raw(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	const gpio_port_value_t test_vector[][2] = {
		/* set value, clear value */
		{0xEE11EE11, 0xEE11EE11},
		{0x11EE11EE, TEST_GPIO_PORT_VALUE_MAX},
		{0x00000000, 0x55555555},
		{TEST_GPIO_PORT_VALUE_MAX, 0xAAAAAAAA},
		{TEST_GPIO_PORT_VALUE_MAX, TEST_GPIO_PORT_VALUE_MAX},
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get_raw(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get logical port value");

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_bits_raw_and_verify(port, test_vector[i][0], i);
		val_expected |= test_vector[i][0] & (1 << TEST_PIN);
		port_get_raw_and_verify(port, val_expected, i);

		port_clear_bits_raw_and_verify(port, test_vector[i][1], i);
		val_expected &= ~(test_vector[i][1] & (1 << TEST_PIN));
		port_get_raw_and_verify(port, val_expected, i);
	}
}

void test_gpio_port_set_bits_clear_bits(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	const gpio_port_value_t test_vector[][2] = {
		/* set value, clear value */
		{TEST_GPIO_PORT_VALUE_MAX, 0xAAAAAAAA},
		{0x00000000, TEST_GPIO_PORT_VALUE_MAX},
		{0xCC33CC33, 0x33CC33CC},
		{0x33CC33CC, 0x33CC33CC},
		{0x00000000, 0x55555555},
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get logical port value");

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_bits_and_verify(port, test_vector[i][0], i);
		val_expected |= test_vector[i][0] & (1 << TEST_PIN);
		port_get_and_verify(port, val_expected, i);

		port_clear_bits_and_verify(port, test_vector[i][1], i);
		val_expected &= ~(test_vector[i][1] & (1 << TEST_PIN));
		port_get_and_verify(port, val_expected, i);
	}
}

void test_gpio_port_set_clr_bits_raw(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	const gpio_port_value_t test_vector[][2] = {
		/* set value, clear value */
		{0xEE11EE11, 0x11EE11EE},
		{0x00000000, TEST_GPIO_PORT_VALUE_MAX},
		{0x55555555, 0x00000000},
		{TEST_GPIO_PORT_VALUE_MAX, 0x00000000},
		{0xAAAAAAAA, 0x00000000},
		{0x00000000, TEST_GPIO_PORT_VALUE_MAX},
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get_raw(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get logical port value");

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_clr_bits_raw(port, test_vector[i][0], test_vector[i][1], i);
		val_expected |= test_vector[i][0] & (1 << TEST_PIN);
		val_expected &= ~(test_vector[i][1] & (1 << TEST_PIN));
		port_get_raw_and_verify(port, val_expected, i);
	}
}

void test_gpio_port_set_clr_bits(void)
{
	struct device *port;
	gpio_port_value_t val_expected;
	int ret;

	const gpio_port_value_t test_vector[][2] = {
		/* set value, clear value */
		{0xEE11EE11, 0x11EE11EE},
		{0x00000000, TEST_GPIO_PORT_VALUE_MAX},
		{0x55555555, 0x00000000},
		{TEST_GPIO_PORT_VALUE_MAX, 0x00000000},
		{0xAAAAAAAA, 0x00000000},
		{0x00000000, TEST_GPIO_PORT_VALUE_MAX},
	};

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	ret = gpio_port_get(port, &val_expected);
	zassert_equal(ret, 0, "Failed to get logical port value");

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		port_set_clr_bits(port, test_vector[i][0], test_vector[i][1], i);
		val_expected |= test_vector[i][0] & (1 << TEST_PIN);
		val_expected &= ~(test_vector[i][1] & (1 << TEST_PIN));
		port_get_and_verify(port, val_expected, i);
	}
}
