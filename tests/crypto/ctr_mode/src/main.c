/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_1_and_2(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_ctr_mode_fn,
		ztest_unit_test(test_1_and_2));
	ztest_run_test_suite(test_ctr_mode_fn);
}
