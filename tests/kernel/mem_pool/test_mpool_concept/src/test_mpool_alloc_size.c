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

#define TEST_SIZE ((BLK_SIZE_MAX>>2)+1)

extern struct k_mem_pool mpool1;

/*test cases*/
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
		assert_true(k_mem_pool_alloc(&mpool1, &block[i], TEST_SIZE,
			K_NO_WAIT) == 0, NULL);
	}
	/*verify consequently no more blocks available*/
	assert_true(k_mem_pool_alloc(&mpool1, &block_fail, BLK_SIZE_MIN,
		K_NO_WAIT) == -ENOMEM, NULL);

	/*test case tear down*/
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		k_mem_pool_free(&block[i]);
	}
}
