/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_pinmux
 * @{
 * @defgroup t_pinmux_basic test_pinmux_basic_operations
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
 * 1. PIN_OUT is GPIO_SS_4/GPIO_SS_AIN_12
 * 2. PIN_IN is GPIO_SS_5/GPIO_SS_AIN_13
 *
 * arduino_101 - x86
 * --------------------
 *
 * 1. PIN_OUT is GPIO_16
 * 2. PIN_IN is GPIO_19
 *
 * arduino_101_sss - arc
 * --------------------
 *
 * 1. PIN_OUT is GPIO_SS_2
 * 2. PIN_IN is GPIO_SS_3
 *
 * quark_d2000_crb - x86
 * --------------------
 *
 * 1. PIN_OUT is GPIO_8
 * 2. PIN_IN is GPIO_9
 */

#include <ztest.h>

extern void test_pinmux_gpio(void);

void test_main(void)
{
	ztest_test_suite(pinmux_basic_test,
			 ztest_unit_test(test_pinmux_gpio));
	ztest_run_test_suite(pinmux_basic_test);
}
