/*
 * Copyright (c) 2019 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <limits.h>
#include <zephyr/sys/util.h>
#include "test_gpio_api.h"

#define TEST_GPIO_MAX_SINGLE_ENDED_RISE_FALL_TIME_MS    100
#define TEST_POINT(n)   (n)

static void pin_get_raw_and_verify(const struct device *port,
				   unsigned int pin,
				   int val_expected, int idx)
{
	int val_actual;

	val_actual = gpio_pin_get_raw(port, pin);
	zassert_true(val_actual >= 0,
		     "Test point %d: failed to get pin value", idx);
	zassert_equal(val_expected, val_actual,
		      "Test point %d: invalid pin get value", idx);
}

static void pin_set_raw_and_verify(const struct device *port,
				   unsigned int pin,
				   int val, int idx)
{
	zassert_equal(gpio_pin_set_raw(port, pin, val), 0,
		      "Test point %d: failed to set pin value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

/** @brief Verify gpio_pin_configure function in push pull mode.
 *
 * - Configure pin in in/out mode, verify that gpio_pin_set_raw /
 *   gpio_pin_get_raw operations change pin state.
 * - Verify that GPIO_OUTPUT_HIGH flag is initializing the pin to high.
 * - Verify that GPIO_OUTPUT_LOW flag is initializing the pin to low.
 * - Verify that configuring the pin as an output without initializing it
 *   to high or low does not change pin state.
 * - Verify that it is not possible to change value of a pin via
 *   gpio_pin_set_raw function if pin is configured as an input.
 */
ZTEST(gpio_api_1pin_conf, test_gpio_pin_configure_push_pull)
{
	const struct device *port;
	int ret;

	port = DEVICE_DT_GET(TEST_NODE);
	zassert_true(device_is_ready(port), "GPIO dev is not ready");

	TC_PRINT("Running test on port=%s, pin=%d\n", port->name, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT);
	zassert_equal(ret, 0, "Failed to configure the pin as an output");

	pin_set_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(1));
	pin_set_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(1));

	/* Configure pin in in/out mode, verify that gpio_pin_set_raw /
	 * gpio_pin_get_raw operations change pin state.
	 */
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin in in/out mode");

	pin_set_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(2));
	pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(2));

	pin_set_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(3));
	pin_get_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(3));

	pin_set_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(4));
	pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(4));

	/* Verify that GPIO_OUTPUT_HIGH flag is initializing the pin to high. */
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT_HIGH | GPIO_INPUT);
	zassert_equal(ret, 0,
		      "Failed to configure the pin in in/out mode and "
		      "initialize it to high");

	pin_get_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(5));

	/* Verify that configuring the pin as an output without initializing it
	 * to high or low does not change pin state.
	 */
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	zassert_equal(ret, 0, "Failed to configure the pin in in/out mode");

	pin_get_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(6));

	/* Verify that GPIO_OUTPUT_LOW flag is initializing the pin to low. */
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT_LOW | GPIO_INPUT);
	zassert_equal(ret, 0,
		      "Failed to configure the pin in in/out mode and "
		      "initialize it to low");

	pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(7));

	/* Verify that configuring the pin as an output without initializing it
	 * to high or low does not change pin state.
	 */
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT | GPIO_INPUT);
	zassert_equal(ret, 0, "Failed to configure the pin in in/out mode");

	pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(8));

	/* Verify that it is not possible to change value of a pin via
	 * gpio_pin_set_raw function if pin is configured as an input.
	 */
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_INPUT);
	zassert_equal(ret, 0, "Failed to configure the pin as an input");
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);

	int pin_in_val;

	pin_in_val = gpio_pin_get_raw(port, TEST_PIN);
	zassert_true(pin_in_val >= 0,
		     "Test point %d: failed to get pin value", TEST_POINT(9));

	pin_set_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(10));
	pin_get_raw_and_verify(port, TEST_PIN, pin_in_val, TEST_POINT(10));

	pin_set_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(11));
	pin_get_raw_and_verify(port, TEST_PIN, pin_in_val, TEST_POINT(11));
}

/** @brief Verify gpio_pin_configure function in single ended mode.
 *
 * @note This test verifies single ended mode only partially. It should not be
 *       used as a sign off test.
 *
 * - Verify that pin configured in Open Drain mode and initialized
 *   to high results in pin high value if the same pin configured
 *   as input is high. Drivers that do not support Open Drain flag
 *   return ENOTSUP.
 * - Setting pin configured in Open Drain mode to low results in
 *   pin low value if the same pin configured as input is high.
 * - Verify that pin configured in Open Source mode and
 *   initialized to low results in pin high value if the same pin
 *   configured as input is high. Drivers that do not support Open
 *   Source flag return ENOTSUP.
 * - Verify that pin configured in Open Source mode and
 *   initialized to low results in pin low value if the same pin
 *   configured as input is low. Drivers that do not support Open
 *   Source flag return ENOTSUP.
 * - Setting pin configured in Open Source mode to high results in
 *   pin high value if the same pin configured as input is low.
 * - Verify that pin configured in Open Drain mode and
 *   initialized to high results in pin low value if the same pin
 *   configured as input is low. Drivers that do not support Open
 *   Drain flag return ENOTSUP.
 */
