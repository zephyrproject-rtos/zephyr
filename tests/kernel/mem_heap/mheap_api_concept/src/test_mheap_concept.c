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
 * @brief The test case to ensure heap minimum block size is 64 bytes.
 *
 * @ingroup kernel_heap_tests
 *
 * @see k_malloc(), k_free()
 *
 * @details Heap pool's minimum block size is 64 bytes. The test case tries
 * to ensure it by allocating blocks lesser than minimum block size.
 * The test allocates 8 blocks of size 0. The algorithm has to allocate 64
 * bytes of blocks, this is ensured by allocating one more block of max size
 * which results in failure. Finally all the blocks are freed and added back
 * to heap memory pool.
 */
void test_mheap_min_block_size(void)
{
	void *block[BLK_NUM_MAX], *block_fail;

	/* The k_heap backend doesn't have the splitting behavior
	 * expected here, this test is too specific, and a more
	 * general version of the same test is available in
	 * test_mheap_malloc_free()
	 */
	if (IS_ENABLED(CONFIG_MEM_POOL_HEAP_BACKEND)) {
		ztest_test_skip();
	}

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
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		block[i] = k_malloc(TEST_SIZE_0);
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
	 * TESTPOINT: The kernel uses the first 16 bytes of any memory block
	 * allocated from the heap memory pool to save the block descriptor
	 * information it needs to later free the block. Consequently, an
	 * application's request for an N byte chunk of heap memory requires a
	 * block that is at least (N+16) bytes long.
	 * Test steps:
	 * initial memory heap status (F for free, U for used):
	 *    64F, 64F, 64F, 64F
	 * 1. request 4 blocks: each (64-16) bytes, indeed 64-byte allocated
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
