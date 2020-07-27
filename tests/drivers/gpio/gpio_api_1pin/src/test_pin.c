/*
 * Copyright (c) 2019 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <limits.h>
#include <sys/util.h>
#include "test_gpio_api.h"

static void pin_get_raw_and_verify(struct device *port, unsigned int pin,
				   int val_expected, int idx)
{
	int val_actual;

	val_actual = gpio_pin_get_raw(port, pin);
	zassert_true(val_actual >= 0,
		     "Test point %d: failed to get physical pin value", idx);
	zassert_equal(val_expected, val_actual,
		      "Test point %d: invalid physical pin get value", idx);
}

static void pin_get_and_verify(struct device *port, unsigned int pin,
			       int val_expected, int idx)
{
	int val_actual;

	val_actual = gpio_pin_get(port, pin);
	zassert_true(val_actual >= 0,
		     "Test point %d: failed to get logical pin value", idx);
	zassert_equal(val_expected, val_actual,
		      "Test point %d: invalid logical pin get value", idx);
}

static void pin_set_raw_and_verify(struct device *port, unsigned int pin,
				   int val, int idx)
{
	zassert_equal(gpio_pin_set_raw(port, pin, val), 0,
		      "Test point %d: failed to set physical pin value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

static void pin_set_and_verify(struct device *port, unsigned int pin, int val,
			       int idx)
{
	zassert_equal(gpio_pin_set(port, pin, val), 0,
		      "Test point %d: failed to set logical pin value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

/** @brief Verify gpio_pin_toggle function.
 *
 * - Verify that gpio_pin_toggle function changes pin state from active to
 *   inactive and vice versa.
 */
void test_gpio_pin_toggle(void)
{
	struct device *port;
	int val_expected;
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

	pin_set_raw_and_verify(port, TEST_PIN, 1, 0);

	for (int i = 0; i < 5; i++) {
		ret = gpio_pin_toggle(port, TEST_PIN);
		zassert_equal(ret, 0, "Failed to toggle pin value");
		k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);

		val_expected = i % 2;

		pin_get_raw_and_verify(port, TEST_PIN, val_expected, i);
	}
}

/** @brief Verify visually gpio_pin_toggle function.
 *
 * This test configures the pin using board DTS flags which should
 * correctly set pin active state via GPIO_ACTIVE_LOW/_HIGH flags.
 * It is possible to do a visual check to confirm that "LED ON", "LED OFF"
 * messages correspond to the LED being turned ON or OFF.
 *
 * - Verify visually that gpio_pin_toggle function changes pin state from active
 *   to inactive and vice versa.
 */
void test_gpio_pin_toggle_visual(void)
{
	struct device *port;
	int val_expected;
	int ret;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT |
				 TEST_PIN_DTS_FLAGS);
	zassert_equal(ret, 0, "Failed to configure the pin");

	pin_set_and_verify(port, TEST_PIN, 1, 0);
	TC_PRINT("LED ON\n");

	for (int i = 0; i < 3; i++) {
		k_sleep(K_SECONDS(2));

		ret = gpio_pin_toggle(port, TEST_PIN);
		zassert_equal(ret, 0, "Failed to toggle pin value");
		k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);

		val_expected = i % 2;
		TC_PRINT("LED %s\n", val_expected == 1 ? "ON" : "OFF");
	}
}

/** @brief Verify gpio_pin_set_raw, gpio_pin_get_raw functions.
 *
 * - Verify that gpio_pin_get_raw reads the same value as set by
 *   gpio_pin_set_raw function.
 */
void test_gpio_pin_set_get_raw(void)
{
	struct device *port;
	int val_expected;
	int ret;

	const int test_vector[] = {
		4, 1, 45, 0, 0, -7, 0, 0, 0, INT_MAX, INT_MIN, 0
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

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		pin_set_raw_and_verify(port, TEST_PIN, test_vector[i], i);

		val_expected = test_vector[i] != 0 ? 1 : 0;

		pin_get_raw_and_verify(port, TEST_PIN, val_expected, i);
	}
}

/** @brief Verify gpio_pin_set, gpio_pin_get functions.
 *
 * - Verify that gpio_pin_get reads the same value as set by gpio_pin_set
 *   function.
 */
void test_gpio_pin_set_get(void)
{
	struct device *port;
	int val_expected;
	int ret;

	const int test_vector[] = {
		1, 2, 3, 0, 4, 0, 0, 0, 17, INT_MAX, INT_MIN, 0
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

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		pin_set_and_verify(port, TEST_PIN, test_vector[i], i);

		val_expected = test_vector[i] != 0 ? 1 : 0;

		pin_get_and_verify(port, TEST_PIN, val_expected, i);
	}
}

/** @brief Verify GPIO_ACTIVE_HIGH flag.
 *
 * - Verify that there is no functional difference between gpio_pin_set_raw and
 *   gpio_pin_set functions if the pin is configured as Active High.
 * - Verify that there is no functional difference between gpio_pin_get_raw and
 *   gpio_pin_get functions if the pin is configured as Active High.
 */
void test_gpio_pin_set_get_active_high(void)
{
	struct device *port;
	int val_expected;
	int ret;

	const int test_vector[] = {0, 2, 0, 9, -1, 0, 0, 1, INT_MAX, INT_MIN};

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

	TC_PRINT("Step 1: Set logical, get logical and physical pin value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		pin_set_and_verify(port, TEST_PIN, test_vector[i], i);

		val_expected = test_vector[i] != 0 ? 1 : 0;

		pin_get_and_verify(port, TEST_PIN, val_expected, i);
		pin_get_raw_and_verify(port, TEST_PIN, val_expected, i);
	}

	TC_PRINT("Step 2: Set physical, get logical and physical pin value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		pin_set_raw_and_verify(port, TEST_PIN, test_vector[i], i);

		val_expected = test_vector[i] != 0 ? 1 : 0;

		pin_get_and_verify(port, TEST_PIN, val_expected, i);
		pin_get_raw_and_verify(port, TEST_PIN, val_expected, i);
	}
}

/** @brief Verify GPIO_ACTIVE_LOW flag.
 *
 * - Verify that value set by gpio_pin_set function is inverted compared to
 *   gpio_pin_set_raw if the pin is configured as Active Low.
 * - Verify that value read by gpio_pin_get function is inverted compared to
 *   gpio_pin_get_raw if the pin is configured as Active Low.
 */
void test_gpio_pin_set_get_active_low(void)
{
	struct device *port;
	int val_expected, val_raw_expected;
	int ret;

	const int test_vector[] = {0, 4, 0, 0, 1, 8, -3, -12, 0};

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

	TC_PRINT("Step 1: Set logical, get logical and physical pin value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		pin_set_and_verify(port, TEST_PIN, test_vector[i], i);

		val_expected = (test_vector[i] != 0) ? 1 : 0;
		val_raw_expected = (val_expected != 0) ? 0 : 1;

		pin_get_and_verify(port, TEST_PIN, val_expected, i);
		pin_get_raw_and_verify(port, TEST_PIN, val_raw_expected, i);
	}

	TC_PRINT("Step 2: Set physical, get logical and physical pin value\n");
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		pin_set_raw_and_verify(port, TEST_PIN, test_vector[i], i);

		val_expected = (test_vector[i] != 0) ? 0 : 1;
		val_raw_expected = (val_expected != 0) ? 0 : 1;

		pin_get_and_verify(port, TEST_PIN, val_expected, i);
		pin_get_raw_and_verify(port, TEST_PIN, val_raw_expected, i);
	}
}
