/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mslab
 * @{
 * @defgroup t_mslab_api test_mslab_api
 * @brief TestPurpose: verify memory slab APIs.
 * @details All TESTPOINTs extracted from kernel-doc comments in <kernel.h>
 * - API coverage
 *   - K_MEM_SLAB_DEFINE
 *   - k_mem_slab_init
 *   - k_mem_slab_alloc
 *   - k_mem_slab_free
 *   - k_mem_slab_num_used_get
 *   - k_mem_slab_num_free_get
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>
#include "test_mslab.h"

/** TESTPOINT: Statically define and initialize a memory slab*/
K_MEM_SLAB_DEFINE(kmslab, BLK_SIZE, BLK_NUM, BLK_ALIGN);
static char __aligned(BLK_ALIGN) tslab[BLK_SIZE * BLK_NUM];
static struct k_mem_slab mslab;

void tmslab_alloc_free(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM];

	memset(block, 0, sizeof(block));
	/**
	 * TESTPOINT: The memory slab's buffer contains @a slab_num_blocks
	 * memory blocks that are @a slab_block_size bytes long.
	 */
	for (int i = 0; i < BLK_NUM; i++) {
		/** TESTPOINT: Allocate memory from a memory slab.*/
		/** TESTPOINT: @retval 0 Memory allocated.*/
		assert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			NULL);
		/**
		 * TESTPOINT: The block address area pointed at by @a mem is set
		 * to the starting address of the memory block.
		 */
		 assert_not_null(block[i], NULL);
	}
	for (int i = 0; i < BLK_NUM; i++) {
		/** TESTPOINT: Free memory allocated from a memory slab.*/
		k_mem_slab_free(pslab, &block[i]);
	}
}

static void tmslab_alloc_align(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM];

	for (int i = 0; i < BLK_NUM; i++) {
		assert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			NULL);
		/**
		 * TESTPOINT: To ensure that each memory block is similarly
		 * aligned to this boundary
		 */
		assert_true((uint32_t)block[i] % BLK_ALIGN == 0, NULL);
	}
	for (int i = 0; i < BLK_NUM; i++) {
		k_mem_slab_free(pslab, &block[i]);
	}
}

static void tmslab_alloc_timeout(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM], *block_fail;
	uint64_t tms;

	for (int i = 0; i < BLK_NUM; i++) {
		assert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			NULL);
	}

	/** TESTPOINT: Use K_NO_WAIT to return without waiting*/
	/** TESTPOINT: @retval -ENOMEM Returned without waiting.*/
	assert_equal(k_mem_slab_alloc(pslab, &block_fail, K_NO_WAIT), -ENOMEM,
		NULL);
	/** TESTPOINT: @retval -EAGAIN Waiting period timed out*/
	tms = k_uptime_get();
	assert_equal(k_mem_slab_alloc(pslab, &block_fail, TIMEOUT), -EAGAIN,
		NULL);
	/**
	 * TESTPOINT: @param timeout Maximum time to wait for operation to
	 * complete (in milliseconds)
	 */
	assert_true(k_uptime_delta(&tms) >= TIMEOUT, NULL);

	for (int i = 0; i < BLK_NUM; i++) {
		k_mem_slab_free(pslab, &block[i]);
	}
}

static void tmslab_used_get(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM], *block_fail;

	for (int i = 0; i < BLK_NUM; i++) {
		assert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			NULL);
		/** TESTPOINT: Get the number of used blocks in a memory slab.*/
		assert_equal(k_mem_slab_num_used_get(pslab), i+1, NULL);
		/**
		 * TESTPOINT: Get the number of unused blocks in a memory slab.
		 */
		assert_equal(k_mem_slab_num_free_get(pslab), BLK_NUM-1-i, NULL);
	}

	assert_equal(k_mem_slab_alloc(pslab, &block_fail, K_NO_WAIT), -ENOMEM,
		NULL);
	/* free get on allocation failure*/
	assert_equal(k_mem_slab_num_free_get(pslab), 0, NULL);
	/* used get on allocation failure*/
	assert_equal(k_mem_slab_num_used_get(pslab), BLK_NUM, NULL);

	assert_equal(k_mem_slab_alloc(pslab, &block_fail, TIMEOUT), -EAGAIN,
		NULL);
	assert_equal(k_mem_slab_num_free_get(pslab), 0, NULL);
	assert_equal(k_mem_slab_num_used_get(pslab), BLK_NUM, NULL);

	for (int i = 0; i < BLK_NUM; i++) {
		k_mem_slab_free(pslab, &block[i]);
		assert_equal(k_mem_slab_num_free_get(pslab), i+1, NULL);
		assert_equal(k_mem_slab_num_used_get(pslab), BLK_NUM-1-i, NULL);
	}
}

/*test cases*/
void test_mslab_kinit(void)
{
	k_mem_slab_init(&mslab, tslab, BLK_SIZE, BLK_NUM);
	assert_equal(k_mem_slab_num_used_get(&mslab), 0, NULL);
	assert_equal(k_mem_slab_num_free_get(&mslab), BLK_NUM, NULL);
}

void test_mslab_kdefine(void)
{
	assert_equal(k_mem_slab_num_used_get(&kmslab), 0, NULL);
	assert_equal(k_mem_slab_num_free_get(&kmslab), BLK_NUM, NULL);
}

void test_mslab_alloc_free_thread(void)
{

	tmslab_alloc_free(&mslab);
}

void test_mslab_alloc_free_isr(void)
{
	irq_offload(tmslab_alloc_free, &mslab);
}

void test_mslab_alloc_align(void)
{
	tmslab_alloc_align(&mslab);
	tmslab_alloc_align(&kmslab);
}

void test_mslab_alloc_timeout(void)
{
	tmslab_alloc_timeout(&mslab);
}

void test_mslab_used_get(void)
{
	tmslab_used_get(&mslab);
	tmslab_used_get(&kmslab);
}
