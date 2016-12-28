/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ztest.h>
#include "test_mpool.h"

extern struct k_mem_pool mpool1;

/*test cases*/
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
		assert_true(k_mem_pool_alloc(&mpool1, &block[i], BLK_SIZE_MIN,
			K_NO_WAIT) == 0, NULL);
	}
	/* 2. free adjacent blocks belong to different parent quad-blocks*/
	for (int i = 2; i < 6; i++) {
		k_mem_pool_free(&block[i]);
	}
	/* 3. request a big block, expected failed to merge*/
	k_mem_pool_defrag(&mpool1);
	assert_true(k_mem_pool_alloc(&mpool1, &block_fail, BLK_SIZE_MAX,
		TIMEOUT) == -EAGAIN, NULL);

	/* 4. test case tear down*/
	k_mem_pool_free(&block[0]);
	k_mem_pool_free(&block[1]);
	k_mem_pool_free(&block[6]);
	k_mem_pool_free(&block[7]);
}
