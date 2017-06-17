/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mslab
 * @{
 * @defgroup t_mslab_threadsafe test_mslab_threadsafe
 * @brief TestPurpose: verify API thread safe in multi-threads environment
 * @}
 */

#include <ztest.h>
#include <atomic.h>
#define LOOP 10
#define STACK_SIZE 512
#define THREAD_NUM 4
#define SLAB_NUM 2
#define TIMEOUT 200
#define BLK_NUM 3
#define BLK_ALIGN 4
#define BLK_SIZE1 8
#define BLK_SIZE2 4

K_MEM_SLAB_DEFINE(mslab1, BLK_SIZE1, BLK_NUM, BLK_ALIGN);
static struct k_mem_slab mslab2, *slabs[SLAB_NUM] = {&mslab1, &mslab2};
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREAD_NUM, STACK_SIZE);
static struct k_thread tdata[THREAD_NUM];
static char __aligned(BLK_ALIGN) tslab[BLK_SIZE2 * BLK_NUM];
static struct k_sem sync_sema;
static atomic_t slab_id;

/* thread entry simply invoke the APIs*/
static void tmslab_api(void *p1, void *p2, void *p3)
{
	void *block[BLK_NUM];
	struct k_mem_slab *slab = slabs[atomic_inc(&slab_id) % SLAB_NUM];
	int i = LOOP;

	while (i--) {
		memset(block, 0, sizeof(block));

		for (int i = 0; i < BLK_NUM; i++) {
			k_mem_slab_alloc(slab, &block[i], TIMEOUT);
		}
		for (int i = 0; i < BLK_NUM; i++) {
			if (block[i]) {
				k_mem_slab_free(slab, &block[i]);
				block[i] = NULL;
			}
		}
	}

	k_sem_give(&sync_sema);
}

/* test cases*/
void test_mslab_threadsafe(void)
{
	k_tid_t tid[THREAD_NUM];

	k_mem_slab_init(&mslab2, tslab, BLK_SIZE2, BLK_NUM);
	k_sem_init(&sync_sema, 0, THREAD_NUM);

	/* create multiple threads to invoke same memory slab APIs*/
	for (int i = 0; i < THREAD_NUM; i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
			tmslab_api, NULL, NULL, NULL,
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
