/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_mheap.h"

#define THREADSAFE_THREAD_NUM 3
#define THREADSAFE_BLOCK_SIZE 16
#define THREADSAFE_STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define MALLOC_ALIGN4_STACK_SIZE (512 + (BLK_NUM_MAX * sizeof(void *)))

struct k_sem threadsafe_sema;
static K_THREAD_STACK_ARRAY_DEFINE(threadsafe_tstack, THREADSAFE_THREAD_NUM, THREADSAFE_STACK_SIZE);
static struct k_thread threadsafe_tdata[THREADSAFE_THREAD_NUM];

static void *threadsafe_pool_blocks[BLK_NUM_MAX];

static K_THREAD_STACK_DEFINE(malloc_align4_tstack, MALLOC_ALIGN4_STACK_SIZE);
static struct k_thread malloc_align4_tdata;

/*test cases*/

static void tmheap_malloc_align4_handler(void *p1, void *p2, void *p3)
{
	void *block[BLK_NUM_MAX];
	int i;

	/**
	 * TESTPOINT: The address of the allocated chunk is guaranteed to be
	 * aligned on a word boundary (4 or 8 bytes).
	 */
	for (i = 0; i < BLK_NUM_MAX; i++) {
		block[i] = k_malloc(i);
		if (block[i] == NULL) {
			break;
		}
		zassert_false((uintptr_t)block[i] % sizeof(void *));
	}

	/* test case tear down*/
	for (int j = 0; j < i; j++) {
		k_free(block[j]);
	}
}

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
ZTEST(mheap_api, test_mheap_malloc_align4)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	k_tid_t tid = k_thread_create(&malloc_align4_tdata, malloc_align4_tstack,
				 MALLOC_ALIGN4_STACK_SIZE,
				 tmheap_malloc_align4_handler,
				 NULL, NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void tmheap_threadsafe_handler(void *p1, void *p2, void *p3)
{
	int thread_id = POINTER_TO_INT(p1);

	threadsafe_pool_blocks[thread_id] = k_malloc(THREADSAFE_BLOCK_SIZE);

	zassert_not_null(threadsafe_pool_blocks[thread_id], "memory is not allocated");

	k_sem_give(&threadsafe_sema);
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
ZTEST(mheap_api, test_mheap_threadsafe)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	k_tid_t tid[THREADSAFE_THREAD_NUM];

	k_sem_init(&threadsafe_sema, 0, THREADSAFE_THREAD_NUM);

	/* create multiple threads to invoke same memory heap APIs*/
	for (int i = 0; i < THREADSAFE_THREAD_NUM; i++) {
		tid[i] = k_thread_create(&threadsafe_tdata[i], threadsafe_tstack[i],
					 THREADSAFE_STACK_SIZE, tmheap_threadsafe_handler,
					 INT_TO_POINTER(i), NULL, NULL,
					 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	for (int i = 0; i < THREADSAFE_THREAD_NUM; i++) {
		k_sem_take(&threadsafe_sema, K_FOREVER);
	}

	for (int i = 0; i < THREADSAFE_THREAD_NUM; i++) {
		/* verify free mheap in main thread */
		k_free(threadsafe_pool_blocks[i]);
		k_thread_abort(tid[i]);
	}
}
