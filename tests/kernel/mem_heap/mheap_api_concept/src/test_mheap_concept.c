/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mheap
 * @{
 * @defgroup t_mheap_concept test_mheap_concept
 * @brief TestPurpose: verify memory pool concepts.
 * @details All TESTPOINTs extracted from kernel documentation.
 * https://www.zephyrproject.org/doc/kernel/memory/heap.html#concepts
 *
 * TESTPOINTs cover testable kernel behaviours that preserve across internal
 * implementation change or kernel version change.
 * As a black-box test, TESTPOINTs do not cover internal operations.
 * @}
 */

#include <ztest.h>
#include "test_mheap.h"

/* request 0 bytes*/
#define TEST_SIZE_0 0

/*test cases*/
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
