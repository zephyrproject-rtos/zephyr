/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_NODE DT_ALIAS(test_gpio)

static const struct device *const test_dev = DEVICE_DT_GET(TEST_NODE);

#define PIN_FAULTED 0
#define PIN_CONTROL 1

/* Counts gpio_fire_callbacks() invocations to detect side effects of faulted calls. */
static unsigned int callback_count;
static struct gpio_callback test_callback;

static void test_callback_handler(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	callback_count++;
}

static void *gpio_emul_fault_setup(void)
{
	zassert_true(device_is_ready(test_dev), "GPIO device is not ready");

	gpio_init_callback(&test_callback, test_callback_handler, BIT(PIN_FAULTED));
	zassert_ok(gpio_add_callback(test_dev, &test_callback), "gpio_add_callback() failed");

	return NULL;
}

static void gpio_emul_fault_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Start each test with no faults armed. */
	gpio_emul_set_pin_fault(test_dev, 0, 0);
}

static void gpio_emul_fault_after(void *fixture)
{
	ARG_UNUSED(fixture);

	gpio_emul_set_pin_fault(test_dev, 0, 0);
}

ZTEST(gpio_emul_fault, test_pin_configure_fails_while_faulted)
{
	int ret;
	gpio_flags_t flags_before;
	gpio_flags_t flags_after;

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_OUTPUT);
	zassert_ok(ret, "baseline gpio_pin_configure() failed: %d", ret);

	ret = gpio_emul_flags_get(test_dev, PIN_FAULTED, &flags_before);
	zassert_ok(ret, "gpio_emul_flags_get() failed: %d", ret);

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);
	/* The baseline configure above already fired its own callback. */
	callback_count = 0;

	/* Use flags different from the baseline so a leaked write would be caught. */
	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_INPUT);
	zassert_equal(ret, -EIO, "gpio_pin_configure() returned %d, expected -EIO", ret);

	ret = gpio_emul_flags_get(test_dev, PIN_FAULTED, &flags_after);
	zassert_ok(ret, "gpio_emul_flags_get() failed: %d", ret);
	zassert_equal(flags_after, flags_before,
		      "pin flags changed despite faulted gpio_pin_configure()");
	zassert_equal(callback_count, 0, "callback fired despite faulted gpio_pin_configure()");

	/* A pin outside the fault mask must be unaffected. */
	ret = gpio_pin_configure(test_dev, PIN_CONTROL, GPIO_OUTPUT);
	zassert_ok(ret, "gpio_pin_configure(PIN_CONTROL) failed: %d", ret);
}

ZTEST(gpio_emul_fault, test_pin_interrupt_configure_fails_while_faulted)
{
	int ret;
	gpio_flags_t flags_before;
	gpio_flags_t flags_after;
	int pending_before;
	int pending_after;

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_INPUT);
	zassert_ok(ret, "gpio_pin_configure(GPIO_INPUT) failed: %d", ret);

	ret = gpio_pin_interrupt_configure(test_dev, PIN_FAULTED, GPIO_INT_EDGE_RISING);
	zassert_ok(ret, "baseline gpio_pin_interrupt_configure() failed: %d", ret);

	ret = gpio_emul_flags_get(test_dev, PIN_FAULTED, &flags_before);
	zassert_ok(ret, "gpio_emul_flags_get() failed: %d", ret);

	pending_before = gpio_get_pending_int(test_dev);
	zassert_true(pending_before >= 0, "gpio_get_pending_int() failed: %d", pending_before);

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);
	/* The baseline calls above already fired their own callbacks. */
	callback_count = 0;

	/* Use a trigger different from the baseline so a leaked write would be caught. */
	ret = gpio_pin_interrupt_configure(test_dev, PIN_FAULTED, GPIO_INT_DISABLE);
	zassert_equal(ret, -EIO, "gpio_pin_interrupt_configure() returned %d, expected -EIO", ret);

	ret = gpio_emul_flags_get(test_dev, PIN_FAULTED, &flags_after);
	zassert_ok(ret, "gpio_emul_flags_get() failed: %d", ret);
	zassert_equal(flags_after, flags_before,
		      "interrupt config changed despite faulted gpio_pin_interrupt_configure()");

	pending_after = gpio_get_pending_int(test_dev);
	zassert_equal(pending_after, pending_before,
		      "pending interrupt changed despite faulted gpio_pin_interrupt_configure()");
	zassert_equal(callback_count, 0,
		      "callback fired despite faulted gpio_pin_interrupt_configure()");
}

ZTEST(gpio_emul_fault, test_port_get_raw_fails_while_faulted)
{
	int ret;
	gpio_port_value_t value;

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_INPUT);
	zassert_ok(ret, "gpio_pin_configure(GPIO_INPUT) failed: %d", ret);

	ret = gpio_port_get_raw(test_dev, &value);
	zassert_ok(ret, "baseline gpio_port_get_raw() failed: %d", ret);

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);

	ret = gpio_port_get_raw(test_dev, &value);
	zassert_equal(ret, -EIO, "gpio_port_get_raw() returned %d, expected -EIO", ret);
}

/*
 * A faulted pin must fail port reads even if it was never configured as
 * input: fault checks must not be gated on the pin's current direction.
 */
ZTEST(gpio_emul_fault, test_port_get_raw_fails_for_unconfigured_faulted_pin)
{
	int ret;
	gpio_port_value_t value;

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);

	ret = gpio_port_get_raw(test_dev, &value);
	zassert_equal(ret, -EIO, "gpio_port_get_raw() returned %d, expected -EIO", ret);
}

ZTEST(gpio_emul_fault, test_pin_toggle_fails_while_faulted)
{
	int ret;
	int value_before;
	int value_after;

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "gpio_pin_configure() failed: %d", ret);

	value_before = gpio_emul_output_get(test_dev, PIN_FAULTED);
	zassert_true(value_before >= 0, "gpio_emul_output_get() failed: %d", value_before);

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);
	/* The baseline configure above already fired its own callback. */
	callback_count = 0;

	ret = gpio_pin_toggle(test_dev, PIN_FAULTED);
	zassert_equal(ret, -EIO, "gpio_pin_toggle() returned %d, expected -EIO", ret);

	value_after = gpio_emul_output_get(test_dev, PIN_FAULTED);
	zassert_equal(value_after, value_before,
		      "output value changed despite faulted gpio_pin_toggle()");
	zassert_equal(callback_count, 0, "callback fired despite faulted gpio_pin_toggle()");
}

ZTEST(gpio_emul_fault, test_clearing_fault_restores_normal_operation)
{
	int ret;

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_OUTPUT);
	zassert_equal(ret, -EIO, "gpio_pin_configure() returned %d, expected -EIO", ret);

	/* Clearing all faults (pins = 0) must restore normal behavior. */
	gpio_emul_set_pin_fault(test_dev, 0, 0);

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_OUTPUT);
	zassert_ok(ret, "gpio_pin_configure() failed after clearing fault: %d", ret);
}

ZTEST_SUITE(gpio_emul_fault, NULL, gpio_emul_fault_setup, gpio_emul_fault_before,
	    gpio_emul_fault_after, NULL);
