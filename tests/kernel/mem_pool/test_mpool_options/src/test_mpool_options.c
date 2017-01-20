/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mpool
 * @{
 * @defgroup t_mpool_options test_mpool_options
 * @brief TestPurpose: verify memory pool configure options
 * @details All TESTPOINTs extracted from kernel documentation
 * - Configure options covered
 *   - CONFIG_MEM_POOL_SPLIT_BEFORE_DEFRAG
 *   - CONFIG_MEM_POOL_DEFRAG_BEFORE_SPLIT
 *   - CONFIG_MEM_POOL_SPLIT_ONLY
 * @}
 */

#include <ztest.h>
#define TIMEOUT 2000
#define BLK_SIZE_MIN 4
#define BLK_SIZE_MID 16
#define BLK_SIZE_MAX 64
#define BLK_NUM_MIN 32
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN

K_MEM_POOL_DEFINE(mpool1, BLK_SIZE_MIN, BLK_SIZE_MAX, BLK_NUM_MAX, BLK_ALIGN);

static struct k_mem_block block[BLK_NUM_MIN];
static size_t block_size[] = {
	BLK_SIZE_MIN, BLK_SIZE_MIN, BLK_SIZE_MIN, BLK_SIZE_MIN,
	BLK_SIZE_MID, BLK_SIZE_MID, BLK_SIZE_MID};
static int block_count = sizeof(block_size)/sizeof(size_t);

#ifdef CONFIG_MEM_POOL_SPLIT_BEFORE_DEFRAG
static void tmpool_split_before_defrag(void)
{
	struct k_mem_block block_split, block_max;
	/**
	 * TESTPOINT: This option instructs a memory pool to try splitting a
	 * larger unused block if an unused block of the required size is not
	 * available; only if no such blocks exist will the memory pool try
	 * merging smaller unused blocks.
	 * Test steps: mpool1 initial status (F for free, U for used)
	 *             4F 4F 4F 4F 16U 16U 16U 64F
	 *             1. request a mid-size (16) block
	 *             2. verify the previous block was splited from 64F, since
	 *                consequently a further request to max-size block fail
	 */
	TC_PRINT("CONFIG_MEM_POOL_SPLIT_BEFORE_DEFRAG\n");
	/* 1. request a mid-size block*/
	assert_true(k_mem_pool_alloc(&mpool1, &block_split, BLK_SIZE_MID,
		K_NO_WAIT) == 0, NULL);
	/* 2. verify the previous block was split from the 2nd max block*/
	assert_true(k_mem_pool_alloc(&mpool1, &block_max, BLK_SIZE_MAX,
		TIMEOUT) == -EAGAIN, NULL);
	k_mem_pool_free(&block_split);
}
#endif

#ifdef CONFIG_MEM_POOL_DEFRAG_BEFORE_SPLIT
static void tmpool_defrag_before_split(void)
{
	struct k_mem_block block_defrag, block_max;
	/**
	 * TESTPOINT: This option instructs a memory pool to try merging smaller
	 * unused blocks if an unused block of the required size is not
	 * available; only if this does not generate a sufficiently large block
	 * will the memory pool try splitting a larger unused block.
	 * Test steps: mpool1 initial status (F for free, U for used)
	 *             4F 4F 4F 4F 16U 16U 16U 64F
	 *             1. request a mid-size (16) block
	 *             2. verify the previous block was defrag from 4*4F, since
	 *                consequently a further request to max-size block pass
	 */
	TC_PRINT("CONFIG_MEM_POOL_DEFRAG_BEFORE_SPLIT\n");
	/* 1. request a mid-size block*/
	assert_true(k_mem_pool_alloc(&mpool1, &block_defrag, BLK_SIZE_MID,
		TIMEOUT) == 0, NULL);
	/* 2. verify the previous block was defrag from block[0~3]*/
	assert_true(k_mem_pool_alloc(&mpool1, &block_max, BLK_SIZE_MAX,
		K_NO_WAIT) == 0, NULL);
	k_mem_pool_free(&block_defrag);
	k_mem_pool_free(&block_max);
}
#endif

#ifdef CONFIG_MEM_POOL_SPLIT_ONLY
static void tmpool_split_only(void)
{
	struct k_mem_block block_mid[4], block_fail;
	/**
	 * TESTPOINT: This option instructs a memory pool to try splitting a
	 * larger unused block if an unused block of the required size is not
	 * available; if no such blocks exist the block allocation operation
	 * fails.
	 * Test steps: mpool1 initial status (F for free, U for used)
	 *             4F 4F 4F 4F 16U 16U 16U 64F
	 *             1. request 4 mid-size (16) blocks, verify allocation
	 *                ok via splitting the 64F max block
	 *             2. request another mid-size (16) block, verify allocation
	 *                failed since no large blocks to split, nor the memory
	 *                pool is configured to do defrag (merging)
	 */
	TC_PRINT("CONFIG_MEM_POOL_SPLIT_ONLY\n");
	for (int i = 0; i < 4; i++) {
		/* 1. verify allocation ok via spliting the max block*/
		assert_true(k_mem_pool_alloc(&mpool1, &block_mid[i],
			BLK_SIZE_MID, K_NO_WAIT) == 0, NULL);
	}
	/* 2. verify allocation failed since no large blocks nor defrag*/
	assert_true(k_mem_pool_alloc(&mpool1, &block_fail, BLK_SIZE_MID,
		TIMEOUT) == -EAGAIN, NULL);
	for (int i = 0; i < 4; i++) {
		k_mem_pool_free(&block_mid[i]);
	}
}
#endif

/* test cases*/
void test_mpool_alloc_options(void)
{
	/* allocate 7 blocks, in size 4 4 4 4 16 16 16, respectively*/
	for (int i = 0; i < block_count; i++) {
		assert_true(k_mem_pool_alloc(&mpool1, &block[i], block_size[i],
			K_NO_WAIT) == 0, NULL);
	}
	/* free block [0~3]*/
	for (int i = 0; i < 4; i++) {
		k_mem_pool_free(&block[i]);
	}

	#ifdef CONFIG_MEM_POOL_SPLIT_BEFORE_DEFRAG
	tmpool_split_before_defrag();
	#endif
	#ifdef CONFIG_MEM_POOL_DEFRAG_BEFORE_SPLIT
	tmpool_defrag_before_split();
	#endif
	#ifdef CONFIG_MEM_POOL_SPLIT_ONLY
	tmpool_split_only();
	#endif

	/* test case tear down*/
	for (int i = 4; i < block_count; i++) {
		k_mem_pool_free(&block[i]);
	}
}

