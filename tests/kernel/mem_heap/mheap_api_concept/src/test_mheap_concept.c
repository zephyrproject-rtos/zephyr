/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_mheap.h"

/* request 0 bytes*/
#define TEST_SIZE_0 0

/*test cases*/

/**
 * @brief The test validates k_calloc() API.
 *
 * @ingroup kernel_heap_tests
 *
 * @details The 8 blocks of memory of size 16 bytes are allocated
 * by k_calloc() API. When allocated using k_calloc() the memory buffers
 * have to be zeroed. Check is done, if the blocks are memset to 0 and
 * read/write is allowed. The test is then teared up by freeing all the
 * blocks allocated.
 *
 * @see k_malloc(), k_free()
 */
void test_mheap_malloc_align4(void)
{
	void *block[BLK_NUM_MAX];

	/**
	 * TESTPOINT: The address of the allocated chunk is guaranteed to be
	 * aligned on a word boundary (4 or 8 bytes).
	 */
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		block[i] = k_malloc(i);
		zassert_not_null(block[i], NULL);
		zassert_false((uintptr_t)block[i] % sizeof(void *), NULL);
	}

	/* test case tear down*/
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		k_free(block[i]);
	}
}

/**
 * @brief Verify if the block descriptor is included
 * in every block which is allocated
 *
 * @ingroup kernel_heap_tests
 *
 * @see k_malloc(), k_free()
 */
void test_mheap_block_desc(void)
{
	void *block[BLK_NUM_MAX], *block_fail;

	/**
	 * TESTPOINT: The kernel uses the first bytes of any memory
	 * block allocated from the heap memory pool to save the heap
	 * pointer it needs to later free the block.
	 * Test steps:
	 * initial memory heap status (F for free, U for used):
	 *    64F, 64F, 64F, 64F
	 * 1. request 4 blocks: each (64-N) bytes, indeed 64-byte allocated
	 * 2. verify no more free blocks, any further allocation failed
	 */
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		block[i] = k_malloc(BLK_SIZE_EXCLUDE_DESC);
		zassert_not_null(block[i], NULL);
	}
	/* verify no more free blocks available*/
	block_fail = k_malloc(BLK_SIZE_MIN);
	zassert_is_null(block_fail, NULL);

	/* test case tear down*/
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		k_free(block[i]);
	}
}

#define NMEMB   8
#define SIZE    16
/**
 * @brief Verify a region would be released back to
 * heap memory pool using k_free function.
 *
 * @ingroup kernel_heap_tests
 *
 * @see k_calloc(), k_free()
 */
void test_mheap_block_release(void)
{
	void *block[4 * BLK_NUM_MAX], *block_fail;
	int nb;

	/**
	 * TESTPOINT: When the blocks in the heap memory pool are free by
	 * the function k_free, the region would be released back to the
	 * heap memory pool.
	 */
	for (nb = 0; nb < ARRAY_SIZE(block); nb++) {
		/**
		 * TESTPOINT: This routine provides traditional malloc()
		 * semantics. Memory is allocated from the heap memory pool.
		 */
		block[nb] = k_calloc(NMEMB, SIZE);
		if (block[nb] == NULL) {
			break;
		}
	}

	/* verify no more free blocks available*/
	block_fail = k_calloc(NMEMB, SIZE);
	zassert_is_null(block_fail, NULL);

	k_free(block[0]);

	/* one free block is available*/
	block[0] = k_calloc(NMEMB, SIZE);
	zassert_not_null(block[0], NULL);

	for (int i = 0; i < nb; i++) {
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
