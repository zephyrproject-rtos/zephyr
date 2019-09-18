/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include <drivers/gpio.h>

#include "gpio_utils.h"

static void gpio_test(void)
{
	int flags;

	flags = GPIO_INT_ENABLE;
	zassert_true(gpio_flags_int_enabled(flags), NULL);
	zassert_false(gpio_flags_int_disabled(flags), NULL);

	flags = GPIO_INT_DISABLE;
	zassert_true(gpio_flags_int_disabled(flags), NULL);
	zassert_false(gpio_flags_int_enabled(flags), NULL);

	flags = GPIO_INT_LEVEL;
	zassert_true(gpio_flags_int_level(flags), NULL);
	zassert_false(gpio_flags_int_edge(flags), NULL);

	flags = GPIO_INT_LEVEL_LOW;
	zassert_true(gpio_flags_int_level_low(flags), NULL);
	zassert_true(gpio_flags_int_level(flags), NULL);
	zassert_false(gpio_flags_int_level_high(flags), NULL);

	flags = GPIO_INT_LEVEL_HIGH;
	zassert_true(gpio_flags_int_level_high(flags), NULL);
	zassert_true(gpio_flags_int_level(flags), NULL);
	zassert_false(gpio_flags_int_level_low(flags), NULL);

	flags = GPIO_INT_EDGE;
	zassert_true(gpio_flags_int_edge(flags), NULL);
	zassert_false(gpio_flags_int_level(flags), NULL);

	flags = GPIO_INT_EDGE_RISING;
	zassert_true(gpio_flags_int_edge_rising(flags), NULL);
	zassert_true(gpio_flags_int_edge(flags), NULL);
	zassert_false(gpio_flags_int_edge_falling(flags), NULL);
	zassert_false(gpio_flags_int_edge_both(flags), NULL);

	flags = GPIO_INT_EDGE_FALLING;
	zassert_true(gpio_flags_int_edge_falling(flags), NULL);
	zassert_true(gpio_flags_int_edge(flags), NULL);
	zassert_false(gpio_flags_int_edge_rising(flags), NULL);
	zassert_false(gpio_flags_int_edge_both(flags), NULL);

	flags = GPIO_INT_EDGE_BOTH;
	zassert_true(gpio_flags_int_edge_both(flags), NULL);
	zassert_true(gpio_flags_int_edge(flags), NULL);
	zassert_false(gpio_flags_int_edge_rising(flags), NULL);
	zassert_false(gpio_flags_int_edge_falling(flags), NULL);
}

void test_main(void)
{
	ztest_test_suite(gpio_tests,
		ztest_unit_test(gpio_test)
	);

	ztest_run_test_suite(gpio_tests);
}
