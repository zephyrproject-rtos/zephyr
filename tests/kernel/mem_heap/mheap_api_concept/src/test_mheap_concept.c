/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_mheap.h"

#define THREAD_NUM 3
#define BLOCK_SIZE 16
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

struct k_sem sync_sema;
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREAD_NUM, STACK_SIZE);
static struct k_thread tdata[THREAD_NUM];
static void *block[BLK_NUM_MAX];

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

static void tmheap_handler(void *p1, void *p2, void *p3)
{
	int thread_id = POINTER_TO_INT(p1);

	block[thread_id] = k_malloc(BLOCK_SIZE);

	zassert_not_null(block[thread_id], "memory is not allocated");

	k_sem_give(&sync_sema);
}

/**
 * @brief Verify alloc from multiple equal priority threads
 *
 * @details Test creates three preemptive threads of equal priority.
 * In each child thread , call k_malloc() to alloc a block of memory.
 * Check These four threads can share the same heap space without
 * interfering with each other.
 *
 * @ingroup kernel_memory_slab_tests
 */
void test_mheap_threadsafe(void)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	k_tid_t tid[THREAD_NUM];

	k_sem_init(&sync_sema, 0, THREAD_NUM);

	/* create multiple threads to invoke same memory heap APIs*/
	for (int i = 0; i < THREAD_NUM; i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
					 tmheap_handler, INT_TO_POINTER(i), NULL, NULL,
					 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	for (int i = 0; i < THREAD_NUM; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}

	for (int i = 0; i < THREAD_NUM; i++) {
		/* verify free mheap in main thread */
		k_free(block[i]);
		k_thread_abort(tid[i]);
	}
}
