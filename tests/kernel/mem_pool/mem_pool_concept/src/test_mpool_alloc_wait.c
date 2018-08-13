/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_mpool.h"

#define THREAD_NUM 3
K_MEM_POOL_DEFINE(mpool1, BLK_SIZE_MIN, BLK_SIZE_MAX, BLK_NUM_MAX, BLK_ALIGN);

static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREAD_NUM, STACK_SIZE);
static struct k_thread tdata[THREAD_NUM];
static struct k_sem sync_sema;
static struct k_mem_block block_ok;

/*thread entry*/
void tmpool_alloc_wait_timeout(void *p1, void *p2, void *p3)
{
	struct k_mem_block block;

	zassert_true(k_mem_pool_alloc(&mpool1, &block, BLK_SIZE_MIN,
				      TIMEOUT) == -EAGAIN, NULL);
	k_sem_give(&sync_sema);
}

void tmpool_alloc_wait_ok(void *p1, void *p2, void *p3)
{
	zassert_true(k_mem_pool_alloc(&mpool1, &block_ok, BLK_SIZE_MIN,
				      TIMEOUT) == 0, NULL);
	k_sem_give(&sync_sema);
}

/*test cases*/
/**
 * @brief Verify alloc and free with different prio threads
 *
 * @ingroup kernel_memory_pool_tests
 *
 * @details The test case allocates 3 blocks of 64 bytes,
 * and spawns 3 threads with lowest priority T1 and other
 * 2 threads, T2 and T3 of same but higher than T1 with
 * delayed start 10ms and 20ms respectively. Then checks
 * the behavior of allocation.
 *
 * @see k_mem_pool_alloc()
 * @see k_mem_pool_free()
 */
void test_mpool_alloc_wait_prio(void)
{
	struct k_mem_block block[BLK_NUM_MIN];
	k_tid_t tid[THREAD_NUM];

	k_sem_init(&sync_sema, 0, THREAD_NUM);
	/*allocated up all blocks*/
	for (int i = 0; i < BLK_NUM_MIN; i++) {
		zassert_true(k_mem_pool_alloc(&mpool1, &block[i], BLK_SIZE_MIN,
					      K_NO_WAIT) == 0, NULL);
	}

	/**
	 * TESTPOINT: when a suitable memory block becomes available, it is
	 * given to the highest-priority thread that has waited the longest
	 */
	/**
	 * TESTPOINT: If a block of the desired size is unavailable, a thread
	 * can optionally wait for one to become available
	 */
	/*the low-priority thread*/
	tid[0] = k_thread_create(&tdata[0], tstack[0], STACK_SIZE,
				 tmpool_alloc_wait_timeout, NULL, NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, 0);
	/*the highest-priority thread that has waited the longest*/
	tid[1] = k_thread_create(&tdata[1], tstack[1], STACK_SIZE,
				 tmpool_alloc_wait_ok, NULL, NULL, NULL,
				 K_PRIO_PREEMPT(0), 0, 10);
	/*the highest-priority thread that has waited shorter*/
	tid[2] = k_thread_create(&tdata[2], tstack[2], STACK_SIZE,
				 tmpool_alloc_wait_timeout, NULL, NULL, NULL,
				 K_PRIO_PREEMPT(0), 0, 20);
	/*relinquish CPU for above threads to start */
	k_sleep(30);
	/*free one block, expected to unblock thread "tid[1]"*/
	k_mem_pool_free(&block[0]);
	/*wait for all threads exit*/
	for (int i = 0; i < THREAD_NUM; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}

	/*test case tear down*/
	for (int i = 0; i < THREAD_NUM; i++) {
		k_thread_abort(tid[i]);
	}
	k_mem_pool_free(&block_ok);
	for (int i = 1; i < BLK_NUM_MIN; i++) {
		k_mem_pool_free(&block[i]);
	}
}
