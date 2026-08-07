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
 * @brief Verify data passes between two threads through a queue in order.
 *
 * @details
 * A consumer thread blocks on k_queue_get() while the main thread enqueues items
 * using every insertion API (insert, append, prepend, append_list, merge_slist).
 * The consumer must receive all items in the resulting dequeue order. Run against
 * both a k_queue_init()ed and a K_QUEUE_DEFINE()d queue.
 *
 * Test steps:
 * - Start a consumer thread that drains the queue.
 * - Insert items via insert/append/prepend/append_list/merge_slist.
 * - Synchronize on a semaphore once the consumer has read all items.
 * - Repeat for a statically defined queue.
 *
 * Expected result:
 * - The consumer dequeues every item, matching the expected ordering.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_append()
 * @see k_queue_prepend()
 * @see k_queue_get()
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
 * @brief Verify a queue defined at compile time is ready for use.
 *
 * @details
 * A queue created with K_QUEUE_DEFINE() must be fully initialized at boot:
 * empty and immediately usable for data passing without any run-time
 * initialization call.
 *
 * Test steps:
 * - Check the statically defined queue is empty.
 * - Append an item and get it back, verifying the same item is returned.
 *
 * Expected result:
 * - The statically defined queue accepts and delivers items as-is.
 *
 * @ingroup tests_kernel_queue
 *
 * @see K_QUEUE_DEFINE
 */
ZTEST(queue_api, test_queue_define)
{
	/* usable at boot without any run-time initialization */
	zassert_true(k_queue_is_empty(&kqueue));

	k_queue_append(&kqueue, (void *)&data[0]);
	zassert_equal(k_queue_get(&kqueue, K_NO_WAIT), (void *)&data[0]);

	/* the queue is drained: a non-blocking get returns NULL */
	zassert_is_null(k_queue_get(&kqueue, K_NO_WAIT));
}

/**
 * @brief Verify run-time initialization of a queue.
 *
 * @details
 * k_queue_init() must yield an empty queue that is immediately usable for
 * data passing.
 *
 * Test steps:
 * - Initialize a queue at run time and check it is empty.
 * - Append an item and get it back, verifying the same item is returned.
 *
 * Expected result:
 * - The initialized queue is empty and accepts and delivers items.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_init()
 */
ZTEST(queue_api, test_queue_init)
{
	k_queue_init(&queue);

	zassert_true(k_queue_is_empty(&queue));
	k_queue_append(&queue, (void *)&data[0]);
	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data[0]);

	/* the queue is drained: a non-blocking get returns NULL */
	zassert_is_null(k_queue_get(&queue, K_NO_WAIT));
}

/**
 * @brief Verify prepended items are dequeued before appended ones.
 *
 * @details
 * k_queue_prepend() adds an item to the front of the queue, so items
 * prepended after an append must be dequeued first, with the most recently
 * prepended item coming out first.
 *
 * Test steps:
 * - Add one item, then prepend two items.
 * - Get all three items and verify the order: most recently prepended
 *   first, appended item last.
 *
 * Expected result:
 * - Items are dequeued in prepend-reverse order followed by the appended
 *   item.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_prepend()
 * @see k_queue_get()
 */
ZTEST(queue_api, test_queue_prepend_order)
{
	k_queue_init(&queue);

	k_queue_append(&queue, (void *)&data[0]);
	k_queue_prepend(&queue, (void *)&data_p[1]);
	k_queue_prepend(&queue, (void *)&data_p[0]);

	/* prepended items precede the appended one, most recent first */
	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data_p[0]);
	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data_p[1]);
	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data[0]);
}

/**
 * @brief Verify k_queue_insert() places an item after a given node.
 *
 * @details
 * k_queue_insert() adds an item immediately after a caller-specified node
 * already in the queue, so dequeuing must return the inserted item between
 * its predecessor and the items that followed it.
 *
 * Test steps:
 * - Append two items.
 * - Insert a third item after the first one.
 * - Get all items and verify the inserted item comes out second.
 *
 * Expected result:
 * - The inserted item is dequeued directly after the node it was inserted
 *   behind.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_insert()
 * @see k_queue_get()
 */
ZTEST(queue_api, test_queue_insert_after)
{
	k_queue_init(&queue);

	k_queue_append(&queue, (void *)&data[0]);
	k_queue_append(&queue, (void *)&data[1]);

	/* insert between the two appended items */
	k_queue_insert(&queue, (void *)&data[0], (void *)&data_p[0]);

	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data[0]);
	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data_p[0]);
	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data[1]);
}

/**
 * @brief Verify appending a singly-linked item list to the back of a queue.
 *
 * @details
 * k_queue_append_list() must transfer a caller-built singly-linked list to
 * the back of the queue with its order preserved.
 *
 * Test steps:
 * - Append one item so the queue is non-empty.
 * - Link the items of an array into a list.
 * - Append the list and confirm it lands behind the pre-existing item.
 * - Get all items and verify list order is preserved.
 *
 * Expected result:
 * - The pre-existing item is delivered first, then every list item in its
 *   original order.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_append_list()
 * @see k_queue_get()
 */
