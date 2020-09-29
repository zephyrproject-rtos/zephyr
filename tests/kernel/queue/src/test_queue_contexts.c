/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_queue.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LIST_LEN 2
/**TESTPOINT: init via K_QUEUE_DEFINE*/
K_QUEUE_DEFINE(kqueue);

K_MEM_POOL_DEFINE(mem_pool_fail, 4, _MPOOL_MINBLK, 1, 4);
K_MEM_POOL_DEFINE(mem_pool_pass, 4, 64, 4, 4);

struct k_queue queue;
static qdata_t data[LIST_LEN];
static qdata_t data_p[LIST_LEN];
static qdata_t data_l[LIST_LEN];
static qdata_t data_sl[LIST_LEN];

static qdata_t *data_append;
static qdata_t *data_prepend;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static K_THREAD_STACK_DEFINE(tstack1, STACK_SIZE);
static struct k_thread tdata1;
static struct k_sem end_sema;

static void tqueue_append(struct k_queue *pqueue)
{
	k_queue_insert(pqueue, k_queue_peek_tail(pqueue),
		       (void *)&data[0]);

	for (int i = 1; i < LIST_LEN; i++) {
		/**TESTPOINT: queue append */
		k_queue_append(pqueue, (void *)&data[i]);
	}

	for (int i = LIST_LEN - 1; i >= 0; i--) {
		/**TESTPOINT: queue prepend */
		k_queue_prepend(pqueue, (void *)&data_p[i]);
	}

	/**TESTPOINT: queue append list*/
	static qdata_t *head = &data_l[0], *tail = &data_l[LIST_LEN - 1];

	head->snode.next = (sys_snode_t *)tail;
	tail->snode.next = NULL;
	k_queue_append_list(pqueue, (uint32_t *)head, (uint32_t *)tail);

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
static void tIsr_entry_append(const void *p)
{
	tqueue_append((struct k_queue *)p);
}

static void tIsr_entry_get(const void *p)
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
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	tqueue_append(pqueue);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);
}

static void tqueue_thread_isr(struct k_queue *pqueue)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr data passing via queue*/
	irq_offload(tIsr_entry_append, (const void *)pqueue);
	tqueue_get(pqueue);
}

static void tqueue_isr_thread(struct k_queue *pqueue)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: isr-thread data passing via queue*/
	tqueue_append(pqueue);
	irq_offload(tIsr_entry_get, (const void *)pqueue);
}

/*test cases*/
/**
 * @brief Verify data passing between threads using queue
 *
 * @details Static define and Dynamic define queues,
 * Then initialize them.
 * Create a new thread to wait for reading data.
 * Current thread will append item into queue.
 * Verify if rx_data is equal insert-data address.
 * Verify queue can be define at compile time.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_init(), k_queue_insert(), k_queue_append()
 * K_THREAD_STACK_DEFINE()
 */
void test_queue_thread2thread(void)
{
	/**TESTPOINT: init via k_queue_init*/
	k_queue_init(&queue);
	tqueue_thread_thread(&queue);

	/**TESTPOINT: test K_QUEUE_DEFINEed queue*/
	tqueue_thread_thread(&kqueue);
}

/**
 * @brief Verify data passing between thread and ISR
 *
 * @details Create a new ISR to insert data
 * And current thread is used for getting data
 * Verify if the rx_data is equal insert-data address.
 * If the received data address is the same as
 * the created array, prove that the queue data structures
 * are stored within the provided data items.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_init(), k_queue_insert(), k_queue_append()
 */
void test_queue_thread2isr(void)
{
	/**TESTPOINT: init via k_queue_init*/
	k_queue_init(&queue);
	tqueue_thread_isr(&queue);

	/**TESTPOINT: test K_QUEUE_DEFINEed queue*/
	tqueue_thread_isr(&kqueue);
}

/**
 * @brief Verify data passing between ISR and thread
 *
 * @details Create a new ISR and ready for getting data
 * And current thread is used for inserting data
 * Verify if the rx_data is equal insert-data address.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_init(), k_queue_insert(), k_queue_get(),
 * k_queue_append(), k_queue_remove()
 */
void test_queue_isr2thread(void)
{
	/**TESTPOINT: test k_queue_init queue*/
	k_queue_init(&queue);
	tqueue_isr_thread(&queue);

	/**TESTPOINT: test K_QUEUE_DEFINE queue*/
	tqueue_isr_thread(&kqueue);
}

static void tThread_get(void *p1, void *p2, void *p3)
{
	zassert_true(k_queue_get((struct k_queue *)p1, K_FOREVER) != NULL,
		     NULL);
	k_sem_give(&end_sema);
}

