/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_mslab_threadsafe(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(mslab_threadsafe,
			 ztest_1cpu_unit_test(test_mslab_threadsafe));
	ztest_run_test_suite(mslab_threadsafe);
}