ZTEST(gpio_api_1pin_conf, test_gpio_pin_configure_single_ended)
{
	const struct device *port;
	int pin_in_val;
	int pin_val;
	unsigned int cfg_flag;
	int ret;

	port = DEVICE_DT_GET(TEST_NODE);
	zassert_true(device_is_ready(port), "GPIO dev is not ready");

	TC_PRINT("Running test on port=%s, pin=%d\n", port->name, TEST_PIN);

	/* If the LED is connected directly between the MCU pin and power or
	 * ground we can test only one of the Open Drain / Open Source modes.
	 * Guess pin level when the LED is off. If the pin is not connected
	 * directly to LED but instead the signal is routed to an input of
	 * another chip we could test both modes. However, there is no way to
	 * find it out so only one mode is tested.
	 */
	if ((TEST_PIN_DTS_FLAGS & GPIO_ACTIVE_LOW) != 0) {
		cfg_flag = GPIO_PULL_UP;
		pin_val = 1;
	} else {
		cfg_flag = GPIO_PULL_DOWN;
		pin_val = 0;
	}

	/* Configure pin as an input with pull up/down and check input level. */
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_INPUT | cfg_flag);
	if (ret == -ENOTSUP) {
		TC_PRINT("Pull Up / Pull Down pin bias is not supported\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure pin as an input");

	k_sleep(K_MSEC(TEST_GPIO_MAX_SINGLE_ENDED_RISE_FALL_TIME_MS));

	pin_in_val = gpio_pin_get_raw(port, TEST_PIN);
	zassert_true(pin_in_val >= 0, "Failed to get pin value");

	if (pin_val != pin_in_val) {
		TC_PRINT("Board configuration does not allow to run the test\n");
		ztest_test_skip();
		return;
	}

	if (pin_val == 1) {
		TC_PRINT("When configured as input test pin value is high\n");
		/* Verify that pin configured in Open Drain mode and initialized
		 * to high results in pin high value if the same pin configured
		 * as input is high. Drivers that do not support Open Drain flag
		 * return -ENOTSUP.
		 */
		ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT_HIGH |
				GPIO_OPEN_DRAIN | GPIO_INPUT | GPIO_PULL_UP);
		if (ret == -ENOTSUP) {
			TC_PRINT("Open Drain configuration or Pull Up pin "
				 "bias is not supported\n");
			ztest_test_skip();
			return;
		}
		zassert_equal(ret, 0,
			      "Failed to configure the pin in Open Drain mode");

		pin_get_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(1));

		/* Setting pin configured in Open Drain mode to low results in
		 * pin low value if the same pin configured as input is high.
		 */
		pin_set_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(2));
		pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(2));

		/* Verify that pin configured in Open Source mode and
		 * initialized to low results in pin high value if the same pin
		 * configured as input is high. Drivers that do not support Open
		 * Source flag return -ENOTSUP.
		 */
		ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT_LOW |
				GPIO_OPEN_SOURCE | GPIO_INPUT | GPIO_PULL_UP);
		if (ret == -ENOTSUP) {
			TC_PRINT("Open Source configuration or Pull Up pin "
				 "bias is not supported\n");
			return;
		}
		zassert_equal(ret, 0,
			      "Failed to configure the pin in Open Source mode");

		k_sleep(K_MSEC(TEST_GPIO_MAX_SINGLE_ENDED_RISE_FALL_TIME_MS));

		pin_get_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(3));

		pin_set_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(4));
		pin_get_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(4));
	} else {
		TC_PRINT("When configured as input test pin value is low\n");
		/* Verify that pin configured in Open Source mode and
		 * initialized to low results in pin low value if the same pin
		 * configured as input is low. Drivers that do not support Open
		 * Source flag return -ENOTSUP.
		 */
		ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT_LOW |
				GPIO_OPEN_SOURCE | GPIO_INPUT | GPIO_PULL_DOWN);
		if (ret == -ENOTSUP) {
			TC_PRINT("Open Source configuration or Pull Down pin "
				 "bias is not supported\n");
			ztest_test_skip();
			return;
		}
		zassert_equal(ret, 0,
			      "Failed to configure the pin in Open Source mode");

		pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(5));

		/* Setting pin configured in Open Source mode to high results in
		 * pin high value if the same pin configured as input is low.
		 */
		pin_set_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(6));
		pin_get_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(6));

		/* Verify that pin configured in Open Drain mode and
		 * initialized to high results in pin low value if the same pin
		 * configured as input is low. Drivers that do not support Open
		 * Drain flag return -ENOTSUP.
		 */
		ret = gpio_pin_configure(port, TEST_PIN, GPIO_OUTPUT_HIGH |
				GPIO_OPEN_DRAIN | GPIO_INPUT | GPIO_PULL_DOWN);
		if (ret == -ENOTSUP) {
			TC_PRINT("Open Drain configuration or Pull Down pin "
				 "bias is not supported\n");
			return;
		}
		zassert_equal(ret, 0,
			      "Failed to configure the pin in Open Drain mode");

		k_sleep(K_MSEC(TEST_GPIO_MAX_SINGLE_ENDED_RISE_FALL_TIME_MS));

		pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(7));

		pin_set_raw_and_verify(port, TEST_PIN, 1, TEST_POINT(8));
		pin_get_raw_and_verify(port, TEST_PIN, 0, TEST_POINT(8));
	}
}

ZTEST_SUITE(gpio_api_1pin_conf, NULL, NULL, NULL, NULL, NULL);
