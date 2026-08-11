/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_queue.h"

#ifdef CONFIG_USERSPACE

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN        5

/** @cond INTERNAL_HIDDEN */
static K_THREAD_STACK_DEFINE(child_stack, STACK_SIZE);
static struct k_thread child_thread;
static ZTEST_BMEM struct qdata qdata[LIST_LEN * 2];
/** @endcond */

/**
 * @brief Tests for queue
 * @defgroup tests_kernel_queue Queue tests
 * @ingroup all_tests
 * @{
 * @}
 */

/* Higher priority than the thread putting stuff in the queue */
void child_thread_get(void *p1, void *p2, void *p3)
{
	struct qdata *qd;
	struct k_queue *q = p1;
	struct k_sem *sem = p2;

	zassert_false(k_queue_is_empty(q));
	qd = k_queue_peek_head(q);
	zassert_equal(qd->data, 0);
	qd = k_queue_peek_tail(q);
	zassert_equal(qd->data, (LIST_LEN * 2) - 1,
		      "got %d expected %d", qd->data, (LIST_LEN * 2) - 1);

	for (int i = 0; i < (LIST_LEN * 2); i++) {
		qd = k_queue_get(q, K_FOREVER);

		zassert_equal(qd->data, i);
		if (qd->allocated) {
			/* snode should never have been touched */
			zassert_is_null(qd->snode.next, NULL);
		}
	}


	zassert_true(k_queue_is_empty(q));

	/* This one gets canceled */
	qd = k_queue_get(q, K_FOREVER);
	zassert_is_null(qd, NULL);

	k_sem_give(sem);
}

/**
 * @brief Verify supervisor-queued data reaches a user thread and cancel wait.
 *
 * @details
 * A supervisor thread fills a queue with both plain and alloc-appended elements,
 * which a user-mode child thread must read back in order (with correct head/tail
 * peeks). The child then blocks on an empty queue and must be released by
 * k_queue_cancel_wait() returning NULL.
 *
 * Test steps:
 * - Supervisor appends interleaved plain and alloc-appended items.
 * - Start a user child that peeks head/tail, gets all items in order, then
 *   blocks on an empty get.
 * - Call k_queue_cancel_wait() and verify the child's blocked get returns NULL.
 *
 * Expected result:
 * - The user thread reads every item in order and the cancel releases its wait.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_alloc_append()
 * @see k_queue_get()
 * @see k_queue_cancel_wait()
 */
