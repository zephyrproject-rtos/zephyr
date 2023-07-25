/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_mslab.h"

/* TESTPOINT: Statically define and initialize a memory slab*/
K_MEM_SLAB_DEFINE(kmslab, BLK_SIZE, BLK_NUM, BLK_ALIGN);
static char __aligned(BLK_ALIGN) tslab[BLK_SIZE * BLK_NUM];
static struct k_mem_slab mslab;
K_SEM_DEFINE(SEM_HELPERDONE, 0, 1);
K_SEM_DEFINE(SEM_REGRESSDONE, 0, 1);
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread HELPER;

void *mslab_setup(void)
{
	k_mem_slab_init(&mslab, tslab, BLK_SIZE, BLK_NUM);

	return NULL;
}

void tmslab_alloc_free(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM];

	(void)memset(block, 0, sizeof(block));
	/*
	 * TESTPOINT: The memory slab's buffer contains @a slab_num_blocks
	 * memory blocks that are @a slab_block_size bytes long.
	 */
	for (int i = 0; i < BLK_NUM; i++) {
		/* TESTPOINT: Allocate memory from a memory slab.*/
		/* TESTPOINT: @retval 0 Memory allocated.*/
		zassert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			     NULL);
		/*
		 * TESTPOINT: The block address area pointed at by @a mem is set
		 * to the starting address of the memory block.
		 */
		zassert_not_null(block[i], NULL);
	}
	for (int i = 0; i < BLK_NUM; i++) {
		/* TESTPOINT: Free memory allocated from a memory slab.*/
		k_mem_slab_free(pslab, &block[i]);
	}
}

static void tmslab_alloc_align(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM];

	for (int i = 0; i < BLK_NUM; i++) {
		zassert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			     NULL);
		/*
		 * TESTPOINT: To ensure that each memory block is similarly
		 * aligned to this boundary
		 */
		zassert_true((uintptr_t)block[i] % BLK_ALIGN == 0U);
	}
	for (int i = 0; i < BLK_NUM; i++) {
		k_mem_slab_free(pslab, &block[i]);
	}
}

static void tmslab_alloc_timeout(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM], *block_fail;
	int64_t tms;
	int err;

	for (int i = 0; i < BLK_NUM; i++) {
		zassert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			     NULL);
	}

	/* TESTPOINT: Use K_NO_WAIT to return without waiting*/
	/* TESTPOINT: -ENOMEM Returned without waiting.*/
	zassert_equal(k_mem_slab_alloc(pslab, &block_fail, K_NO_WAIT), -ENOMEM,
		      NULL);
	tms = k_uptime_get();
	err = k_mem_slab_alloc(pslab, &block_fail, K_MSEC(TIMEOUT));
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		/* TESTPOINT: -EAGAIN Waiting period timed out*/
		zassert_equal(err, -EAGAIN);
		/*
		 * TESTPOINT: timeout Maximum time to wait for operation to
		 * complete (in milliseconds)
		 */
		zassert_true(k_uptime_delta(&tms) >= TIMEOUT);
	} else {
		/* If no multithreading any timeout is treated as K_NO_WAIT */
		zassert_equal(err, -ENOMEM);
		zassert_true(k_uptime_delta(&tms) < TIMEOUT);
	}

	for (int i = 0; i < BLK_NUM; i++) {
		k_mem_slab_free(pslab, &block[i]);
	}
}

static void tmslab_used_get(void *data)
{
	struct k_mem_slab *pslab = (struct k_mem_slab *)data;
	void *block[BLK_NUM], *block_fail;

	for (int i = 0; i < BLK_NUM; i++) {
		zassert_true(k_mem_slab_alloc(pslab, &block[i], K_NO_WAIT) == 0,
			     NULL);
		/* TESTPOINT: Get the number of used blocks in a memory slab.*/
		zassert_equal(k_mem_slab_num_used_get(pslab), i + 1);
		/*
		 * TESTPOINT: Get the number of unused blocks in a memory slab.
		 */
		zassert_equal(k_mem_slab_num_free_get(pslab), BLK_NUM - 1 - i);
	}

	zassert_equal(k_mem_slab_alloc(pslab, &block_fail, K_NO_WAIT), -ENOMEM,
		      NULL);
	/* free get on allocation failure*/
	zassert_equal(k_mem_slab_num_free_get(pslab), 0);
	/* used get on allocation failure*/
	zassert_equal(k_mem_slab_num_used_get(pslab), BLK_NUM);

	zassert_equal(k_mem_slab_alloc(pslab, &block_fail, K_MSEC(TIMEOUT)),
		      IS_ENABLED(CONFIG_MULTITHREADING) ? -EAGAIN : -ENOMEM,
		      NULL);
	zassert_equal(k_mem_slab_num_free_get(pslab), 0);
	zassert_equal(k_mem_slab_num_used_get(pslab), BLK_NUM);

	for (int i = 0; i < BLK_NUM; i++) {
		k_mem_slab_free(pslab, &block[i]);
		zassert_equal(k_mem_slab_num_free_get(pslab), i + 1);
		zassert_equal(k_mem_slab_num_used_get(pslab), BLK_NUM - 1 - i);
	}
}

