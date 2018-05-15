/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_mpool.h"

#define TEST_SIZE ((BLK_SIZE_MAX >> 2) + 1)

extern struct k_mem_pool mpool1;

/*test cases*/
/**
 * @brief Test to validate alloc and free of blocks which are
 * of different block sizes.
 *
 * @details The test demonstrates how the request is handled
 * to allocate the minimum available size block in memory pool
 * to satisfy the requirement of the application. The test
 * requests for 4 memory blocks of 33 bytes each. But the
 * algorithm allocates 4 64 bytes blocks as the MIN_BLCK_SIZE
 * is 64 bytes. Now the mem_pool is allocated for 256 bytes,
 * so all the blocks must have been allocated. So another 64
 * bytes of block is tried to allocate to ensure that all the
 * memory is allocated in pool. Finally, all the blocks which
 * are allocated will be freed up.
 */
void test_mpool_alloc_size_roundup(void)
{
	struct k_mem_block block[BLK_NUM_MAX], block_fail;

	/**
	 * TESTPOINT: When an application issues a request for a memory block,
	 * the memory pool first determines the size of the smallest block that
	 * will satisfy the request
	 */
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		/*request a size for the mpool to round up to "BLK_SIZE_MAX"*/
		zassert_true(k_mem_pool_alloc(&mpool1, &block[i], TEST_SIZE,
					      K_NO_WAIT) == 0, NULL);
	}
	/*verify consequently no more blocks available*/
	zassert_true(k_mem_pool_alloc(&mpool1, &block_fail, BLK_SIZE_MIN,
				      K_NO_WAIT) == -ENOMEM, NULL);

	/*test case tear down*/
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		k_mem_pool_free(&block[i]);
	}
}