ZTEST(queue_api_1cpu, test_queue_supv_to_user)
{
	/* Supervisor mode will add a bunch of data, some with alloc
	 * and some not
	 */

	struct k_queue *q;
	struct k_sem *sem;

	if (!(IS_ENABLED(CONFIG_USERSPACE))) {
		ztest_test_skip();
	}

	q = k_object_alloc(K_OBJ_QUEUE);
	zassert_not_null(q, "no memory for allocated queue object");
	k_queue_init(q);

	sem = k_object_alloc(K_OBJ_SEM);
	zassert_not_null(sem, "no memory for semaphore object");
	k_sem_init(sem, 0, 1);

	for (int i = 0; i < (LIST_LEN * 2); i = i + 2) {
		/* Just for test purposes -- not safe to do this in the
		 * real world as user mode shouldn't have any access to the
		 * snode struct
		 */
		qdata[i].data = i;
		qdata[i].allocated = false;
		qdata[i].snode.next = NULL;
		k_queue_append(q, &qdata[i]);

		qdata[i + 1].data = i + 1;
		qdata[i + 1].allocated = true;
		qdata[i + 1].snode.next = NULL;
		zassert_false(k_queue_alloc_append(q, &qdata[i + 1]));
	}

	k_thread_create(&child_thread, child_stack, STACK_SIZE,
			child_thread_get, q, sem, NULL, K_HIGHEST_THREAD_PRIO,
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();

	/* child thread runs until blocking on the last k_queue_get() call */
	k_queue_cancel_wait(q);
	k_sem_take(sem, K_FOREVER);
}

/**
 * @brief Verify k_queue_alloc_prepend() yields LIFO order from user mode.
 *
 * @details
 * Prepending items pushes each to the front, so reading the queue back returns
 * them in reverse insertion (LIFO) order. Exercised from a user-mode thread on an
 * allocated queue object.
 *
 * Test steps:
 * - Alloc-prepend a sequence of items 0..N-1 from a user thread.
 * - Get all items and verify they come out in reverse order (N-1..0).
 *
 * Expected result:
 * - Items are returned newest-first (LIFO).
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_alloc_prepend()
 * @see k_queue_get()
 */
ZTEST_USER(queue_api, test_queue_alloc_prepend_user)
{
	struct k_queue *q;

	q = k_object_alloc(K_OBJ_QUEUE);
	zassert_not_null(q, "no memory for allocated queue object");
	k_queue_init(q);

	for (int i = 0; i < LIST_LEN * 2; i++) {
		qdata[i].data = i;
		zassert_false(k_queue_alloc_prepend(q, &qdata[i]));
	}

	for (int i = (LIST_LEN * 2) - 1; i >= 0; i--) {
		struct qdata *qd;

		qd = k_queue_get(q, K_NO_WAIT);
		zassert_true(qd != NULL);
		zassert_equal(qd->data, i);
	}
}

/**
 * @brief Verify k_queue_alloc_append() yields FIFO order from user mode.
 *
 * @details
 * Appending items adds each to the tail, so reading the queue back returns them
 * in insertion (FIFO) order. Exercised from a user-mode thread on an allocated
 * queue object.
 *
 * Test steps:
 * - Alloc-append a sequence of items 0..N-1 from a user thread.
 * - Get all items and verify they come out in insertion order (0..N-1).
 *
 * Expected result:
 * - Items are returned oldest-first (FIFO).
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_alloc_append()
 * @see k_queue_get()
 */
ZTEST_USER(queue_api, test_queue_alloc_append_user)
{
	struct k_queue *q;

	q = k_object_alloc(K_OBJ_QUEUE);
	zassert_not_null(q, "no memory for allocated queue object");
	k_queue_init(q);

	for (int i = 0; i < LIST_LEN * 2; i++) {
		qdata[i].data = i;
		zassert_false(k_queue_alloc_append(q, &qdata[i]));
	}

	for (int i = 0; i < LIST_LEN * 2; i++) {
		struct qdata *qd;

		qd = k_queue_get(q, K_NO_WAIT);
		zassert_true(qd != NULL);
		zassert_equal(qd->data, i);
	}
}

/**
 * @brief Verify allocated queue resources are auto-freed back to the pool.
 *
 * @details
 * Elements added with the alloc APIs and the kernel objects allocated by the
 * previous user-mode test must be released automatically (queue elements when
 * dequeued, objects when permitted threads exit). The test confirms the whole
 * pool is available again by reallocating all of it.
 *
 * Test steps:
 * - After the preceding alloc-heavy test, allocate the entire pool in chunks.
 * - Verify every allocation succeeds, then free them.
 *
 * Expected result:
 * - All pool memory is reclaimable, proving prior resources were auto-freed.
 *
 * @ingroup tests_kernel_queue
 *
 * @see k_queue_alloc_append()
 */
ZTEST(queue_api, test_auto_free)
{
	/* Ensure any resources requested by the previous test were released
	 * by allocating the entire pool. It would have allocated two kernel
	 * objects and five queue elements. The queue elements should be
	 * auto-freed when they are de-queued, and the objects when all
	 * threads with permissions exit.
	 */
	void *b[4];
	int i;

	if (!(IS_ENABLED(CONFIG_USERSPACE))) {
		ztest_test_skip();
	}

	for (i = 0; i < 4; i++) {
		b[i] = k_heap_alloc(&test_pool, 64, K_FOREVER);
		zassert_true(b[i] != NULL, "memory not auto released!");
	}

	for (i = 0; i < 4; i++) {
		k_heap_free(&test_pool, b[i]);
	}
}

#endif /* CONFIG_USERSPACE */
