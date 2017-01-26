/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mheap
 * @{
 * @defgroup t_mheap_api test_mheap_api
 * @brief TestPurpose: verify heap memory pool APIs.
 * @details All TESTPOINTs extracted from API doc
 * https://www.zephyrproject.org/doc/api/kernel_api.html#heap-memory-pool
 * - API coverage
 *   -# k_malloc
 *   -# k_free
 * @}
 */

#include <ztest.h>
#include "test_mheap.h"

/*test cases*/
void test_mheap_malloc_free(void)
{
	void *block[BLK_NUM_MAX], *block_fail;

	for (int i = 0; i < BLK_NUM_MAX; i++) {
		/**
		 * TESTPOINT: This routine provides traditional malloc()
		 * semantics. Memory is allocated from the heap memory pool.
		 */
		block[i] = k_malloc(i);
		/** TESTPOINT: Address of the allocated memory if successful;*/
		assert_not_null(block[i], NULL);
	}

	block_fail = k_malloc(BLK_SIZE_MIN);
	/** TESTPOINT: Return NULL if fail.*/
	assert_is_null(block_fail, NULL);

	for (int i = 0; i < BLK_NUM_MAX; i++) {
		/**
		 * TESTPOINT: This routine provides traditional free()
		 * semantics. The memory being returned must have been allocated
		 * from the heap memory pool.
		 */
		k_free(block[i]);
	}
	/** TESTPOINT: If ptr is NULL, no operation is performed.*/
	k_free(NULL);
}
