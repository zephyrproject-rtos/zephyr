/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_queue.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 4
#define LOOPS 32

static qdata_t data[LIST_LEN];
static qdata_t data_p[LIST_LEN];
static qdata_t data_r[LIST_LEN];
static struct k_queue queue;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem end_sema;

static void tqueue_append(struct k_queue *pqueue)
{
	/**TESTPOINT: queue append*/
	for (int i = 0; i < LIST_LEN; i++) {
		k_queue_append(pqueue, (void *)&data[i]);
	}

	/**TESTPOINT: queue prepend*/
	for (int i = LIST_LEN - 1; i >= 0; i--) {
		k_queue_prepend(pqueue, (void *)&data_p[i]);
	}

	/**TESTPOINT: queue find and remove*/
	for (int i = LIST_LEN - 1; i >= 0; i--) {
		k_queue_prepend(pqueue, (void *)&data_r[i]);
	}
}

static void tqueue_get(struct k_queue *pqueue)
{
	void *rx_data;

	/*get queue data from "queue_prepend"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: queue get*/
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_p[i]);
	}

	/*get queue data from "queue_append"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: queue get*/
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data[i]);
	}
}

static void tqueue_find_and_remove(struct k_queue *pqueue)
{
	/*remove queue data from "queue_find_and_remove"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: queue find and remove*/
		zassert_true(k_queue_remove(pqueue, &data_r[i]));
	}
}

/*entry of contexts*/
static void tIsr_entry(const void *p)
{
	tqueue_find_and_remove((struct k_queue *)p);
	tqueue_get((struct k_queue *)p);
	tqueue_append((struct k_queue *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tqueue_find_and_remove((struct k_queue *)p1);
	tqueue_get((struct k_queue *)p1);
	k_sem_give(&end_sema);
	tqueue_append((struct k_queue *)p1);
	k_sem_give(&end_sema);
}

/* queue read write job */
static void tqueue_read_write(struct k_queue *pqueue)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr-thread data passing via queue*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_entry, pqueue, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	tqueue_append(pqueue);
	irq_offload(tIsr_entry, (const void *)pqueue);
	k_sem_take(&end_sema, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);

	tqueue_find_and_remove(pqueue);
	tqueue_get(pqueue);
	k_thread_abort(tid);
}

/*test cases*/
/**
 * @brief Test queue operations in loop
 * @ingroup kernel_queue_tests
 * @see k_queue_append(), k_queue_get(),
 * k_queue_init(), k_queue_remove()
 */
ZTEST(queue_api_1cpu, test_queue_loop)
{
	k_queue_init(&queue);
	for (int i = 0; i < LOOPS; i++) {
		TC_PRINT("* Pass data by queue in loop %d\n", i);
		tqueue_read_write(&queue);
	}
}