ZTEST(queue_api, test_queue_append_list_order)
{
	static qdata_t *head = &data_l[0], *tail = &data_l[LIST_LEN - 1];

	k_queue_init(&queue);

	/* a pre-existing item so the list is demonstrably appended behind it */
	k_queue_append(&queue, (void *)&data[0]);

	head->snode.next = (sys_snode_t *)tail;
	tail->snode.next = NULL;
	k_queue_append_list(&queue, (uint32_t *)head, (uint32_t *)tail);

	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data[0]);
	for (int i = 0; i < LIST_LEN; i++) {
		zassert_equal(k_queue_get(&queue, K_NO_WAIT),
			      (void *)&data_l[i]);
	}
	zassert_is_null(k_queue_get(&queue, K_NO_WAIT));
}

/**
 * @brief Verify merging a sys_slist empties the list into the queue.
 *
 * @details
 * k_queue_merge_slist() must move every node of a sys_slist to the back of
 * the queue in list order and leave the source list empty.
 *
 * Test steps:
 * - Append one item so the queue is non-empty.
 * - Build a sys_slist with two nodes and merge it into the queue.
 * - Verify the source list is empty after the merge.
 * - Get all items and confirm the merged nodes land behind the pre-existing
 *   item, in list order.
 *
 * Expected result:
 * - The pre-existing item is delivered first, then every merged node in
 *   order, and the source list is emptied.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_merge_slist()
 * @see k_queue_get()
 */
ZTEST(queue_api, test_queue_merge_slist_order)
{
	sys_slist_t slist;

	k_queue_init(&queue);

	/* a pre-existing item so the merged nodes land behind it */
	k_queue_append(&queue, (void *)&data[0]);

	sys_slist_init(&slist);
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[0].snode));
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[1].snode));
	k_queue_merge_slist(&queue, &slist);

	/* the source list is emptied by the merge */
	zassert_true(sys_slist_is_empty(&slist));

	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data[0]);
	for (int i = 0; i < LIST_LEN; i++) {
		zassert_equal(k_queue_get(&queue, K_NO_WAIT),
			      (void *)&data_sl[i]);
	}
	zassert_is_null(k_queue_get(&queue, K_NO_WAIT));
}

static void tqueue_merge_slist_wake_entry(void *p1, void *p2, void *p3)
{
	/* blocks until the main thread's k_queue_merge_slist() wakes us */
	void *rx_data = k_queue_get(&queue, K_FOREVER);

	zassert_equal(rx_data, (void *)&data_sl[0]);
	k_sem_give(&end_sema);
}

/**
 * @brief Verify k_queue_merge_slist() wakes a thread pending on the queue.
 *
 * @details
 * A thread blocked in k_queue_get() on an empty queue must be woken when
 * another context makes items available with k_queue_merge_slist(), and it
 * must receive the first node of the merged list.
 *
 * Test steps:
 * - Start a thread that blocks on an empty queue with k_queue_get(K_FOREVER).
 * - Merge a two-node sys_slist into the queue.
 * - Verify the woken thread received the first merged node and the second
 *   node stays queued behind it.
 *
 * Expected result:
 * - The pending thread is woken and dequeues the first merged node; the
 *   remaining node stays in the queue.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_merge_slist()
 * @see k_queue_get()
 */
ZTEST(queue_api_1cpu, test_queue_merge_slist_wake)
{
	sys_slist_t slist;

	k_queue_init(&queue);
	k_sem_init(&end_sema, 0, 1);

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tqueue_merge_slist_wake_entry, NULL, NULL,
				      NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/* let the thread reach k_queue_get() and pend on the empty queue */
	k_sleep(K_MSEC(10));

	sys_slist_init(&slist);
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[0].snode));
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[1].snode));
	k_queue_merge_slist(&queue, &slist);

	/* the merge woke the pending thread, which took the first node */
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/* the second node remains queued behind the consumed one */
	zassert_equal(k_queue_get(&queue, K_NO_WAIT), (void *)&data_sl[1]);
	zassert_is_null(k_queue_get(&queue, K_NO_WAIT));
}

