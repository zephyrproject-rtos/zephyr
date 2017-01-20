/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_mpool_threadsafe(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_mpool_threadsafe,
		ztest_unit_test(test_mpool_threadsafe));
	ztest_run_test_suite(test_mpool_threadsafe);
}
