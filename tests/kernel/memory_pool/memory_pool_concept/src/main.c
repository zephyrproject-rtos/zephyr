/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_mpool_alloc_wait_prio(void);
extern void test_mpool_alloc_size_roundup(void);
extern void test_mpool_alloc_merge_failed_diff_size(void);
extern void test_mpool_alloc_merge_failed_diff_parent(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_mpool_concept,
		ztest_unit_test(test_mpool_alloc_wait_prio),
		ztest_unit_test(test_mpool_alloc_size_roundup),
		ztest_unit_test(test_mpool_alloc_merge_failed_diff_size),
		ztest_unit_test(test_mpool_alloc_merge_failed_diff_parent));
	ztest_run_test_suite(test_mpool_concept);
}

