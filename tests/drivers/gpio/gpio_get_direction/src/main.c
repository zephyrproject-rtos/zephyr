/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <ztest.h>

#define TEST_NODE            DT_GPIO_CTLR(DT_ALIAS(led0), gpios)
#define TEST_PIN             DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#define TEST_PIN_DTS_FLAGS   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios)

struct gpio_get_direction_fixture {
	const struct device *port;
	gpio_pin_t pin;
	gpio_flags_t flags;
};

static void *gpio_get_direction_setup(void)
{
	static struct gpio_get_direction_fixture fixture;

	fixture.pin = TEST_PIN;
	fixture.port = DEVICE_DT_GET(TEST_NODE);

	return &fixture;
}

static void gpio_get_direction_before(void *arg)
{
	struct gpio_get_direction_fixture *fixture = (struct gpio_get_direction_fixture *)arg;

	zassert_true(device_is_ready(fixture->port), "GPIO device is not ready");
}

static void common(struct gpio_get_direction_fixture *fixture)
{
	int rv;

	rv = gpio_pin_configure(fixture->port, fixture->pin, fixture->flags);
	if (rv == -ENOTSUP) {
		/* some drivers / hw might not support e.g. input-output or disconnect */
		ztest_test_skip();
	}

	zassert_ok(rv, "gpio_pin_configure() failed: %d", rv);
}

ZTEST_F(gpio_get_direction, test_disconnect)
{
	int rv;

	fixture->flags = GPIO_DISCONNECTED;
	common(fixture);

	rv = gpio_pin_is_input(fixture->port, fixture->pin);
	if (rv == -ENOSYS) {
		/* gpio_pin_direction() is not supported in the driver */
		ztest_test_skip();
	}

	zassert_equal(false, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(fixture->port, fixture->pin);
	zassert_equal(false, rv, "gpio_pin_is_output() failed: %d", rv);
}

ZTEST_F(gpio_get_direction, test_input)
{
	int rv;
	fixture->flags = GPIO_INPUT;

	common(fixture);

	rv = gpio_pin_is_input(fixture->port, fixture->pin);
	if (rv == -ENOSYS) {
		/* gpio_pin_direction() is not supported in the driver */
		ztest_test_skip();
	}

	zassert_equal(true, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(fixture->port, fixture->pin);
	zassert_equal(false, rv, "gpio_pin_is_output() failed: %d", rv);
}

ZTEST_F(gpio_get_direction, test_output)
{
	int rv;
	fixture->flags = GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;

	common(fixture);

	rv = gpio_pin_is_input(fixture->port, fixture->pin);
	if (rv == -ENOSYS) {
		/* gpio_pin_direction() is not supported in the driver */
		ztest_test_skip();
	}

	zassert_equal(false, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(fixture->port, fixture->pin);
	zassert_equal(true, rv, "gpio_pin_is_output() failed: %d", rv);
}

ZTEST_F(gpio_get_direction, test_input_output)
{
	int rv;
	fixture->flags = GPIO_INPUT | GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;

	common(fixture);

	rv = gpio_pin_is_input(fixture->port, fixture->pin);
	if (rv == -ENOSYS) {
		/* some drivers / gpio hw do not support input-output mode */
		ztest_test_skip();
	}

	zassert_equal(true, rv, "gpio_pin_is_input() failed: %d", rv);

	rv = gpio_pin_is_output(fixture->port, fixture->pin);
	zassert_equal(true, rv, "gpio_pin_is_output() failed: %d", rv);
}

ZTEST_SUITE(gpio_get_direction, NULL, gpio_get_direction_setup, gpio_get_direction_before, NULL,
	    NULL);