static void helper_thread(void *p0, void *p1, void *p2)
{
	void *ptr[BLK_NUM];           /* Pointer to memory block */

	ARG_UNUSED(p0);
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);

	(void)memset(ptr, 0, sizeof(ptr));

	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/* Get all blocks from the memory slab */
	for (int i = 0; i < BLK_NUM; i++) {
		/* Verify number of used blocks in the map */
		zassert_equal(k_mem_slab_num_used_get(&kmslab), i,
			      "Failed k_mem_slab_num_used_get");

		/* Get memory block */
		zassert_equal(k_mem_slab_alloc(&kmslab, &ptr[i], K_NO_WAIT), 0,
			      "Failed k_mem_slab_alloc");
	}

	k_sem_give(&SEM_HELPERDONE);

	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);
	k_mem_slab_free(&kmslab, &ptr[0]);


	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/* Free all the other blocks.  The first block are freed by this task */
	for (int i = 1; i < BLK_NUM; i++) {
		k_mem_slab_free(&kmslab, &ptr[i]);
	}

	k_sem_give(&SEM_HELPERDONE);

}  /* helper thread */

/*test cases*/
/**
 * @brief Initialize the memory slab using k_mem_slab_init()
 * and allocates/frees blocks.
 *
 * @details Initialize 3 memory blocks of block size 8 bytes
 * using @see k_mem_slab_init() and check if number of used blocks
 * is 0 and free blocks is equal to number of blocks initialized.
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_kinit)
{
	/* if a block_size is not word aligned, slab init return error */
	zassert_equal(k_mem_slab_init(&mslab, tslab, BLK_SIZE + 1, BLK_NUM),
				-EINVAL, NULL);
	k_mem_slab_init(&mslab, tslab, BLK_SIZE, BLK_NUM);
	zassert_equal(k_mem_slab_num_used_get(&mslab), 0);
	zassert_equal(k_mem_slab_num_free_get(&mslab), BLK_NUM);
}

/**
 * @brief Verify K_MEM_SLAB_DEFINE() with allocates/frees blocks.
 *
 * @details Initialize 3 memory blocks of block size 8 bytes
 * using @see K_MEM_SLAB_DEFINE() and check if number of used blocks
 * is 0 and free blocks is equal to number of blocks initialized.
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_kdefine)
{
	zassert_equal(k_mem_slab_num_used_get(&kmslab), 0);
	zassert_equal(k_mem_slab_num_free_get(&kmslab), BLK_NUM);
}

/**
 * @brief Verify alloc and free of blocks from mem_slab
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_alloc_free_thread)
{

	tmslab_alloc_free(&mslab);
}

/**
 * @brief Allocate memory blocks and check for alignment of 8 bytes
 *
 * @details Allocate 3 blocks of memory from 2 memory slabs
 * respectively and check if all blocks are aligned to 8 bytes
 * and free them.
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_alloc_align)
{
	tmslab_alloc_align(&mslab);
	tmslab_alloc_align(&kmslab);
}

/**
 * @brief Verify allocation of memory blocks with timeouts
 *
 * @details Allocate 3 memory blocks from memory slab. Check
 * allocation of another memory block with NO_WAIT set, since
 * there are no blocks left to allocate in the memory slab,
 * the allocation fails with return value -ENOMEM. Then the
 * system up time is obtained, memory block allocation is
 * tried with timeout of 2000 ms. Now the allocation API
 * returns -EAGAIN as the waiting period is timeout. The
 * test case also checks if timeout has really happened by
 * checking delta period between the allocation request
 * was made and return of -EAGAIN.
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_alloc_timeout)
{
	if (arch_num_cpus() != 1) {
		ztest_test_skip();
	}

	tmslab_alloc_timeout(&mslab);
}

/**
 * @brief Verify count of allocated blocks
 *
 * @details The test case allocates 3 blocks one after the
 * other by checking for used block and free blocks in the
 * memory slab - mslab. Once all 3 blocks are allocated,
 * one more block is tried to allocates, which fails with
 * return value -ENOMEM. It also checks the allocation with
 * timeout. Again checks for used block and free blocks
 * number using @see k_mem_slab_num_used_get() and
 * @see k_mem_slab_num_free_get().
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_used_get)
{
	tmslab_used_get(&mslab);
	tmslab_used_get(&kmslab);
}

/**
 * @brief Verify pending of allocating blocks
 *
 * @details First, helper thread got all memory blocks,
 * and there is no free block left. k_mem_slab_alloc() with
 * time out will fail and return -EAGAIN.
 * Then k_mem_slab_alloc() without timeout tries to wait for
 * a memory block until helper thread free one.
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_pending)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		ztest_test_skip();
		return;
	}

	int ret_value;
	void *b;                        /* Pointer to memory block */

	(void)k_thread_create(&HELPER, stack, STACKSIZE,
			helper_thread, NULL, NULL, NULL,
			7, 0, K_NO_WAIT);

	k_sem_give(&SEM_REGRESSDONE);   /* Allow helper thread to run */

	k_sem_take(&SEM_HELPERDONE, K_FOREVER);		/* Wait for helper thread to finish */

	ret_value = k_mem_slab_alloc(&kmslab, &b, K_MSEC(20));
	zassert_equal(-EAGAIN, ret_value,
		      "Failed k_mem_slab_alloc, retValue %d\n", ret_value);

	k_sem_give(&SEM_REGRESSDONE);

	/* Wait for helper thread to free a block */

	ret_value = k_mem_slab_alloc(&kmslab, &b, K_FOREVER);
	zassert_equal(0, ret_value,
		      "Failed k_mem_slab_alloc, ret_value %d\n", ret_value);

	k_sem_give(&SEM_REGRESSDONE);

	/* Wait for helper thread to complete */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);

	/* Free memory block */
	k_mem_slab_free(&kmslab, &b);
}
