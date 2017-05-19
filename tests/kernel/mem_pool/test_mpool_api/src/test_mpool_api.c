/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mpool
 * @{
 * @defgroup t_mpool_api test_mpool_api
 * @brief TestPurpose: verify memory pool APIs.
 * @details All TESTPOINTs extracted from kernel-doc comments in <kernel.h>
 * - API coverage
 *   -# K_MEM_POOL_DEFINE
 *   -# k_mem_pool_alloc
 *   -# k_mem_pool_free
 *   -# k_mem_pool_defrag
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>
#include "test_mpool.h"

/** TESTPOINT: Statically define and initialize a memory pool*/
K_MEM_POOL_DEFINE(kmpool, BLK_SIZE_MIN, BLK_SIZE_MAX, BLK_NUM_MAX, BLK_ALIGN);

void tmpool_alloc_free(void *data)
{
	ARG_UNUSED(data);
	struct k_mem_block block[BLK_NUM_MIN];

	for (int i = 0; i < BLK_NUM_MIN; i++) {
		/**
		 * TESTPOINT: This routine allocates a memory block from a
		 * memory pool.
		 */
		/**
		 * TESTPOINT: @retval 0 Memory allocated. The @a data field of
		 * the block descriptor is set to the starting address of the
		 * memory block.
		 */
		zassert_true(k_mem_pool_alloc(&kmpool, &block[i], BLK_SIZE_MIN,
			K_NO_WAIT) == 0, NULL);
		zassert_not_null(block[i].data, NULL);
	}

	for (int i = 0; i < BLK_NUM_MIN; i++) {
		/**
		 * TESTPOINT: This routine releases a previously allocated
		 * memory block back to its memory pool.
		 */
		k_mem_pool_free(&block[i]);
		block[i].data = NULL;
	}

	/**
	 * TESTPOINT: The memory pool's buffer contains @a n_max blocks that are
	 * @a max_size bytes long.
	 */
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		zassert_true(k_mem_pool_alloc(&kmpool, &block[i], BLK_SIZE_MAX,
			K_NO_WAIT) == 0, NULL);
		zassert_not_null(block[i].data, NULL);
	}

	for (int i = 0; i < BLK_NUM_MAX; i++) {
		k_mem_pool_free(&block[i]);
		block[i].data = NULL;
	}
}

/*test cases*/
void test_mpool_alloc_free_thread(void)
{
	tmpool_alloc_free(NULL);
}

void test_mpool_alloc_free_isr(void)
{
	irq_offload(tmpool_alloc_free, NULL);
}

void test_mpool_alloc_size(void)
{
	struct k_mem_block block[BLK_NUM_MIN];
	size_t size = BLK_SIZE_MAX;
	int i = 0;

	/**TESTPOINT: The memory pool allows blocks to be repeatedly partitioned
	 * into quarters, down to blocks of @a min_size bytes long.
	 */
	while (size >= BLK_SIZE_MIN) {
		zassert_true(k_mem_pool_alloc(&kmpool, &block[i], size,
			K_NO_WAIT) == 0, NULL);
		zassert_not_null(block[i].data, NULL);
		zassert_true((u32_t)(block[i].data) % BLK_ALIGN == 0, NULL);
		i++;
		size = size >> 2;
	}
	while (i--) {
		k_mem_pool_free(&block[i]);
		block[i].data = NULL;
	}

	i = 0;
	size = BLK_SIZE_MIN;
	/**TESTPOINT: To ensure that all blocks in the buffer are similarly
	 * aligned to this boundary, min_size must also be a multiple of align.
	 */
	while (size <= BLK_SIZE_MAX) {
		zassert_true(k_mem_pool_alloc(&kmpool, &block[i], size,
			K_NO_WAIT) == 0, NULL);
		zassert_not_null(block[i].data, NULL);
		zassert_true((u32_t)(block[i].data) % BLK_ALIGN == 0, NULL);
		i++;
		size = size << 2;
	}
	while (i--) {
		k_mem_pool_free(&block[i]);
		block[i].data = NULL;
	}
}

void test_mpool_alloc_timeout(void)
{
	struct k_mem_block block[BLK_NUM_MIN], fblock;
	s64_t tms;

	for (int i = 0; i < BLK_NUM_MIN; i++) {
		zassert_equal(k_mem_pool_alloc(&kmpool, &block[i], BLK_SIZE_MIN,
			K_NO_WAIT), 0, NULL);
	}

	/** TESTPOINT: Use K_NO_WAIT to return without waiting*/
	/** TESTPOINT: @retval -ENOMEM Returned without waiting*/
	zassert_equal(k_mem_pool_alloc(&kmpool, &fblock, BLK_SIZE_MIN,
		K_NO_WAIT), -ENOMEM, NULL);
	/** TESTPOINT: @retval -EAGAIN Waiting period timed out*/
	tms = k_uptime_get();
	zassert_equal(k_mem_pool_alloc(&kmpool, &fblock, BLK_SIZE_MIN, TIMEOUT),
		-EAGAIN, NULL);
	/**
	 * TESTPOINT: Maximum time to wait for operation to complete (in
	 * milliseconds)
	 */
	zassert_true(k_uptime_delta(&tms) >= TIMEOUT, NULL);

	for (int i = 0; i < BLK_NUM_MIN; i++) {
		k_mem_pool_free(&block[i]);
		block[i].data = NULL;
	}
}

void test_mpool_defrag(void)
{
	struct k_mem_block block[BLK_NUM_MIN];

	/*fragment the memory pool into small blocks*/
	for (int i = 0; i < BLK_NUM_MIN; i++) {
		zassert_true(k_mem_pool_alloc(&kmpool, &block[i], BLK_SIZE_MIN,
			K_NO_WAIT) == 0, NULL);
	}
	/*free the small blocks in the 1st half of the pool*/
	for (int i = 0; i < (BLK_NUM_MIN >> 1); i++) {
		k_mem_pool_free(&block[i]);
	}
	/*request a big block, the pool has "adjacent free blocks" to merge*/
	zassert_true(k_mem_pool_alloc(&kmpool, &block[0], BLK_SIZE_MAX,
		K_FOREVER) == 0, NULL);
	/*free the small blocks in the 2nd half of the pool*/
	for (int i = (BLK_NUM_MIN >> 1); i < BLK_NUM_MIN; i++) {
		k_mem_pool_free(&block[i]);
	}
	/**
	 * TESTPOINT: This routine instructs a memory pool to concatenate unused
	 * memory blocks into larger blocks wherever possible.
	 */
	/*do manual de-fragment*/
	k_mem_pool_defrag(&kmpool);
	/*request a big block, the pool is de-fragmented*/
	zassert_true(k_mem_pool_alloc(&kmpool, &block[1], BLK_SIZE_MAX,
		K_NO_WAIT) == 0, NULL);
	/*free the big blocks*/
	for (int i = 0; i < BLK_NUM_MAX; i++) {
		k_mem_pool_free(&block[i]);
	}
}
