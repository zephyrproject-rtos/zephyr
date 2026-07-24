/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tests for the fast GPIO API using the generic fallback driver.
 *
 * Exercises all fast GPIO functions (configure, set, clear, toggle, get,
 * set_input, set_output) and compile-time macros (GPIO_FAST_NS_TO_CYCLES,
 * GPIO_FAST_DELAY_NOPS) on the emulated GPIO driver via the generic
 * fallback path. The dispatch macros automatically fall back to the
 * generic backend because no vendor backend matches the emulated GPIO
 * controller's compatible (zephyr,gpio-emul).
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_fast.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#define TEST_NODE DT_NODELABEL(test_fast_gpio)
#define TEST_BACKEND GPIO_FAST_COMPAT(TEST_NODE, gpios)
#define TEST_PIN  DT_GPIO_PIN(TEST_NODE, gpios)

static const struct gpio_dt_spec test_gpio = GPIO_DT_SPEC_GET(TEST_NODE, gpios);
static struct GPIO_FAST_DISPATCH_TYPE(TEST_BACKEND) fast;

/* --- Compile-time checks --- */

BUILD_ASSERT(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_SET_CYCLES) > 0,
	"GPIO_FAST_SET_CYCLES must be defined and greater than 0");
BUILD_ASSERT(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_CLEAR_CYCLES) > 0,
	"GPIO_FAST_CLEAR_CYCLES must be defined and greater than 0");
BUILD_ASSERT(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_TOGGLE_CYCLES) > 0,
	"GPIO_FAST_TOGGLE_CYCLES must be defined and greater than 0");
BUILD_ASSERT(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_GET_CYCLES) > 0,
	"GPIO_FAST_GET_CYCLES must be defined and greater than 0");
BUILD_ASSERT(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_SET_INPUT_CYCLES) > 0,
	"GPIO_FAST_SET_INPUT_CYCLES must be defined and greater than 0");
BUILD_ASSERT(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_SET_OUTPUT_CYCLES) > 0,
	"GPIO_FAST_SET_OUTPUT_CYCLES must be defined and greater than 0");

BUILD_ASSERT(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_SET_CYCLES) ==
	     GPIO_FAST_SET_CYCLES_generic,
	     "Dispatch should resolve to generic backend cycle costs");

/*
 * Verify GPIO_FAST_SEND_STREAM_ATTR is defined for the resolved backend.
 * The macro expands to either empty or __ramfunc. Applying it to a
 * function declaration triggers a compile error if the backend forgot
 * to define it.
 */
GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_SEND_STREAM_ATTR)
static void z_gpio_fast_send_stream_attr_check(void)
{
	/** No-op; this function exists to check that GPIO_FAST_SEND_STREAM_ATTR is defined */
}

/* GPIO_FAST_NS_TO_CYCLES compile-time conversion checks. */
BUILD_ASSERT(GPIO_FAST_NS_TO_CYCLES(0) == 0,
	     "0 ns should be 0 cycles");
BUILD_ASSERT(GPIO_FAST_NS_TO_CYCLES(1000000000ULL) ==
	     CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "1 second in ns should equal CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC");
BUILD_ASSERT(GPIO_FAST_NS_TO_CYCLES(1000) > GPIO_FAST_NS_TO_CYCLES(100),
	     "1000 ns should be more cycles than 100 ns");

/* --- Test setup --- */

static void *gpio_fast_setup(void)
{
	zassert_true(gpio_is_ready_dt(&test_gpio), "GPIO device not ready");
	return NULL;
}

/* --- Tests --- */

ZTEST(gpio_fast_api, test_configure)
{
	int ret = GPIO_FAST_CONFIGURE_DT(TEST_BACKEND, &fast,
					 &test_gpio, GPIO_OUTPUT);

	zassert_equal(ret, 0, "GPIO_FAST_CONFIGURE_DT failed: %d", ret);
}

ZTEST(gpio_fast_api, test_configure_raw)
{
	int ret = GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_configure,
					  &fast, test_gpio.port,
					  BIT(test_gpio.pin), GPIO_OUTPUT);

	zassert_equal(ret, 0, "gpio_fast_configure dispatch failed: %d", ret);
}

ZTEST(gpio_fast_api, test_set_clear)
{
	int ret;
	int val;

	ret = GPIO_FAST_CONFIGURE_DT(TEST_BACKEND, &fast, &test_gpio, GPIO_OUTPUT);
	zassert_equal(ret, 0, "configure failed: %d", ret);

	/* Use standard API to establish known initial state */
	ret = gpio_pin_set_raw(test_gpio.port, test_gpio.pin, 0);
	zassert_equal(ret, 0, "gpio_pin_set_raw failed: %d", ret);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 0, "pin should be low after standard API clear");

	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 1, "pin should be high after set, got %d", val);

	/* Set again to check does not toggle */
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 1, "pin should still be high after set, got %d", val);

	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_clear, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 0, "pin should be low after clear, got %d", val);

	/* Clear again to check does not toggle */
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_clear, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 0, "pin should still be low after clear, got %d", val);
}

