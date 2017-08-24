/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_cmac_mode(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_cmac_fn,
		ztest_unit_test(test_cmac_mode));
	ztest_run_test_suite(test_cmac_fn);
}
