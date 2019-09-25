/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>

extern void test_pinmux_gpio(void);

void test_main(void)
{
	ztest_test_suite(pinmux_basic_test,
			 ztest_unit_test(test_pinmux_gpio));
	ztest_run_test_suite(pinmux_basic_test);
}
