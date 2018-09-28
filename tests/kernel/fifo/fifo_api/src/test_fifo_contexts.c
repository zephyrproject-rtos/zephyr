/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fifo.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LIST_LEN 2
/**TESTPOINT: init via K_FIFO_DEFINE*/
K_FIFO_DEFINE(kfifo);

struct k_fifo fifo;
static fdata_t data[LIST_LEN];
static fdata_t data_l[LIST_LEN];
static fdata_t data_sl[LIST_LEN];

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem end_sema;

static void tfifo_put(struct k_fifo *pfifo)
{
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: fifo put*/
		k_fifo_put(pfifo, (void *)&data[i]);
	}

	/**TESTPOINT: fifo put list*/
	static fdata_t *head = &data_l[0], *tail = &data_l[LIST_LEN - 1];

	head->snode.next = (sys_snode_t *)tail;
	tail->snode.next = NULL;
	k_fifo_put_list(pfifo, (u32_t *)head, (u32_t *)tail);

	/**TESTPOINT: fifo put slist*/
	sys_slist_t slist;

	sys_slist_init(&slist);
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[0].snode));
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[1].snode));
	k_fifo_put_slist(pfifo, &slist);
}

static void tfifo_get(struct k_fifo *pfifo)
{
	void *rx_data;

	/*get fifo data from "fifo_put"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: fifo get*/
		rx_data = k_fifo_get(pfifo, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data[i], NULL);
	}
	/*get fifo data from "fifo_put_list"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_fifo_get(pfifo, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_l[i], NULL);
	}
	/*get fifo data from "fifo_put_slist"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_fifo_get(pfifo, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_sl[i], NULL);
	}
}

/*entry of contexts*/
static void tIsr_entry_put(void *p)
{
	tfifo_put((struct k_fifo *)p);
	zassert_false(k_fifo_is_empty((struct k_fifo *)p), NULL);
}

static void tIsr_entry_get(void *p)
{
	tfifo_get((struct k_fifo *)p);
	zassert_true(k_fifo_is_empty((struct k_fifo *)p), NULL);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tfifo_get((struct k_fifo *)p1);
	k_sem_give(&end_sema);
}

static void tfifo_thread_thread(struct k_fifo *pfifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-thread data passing via fifo*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_entry, pfifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, 0);
	tfifo_put(pfifo);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);
}

static void tfifo_thread_isr(struct k_fifo *pfifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: isr-thread data passing via fifo*/
	irq_offload(tIsr_entry_put, pfifo);
	tfifo_get(pfifo);
}

static void tfifo_isr_thread(struct k_fifo *pfifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr data passing via fifo*/
	tfifo_put(pfifo);
	irq_offload(tIsr_entry_get, pfifo);
}

static void tfifo_is_empty(void *p)
{
	struct k_fifo *pfifo = (struct k_fifo *)p;

	tfifo_put(&fifo);
	/**TESTPOINT: return false when data available*/
	zassert_false(k_fifo_is_empty(pfifo), NULL);

	tfifo_get(&fifo);
	/**TESTPOINT: return true with data unavailable*/
	zassert_true(k_fifo_is_empty(pfifo), NULL);
}

/**
 * @addtogroup kernel_fifo_tests
 * @{
 */

/**
 * @brief Test thread to thread data passing via fifo
 * @see k_fifo_init(), k_fifo_put(), k_fifo_get(), k_fifo_put_list()
 */
void test_fifo_thread2thread(void)
{
	/**TESTPOINT: init via k_fifo_init*/
	k_fifo_init(&fifo);
	tfifo_thread_thread(&fifo);

	/**TESTPOINT: test K_FIFO_DEFINEed fifo*/
	tfifo_thread_thread(&kfifo);
}

/**
 * @brief Test isr to thread data passing via fifo
 * @see k_fifo_init(), k_fifo_put(), k_fifo_get()
 */
void test_fifo_thread2isr(void)
{
	/**TESTPOINT: init via k_fifo_init*/
	k_fifo_init(&fifo);
	tfifo_thread_isr(&fifo);

	/**TESTPOINT: test K_FIFO_DEFINEed fifo*/
	tfifo_thread_isr(&kfifo);
}

/**
 * @brief Test thread to isr data passing via fifo
 * @see k_fifo_init(), k_fifo_put(), k_fifo_get()
 */
void test_fifo_isr2thread(void)
{
	/**TESTPOINT: test k_fifo_init fifo*/
	k_fifo_init(&fifo);
	tfifo_isr_thread(&fifo);

	/**TESTPOINT: test K_FIFO_DEFINE fifo*/
	tfifo_isr_thread(&kfifo);
}

/**
 * @brief Test empty fifo
 * @see k_fifo_init(), k_fifo_is_empty(), k_fifo_put(), k_fifo_get()
 */
void test_fifo_is_empty_thread(void)
{
	k_fifo_init(&fifo);
	/**TESTPOINT: k_fifo_is_empty after init*/
	zassert_true(k_fifo_is_empty(&fifo), NULL);

	/**TESTPONT: check fifo is empty from thread*/
	tfifo_is_empty(&fifo);
}

/**
 * @brief Test empty fifo in interrupt context
 * @see k_fifo_init(), fifo_is_empty(), k_fifo_put(), k_fifo_get()
 */
void test_fifo_is_empty_isr(void)
{
	k_fifo_init(&fifo);
	/**TESTPOINT: check fifo is empty from isr*/
	irq_offload(tfifo_is_empty, &fifo);
}
/**
 * @}
 */
