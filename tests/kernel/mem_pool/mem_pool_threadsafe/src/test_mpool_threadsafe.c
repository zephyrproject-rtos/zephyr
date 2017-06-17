/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mpool
 * @{
 * @defgroup t_mpool_threadsafe test_mpool_threadsafe
 * @brief TestPurpose: verify API thread safe in multi-threads environment.
 * @details This's an extensive test. Multiple threads in same priority are
 * created, with time slice scheduling enabled in a very small slicing size:
 * 1 millisecond (refer to prj.conf).
 * All threads share a same entry function to invoke same kernel APIs.
 * Unless explicitly stated, kernel APIs are supposed to be thread safe.
 * Expect all threads should complete and exit the entry function normally.
 *
 * NOTE:
 * # API functionality is not TESTPOINT here. When invoked by multiple
 * threads, each API following its own behavior specification returns either
 * success or failure. Like, memory allocation successful or fail is pending on
 * whether any free memory blocks are available at the moment when the API is
 * invoked by a thread just got scheduled in.
 * # For kernel object APIs, more than one instance of the same object type are
 * created. Like, 4 threads operating on 2 instances, the test would cover the
 * kernel's code branches that handling "multiple threads accessing to a same
 * instance" and that handling "multiple instances serving simultaneously".
 * # The test adopts a slicing size in 1 millisecond. Thread safe theoretically
 * should work in a smaller slicing size. But this does not intent to stress
 * test that.
 * @}
 */

#include <ztest.h>
#include <atomic.h>
#define THREAD_NUM 4
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define POOL_NUM 2
#define TIMEOUT 200
#define BLK_SIZE_MIN 4
#define BLK_SIZE_MAX 16
#define BLK_NUM_MIN 8
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN

K_MEM_POOL_DEFINE(mpool1, BLK_SIZE_MIN, BLK_SIZE_MAX, BLK_NUM_MAX, BLK_ALIGN);
K_MEM_POOL_DEFINE(mpool2, BLK_SIZE_MIN, BLK_SIZE_MAX, BLK_NUM_MAX, BLK_ALIGN);
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREAD_NUM, STACK_SIZE);
static struct k_thread tdata[THREAD_NUM];
static struct k_mem_pool *pools[POOL_NUM] = {&mpool1, &mpool2};
static struct k_sem sync_sema;
static atomic_t pool_id;

/* thread entry simply invoke the APIs*/
static void tmpool_api(void *p1, void *p2, void *p3)
{
	struct k_mem_block block[BLK_NUM_MIN];
	int ret[BLK_NUM_MIN];
	struct k_mem_pool *pool = pools[atomic_inc(&pool_id) % POOL_NUM];

	memset(block, 0, sizeof(block));

	for (int i = 0; i < 4; i++) {
		ret[i] = k_mem_pool_alloc(pool, &block[i], BLK_SIZE_MIN,
			TIMEOUT);
	}
	ret[4] = k_mem_pool_alloc(pool, &block[4], BLK_SIZE_MAX, TIMEOUT);
	for (int i = 0; i < 5; i++) {
		if (ret[i] == 0) {
			k_mem_pool_free(&block[i]);
		}
	}
	k_mem_pool_defrag(pool);

	k_sem_give(&sync_sema);
}

/* test cases*/
void test_mpool_threadsafe(void)
{
	k_tid_t tid[THREAD_NUM];

	k_sem_init(&sync_sema, 0, THREAD_NUM);

	/* create multiple threads to invoke same memory pool APIs*/
	for (int i = 0; i < THREAD_NUM; i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
			tmpool_api, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), 0, 0);
	}
	/* TESTPOINT: all threads complete and exit the entry function*/
	for (int i = 0; i < THREAD_NUM; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}

	/* test case tear down*/
	for (int i = 0; i < THREAD_NUM; i++) {
		k_thread_abort(tid[i]);
	}
}
