/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_mheap_malloc_free(void);
extern void test_mheap_malloc_align4(void);
extern void test_mheap_min_block_size(void);
extern void test_mheap_block_desc(void);
extern void test_mheap_calloc(void);
extern void test_mheap_block_release(void);

/**
 * @brief Heap tests
 *
 * @defgroup kernel_heap_tests Heap Memory Tests
 *
 * @ingroup all_tests
 * @{
 * @}
 */
/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(mheap_api,
			 ztest_unit_test(test_mheap_malloc_free),
			 ztest_unit_test(test_mheap_calloc),
			 ztest_unit_test(test_mheap_malloc_align4),
			 ztest_unit_test(test_mheap_min_block_size),
			 ztest_unit_test(test_mheap_block_desc),
			 ztest_unit_test(test_mheap_block_release));
	ztest_run_test_suite(mheap_api);
}
