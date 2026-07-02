/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fifo.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 2
/**TESTPOINT: init via K_FIFO_DEFINE*/
K_FIFO_DEFINE(kfifo);

struct k_fifo fifo;
static fdata_t data[LIST_LEN];
static fdata_t data_l[LIST_LEN];
static fdata_t data_sl[LIST_LEN];

static K_THREAD_STACK_DEFINE(tstack_contexts, STACK_SIZE);
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
	k_fifo_put_list(pfifo, (uint32_t *)head, (uint32_t *)tail);

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
		zassert_equal(rx_data, (void *)&data[i]);
	}
	/*get fifo data from "fifo_put_list"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_fifo_get(pfifo, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_l[i]);
	}
	/*get fifo data from "fifo_put_slist"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_fifo_get(pfifo, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_sl[i]);
	}
}

/*entry of contexts*/
static void tIsr_entry_put(const void *p)
{
	tfifo_put((struct k_fifo *)p);
	zassert_false(k_fifo_is_empty((struct k_fifo *)p));
}

static void tIsr_entry_get(const void *p)
{
	tfifo_get((struct k_fifo *)p);
	zassert_true(k_fifo_is_empty((struct k_fifo *)p));
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
	k_tid_t tid = k_thread_create(&tdata, tstack_contexts, STACK_SIZE,
				      tThread_entry, pfifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	tfifo_put(pfifo);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);
}

static void tfifo_thread_isr(struct k_fifo *pfifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: isr-thread data passing via fifo*/
	irq_offload(tIsr_entry_put, (const void *)pfifo);
	tfifo_get(pfifo);
}

static void tfifo_isr_thread(struct k_fifo *pfifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr data passing via fifo*/
	tfifo_put(pfifo);
	irq_offload(tIsr_entry_get, (const void *)pfifo);
}

static void tfifo_is_empty(void *p)
{
	struct k_fifo *pfifo = (struct k_fifo *)p;

	tfifo_put(&fifo);
	/**TESTPOINT: return false when data available*/
	zassert_false(k_fifo_is_empty(pfifo));

	tfifo_get(&fifo);
	/**TESTPOINT: return true with data unavailable*/
	zassert_true(k_fifo_is_empty(pfifo));
}

/**
 * @addtogroup tests_kernel_fifo
 * @{
 */

/**
 * @brief Verify FIFO data passing between two threads.
 *
 * @details
 * A consumer thread blocks on k_fifo_get() while the main thread enqueues
 * items via the single, list and slist put APIs. The consumer must receive
 * every item in FIFO order. The scenario is run against both a k_fifo_init()ed
 * and a K_FIFO_DEFINE()d FIFO to confirm both initialization paths behave the
 * same.
 *
 * Test steps:
 * - Create a preemptible consumer thread that drains the FIFO.
 * - From the main thread, put items using k_fifo_put(), k_fifo_put_list() and
 *   k_fifo_put_slist().
 * - Synchronize on a semaphore once the consumer has read all items.
 * - Repeat for a statically defined FIFO.
 *
 * Expected result:
 * - The consumer dequeues every item in the order it was enqueued.
 *
 * @see k_fifo_put()
 * @see k_fifo_put_list()
 * @see k_fifo_put_slist()
 * @see k_fifo_get()
 */
ZTEST(fifo_api_1cpu, test_fifo_thread2thread)
{
	/**TESTPOINT: init via k_fifo_init*/
	k_fifo_init(&fifo);
	tfifo_thread_thread(&fifo);

	/**TESTPOINT: test K_FIFO_DEFINEed fifo*/
	tfifo_thread_thread(&kfifo);
}

/**
 * @brief Verify FIFO data passing from an ISR to a thread.
 *
 * @details
 * Items are enqueued from interrupt context (via irq_offload()) and dequeued in
 * thread context, confirming k_fifo_put() is ISR-safe and the thread receives
 * every item in FIFO order. Run against both an init()ed and a DEFINE()d FIFO.
 *
 * Test steps:
 * - From an ISR, put items into the FIFO and assert it is not empty.
 * - In thread context, get all items and verify their order.
 * - Repeat for a statically defined FIFO.
 *
 * Expected result:
 * - The thread dequeues every ISR-enqueued item in order.
 *
 * @see k_fifo_put()
 * @see k_fifo_get()
 */
ZTEST(fifo_api, test_fifo_thread2isr)
{
	/**TESTPOINT: init via k_fifo_init*/
	k_fifo_init(&fifo);
	tfifo_thread_isr(&fifo);

	/**TESTPOINT: test K_FIFO_DEFINEed fifo*/
	tfifo_thread_isr(&kfifo);
}

