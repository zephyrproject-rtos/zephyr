/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Memory Pool Tests
 * @defgroup kernel_memory_pool_tests Memory Pool
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>

extern void test_mpool_alloc_free_thread(void);
extern void test_mpool_alloc_free_isr(void);
extern void test_mpool_kdefine_extern(void);
extern void test_mpool_alloc_size(void);
extern void test_mpool_alloc_timeout(void);
extern void test_sys_heap_mem_pool_assign(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(mpool_api,
			 ztest_unit_test(test_mpool_alloc_free_thread),
			 ztest_unit_test(test_mpool_alloc_free_isr),
			 ztest_unit_test(test_mpool_kdefine_extern),
			 ztest_unit_test(test_mpool_alloc_size),
			 ztest_unit_test(test_mpool_alloc_timeout),
			 ztest_unit_test(test_sys_heap_mem_pool_assign)
			 );
	ztest_run_test_suite(mpool_api);
}
