/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_mpool_alloc_free_thread(void);
extern void test_mpool_alloc_free_isr(void);
extern void test_mpool_kdefine_extern(void);
extern void test_mpool_alloc_size(void);
extern void test_mpool_alloc_timeout(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_mpool_api,
		ztest_unit_test(test_mpool_alloc_free_thread),
		ztest_unit_test(test_mpool_alloc_free_isr),
		ztest_unit_test(test_mpool_kdefine_extern),
		ztest_unit_test(test_mpool_alloc_size),
		ztest_unit_test(test_mpool_alloc_timeout)
		);
	ztest_run_test_suite(test_mpool_api);
}
