/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_gpio.h"

void test_main(void)
{
	ztest_test_suite(gpio_basic_test,
			 ztest_unit_test(test_gpio_pin_read_write),
			 ztest_unit_test(test_gpio_callback_edge_high),
			 ztest_unit_test(test_gpio_callback_edge_low),
			 ztest_unit_test(test_gpio_callback_level_high),
			 ztest_unit_test(test_gpio_callback_add_remove),
			 ztest_unit_test(test_gpio_callback_self_remove),
			 ztest_unit_test(test_gpio_callback_enable_disable),
			 ztest_unit_test(test_gpio_callback_level_low));
	ztest_run_test_suite(gpio_basic_test);
}
