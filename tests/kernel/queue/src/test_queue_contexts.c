/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_queue_api
 * @{
 * @defgroup t_queue_api_basic test_queue_api_basic
 * @brief TestPurpose: verify zephyr queue apis under different context
 * - API coverage
 *   -# k_queue_init K_QUEUE_DEFINE
 *   -# k_queue_append k_queue_prepend k_queue_append_list k_queue_merge_slist
 *   -# k_queue_get
 * @}
 */

#include "test_queue.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LIST_LEN 2
/**TESTPOINT: init via K_QUEUE_DEFINE*/
K_QUEUE_DEFINE(kqueue);

struct k_queue queue;
static qdata_t data[LIST_LEN];
static qdata_t data_p[LIST_LEN];
static qdata_t data_l[LIST_LEN];
static qdata_t data_sl[LIST_LEN];

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem end_sema;

static void tqueue_append(struct k_queue *pqueue)
{
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: queue append */
		k_queue_append(pqueue, (void *)&data[i]);
	}

	for (int i = LIST_LEN - 1; i >= 0; i--) {
		/**TESTPOINT: queue prepend */
		k_queue_prepend(pqueue, (void *)&data_p[i]);
	}

	/**TESTPOINT: queue append list*/
	static qdata_t *head = &data_l[0], *tail = &data_l[LIST_LEN-1];

	head->snode.next = (sys_snode_t *)tail;
	tail->snode.next = NULL;
	k_queue_append_list(pqueue, (u32_t *)head, (u32_t *)tail);

	/**TESTPOINT: queue merge slist*/
	sys_slist_t slist;

	sys_slist_init(&slist);
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[0].snode));
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[1].snode));
	k_queue_merge_slist(pqueue, &slist);
}

static void tqueue_get(struct k_queue *pqueue)
{
	void *rx_data;

	/*get queue data from "queue_prepend"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: queue get*/
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_p[i], NULL);
	}
	/*get queue data from "queue_append"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: queue get*/
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data[i], NULL);
	}
	/*get queue data from "queue_append_list"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_l[i], NULL);
	}
	/*get queue data from "queue_merge_slist"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_sl[i], NULL);
	}
}

/*entry of contexts*/
static void tIsr_entry_append(void *p)
{
	tqueue_append((struct k_queue *)p);
}

static void tIsr_entry_get(void *p)
{
	tqueue_get((struct k_queue *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tqueue_get((struct k_queue *)p1);
	k_sem_give(&end_sema);
}

static void tqueue_thread_thread(struct k_queue *pqueue)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-thread data passing via queue*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
		tThread_entry, pqueue, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, 0);
	tqueue_append(pqueue);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);
}

static void tqueue_thread_isr(struct k_queue *pqueue)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr data passing via queue*/
	irq_offload(tIsr_entry_append, pqueue);
	tqueue_get(pqueue);
}

static void tqueue_isr_thread(struct k_queue *pqueue)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: isr-thread data passing via queue*/
	tqueue_append(pqueue);
	irq_offload(tIsr_entry_get, pqueue);
}

/*test cases*/
void test_queue_thread2thread(void)
{
	/**TESTPOINT: init via k_queue_init*/
	k_queue_init(&queue);
	tqueue_thread_thread(&queue);

	/**TESTPOINT: test K_QUEUE_DEFINEed queue*/
	tqueue_thread_thread(&kqueue);
}

void test_queue_thread2isr(void)
{
	/**TESTPOINT: init via k_queue_init*/
	k_queue_init(&queue);
	tqueue_thread_isr(&queue);

	/**TESTPOINT: test K_QUEUE_DEFINEed queue*/
	tqueue_thread_isr(&kqueue);
}

void test_queue_isr2thread(void)
{
	/**TESTPOINT: test k_queue_init queue*/
	k_queue_init(&queue);
	tqueue_isr_thread(&queue);

	/**TESTPOINT: test K_QUEUE_DEFINE queue*/
	tqueue_isr_thread(&kqueue);
}
