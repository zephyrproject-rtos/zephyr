/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#define TIMEOUT 2000
#define BLK_SIZE_MIN 16
#define BLK_SIZE_MID 32
#define BLK_SIZE_MAX 256
#define BLK_NUM_MIN 32
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN

K_MEM_POOL_DEFINE(mpool3, BLK_SIZE_MIN, BLK_SIZE_MAX, BLK_NUM_MAX, BLK_ALIGN);

/*test cases*/
/**
 * @brief Verify blocks of different sizes cannot be merged.
 *
 * @ingroup kernel_memory_pool_tests
 *
 * @details The merging algorithm cannot combine adjacent free blocks
 * of different sizes
 * Test steps: 1. allocate 14 blocks in different sizes
 * 2. free block [2~8], in different sizes
 * 3. request a big block verify blocks [2~8] can't be merged
 * 4. tear down, free blocks [0, 1, 9~13]
 */
void test_mpool_alloc_merge_failed_diff_size(void)
{
	struct k_mem_block block[BLK_NUM_MIN], block_fail;
	size_t block_size[] = {
		BLK_SIZE_MIN, BLK_SIZE_MIN, BLK_SIZE_MIN, BLK_SIZE_MIN,
		BLK_SIZE_MID, BLK_SIZE_MID, BLK_SIZE_MID,
		BLK_SIZE_MIN, BLK_SIZE_MIN, BLK_SIZE_MIN, BLK_SIZE_MIN,
		BLK_SIZE_MID, BLK_SIZE_MID, BLK_SIZE_MID
	};
	int block_count = ARRAY_SIZE(block_size);


	for (int i = 0; i < block_count; i++) {
		/* 1. allocate blocks in different sizes*/
		zassert_true(k_mem_pool_alloc(&mpool3, &block[i], block_size[i],
					      K_NO_WAIT) == 0, NULL);
	}
	/* 2. free block [2~8], in different sizes*/
	for (int i = 2; i < 9; i++) {
		k_mem_pool_free(&block[i]);
	}
	/* 3. request a big block, expected failed to merge*/
	zassert_true(k_mem_pool_alloc(&mpool3, &block_fail, BLK_SIZE_MAX,
				      TIMEOUT) == -EAGAIN, NULL);

	/* 4. test case tear down*/
	k_mem_pool_free(&block[0]);
	k_mem_pool_free(&block[1]);
	for (int i = 9; i < block_count; i++) {
		k_mem_pool_free(&block[i]);
	}
}
