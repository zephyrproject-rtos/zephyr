/*
 * Copyright (c) 2016 Intel Corporation
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
 * @see k_malloc(), k_free()
 *
 * @details The 8 blocks of memory of size 16 bytes are allocated
 * by k_calloc() API. When allocated using k_calloc() the memory buffers
 * have to be zeroed. Check is done, if the blocks are memset to 0 and
 * read/write is allowed. The test is then teared up by freeing all the
 * blocks allocated.
 */
void test_mheap_malloc_align4(void)
{
	void *block[BLK_NUM_MAX];

	/**
	 * TESTPOINT: The address of the allocated chunk is guaranteed to be
	 * aligned on a multiple of 4 bytes.
	 */
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		block[i] = k_malloc(i);
		zassert_not_null(block[i], NULL);
		zassert_false((int)block[i] % 4, NULL);
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
