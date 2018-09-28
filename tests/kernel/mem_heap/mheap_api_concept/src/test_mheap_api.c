/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_mheap.h"

/*test cases*/

/**
 * @brief Test to demonstrate k_malloc() and k_free() API usage
 *
 * @ingroup kernel_heap_tests
 *
 * @details The test allocates 4 blocks from heap memory pool
 * using k_malloc() API. It also tries to allocate a block of size
 * 64 bytes which fails as all the memory is allocated up. It then
 * validates k_free() API by freeing up all the blocks which were
 * allocated from the heap memory.
 *
 * @see k_malloc()
 */
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
		zassert_not_null(block[i], NULL);
	}

	block_fail = k_malloc(BLK_SIZE_MIN);
	/** TESTPOINT: Return NULL if fail.*/
	zassert_is_null(block_fail, NULL);

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

#define NMEMB   8
#define SIZE    16
#define BOUNDS  (NMEMB * SIZE)

/**
 * @brief Test to demonstrate k_calloc() API functionality.
 *
 * @ingroup kernel_heap_tests
 *
 * @details The test validates k_calloc() API. The 8 blocks of memory of
 * size 16 bytes are allocated by k_calloc() API. When allocated using
 * k_calloc() the memory buffers have to be zeroed. Check is done, if the
 * blocks are memset to 0 and read/write is allowed. The test is then
 * teared up by freeing all the blocks allocated.
 *
 * @see k_calloc()
 */
void test_mheap_calloc(void)
{
	char *mem;

	mem = k_calloc(NMEMB, SIZE);

	zassert_not_null(mem, "calloc operation failed");

	/* Memory should be zeroed and not crash us if we read/write to it */
	for (int i = 0; i < BOUNDS; i++) {
		zassert_equal(mem[i], 0, NULL);
		mem[i] = 1;
	}

	k_free(mem);
}
