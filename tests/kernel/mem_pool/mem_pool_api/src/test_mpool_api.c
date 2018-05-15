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

/**
 * @brief Test to validate k_mem_pool_allocate and k_mem_pool_free()
 * of different block sizes.
 *
 * @details The test defines a memory pool which has 2 blocks of
 * maximum size 128 bytes and minimum size 64 bytes with block
 * alignment of 8 bytes. The test allocates 2 blocks of size 64
 * bytes and frees up both the blocks. Then it allocates 2 blocks
 * of size 128 bytes and frees them up. The test is basically
 * checking if allocation happens for MAX_SIZE and MIN_SIZE
 * defined in memory pool.
 */
void test_mpool_alloc_free_thread(void)
{
	tmpool_alloc_free(NULL);
}

/**
 * @brief Test to validate alloc and free on mem_pool on IRQ context
 *
 * @details The test is run on IRQ context. The test calls the same
 * function which mem_pool_alloc_free_thread does.
 * The test defines a memory pool which has 2 blocks of maximum
 * size 128 bytes and minimum size 64 bytes with block alignment
 * of 8 bytes. The test allocates 2 blocks of size 64 bytes and
 * frees up both the blocks. Then it allocates 2 blocks of size 128
 * bytes and frees them up. The test is basically checking if allocation
 * happens for MAX_SIZE and MIN_SIZE defined in memory pool.
 */
void test_mpool_alloc_free_isr(void)
{
	irq_offload(tmpool_alloc_free, NULL);
}

/**
 * @brief Test to validate how a mem_pool provides functionality
 * to break a block into quarters.
 *
 * @details The test case validates how a mem_pool provides
 * functionality to break a block into quarters and repeatedly
 * allocate and free the blocks. Initially a block of 128
 * bytes(MAX_BLCK_SIZE) is allocated and checked if it is 8 byte
 * aligned. Checked if the block size allocated is greater than
 * or equal to MIN_BLCK_SIZE which is 32. Now a block of 32 bytes
 * is allocated and checked for 8 byte alignment and >= to
 * MIN_BLCK_SIZE. All the blocks are freed. Now do the same
 * procedure starting with allocation of MIN_BLCK_SIZE and gradually
 * increase the size of block upto MAX_BLCK_SIZE along with checking
 * for alignment. finally realise all the blocks allocated using
 * k_mem_pool_free(&block_id).
 */
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

/**
 * @brief Verify memory pool allocation with timeouts
 */
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

