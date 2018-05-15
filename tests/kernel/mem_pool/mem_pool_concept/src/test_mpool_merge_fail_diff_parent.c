/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_mpool.h"

extern struct k_mem_pool mpool1;

/*test cases*/
/**
 * @brief Test to check blocks belonging to different quad-block
 * cannot be merged even though they are of same size.
 *
 * @details Allocate 8 blocks of size 8 bytes. All the blocks
 * in memory pool is allocated up. Free up 3rd to 6th blocks,
 * i.e 4 blocks belonging to different quad-block ( block 1,2,3,4
 * belong to one quad-block and block 5,6,7,8 belong to another
 * quad-block). Now request for 32 bytes block. In the memory pool,
 * 4 blocks of 8 bytes which are adjacent is present. But since they
 * belong to different quad-block, they are not allocated. Thus alloc
 * function will fail with return code -EAGAIN. Finally all blocks
 * are freed.
 */
void test_mpool_alloc_merge_failed_diff_parent(void)
{
	struct k_mem_block block[BLK_NUM_MIN], block_fail;

	/**
	 * TESTPOINT: nor can it merge adjacent free blocks of the same
	 * size if they belong to different parent quad-blocks
	 * Test steps: 1. allocate block [0~7] in minimum block size
	 *             2. free block [2~5], belong to diff parental quad-blocks
	 *             3. request a big block
	 *                verify blocks [2, 3] and blocks [4, 5] can't be merged
	 *             4. tear down, free blocks [0, 1, 6, 7]
	 */
	for (int i = 0; i < BLK_NUM_MIN; i++) {
		/* 1. allocated up all blocks*/
		zassert_true(k_mem_pool_alloc(&mpool1, &block[i], BLK_SIZE_MIN,
					      K_NO_WAIT) == 0, NULL);
	}
	/* 2. free adjacent blocks belong to different parent quad-blocks*/
	for (int i = 2; i < 6; i++) {
		k_mem_pool_free(&block[i]);
	}
	/* 3. request a big block, expected failed to merge*/
	zassert_true(k_mem_pool_alloc(&mpool1, &block_fail, BLK_SIZE_MAX,
				      TIMEOUT) == -EAGAIN, NULL);

	/* 4. test case tear down*/
	k_mem_pool_free(&block[0]);
	k_mem_pool_free(&block[1]);
	k_mem_pool_free(&block[6]);
	k_mem_pool_free(&block[7]);
}