static void tqueue_get_2threads(struct k_queue *pqueue)
{
	k_sem_init(&end_sema, 0, 1);
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_get, pqueue, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_tid_t tid1 = k_thread_create(&tdata1, tstack1, STACK_SIZE,
				       tThread_get, pqueue, NULL, NULL,
				       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/* Wait threads to initialize */
	k_sleep(K_MSEC(10));

	k_queue_append(pqueue, (void *)&data[0]);
	k_queue_append(pqueue, (void *)&data[1]);
	/* Wait threads to finalize */
	k_sem_take(&end_sema, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);

	k_thread_abort(tid);
	k_thread_abort(tid1);
}

/**
 * @brief Verify k_queue_get()
 * @ingroup kernel_queue_tests
 * @see k_queue_init(), k_queue_get(),
 * k_queue_append(), k_queue_alloc_prepend()
 */
void test_queue_get_2threads(void)
{
	/**TESTPOINT: test k_queue_init queue*/
	k_queue_init(&queue);

	tqueue_get_2threads(&queue);
}

static void tqueue_alloc(struct k_queue *pqueue)
{
	k_thread_resource_pool_assign(k_current_get(), NULL);

	/* Alloc append without resource pool */
	k_queue_alloc_append(pqueue, (void *)&data_append);

	/* Insertion fails and alloc returns NOMEM */
	zassert_false(k_queue_remove(pqueue, &data_append), NULL);

	/* Assign resource pool of lower size */
	k_thread_resource_pool_assign(k_current_get(), &mem_pool_fail);

	/* Prepend to the queue, but fails because of
	 * insufficient memory
	 */
	k_queue_alloc_prepend(pqueue, (void *)&data_prepend);

	zassert_false(k_queue_remove(pqueue, &data_prepend), NULL);

	/* No element must be present in the queue, as all
	 * operations failed
	 */
	zassert_true(k_queue_is_empty(pqueue), NULL);

	/* Assign resource pool of sufficient size */
	k_thread_resource_pool_assign(k_current_get(),
				      &mem_pool_pass);

	zassert_false(k_queue_alloc_prepend(pqueue, (void *)&data_prepend),
		      NULL);

	/* Now queue shouldn't be empty */
	zassert_false(k_queue_is_empty(pqueue), NULL);

	zassert_true(k_queue_get(pqueue, K_FOREVER) != NULL,
		     NULL);
}

/**
 * @brief Test queue alloc append and prepend
 * @ingroup kernel_queue_tests
 * @see k_queue_alloc_append(), k_queue_alloc_prepend(),
 * k_thread_resource_pool_assign(), k_queue_is_empty(),
 * k_queue_get(), k_queue_remove()
 */
void test_queue_alloc(void)
{
	struct k_mem_block block;

	/* The mem_pool_fail pool is supposed to be too small to
	 * succeed any allocations, but in fact with the heap backend
	 * there's some base minimal memory in there that can be used.
	 * Make sure it's really truly full.
	 */
	while (k_mem_pool_alloc(&mem_pool_fail, &block, 1, K_NO_WAIT) == 0) {
	}

	k_queue_init(&queue);

	tqueue_alloc(&queue);
}


/* Does nothing but read items out of the queue and verify that they
 * are non-null.  Two such threads will be created.
 */
static void queue_poll_race_consume(void *p1, void *p2, void *p3)
{
	struct k_queue *q = p1;
	int *count = p2;

	while (true) {
		zassert_true(k_queue_get(q, K_FOREVER) != NULL, NULL);
		*count += 1;
	}
}

/* There was a historical race in the queue internals when CONFIG_POLL
 * was enabled -- it was possible to wake up a lower priority thread
 * with an insert but then steal it with a higher priority thread
 * before it got a chance to run, and the lower priority thread would
 * then return NULL before its timeout expired.
 */
void test_queue_poll_race(void)
{
	int prio = k_thread_priority_get(k_current_get());
	int mid_count = 0, low_count = 0;

	k_queue_init(&queue);

	k_thread_create(&tdata, tstack, STACK_SIZE,
			queue_poll_race_consume,
			&queue, &mid_count, NULL,
			prio + 1, 0, K_NO_WAIT);

	k_thread_create(&tdata1, tstack1, STACK_SIZE,
			queue_poll_race_consume,
			&queue, &low_count, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Let them initialize and block */
	k_sleep(K_TICKS(2));

	/* Insert two items.  This will wake up both threads, but the
	 * higher priority thread (tdata1) might (if CONFIG_POLL)
	 * consume both.  The lower priority thread should stay
	 * asleep.
	 */
	k_queue_append(&queue, &data[0]);
	k_queue_append(&queue, &data[1]);

	zassert_true(low_count == 0, NULL);
	zassert_true(mid_count == 0, NULL);

	k_sleep(K_TICKS(2));

	zassert_true(low_count + mid_count == 2, NULL);

	k_thread_abort(&tdata);
	k_thread_abort(&tdata1);
}

/**
 * @brief Verify that multiple queues can be defined
 * simultaneously
 *
 * @details define multiple queues to verify
 * they can work.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_init()
 */
#define QUEUE_NUM 10
void test_multiple_queues(void)
{
	/*define multiple queues*/
	struct k_queue queues[QUEUE_NUM];

	for (int i = 0; i < QUEUE_NUM; i++) {
		k_queue_init(&queues[i]);

		/*Indicating that they are working*/
		tqueue_append(&queues[i]);
		tqueue_get(&queues[i]);
	}
}
