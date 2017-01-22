/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_gpio
 * @{
 * @defgroup t_gpio_basic_api test_gpio_basic_api
 * @}
 *
 * Setup: loop PIN_OUT with PIN_IN on the target board
 *
 * quark_se_c1000_devboard - x86
 * --------------------
 *
 * 1. PIN_OUT is GPIO_15
 * 2. PIN_IN is GPIO_16
 *
 * quark_se_c1000_ss_devboard - arc
 * --------------------
 *
 * 1. PIN_OUT is GPIO_SS_12
 * 2. PIN_IN is GPIO_SS_13
 *
 * arduino_101 - x86
 * --------------------
 *
 * 1. PIN_OUT is GPIO_16
 * 2. PIN_IN is GPIO_19
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
			 ztest_unit_test(test_gpio_callback_enable_disable),
			 ztest_unit_test(test_gpio_callback_level_low));
	ztest_run_test_suite(gpio_basic_test);
}
