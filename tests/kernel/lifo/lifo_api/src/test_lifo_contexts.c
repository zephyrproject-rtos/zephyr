/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_lifo.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 2
/**TESTPOINT: init via K_LIFO_DEFINE*/
K_LIFO_DEFINE(klifo);

struct k_lifo lifo;
static ldata_t data[LIST_LEN];

static K_THREAD_STACK_DEFINE(tstack_contexts, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem end_sema;

static void tlifo_put(struct k_lifo *plifo)
{
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: lifo put*/
		k_lifo_put(plifo, (void *)&data[i]);
	}
}

static void tlifo_get(struct k_lifo *plifo)
{
	void *rx_data;

	/*get lifo data*/
	for (int i = LIST_LEN-1; i >= 0; i--) {
		/**TESTPOINT: lifo get*/
		rx_data = k_lifo_get(plifo, K_FOREVER);
		zassert_equal(rx_data, (void *)&data[i]);
	}
}

/*entry of contexts*/
static void tIsr_entry_put(const void *p)
{
	tlifo_put((struct k_lifo *)p);
}

static void tIsr_entry_get(const void *p)
{
	tlifo_get((struct k_lifo *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tlifo_get((struct k_lifo *)p1);
	k_sem_give(&end_sema);
}

static void tlifo_thread_thread(struct k_lifo *plifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-thread data passing via lifo*/
	k_tid_t tid = k_thread_create(&tdata, tstack_contexts, STACK_SIZE,
		tThread_entry, plifo, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	tlifo_put(plifo);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);
}

static void tlifo_thread_isr(struct k_lifo *plifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr data passing via lifo*/
	irq_offload(tIsr_entry_put, (const void *)plifo);
	tlifo_get(plifo);
}

static void tlifo_isr_thread(struct k_lifo *plifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: isr-thread data passing via lifo*/
	tlifo_put(plifo);
	irq_offload(tIsr_entry_get, (const void *)plifo);
}

/**
 * @addtogroup tests_kernel_lifo
 * @{
 */

/**
 * @brief Verify LIFO data passing between two threads in LIFO order.
 *
 * @details
 * A consumer thread blocks on k_lifo_get() while the main thread enqueues items,
 * and must receive them newest-first (LIFO order). Run against both a
 * k_lifo_init()ed and a K_LIFO_DEFINE()d LIFO to confirm both initialization
 * paths behave the same.
 *
 * Test steps:
 * - Start a preemptible consumer thread that drains the LIFO.
 * - From the main thread, put items onto the LIFO.
 * - Synchronize on a semaphore once the consumer has read all items.
 * - Repeat for a statically defined LIFO.
 *
 * Expected result:
 * - The consumer dequeues items in reverse insertion order.
 *
 * @see k_lifo_put()
 * @see k_lifo_get()
 */
ZTEST(lifo_contexts_1cpu, test_lifo_thread2thread)
{
	/**TESTPOINT: init via k_lifo_init*/
	k_lifo_init(&lifo);
	tlifo_thread_thread(&lifo);

	/**TESTPOINT: test K_LIFO_DEFINEed lifo*/
	tlifo_thread_thread(&klifo);
}

/**
 * @brief Verify LIFO data passing from an ISR to a thread.
 *
 * @details
 * Items are enqueued from interrupt context (via irq_offload()) and dequeued in
 * thread context, confirming k_lifo_put() is ISR-safe and the thread receives
 * the items in LIFO order. Run against both an init()ed and a DEFINE()d LIFO.
 *
 * Test steps:
 * - From an ISR, put items onto the LIFO.
 * - In thread context, get the items and verify reverse insertion order.
 * - Repeat for a statically defined LIFO.
 *
 * Expected result:
 * - The thread dequeues every ISR-enqueued item newest-first.
 *
 * @see k_lifo_put()
 * @see k_lifo_get()
 */
ZTEST(lifo_contexts, test_lifo_thread2isr)
{
	/**TESTPOINT: init via k_lifo_init*/
	k_lifo_init(&lifo);
	tlifo_thread_isr(&lifo);

	/**TESTPOINT: test K_LIFO_DEFINEed lifo*/
	tlifo_thread_isr(&klifo);
}

/**
 * @brief Verify LIFO data passing from a thread to an ISR.
 *
 * @details
 * Items are enqueued in thread context and dequeued from interrupt context (via
 * irq_offload()), confirming k_lifo_get() is ISR-safe and the ISR receives the
 * items in LIFO order. Run against both an init()ed and a DEFINE()d LIFO.
 *
 * Test steps:
 * - In thread context, put items onto the LIFO.
 * - From an ISR, get the items and verify reverse insertion order.
 * - Repeat for a statically defined LIFO.
 *
 * Expected result:
 * - The ISR dequeues every thread-enqueued item newest-first.
 *
 * @see k_lifo_put()
 * @see k_lifo_get()
 */
ZTEST(lifo_contexts, test_lifo_isr2thread)
{
	/**TESTPOINT: test k_lifo_init lifo*/
	k_lifo_init(&lifo);
	tlifo_isr_thread(&lifo);

	/**TESTPOINT: test K_LIFO_DEFINE lifo*/
	tlifo_isr_thread(&klifo);
}

K_HEAP_DEFINE(lifo_alloc_pool, 256);

/**
 * @brief Test enqueuing a data item to a LIFO with implicit memory allocation
 *
 * @details Use k_lifo_alloc_put() to enqueue a data item that does not reserve
 * space for the bookkeeping word, so the kernel allocates the container from the
 * calling thread's resource pool. Verify the call succeeds and that the same
 * data pointer is returned by a subsequent get.
 *
 * @see k_lifo_alloc_put()
 */
ZTEST(lifo_contexts, test_lifo_alloc_put)
{
	static uint32_t payload = 0xDEADBEEF;

	k_lifo_init(&lifo);
	k_thread_heap_assign(k_current_get(), &lifo_alloc_pool);

	/**TESTPOINT: allocate the queue container from the thread resource pool*/
	zassert_equal(k_lifo_alloc_put(&lifo, &payload), 0);

	zassert_equal(k_lifo_get(&lifo, K_NO_WAIT), (void *)&payload);
}

/**
 * @}
 */

ZTEST_SUITE(lifo_contexts_1cpu, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

ZTEST_SUITE(lifo_contexts, NULL, NULL, NULL, NULL, NULL);
