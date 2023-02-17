/*
 * Copyright (c) 2023 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

const struct gpio_dt_spec output_high_gpio_specs[] = {
	DT_FOREACH_PROP_ELEM_SEP(ZEPHYR_USER_NODE, output_high_gpios,
				 GPIO_DT_SPEC_GET_BY_IDX, (,))
};
const struct gpio_dt_spec output_low_gpio =
	GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, output_low_gpios, {0});
const struct gpio_dt_spec input_gpio =
	GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, input_gpios, {0});

ZTEST(gpio_hogs, test_gpio_hog_output_high_direction)
{
	for (int i = 0; i < ARRAY_SIZE(output_high_gpio_specs); i++) {
		assert_gpio_hog_direction(&output_high_gpio_specs[i], true);
	}
}

ZTEST(gpio_hogs, test_gpio_hog_output_low_direction)
{
	assert_gpio_hog_direction(&output_low_gpio, true);
}

ZTEST(gpio_hogs, test_gpio_hog_input_direction)
{
	assert_gpio_hog_direction(&input_gpio, false);
}

ZTEST(gpio_hogs, test_gpio_hog_output_high_config)
{
	gpio_flags_t expected;
	const struct gpio_dt_spec *spec;

	for (int i = 0; i < ARRAY_SIZE(output_high_gpio_specs); i++) {
		spec = &output_high_gpio_specs[i];
		expected = GPIO_OUTPUT;

		if ((spec->dt_flags & GPIO_ACTIVE_LOW) != 0) {
			expected |= GPIO_OUTPUT_INIT_LOW;
		} else {
			expected |= GPIO_OUTPUT_INIT_HIGH;
		}

		assert_gpio_hog_config(spec, expected);
	}
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