ZTEST(gpio_fast_api, test_toggle)
{
	int ret;
	int val;

	ret = GPIO_FAST_CONFIGURE_DT(TEST_BACKEND, &fast, &test_gpio, GPIO_OUTPUT);
	zassert_equal(ret, 0, "configure failed: %d", ret);

	/* Use standard API to establish known initial state */
	ret = gpio_pin_set_raw(test_gpio.port, test_gpio.pin, 0);
	zassert_equal(ret, 0, "gpio_pin_set_raw failed: %d", ret);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 0, "pin should be low after standard API clear");

	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_toggle, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 1, "pin should be high after first toggle");

	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_toggle, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 0, "pin should be low after second toggle");
}

ZTEST(gpio_fast_api, test_get_input)
{
	int ret;
	gpio_port_pins_t val;

	ret = GPIO_FAST_CONFIGURE_DT(TEST_BACKEND, &fast, &test_gpio, GPIO_INPUT);
	zassert_equal(ret, 0, "configure failed: %d", ret);

	gpio_emul_input_set(test_gpio.port, test_gpio.pin, 1);
	val = GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_get, &fast);
	zassert_true(val != 0, "pin should read high");

	gpio_emul_input_set(test_gpio.port, test_gpio.pin, 0);
	val = GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_get, &fast);
	zassert_equal(val, 0, "pin should read low");
}

ZTEST(gpio_fast_api, test_set_input_output)
{
	int ret;
	int val;
	gpio_port_pins_t fast_val;

	/* Start as output, drive high */
	ret = GPIO_FAST_CONFIGURE_DT(TEST_BACKEND, &fast, &test_gpio,
				     GPIO_INPUT | GPIO_OUTPUT);
	zassert_equal(ret, 0, "configure failed: %d", ret);

	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set_output, &fast);
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 1, "pin should be high after set_output + set");

	/* Switch to input and read an emulated value */
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set_input, &fast);
	gpio_emul_input_set(test_gpio.port, test_gpio.pin, 1);
	fast_val = GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_get, &fast);
	zassert_true(fast_val != 0, "pin should read high after set_input");

	gpio_emul_input_set(test_gpio.port, test_gpio.pin, 0);
	fast_val = GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_get, &fast);
	zassert_equal(fast_val, 0, "pin should read low after set_input");

	/* Switch back to output and drive low */
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set_output, &fast);
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_clear, &fast);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 0, "pin should be low after set_output + clear");
}

ZTEST(gpio_fast_api, test_nops)
{
	int ret;
	int val;

	ret = GPIO_FAST_CONFIGURE_DT(TEST_BACKEND, &fast, &test_gpio, GPIO_OUTPUT);
	zassert_equal(ret, 0, "configure failed: %d", ret);

	/* Use standard API to establish known initial state */
	ret = gpio_pin_set_raw(test_gpio.port, test_gpio.pin, 0);
	zassert_equal(ret, 0, "gpio_pin_set_raw failed: %d", ret);

	/*
	 * Verify GPIO_FAST_DELAY_NOPS compiles and doesn't crash.
	 * We can't verify exact timing on the emulator, but we can
	 * verify the set/nops/clear sequence produces correct pin state.
	 */
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set, &fast);
	GPIO_FAST_DELAY_NOPS(5);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 1, "pin should be high during NOP delay");

	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_clear, &fast);
	GPIO_FAST_DELAY_NOPS(5);
	val = gpio_emul_output_get(test_gpio.port, test_gpio.pin);
	zassert_equal(val, 0, "pin should be low during NOP delay");
}

ZTEST(gpio_fast_api, test_pre_post_stream)
{
	int ret;

	ret = GPIO_FAST_CONFIGURE_DT(TEST_BACKEND, &fast, &test_gpio, GPIO_OUTPUT);
	zassert_equal(ret, 0, "configure failed: %d", ret);

	/* Verify pre/post stream hooks compile, return success, and don't crash */
	ret = GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_pre_stream, &fast);
	zassert_equal(ret, 0, "gpio_fast_pre_stream failed: %d", ret);
	GPIO_FAST_DELAY_NOPS(5);
	ret = GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_post_stream, &fast);
	zassert_equal(ret, 0, "gpio_fast_post_stream failed: %d", ret);
}

ZTEST(gpio_fast_api, test_generic_fallback_detected)
{
	zassert_equal(GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_SET_CYCLES),
		      GPIO_FAST_SET_CYCLES_generic,
		      "Dispatch should resolve to generic backend");

	/* Verify SEND_STREAM_ATTR compiled (function exists). */
	z_gpio_fast_send_stream_attr_check();
}

ZTEST_SUITE(gpio_fast_api, NULL, gpio_fast_setup, NULL, NULL, NULL);
