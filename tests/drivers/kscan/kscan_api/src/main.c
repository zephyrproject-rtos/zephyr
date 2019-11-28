/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>

void test_init_callback(void);
void test_control_callback(void);

void test_main(void)
{
	ztest_test_suite(kscan_basic_test,
			 ztest_unit_test(test_init_callback),
			 ztest_unit_test(test_control_callback));
	ztest_run_test_suite(kscan_basic_test);
}