/**
 * @brief Verify FIFO data passing from a thread to an ISR.
 *
 * @details
 * Items are enqueued in thread context and dequeued from interrupt context (via
 * irq_offload()), confirming k_fifo_get() is ISR-safe and the ISR receives every
 * item in FIFO order. Run against both an init()ed and a DEFINE()d FIFO.
 *
 * Test steps:
 * - In thread context, put items into the FIFO.
 * - From an ISR, get all items, verify their order, and assert the FIFO is empty.
 * - Repeat for a statically defined FIFO.
 *
 * Expected result:
 * - The ISR dequeues every thread-enqueued item in order.
 *
 * @see k_fifo_put()
 * @see k_fifo_get()
 */
ZTEST(fifo_api, test_fifo_isr2thread)
{
	/**TESTPOINT: test k_fifo_init fifo*/
	k_fifo_init(&fifo);
	tfifo_isr_thread(&fifo);

	/**TESTPOINT: test K_FIFO_DEFINE fifo*/
	tfifo_isr_thread(&kfifo);
}

/**
 * @brief Verify k_fifo_is_empty() tracks FIFO contents in thread context.
 *
 * @details
 * k_fifo_is_empty() must report true for a freshly initialized FIFO, false once
 * items are enqueued, and true again after they are all dequeued. All operations
 * run in thread context.
 *
 * Test steps:
 * - Initialize a FIFO and assert it is empty.
 * - Put items and assert it is not empty.
 * - Get all items and assert it is empty again.
 *
 * Expected result:
 * - k_fifo_is_empty() reflects the presence or absence of queued data.
 *
 * @see k_fifo_is_empty()
 */
ZTEST(fifo_api, test_fifo_is_empty_thread)
{
	k_fifo_init(&fifo);
	/**TESTPOINT: k_fifo_is_empty after init*/
	zassert_true(k_fifo_is_empty(&fifo));

	/**TESTPONT: check fifo is empty from thread*/
	tfifo_is_empty(&fifo);
}

/**
 * @brief Verify k_fifo_is_empty() tracks FIFO contents in ISR context.
 *
 * @details
 * Same emptiness contract as the thread-context case, but the put/get/is_empty
 * sequence is executed from interrupt context via irq_offload() to confirm
 * k_fifo_is_empty() is ISR-safe.
 *
 * Test steps:
 * - Initialize a FIFO and assert it is empty from thread context.
 * - From an ISR, put items (not empty) then get them all (empty again).
 *
 * Expected result:
 * - k_fifo_is_empty() reports the correct state when called from an ISR.
 *
 * @see k_fifo_is_empty()
 */
ZTEST(fifo_api, test_fifo_is_empty_isr)
{
	k_fifo_init(&fifo);
	/**TESTPOINT: check fifo is empty from isr*/
	irq_offload((irq_offload_routine_t)tfifo_is_empty, &fifo);
}

/**
 * @brief Test peeking at the front and back items of a FIFO
 *
 * @details Enqueue two data items, then use k_fifo_peek_head() and
 * k_fifo_peek_tail() to inspect the items at the front and back of the FIFO and
 * verify the returned pointers match the first and last enqueued items without
 * removing them (a subsequent get still returns both items in order). Peeking an
 * empty FIFO returns NULL.
 *
 * @see k_fifo_peek_head(), k_fifo_peek_tail()
 */
ZTEST(fifo_api, test_fifo_peek)
{
	k_fifo_init(&fifo);

	k_fifo_put(&fifo, (void *)&data[0]);
	k_fifo_put(&fifo, (void *)&data[1]);

	/**TESTPOINT: peek front and back without removing*/
	zassert_equal(k_fifo_peek_head(&fifo), (void *)&data[0]);
	zassert_equal(k_fifo_peek_tail(&fifo), (void *)&data[1]);

	/* Peeking does not dequeue: both items are still retrievable in order. */
	zassert_equal(k_fifo_get(&fifo, K_NO_WAIT), (void *)&data[0]);
	zassert_equal(k_fifo_get(&fifo, K_NO_WAIT), (void *)&data[1]);

	/**TESTPOINT: peek of an empty fifo returns NULL*/
	zassert_is_null(k_fifo_peek_head(&fifo));
	zassert_is_null(k_fifo_peek_tail(&fifo));
}

K_HEAP_DEFINE(fifo_alloc_pool, 256);

/**
 * @brief Test enqueuing a data item to a FIFO with implicit memory allocation
 *
 * @details Use k_fifo_alloc_put() to enqueue a data item that does not reserve
 * space for the bookkeeping word, so the kernel allocates the container from the
 * calling thread's resource pool. Verify the call succeeds and that the same
 * data pointer is returned by a subsequent get.
 *
 * @see k_fifo_alloc_put()
 */
ZTEST(fifo_api, test_fifo_alloc_put)
{
	static uint32_t payload = 0xDEADBEEF;

	k_fifo_init(&fifo);
	k_thread_heap_assign(k_current_get(), &fifo_alloc_pool);

	/**TESTPOINT: allocate the queue container from the thread resource pool*/
	zassert_equal(k_fifo_alloc_put(&fifo, &payload), 0);

	zassert_equal(k_fifo_get(&fifo, K_NO_WAIT), (void *)&payload);
}
/**
 * @}
 */
