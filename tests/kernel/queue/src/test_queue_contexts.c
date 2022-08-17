/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_queue.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 2
/**TESTPOINT: init via K_QUEUE_DEFINE*/
K_QUEUE_DEFINE(kqueue);

K_HEAP_DEFINE(mem_pool_fail, 8 + 128);
K_HEAP_DEFINE(mem_pool_pass, 64 * 4 + 128);

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
static K_THREAD_STACK_DEFINE(tstack2, STACK_SIZE);
static struct k_thread tdata2;
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
		zassert_equal(rx_data, (void *)&data_p[i]);
	}
	/*get queue data from "queue_append"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: queue get*/
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data[i]);
	}
	/*get queue data from "queue_append_list"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_l[i]);
	}
	/*get queue data from "queue_merge_slist"*/
	for (int i = 0; i < LIST_LEN; i++) {
		rx_data = k_queue_get(pqueue, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data_sl[i]);
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
ZTEST(queue_api_1cpu, test_queue_thread2thread)
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
ZTEST(queue_api, test_queue_thread2isr)
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
ZTEST(queue_api, test_queue_isr2thread)
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
ZTEST(queue_api_1cpu, test_queue_get_2threads)
{
	/**TESTPOINT: test k_queue_init queue*/
	k_queue_init(&queue);

	tqueue_get_2threads(&queue);
}

static void tqueue_alloc(struct k_queue *pqueue)
{
	k_thread_heap_assign(k_current_get(), NULL);

	/* Alloc append without resource pool */
	k_queue_alloc_append(pqueue, (void *)&data_append);

	/* Insertion fails and alloc returns NOMEM */
	zassert_false(k_queue_remove(pqueue, &data_append));

	/* Assign resource pool of lower size */
	k_thread_heap_assign(k_current_get(), &mem_pool_fail);

	/* Prepend to the queue, but fails because of
	 * insufficient memory
	 */
	k_queue_alloc_prepend(pqueue, (void *)&data_prepend);

	zassert_false(k_queue_remove(pqueue, &data_prepend));

	/* No element must be present in the queue, as all
	 * operations failed
	 */
	zassert_true(k_queue_is_empty(pqueue));

	/* Assign resource pool of sufficient size */
	k_thread_heap_assign(k_current_get(), &mem_pool_pass);

	zassert_false(k_queue_alloc_prepend(pqueue, (void *)&data_prepend),
		      NULL);

	/* Now queue shouldn't be empty */
	zassert_false(k_queue_is_empty(pqueue));

	zassert_true(k_queue_get(pqueue, K_FOREVER) != NULL,
		     NULL);
}

/**
 * @brief Test queue alloc append and prepend
 * @ingroup kernel_queue_tests
 * @see k_queue_alloc_append(), k_queue_alloc_prepend(),
 * z_thread_heap_assign(), k_queue_is_empty(),
 * k_queue_get(), k_queue_remove()
 */
ZTEST(queue_api, test_queue_alloc)
{
	/* The mem_pool_fail pool is supposed to be too small to
	 * succeed any allocations, but in fact with the heap backend
	 * there's some base minimal memory in there that can be used.
	 * Make sure it's really truly full.
	 */
	while (k_heap_alloc(&mem_pool_fail, 1, K_NO_WAIT) != NULL) {
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
		zassert_true(k_queue_get(q, K_FOREVER) != NULL);
		*count += 1;
	}
}

/* There was a historical race in the queue internals when CONFIG_POLL
 * was enabled -- it was possible to wake up a lower priority thread
 * with an insert but then steal it with a higher priority thread
 * before it got a chance to run, and the lower priority thread would
 * then return NULL before its timeout expired.
 */
ZTEST(queue_api_1cpu, test_queue_poll_race)
{
	int prio = k_thread_priority_get(k_current_get());
	static volatile int mid_count, low_count;

	k_queue_init(&queue);

	k_thread_create(&tdata, tstack, STACK_SIZE,
			queue_poll_race_consume,
			&queue, (void *)&mid_count, NULL,
			prio + 1, 0, K_NO_WAIT);

	k_thread_create(&tdata1, tstack1, STACK_SIZE,
			queue_poll_race_consume,
			&queue, (void *)&low_count, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Let them initialize and block */
	k_sleep(K_MSEC(10));

	/* Insert two items.  This will wake up both threads, but the
	 * higher priority thread (tdata1) might (if CONFIG_POLL)
	 * consume both.  The lower priority thread should stay
	 * asleep.
	 */
	k_queue_append(&queue, &data[0]);
	k_queue_append(&queue, &data[1]);

	zassert_true(low_count == 0);
	zassert_true(mid_count == 0);

	k_sleep(K_MSEC(10));

	zassert_true(low_count + mid_count == 2);

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
ZTEST(queue_api, test_multiple_queues)
{
	/*define multiple queues*/
	static struct k_queue queues[QUEUE_NUM];

	for (int i = 0; i < QUEUE_NUM; i++) {
		k_queue_init(&queues[i]);

		/*Indicating that they are working*/
		tqueue_append(&queues[i]);
		tqueue_get(&queues[i]);
	}
}

void user_access_queue_private_data(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	/* try to access to private kernel data, will happen kernel oops */
	k_queue_is_empty(&queue);
}

/**
 * @brief Test access kernel object with private data using system call
 *
 * @details
 * - When defining system calls, it is very important to ensure that
 *   access to the API’s private data is done exclusively through system call
 *   interfaces. Private kernel data should never be made available to user mode
 *   threads directly. For example, the k_queue APIs were intentionally not made
 *   available as they store bookkeeping information about the queue directly
 *   in the queue buffers which are visible from user mode.
 * - Current test makes user thread try to access private kernel data within
 *   their associated data structures. Kernel will track that system call
 *   access to these object with the kernel object permission system.
 *   Current user thread doesn't have permission on it, trying to access
 *   &pqueue kernel object will happen kernel oops, because current user
 *   thread doesn't have permission on k_queue object with private kernel data.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(queue_api, test_access_kernel_obj_with_priv_data)
{
	k_queue_init(&queue);
	k_queue_insert(&queue, k_queue_peek_tail(&queue), (void *)&data[0]);
	k_thread_create(&tdata, tstack, STACK_SIZE, user_access_queue_private_data,
					NULL, NULL, NULL, 0, K_USER, K_NO_WAIT);
	k_thread_join(&tdata, K_FOREVER);
}

static void low_prio_wait_for_queue(void *p1, void *p2, void *p3)
{
	struct k_queue *q = p1;
	uint32_t *ret = NULL;

	ret = k_queue_get(q, K_FOREVER);
	zassert_true(*ret == 0xccc,
	"The low priority thread get the queue data failed lastly");
}

static void high_prio_t1_wait_for_queue(void *p1, void *p2, void *p3)
{
	struct k_queue *q = p1;
	uint32_t *ret = NULL;

	ret = k_queue_get(q, K_FOREVER);
	zassert_true(*ret == 0xaaa,
	"The highest priority and waited longest get the queue data failed firstly");
}

static void high_prio_t2_wait_for_queue(void *p1, void *p2, void *p3)
{
	struct k_queue *q = p1;
	uint32_t *ret = NULL;

	ret = k_queue_get(q, K_FOREVER);
	zassert_true(*ret == 0xbbb,
	"The higher priority and waited longer get the queue data failed secondly");
}

/**
 * @brief Test multi-threads to get data from a queue.
 *
 * @details Define three threads, and set a higher priority for two of them,
 * and set a lower priority for the last one. Then Add a delay between
 * creating the two high priority threads.
 * Test point:
 * 1. Any number of threads may wait on an empty FIFO simultaneously.
 * 2. When a data item is added, it is given to the highest priority
 * thread that has waited longest.
 *
 * @ingroup kernel_queue_tests
 */
ZTEST(queue_api_1cpu, test_queue_multithread_competition)
{
	int old_prio = k_thread_priority_get(k_current_get());
	int prio = 10;
	uint32_t test_data[3];

	memset(test_data, 0, sizeof(test_data));
	k_thread_priority_set(k_current_get(), prio);
	k_queue_init(&queue);
	zassert_true(k_queue_is_empty(&queue) != 0, " Initializing queue failed");

	/* Set up some values */
	test_data[0] = 0xAAA;
	test_data[1] = 0xBBB;
	test_data[2] = 0xCCC;

	k_thread_create(&tdata, tstack, STACK_SIZE,
			low_prio_wait_for_queue,
			&queue, NULL, NULL,
			prio + 4, 0, K_NO_WAIT);

	k_thread_create(&tdata1, tstack1, STACK_SIZE,
			high_prio_t1_wait_for_queue,
			&queue, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Make thread tdata and tdata1 wait more time */
	k_sleep(K_MSEC(10));

	k_thread_create(&tdata2, tstack2, STACK_SIZE,
			high_prio_t2_wait_for_queue,
			&queue, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Initialize them and block */
	k_sleep(K_MSEC(50));

	/* Insert some data to wake up thread */
	k_queue_append(&queue, &test_data[0]);
	k_queue_append(&queue, &test_data[1]);
	k_queue_append(&queue, &test_data[2]);

	/* Wait for thread exiting */
	k_thread_join(&tdata, K_FOREVER);
	k_thread_join(&tdata1, K_FOREVER);
	k_thread_join(&tdata2, K_FOREVER);

	/* Revert priority of the main thread */
	k_thread_priority_set(k_current_get(), old_prio);
}

/**
 * @brief Verify k_queue_unique_append()
 *
 * @ingroup kernel_queue_tests
 *
 * @details Append the same data to the queue repeatedly,
 * see if it returns expected value.
 * And verify operation succeed if append different data to
 * the queue.
 *
 * @see k_queue_unique_append()
 */
ZTEST(queue_api, test_queue_unique_append)
{
	bool ret;

	k_queue_init(&queue);
	ret = k_queue_unique_append(&queue, (void *)&data[0]);
	zassert_true(ret, "queue unique append failed");

	ret = k_queue_unique_append(&queue, (void *)&data[0]);
	zassert_false(ret, "queue unique append should fail");

	ret = k_queue_unique_append(&queue, (void *)&data[1]);
	zassert_true(ret, "queue unique append failed");
}
