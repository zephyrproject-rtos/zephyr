/*
 * Copyright (c) 2019 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <limits.h>
#include <zephyr/sys/util.h>
#include "test_gpio_api.h"

struct gpio_callback gpio_cb;
static int cb_count;

static void callback_edge(const struct device *port, struct gpio_callback *cb,
			  gpio_port_pins_t pins)
{
	zassert_equal(pins, BIT(TEST_PIN),
		     "Detected interrupt on an invalid pin");
	cb_count++;
}

static void callback_level(const struct device *port,
			   struct gpio_callback *cb,
			   gpio_port_pins_t pins)
{
	int ret;

	zassert_equal(pins, BIT(TEST_PIN),
		     "Detected interrupt on an invalid pin");

	ret = gpio_pin_interrupt_configure(port, TEST_PIN, GPIO_INT_DISABLE);
	zassert_equal(ret, 0,
		      "Failed to disable pin interrupt in the callback");

	cb_count++;
}

static void pin_set_and_verify(const struct device *port, unsigned int pin,
			       int val,
			       int idx)
{
	zassert_equal(gpio_pin_set(port, pin, val), 0,
		      "Test point %d: failed to set logical pin value", idx);
	k_busy_wait(TEST_GPIO_MAX_RISE_FALL_TIME_US);
}

void test_gpio_pin_interrupt_edge(unsigned int cfg_flags,
				  unsigned int int_flags)
{
	const struct device *port;
	int cb_count_expected;
	unsigned int cfg_out_flag;
	int ret;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_INPUT | GPIO_OUTPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	if ((cfg_flags & GPIO_ACTIVE_LOW) != 0) {
		cfg_out_flag = GPIO_OUTPUT_HIGH;
	} else {
		cfg_out_flag = GPIO_OUTPUT_LOW;
	}
	ret = gpio_pin_configure(port, TEST_PIN, GPIO_INPUT | cfg_out_flag |
				 cfg_flags);
	zassert_equal(ret, 0, "Failed to configure the pin");

	cb_count = 0;
	cb_count_expected = 0;

	gpio_init_callback(&gpio_cb, callback_edge, BIT(TEST_PIN));
	ret = gpio_add_callback(port, &gpio_cb);

	ret = gpio_pin_interrupt_configure(port, TEST_PIN, int_flags);
	if (ret == -ENOTSUP) {
		TC_PRINT("Pin interrupt is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure pin interrupt");

	for (int i = 0; i < 6; i++) {
		pin_set_and_verify(port, TEST_PIN, 1, i);
		if (int_flags & GPIO_INT_HIGH_1) {
			cb_count_expected++;
		}
		zassert_equal(cb_count, cb_count_expected,
			      "Test point %d: Pin interrupt triggered invalid "
			      "number of times on rising/to active edge", i);

		pin_set_and_verify(port, TEST_PIN, 0, i);
		if (int_flags & GPIO_INT_LOW_0) {
			cb_count_expected++;
		}
		zassert_equal(cb_count, cb_count_expected,
			      "Test point %d: Pin interrupt triggered invalid "
			      "number of times on falling/to inactive edge", i);
	}

	ret = gpio_pin_interrupt_configure(port, TEST_PIN, GPIO_INT_DISABLE);
	zassert_equal(ret, 0, "Failed to disable pin interrupt");

	for (int i = 0; i < 6; i++) {
		pin_set_and_verify(port, TEST_PIN, 1, i);
		pin_set_and_verify(port, TEST_PIN, 0, i);
		zassert_equal(cb_count, cb_count_expected,
			      "Pin interrupt triggered when disabled");
	}
}

void test_gpio_pin_interrupt_level(unsigned int cfg_flags,
				   unsigned int int_flags)
{
	const struct device *port;
	int cb_count_expected;
	unsigned int cfg_out_flag;
	int pin_out_val;
	int ret;

	port = device_get_binding(TEST_DEV);
	zassert_not_null(port, "device " TEST_DEV " not found");

	TC_PRINT("Running test on port=%s, pin=%d\n", TEST_DEV, TEST_PIN);

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_INPUT | GPIO_OUTPUT);
	if (ret == -ENOTSUP) {
		TC_PRINT("Simultaneous pin in/out mode is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure the pin");

	pin_out_val = ((int_flags & GPIO_INT_HIGH_1) != 0) ? 0 : 1;

	if ((cfg_flags & GPIO_ACTIVE_LOW) != 0) {
		cfg_out_flag = (pin_out_val != 0) ? GPIO_OUTPUT_LOW :
						    GPIO_OUTPUT_HIGH;
	} else {
		cfg_out_flag = (pin_out_val != 0) ? GPIO_OUTPUT_HIGH :
						    GPIO_OUTPUT_LOW;
	}

	ret = gpio_pin_configure(port, TEST_PIN, GPIO_INPUT | cfg_out_flag |
				 cfg_flags);
	zassert_equal(ret, 0, "Failed to configure the pin");

	cb_count = 0;
	cb_count_expected = 0;

	gpio_init_callback(&gpio_cb, callback_level, BIT(TEST_PIN));
	ret = gpio_add_callback(port, &gpio_cb);

	ret = gpio_pin_interrupt_configure(port, TEST_PIN, int_flags);
	if (ret == -ENOTSUP) {
		TC_PRINT("Pin interrupt is not supported.\n");
		ztest_test_skip();
		return;
	}
	zassert_equal(ret, 0, "Failed to configure pin interrupt");

	zassert_equal(cb_count, cb_count_expected,
		      "Pin interrupt triggered on level %d", pin_out_val);

	for (int i = 0; i < 6; i++) {
		pin_out_val = (pin_out_val != 0) ? 0 : 1;
		pin_set_and_verify(port, TEST_PIN, pin_out_val, i);
		cb_count_expected++;
		zassert_equal(cb_count, cb_count_expected,
			      "Test point %d: Pin interrupt triggered invalid "
			      "number of times on level %d", i, pin_out_val);

		pin_out_val = (pin_out_val != 0) ? 0 : 1;
		pin_set_and_verify(port, TEST_PIN, pin_out_val, i);
		zassert_equal(cb_count, cb_count_expected,
			      "Test point %d: Pin interrupt triggered invalid "
			      "number of times on level %d", i, pin_out_val);

		/* Re-enable pin level interrupt */
		ret = gpio_pin_interrupt_configure(port, TEST_PIN, int_flags);
		zassert_equal(ret, 0,
			      "Failed to re-enable pin level interrupt");
	}

	ret = gpio_pin_interrupt_configure(port, TEST_PIN, GPIO_INT_DISABLE);
	zassert_equal(ret, 0, "Failed to disable pin interrupt");

	for (int i = 0; i < 6; i++) {
		pin_set_and_verify(port, TEST_PIN, 1, i);
		pin_set_and_verify(port, TEST_PIN, 0, i);
		zassert_equal(cb_count, cb_count_expected,
			      "Pin interrupt triggered when disabled");
	}
}

