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

static void *gpio_emul_fault_setup(void)
{
	zassert_true(device_is_ready(test_dev), "GPIO device is not ready");

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

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_OUTPUT);
	zassert_ok(ret, "baseline gpio_pin_configure() failed: %d", ret);

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_OUTPUT);
	zassert_equal(ret, -EIO, "gpio_pin_configure() returned %d, expected -EIO", ret);

	/* A pin outside the fault mask must be unaffected. */
	ret = gpio_pin_configure(test_dev, PIN_CONTROL, GPIO_OUTPUT);
	zassert_ok(ret, "gpio_pin_configure(PIN_CONTROL) failed: %d", ret);
}

ZTEST(gpio_emul_fault, test_pin_interrupt_configure_fails_while_faulted)
{
	int ret;

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);

	ret = gpio_pin_interrupt_configure(test_dev, PIN_FAULTED, GPIO_INT_DISABLE);
	zassert_equal(ret, -EIO, "gpio_pin_interrupt_configure() returned %d, expected -EIO", ret);
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

	ret = gpio_pin_configure(test_dev, PIN_FAULTED, GPIO_OUTPUT);
	zassert_ok(ret, "gpio_pin_configure() failed: %d", ret);

	gpio_emul_set_pin_fault(test_dev, BIT(PIN_FAULTED), -EIO);

	ret = gpio_pin_toggle(test_dev, PIN_FAULTED);
	zassert_equal(ret, -EIO, "gpio_pin_toggle() returned %d, expected -EIO", ret);
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