/**
 * @brief Verify data passes from an ISR to a thread through a queue.
 *
 * @details
 * Items are enqueued from interrupt context (via irq_offload()) and dequeued in
 * thread context, confirming the queue insertion APIs are ISR-safe and the
 * thread receives every item. Run against both an init()ed and a DEFINE()d queue.
 *
 * Test steps:
 * - From an ISR, insert items into the queue.
 * - In thread context, get all items and verify their addresses/order.
 * - Repeat for a statically defined queue.
 *
 * Expected result:
 * - The thread dequeues every ISR-enqueued item in the expected order.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_append()
 * @see k_queue_get()
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
 * @brief Verify data passes from a thread to an ISR through a queue.
 *
 * @details
 * Items are enqueued in thread context and dequeued from interrupt context (via
 * irq_offload()), confirming k_queue_get() is ISR-safe and the ISR receives
 * every item. Run against both an init()ed and a DEFINE()d queue.
 *
 * Test steps:
 * - In thread context, insert items into the queue.
 * - From an ISR, get all items and verify their addresses/order.
 * - Repeat for a statically defined queue.
 *
 * Expected result:
 * - The ISR dequeues every thread-enqueued item in the expected order.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_append()
 * @see k_queue_get()
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
 * @brief Verify two threads blocked on a queue are each woken by an append.
 *
 * @details
 * When multiple threads block on an empty queue with K_FOREVER, each appended
 * item must wake exactly one waiter so that every blocked getter eventually
 * receives data.
 *
 * Test steps:
 * - Start two threads that each block in k_queue_get(K_FOREVER).
 * - Append two items to the queue.
 * - Confirm both threads complete (each got one item).
 *
 * Expected result:
 * - Both waiting threads are woken, each receiving one item.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_get()
 * @see k_queue_append()
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
 * @brief Verify alloc append/prepend honor the thread's resource pool.
 *
 * @details
 * k_queue_alloc_append()/k_queue_alloc_prepend() allocate a container from the
 * calling thread's heap. With no pool (or a too-small pool) the insertion must
 * fail and leave the queue empty; with a sufficient pool it must succeed and the
 * item becomes retrievable.
 *
 * Test steps:
 * - With no resource pool assigned, attempt an alloc append; verify it fails.
 * - Assign a too-small pool, attempt an alloc prepend; verify it fails and the
 *   queue stays empty.
 * - Assign a sufficient pool, alloc prepend succeeds and the item can be gotten.
 *
 * Expected result:
 * - Allocation fails without enough pool memory and succeeds with it.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_alloc_append()
 * @see k_queue_alloc_prepend()
 * @see k_thread_heap_assign()
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

/**
 * @brief Verify an append wakes exactly one waiter (CONFIG_POLL race).
 *
 * @details
 * Guards against a historical CONFIG_POLL race: an insert could wake a
 * lower-priority waiter, then a higher-priority waiter could steal the item
 * before the first ran, causing the lower-priority getter to spuriously return
 * NULL before its timeout. Two consumers of different priority block on the
 * queue; appending two items must deliver exactly one to each (two total) with
 * no spurious early return.
 *
 * Test steps:
 * - Start two consumer threads of different priority that block on the queue.
 * - Append two items.
 * - Verify neither consumer woke prematurely and exactly two gets succeeded.
 *
 * Expected result:
 * - The two appended items are consumed (low_count + mid_count == 2) with no
 *   spurious NULL return.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_append()
 * @see k_queue_get()
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
 * @brief Verify multiple independent queues operate without interference.
 *
 * @details
 * Several queues initialized from the same code must each store and return their
 * own items correctly, confirming queue instances do not share state.
 *
 * Test steps:
 * - Initialize an array of queues.
 * - For each, append a set of items and drain it, verifying the contents.
 *
 * Expected result:
 * - Every queue independently delivers its own items.
 *
 * @ingroup tests_kernel_queue
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
 * @brief Verify a user thread faults accessing a queue it has no permission on.
 *
 * @details
 * The k_queue APIs keep bookkeeping data inside structures visible from user
 * mode, so direct access must be gated by the kernel object permission system. A
 * user thread without permission on the queue object that calls a k_queue API
 * must trigger a kernel oops rather than read private kernel data.
 *
 * Test steps:
 * - Initialize a queue and insert an item from supervisor mode.
 * - Spawn a user thread (with no granted access) that calls k_queue_is_empty().
 * - Mark the fault as expected and join the thread.
 *
 * Expected result:
 * - The unprivileged access triggers the expected kernel oops.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_is_empty()
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
 * @brief Verify queued data is delivered by priority then wait time.
 *
 * @details
 * Any number of threads may block on an empty queue. When data arrives it must
 * go to the highest-priority waiter, and among equal-priority waiters to the one
 * that has waited longest. Three threads (one low priority, two equal higher
 * priority created with a delay between them) block, then three items are
 * appended; each thread must receive the item matching its expected order.
 *
 * Test steps:
 * - Start a low-priority waiter and two higher-priority waiters (staggered).
 * - Append three distinct items.
 * - Each thread asserts it received the item matching its priority/wait rank.
 *
 * Expected result:
 * - The longest-waiting highest-priority thread gets the first item, the other
 *   high-priority thread next, and the low-priority thread last.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_get()
 * @see k_queue_append()
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
 * @brief Verify k_queue_unique_append() rejects duplicate entries.
 *
 * @details
 * k_queue_unique_append() must add an item only if it is not already queued: a
 * first append of an item succeeds, a second append of the same item fails, and
 * appending a different item succeeds.
 *
 * Test steps:
 * - Append an item and verify it succeeds.
 * - Append the same item again and verify it fails.
 * - Append a different item and verify it succeeds.
 *
 * Expected result:
 * - Duplicate appends return false; unique appends return true.
 *
 * @ingroup tests_kernel_queue
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
