/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_tickless_sysclock(void);
extern void test_tickless_slice(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_tickless_concept,
		ztest_unit_test(test_tickless_sysclock),
		ztest_unit_test(test_tickless_slice));
	ztest_run_test_suite(test_tickless_concept);
}
