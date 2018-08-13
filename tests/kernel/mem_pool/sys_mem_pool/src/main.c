/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <misc/mempool.h>


#define BLK_SIZE_MIN    256
#define BLK_SIZE_MAX    1024
#define BLK_NUM_MAX     8
#define TOTAL_POOL_SIZE (BLK_SIZE_MAX * BLK_NUM_MAX)
#define TOTAL_MIN_BLKS  (TOTAL_POOL_SIZE / BLK_SIZE_MIN)

#define DESC_SIZE       sizeof(struct sys_mem_pool_block)

#define BLK_SIZE_EXCLUDE_DESC (BLK_SIZE_MIN - 16)
#define BLK_ALIGN BLK_SIZE_MIN


K_MUTEX_DEFINE(pool_mutex);
SYS_MEM_POOL_DEFINE(pool, &pool_mutex, BLK_SIZE_MIN, BLK_SIZE_MAX,
		    BLK_NUM_MAX, BLK_ALIGN, .data);

/**
 * @brief Verify sys_mem_pool allocation and free
 *
 * @ingroup kernel_memory_pool_tests
 *
 * @see sys_mem_pool_alloc(), sys_mem_pool_free()
 */
void test_sys_mem_pool_alloc_free(void)
{
	void *block[BLK_NUM_MAX], *block_fail;

	for (int i = 0; i < BLK_NUM_MAX; i++) {
		/* Allocate the largest possible blocks in the pool, using up
		 * the entire thing.
		 */
		block[i] = sys_mem_pool_alloc(&pool, BLK_SIZE_MAX - DESC_SIZE);
		/** TESTPOINT: Address of the allocated memory if successful;*/
		zassert_not_null(block[i], NULL);
	}

	block_fail = sys_mem_pool_alloc(&pool, BLK_SIZE_MIN);
	/** TESTPOINT: Return NULL if fail.*/
	zassert_is_null(block_fail, NULL);

	for (int i = 0; i < BLK_NUM_MAX; i++) {
		/**
		 * TESTPOINT: This routine provides traditional free()
		 * semantics. The memory being returned must have been allocated
		 * from the heap memory pool.
		 */
		sys_mem_pool_free(block[i]);
	}
	/** TESTPOINT: If ptr is NULL, no operation is performed.*/
	sys_mem_pool_free(NULL);
}

/**
 * @brief Verify sys_mem_pool aligned allocation.
 *
 * @see sys_mem_pool_alloc(), sys_mem_pool_free()
 *
 * @ingroup kernel_memory_pool_tests
 */
void test_sys_mem_pool_alloc_align4(void)
{
	void *block[BLK_NUM_MAX];

	/**
	 * TESTPOINT: The address of the allocated chunk is guaranteed to be
	 * aligned on a multiple of 4 bytes.
	 */
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		block[i] = sys_mem_pool_alloc(&pool, i);
		zassert_not_null(block[i], NULL);
		zassert_false((int)block[i] % 4, NULL);
	}

	/* test case tear down*/
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		sys_mem_pool_free(block[i]);
	}
}

/**
 * @brief Verify allocation of minimum block size which
 * is 64 bytes
 *
 * @ingroup kernel_memory_pool_tests
 *
 * @see sys_mem_pool_alloc(), sys_mem_pool_free()
 */
void test_sys_mem_pool_min_block_size(void)
{
	void *block[TOTAL_MIN_BLKS], *block_fail;

	/**
	 * TESTPOINT: The heap memory pool also defines a minimum block
	 * size of 64 bytes.
	 * Test steps:
	 * initial memory heap status (F for free, U for used):
	 *    64F, 64F, 64F, 64F
	 * 1. request 4 blocks: each 0-byte plus 16-byte block desc,
	 *    indeed 64-byte allocated
	 * 2. verify no more free blocks, any further allocation failed
	 */
	for (int i = 0; i < TOTAL_MIN_BLKS; i++) {
		block[i] = sys_mem_pool_alloc(&pool, 0);
		zassert_not_null(block[i], NULL);
	}
	/* verify no more free blocks available*/
	block_fail = sys_mem_pool_alloc(&pool, BLK_SIZE_MIN);
	zassert_is_null(block_fail, NULL);

	/* test case tear down*/
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		sys_mem_pool_free(block[i]);
	}
}

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &pool_mutex, NULL);
	sys_mem_pool_init(&pool);

	ztest_test_suite(test_sys_mem_pool_api,
			 ztest_user_unit_test(test_sys_mem_pool_alloc_free),
			 ztest_user_unit_test(test_sys_mem_pool_alloc_align4),
			 ztest_user_unit_test(test_sys_mem_pool_min_block_size)
			 );
	ztest_run_test_suite(test_sys_mem_pool_api);
}