/** @brief Verify GPIO_INT_EDGE_RISING flag. */
void test_gpio_int_edge_rising(void)
{
	test_gpio_pin_interrupt_edge(0, GPIO_INT_EDGE_RISING);
}

/** @brief Verify GPIO_INT_EDGE_FALLING flag. */
void test_gpio_int_edge_falling(void)
{
	test_gpio_pin_interrupt_edge(0, GPIO_INT_EDGE_FALLING);
}

/** @brief Verify GPIO_INT_EDGE_BOTH flag. */
void test_gpio_int_edge_both(void)
{
	test_gpio_pin_interrupt_edge(0, GPIO_INT_EDGE_BOTH);
}

/** @brief Verify GPIO_INT_EDGE_TO_ACTIVE flag. */
void test_gpio_int_edge_to_active(void)
{
	TC_PRINT("Step 1: Configure pin as active high\n");
	test_gpio_pin_interrupt_edge(GPIO_ACTIVE_HIGH, GPIO_INT_EDGE_TO_ACTIVE);
	TC_PRINT("Step 2: Configure pin as active low\n");
	test_gpio_pin_interrupt_edge(GPIO_ACTIVE_LOW, GPIO_INT_EDGE_TO_ACTIVE);
}

/** @brief Verify GPIO_INT_EDGE_TO_INACTIVE flag. */
void test_gpio_int_edge_to_inactive(void)
{
	TC_PRINT("Step 1: Configure pin as active high\n");
	test_gpio_pin_interrupt_edge(GPIO_ACTIVE_HIGH, GPIO_INT_EDGE_TO_INACTIVE);
	TC_PRINT("Step 2: Configure pin as active low\n");
	test_gpio_pin_interrupt_edge(GPIO_ACTIVE_LOW, GPIO_INT_EDGE_TO_INACTIVE);
}

/** @brief Verify GPIO_INT_LEVEL_HIGH flag. */
void test_gpio_int_level_high(void)
{
	test_gpio_pin_interrupt_level(0, GPIO_INT_LEVEL_HIGH);
}

/** @brief Verify GPIO_INT_LEVEL_LOW flag. */
void test_gpio_int_level_low(void)
{
	test_gpio_pin_interrupt_level(0, GPIO_INT_LEVEL_LOW);
}

/** @brief Verify GPIO_INT_LEVEL_ACTIVE flag. */
void test_gpio_int_level_active(void)
{
	TC_PRINT("Step 1: Configure pin as active high\n");
	test_gpio_pin_interrupt_level(GPIO_ACTIVE_HIGH, GPIO_INT_LEVEL_ACTIVE);
	TC_PRINT("Step 2: Configure pin as active low\n");
	test_gpio_pin_interrupt_level(GPIO_ACTIVE_LOW, GPIO_INT_LEVEL_ACTIVE);
}

/** @brief Verify GPIO_INT_LEVEL_INACTIVE flag. */
void test_gpio_int_level_inactive(void)
{
	TC_PRINT("Step 1: Configure pin as active high\n");
	test_gpio_pin_interrupt_level(GPIO_ACTIVE_HIGH, GPIO_INT_LEVEL_INACTIVE);
	TC_PRINT("Step 2: Configure pin as active low\n");
	test_gpio_pin_interrupt_level(GPIO_ACTIVE_LOW, GPIO_INT_LEVEL_INACTIVE);
}
