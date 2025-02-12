/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_mslab.h"

#define THREAD_NUM 3
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_MEM_SLAB_DEFINE(mslab1, BLK_SIZE, BLK_NUM, BLK_ALIGN);

static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREAD_NUM, STACK_SIZE);
static struct k_thread tdata[THREAD_NUM];
static struct k_sem sync_sema;
static void *block_ok;

/*thread entry*/
void tmslab_alloc_wait_timeout(void *p1, void *p2, void *p3)
{
	void *block;

	zassert_true(k_mem_slab_alloc(&mslab1, &block, TIMEOUT) == -EAGAIN,
		     NULL);
	k_sem_give(&sync_sema);
}

void tmslab_alloc_wait_ok(void *p1, void *p2, void *p3)
{
	zassert_true(k_mem_slab_alloc(&mslab1, &block_ok, TIMEOUT) == 0);
	k_sem_give(&sync_sema);
}

/*test cases*/
/**
 * @brief Verify alloc with multiple threads
 *
 * @details The test allocates all blocks of memory slab and
 * then spawns 3 threads with lowest priority and 2 more with
 * same priority higher than first thread with delay 10ms and
 * 20ms. Checks the behavior of alloc when requested by multiple
 * threads
 *
 * @ingroup kernel_memory_slab_tests
 *
 * @see k_mem_slab_alloc()
 * @see k_mem_slab_free()
 */
ZTEST(mslab_concept, test_mslab_alloc_wait_prio)
{
	void *block[BLK_NUM];
	k_tid_t tid[THREAD_NUM];

	k_sem_init(&sync_sema, 0, THREAD_NUM);
	/*allocated up all blocks*/
	for (int i = 0; i < BLK_NUM; i++) {
		zassert_equal(k_mem_slab_alloc(&mslab1, &block[i], K_NO_WAIT),
			      0, NULL);
	}

	/**
	 * TESTPOINT: Any number of threads may wait on an empty memory slab
	 * simultaneously; when a memory block becomes available, it is given to
	 * the highest-priority thread that has waited the longest.
	 */
	/**
	 * TESTPOINT: If all the blocks are currently in use, a thread can
	 * optionally wait for one to become available.
	 */
	/*the low-priority thread*/
	tid[0] = k_thread_create(&tdata[0], tstack[0], STACK_SIZE,
				 tmslab_alloc_wait_timeout, NULL, NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	/*the highest-priority thread that has waited the longest*/
	tid[1] = k_thread_create(&tdata[1], tstack[1], STACK_SIZE,
				 tmslab_alloc_wait_ok, NULL, NULL, NULL,
				 K_PRIO_PREEMPT(0), 0, K_MSEC(10));
	/*the highest-priority thread that has waited shorter*/
	tid[2] = k_thread_create(&tdata[2], tstack[2], STACK_SIZE,
				 tmslab_alloc_wait_timeout, NULL, NULL, NULL,
				 K_PRIO_PREEMPT(0), 0, K_MSEC(20));
	/*relinquish CPU for above threads to start */
	k_msleep(30);
	/*free one block, expected to unblock thread "tid[1]"*/
	k_mem_slab_free(&mslab1, block[0]);
	/*wait for all threads exit*/
	for (int i = 0; i < THREAD_NUM; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}

	/*test case tear down*/
	for (int i = 0; i < THREAD_NUM; i++) {
		k_thread_abort(tid[i]);
	}
	k_mem_slab_free(&mslab1, block_ok);
	for (int i = 1; i < BLK_NUM; i++) {
		k_mem_slab_free(&mslab1, block[i]);
	}
}
