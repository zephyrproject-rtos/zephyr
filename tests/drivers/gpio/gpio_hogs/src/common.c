/*
 * Copyright 2023 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_gpio_hogs, CONFIG_GPIO_LOG_LEVEL);

void assert_gpio_hog_direction(const struct gpio_dt_spec *spec, bool output)
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

void assert_gpio_hog_config(const struct gpio_dt_spec *spec, gpio_flags_t expected)
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

	LOG_INF("Get config: Pin %d, flags 0x%08x",
		spec->pin, actual);
	if (actual & GPIO_OUTPUT) {
		LOG_INF("    level = %s", actual & GPIO_OUTPUT_INIT_HIGH ? "HIGH" : "low");
	}

	zassert_equal(err, 0, "failed to get config for GPIO hog %s, pin %d (err %d)",
		      spec->port->name, spec->pin, err);
	zassert_equal(actual & expected, expected,
		      "GPIO hog %s, pin %d flags not set (0x%08x vs. 0x%08x)",
		      spec->port->name, spec->pin, actual, expected);
}
