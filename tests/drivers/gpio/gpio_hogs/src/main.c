/*
 * Copyright (c) 2023 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

const struct gpio_dt_spec output_high_gpio =
	GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, output_high_gpios, {0});
const struct gpio_dt_spec output_low_gpio =
	GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, output_low_gpios, {0});
const struct gpio_dt_spec input_gpio =
	GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, input_gpios, {0});

static void assert_gpio_hog_direction(const struct gpio_dt_spec *spec, bool output)
{
	int err;

	if (spec->port == NULL) {
		ztest_test_skip();
	}

	if (output) {
		err = gpio_pin_is_output(spec->port, spec->pin);
	} else {
		err = gpio_pin_is_input(spec->port, spec->pin);
	}

	if (err == -ENOSYS) {
		ztest_test_skip();
	}

	zassert_equal(err, 1, "GPIO hog %s pin %d not configured as %s",
		      spec->port->name, spec->pin,
		      output ? "output" : "input");
}

ZTEST(gpio_hogs, test_gpio_hog_output_high_direction)
{
	assert_gpio_hog_direction(&output_high_gpio, true);
}

ZTEST(gpio_hogs, test_gpio_hog_output_low_direction)
{
	assert_gpio_hog_direction(&output_low_gpio, true);
}

ZTEST(gpio_hogs, test_gpio_hog_input_direction)
{
	assert_gpio_hog_direction(&input_gpio, false);
}

static void assert_gpio_hog_config(const struct gpio_dt_spec *spec, gpio_flags_t expected)
{
	gpio_flags_t actual;
	int err;

	if (spec->port == NULL) {
		ztest_test_skip();
	}

	err = gpio_pin_get_config_dt(spec, &actual);
	if (err == -ENOSYS) {
		ztest_test_skip();
	}

	zassert_equal(err, 0, "failed to get config for GPIO hog %s, pin %d (err %d)",
		      spec->port->name, spec->pin, err);
	zassert_equal(actual & expected, expected,
		      "GPIO hog %s, pin %d flags not set (0x%08x vs. 0x%08x)",
		      spec->port->name, spec->pin, actual, expected);
}

ZTEST(gpio_hogs, test_gpio_hog_output_high_config)
{
	gpio_flags_t expected = GPIO_OUTPUT;

	if ((output_high_gpio.dt_flags & GPIO_ACTIVE_LOW) != 0) {
		expected |= GPIO_OUTPUT_INIT_LOW;
	} else {
		expected |= GPIO_OUTPUT_INIT_HIGH;
	}

	assert_gpio_hog_config(&output_high_gpio, expected);
}

ZTEST(gpio_hogs, test_gpio_hog_output_low_config)
{
	gpio_flags_t expected = GPIO_OUTPUT;

	if ((output_low_gpio.dt_flags & GPIO_ACTIVE_LOW) == 0) {
		expected |= GPIO_OUTPUT_INIT_LOW;
	} else {
		expected |= GPIO_OUTPUT_INIT_HIGH;
	}

	assert_gpio_hog_config(&output_low_gpio, expected);
}

ZTEST(gpio_hogs, test_gpio_hog_input_config)
{
	assert_gpio_hog_config(&input_gpio, GPIO_INPUT);
}

ZTEST_SUITE(gpio_hogs, NULL, NULL, NULL, NULL, NULL);
