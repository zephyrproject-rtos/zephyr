/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief Use fifo API's in different scenarios
 *
 * This module tests following three basic scenarios:
 *
 * Scenario #1
 * Test Thread enters items into a fifo, starts the Child Thread
 * and waits for a semaphore. Child thread extracts all items from
 * the fifo and enters some items back into the fifo.  Child Thread
 * gives the semaphore for Test Thread to continue.  Once the control
 * is returned back to Test Thread, it extracts all items from the fifo.
 *
 * Scenario #2
 * Test Thread enters an item into fifo2, starts a Child Thread and
 * extract an item from fifo1 once the item is there.  The Child Thread
 * will extract an item from fifo2 once the item is there and enter
 * an item to fifo1.  The flow of control goes from Test Thread to
 * Child Thread and so forth.
 *
 * Scenario #3
 * Tests the ISR interfaces. Test thread puts items into fifo2 and gives
 * control to the Child thread. Child thread gets items from fifo2 and then
 * puts items into fifo1. Child thread gives back control to the Test thread
 * and Test thread gets the items from fifo1.
 * All the Push and Pop operations happen in ISR Context.
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>

#define STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN	4

struct fdata_t {
	sys_snode_t snode;
	uint32_t data;
};

static K_FIFO_DEFINE(fifo1);
static K_FIFO_DEFINE(fifo2);

/* Data to put into FIFO */
static struct fdata_t data1[LIST_LEN];
static struct fdata_t data2[LIST_LEN];
static struct fdata_t data_isr[LIST_LEN];

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem end_sema;

/*entry of contexts*/
static void tIsr_entry_put(const void *p)
{
	uint32_t i;

	/* Put items into fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		k_fifo_put((struct k_fifo *)p, (void *)&data_isr[i]);
	}
	zassert_false(k_fifo_is_empty((struct k_fifo *)p));
}

static void tIsr_entry_get(const void *p)
{
	void *rx_data;
	uint32_t i;

	/* Get items from fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		rx_data = k_fifo_get((struct k_fifo *)p, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_isr[i]);
	}
	zassert_true(k_fifo_is_empty((struct k_fifo *)p));
}

static void thread_entry_fn_single(void *p1, void *p2, void *p3)
{
	void *rx_data;
	uint32_t i;

	/* Get items from fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		rx_data = k_fifo_get((struct k_fifo *)p1, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data1[i]);
	}

	/* Put items into fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		k_fifo_put((struct k_fifo *)p1, (void *)&data2[i]);
	}

	/* Give control back to Test thread */
	k_sem_give(&end_sema);
}

static void thread_entry_fn_dual(void *p1, void *p2, void *p3)
{
	void *rx_data;
	uint32_t i;

	for (i = 0U; i < LIST_LEN; i++) {
		/* Get items from fifo2 */
		rx_data = k_fifo_get((struct k_fifo *)p2, K_FOREVER);
		zassert_equal(rx_data, (void *)&data2[i]);

		/* Put items into fifo1 */
		k_fifo_put((struct k_fifo *)p1, (void *)&data1[i]);
	}
}

static void thread_entry_fn_isr(void *p1, void *p2, void *p3)
{
	/* Get items from fifo2 */
	irq_offload(tIsr_entry_get, (const void *)p2);

	/* Put items into fifo1 */
	irq_offload(tIsr_entry_put, (const void *)p1);

	/* Give control back to Test thread */
	k_sem_give(&end_sema);
}

/**
 * @addtogroup tests_kernel_fifo
 * @{
 */

/**
 * @brief Verify producer/consumer exchange over a single FIFO.
 *
 * @details
 * Exercises a hand-off pattern on one FIFO: the main thread fills it, a child
 * thread drains it and refills it with a second data set, and the main thread
 * drains that. Confirms items survive a full round trip through both threads in
 * the correct order.
 *
 * Test steps:
 * - Main thread puts the first data set, then starts the child.
 * - Child gets all items, puts the second data set, signals via a semaphore.
 * - Main thread gets all items and verifies they match the second data set.
 *
 * Expected result:
 * - The main thread reads back exactly the items the child enqueued, in order.
 *
 * @see k_fifo_put()
 * @see k_fifo_get()
 */
ZTEST(fifo_usage, test_single_fifo_play)
{
	void *rx_data;
	uint32_t i;

	/* Init kernel objects */
	k_sem_init(&end_sema, 0, 1);

	/* Put items into fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		k_fifo_put(&fifo1, (void *)&data1[i]);
	}

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				thread_entry_fn_single, &fifo1, NULL, NULL,
				K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);

	/* Let the child thread run */
	k_sem_take(&end_sema, K_FOREVER);

	/* Get items from fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		rx_data = k_fifo_get(&fifo1, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data2[i]);
	}

	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
}

/**
 * @brief Verify ping-pong synchronization across two FIFOs.
 *
 * @details
 * Two FIFOs are used as a request/response channel: the main thread puts to
 * fifo2 and blocks getting from fifo1, while the child blocks getting from fifo2
 * and puts to fifo1. Control alternates between the threads each iteration,
 * confirming blocking gets correctly serialize the exchange.
 *
 * Test steps:
 * - Start the child, which loops getting from fifo2 and putting to fifo1.
 * - Main thread loops putting to fifo2 and getting from fifo1.
 * - Verify each item received on fifo1 matches the expected sequence.
 *
 * Expected result:
 * - The threads alternate correctly and every item is received in order.
 *
 * @see k_fifo_put()
 * @see k_fifo_get()
 */
ZTEST(fifo_usage, test_dual_fifo_play)
{
	void *rx_data;
	uint32_t i;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				thread_entry_fn_dual, &fifo1, &fifo2, NULL,
				K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);

	for (i = 0U; i < LIST_LEN; i++) {
		/* Put item into fifo */
		k_fifo_put(&fifo2, (void *)&data2[i]);

		/* Get item from fifo */
		rx_data = k_fifo_get(&fifo1, K_FOREVER);
		zassert_equal(rx_data, (void *)&data1[i]);
	}

	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);

}

/**
 * @brief Verify producer/consumer exchange with all puts/gets in ISR context.
 *
 * @details
 * Repeats the two-FIFO hand-off pattern but performs every k_fifo_put() and
 * k_fifo_get() from interrupt context via irq_offload(), confirming the FIFO
 * operations are ISR-safe end to end when both producer and consumer run in an
 * ISR.
 *
 * Test steps:
 * - Main thread (from an ISR) puts items into fifo2, then starts the child.
 * - Child (from ISRs) gets from fifo2 and puts into fifo1, then signals.
 * - Main thread (from an ISR) gets the items from fifo1.
 *
 * Expected result:
 * - All ISR-context put/get operations succeed and items pass in order.
 *
 * @see k_fifo_put()
 * @see k_fifo_get()
 */
ZTEST(fifo_usage, test_isr_fifo_play)
{
	/* Init kernel objects */
	k_sem_init(&end_sema, 0, 1);

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				thread_entry_fn_isr, &fifo1, &fifo2, NULL,
				K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);


	/* Put item into fifo */
	irq_offload(tIsr_entry_put, (const void *)&fifo2);

	/* Let the child thread run */
	k_sem_take(&end_sema, K_FOREVER);

	/* Get item from fifo */
	irq_offload(tIsr_entry_get, (const void *)&fifo1);

	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
}

/**
 * @}
 */

ZTEST_SUITE(fifo_usage, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
