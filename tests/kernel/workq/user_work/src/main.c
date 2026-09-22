/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Workqueue Tests
 * @defgroup kernel_workqueue_tests Workqueue
 * @ingroup all_tests
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_OF_WORK 2
#define SYNC_SEM_INIT_VAL (0U)

static K_THREAD_STACK_DEFINE(user_tstack, STACK_SIZE);
static struct k_work_user_q user_workq;
static ZTEST_BMEM struct k_work_user work[NUM_OF_WORK];
static struct k_sem sync_sema;
static struct k_sem dummy_sema;
static struct k_thread *main_thread;

/* Common handler for the user workqueue tests; the k_sem_give() call in it
 * signals successful execution of the handler. The unused parameter keeps the
 * signature accepted by k_work_user_init().
 */
static void common_work_handler(struct k_work_user *unused)
{
	k_sem_give(&sync_sema);
}

/* Check that K_WORK_USER_DEFINE() leaves the item initialized and unqueued. */
static void check_work_user_define(void)
{
	K_WORK_USER_DEFINE(local, common_work_handler);
	zassert_equal(local.handler, common_work_handler);
	zassert_equal(local.flags, 0);
}

/* Exercise the k_work_user_submit_to_queue() error paths: resubmitting an
 * item that is still pending must not queue it twice, and submission fails
 * once the thread resource pool is exhausted.
 */
static void check_submit_to_queue_errors(void)
{
	int ret = 0;

	k_sem_reset(&sync_sema);
	k_work_user_init(&work[0], common_work_handler);
	k_work_user_init(&work[1], common_work_handler);

	/* TESTPOINT: When a work item be added to a workqueue, its flag will
	 * be in pending state, before the work item be processed, it cannot
	 * be append to a workqueue another time.
	 */
	k_work_user_submit_to_queue(&user_workq, &work[0]);
	zassert_true(k_work_user_is_pending(&work[0]));
	k_work_user_submit_to_queue(&user_workq, &work[0]);

	/* Test the work item's callback function can only be invoked once */
	k_sem_take(&sync_sema, K_FOREVER);
	zassert_true(k_queue_is_empty(&user_workq.queue));
	zassert_false(k_work_user_is_pending(&work[0]));

	/* use up the memory in resource pool */
	do {
		ret = k_queue_alloc_append(&user_workq.queue, &work[1]);
		if (ret == -ENOMEM) {
			break;
		}
	} while (true);

	k_work_user_submit_to_queue(&user_workq, &work[0]);
	/* if memory is used up, the work cannot be append into the workqueue */
	zassert_false(k_work_user_is_pending(&work[0]));
}


static void work_handler(struct k_work_user *w)
{
	/* Just to show an API call on this will succeed */
	k_sem_init(&dummy_sema, 0, 1);

	k_sem_give(&sync_sema);
}

static void twork_submit_1(struct k_work_user_q *work_q, struct k_work_user *w,
			   k_work_user_handler_t handler)
{
	/**TESTPOINT: init via k_work_init*/
	k_work_user_init(w, handler);
	/**TESTPOINT: check pending after work init*/
	zassert_false(k_work_user_is_pending(w));

	/**TESTPOINT: work submit to queue*/
	zassert_false(k_work_user_submit_to_queue(work_q, w),
		      "failed to submit to queue");
}

static void twork_submit(const void *data)
{
	struct k_work_user_q *work_q = (struct k_work_user_q *)data;

	for (int i = 0; i < NUM_OF_WORK; i++) {
		twork_submit_1(work_q, &work[i], work_handler);
	}
}

/* Start the user-mode work queue; must happen before any submission. */
static void start_user_work_queue(void)
{
	k_work_user_queue_start(&user_workq, user_tstack, STACK_SIZE,
				CONFIG_MAIN_THREAD_PRIORITY, "user.wq");
}

/* Set up object permissions for grant_dummy_sema_to_workq_thread(). */
static void grant_dummy_sema_to_main_thread(void)
{
	/* Subsequent test cases will have access to the dummy_sema,
	 * but not the user workqueue since it already started.
	 */
	k_object_access_grant(&dummy_sema, main_thread);
}

/* Grant the user work queue thread access to the semaphore its handler
 * uses, showing permissions can be extended to the work queue thread.
 */
static void grant_dummy_sema_to_workq_thread(void)
{
	k_object_access_grant(&dummy_sema, &user_workq.thread);
}

/* Initialize and submit the user work items and wait until every handler
 * has signalled completion.
 */
static void submit_work_and_wait(void)
{
	k_sem_reset(&sync_sema);
	twork_submit(&user_workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

void *workq_setup(void)
{
	main_thread = k_current_get();
	k_thread_access_grant(main_thread, &sync_sema, &user_workq.thread,
			      &user_workq.queue,
			      &user_tstack);
	k_sem_init(&sync_sema, SYNC_SEM_INIT_VAL, NUM_OF_WORK);
	k_thread_system_pool_assign(k_current_get());

	grant_dummy_sema_to_main_thread();

	return NULL;
}

/**
 * @brief Verify a user-mode work queue runs work submitted from user mode.
 *
 * @details
 * The whole lifecycle is driven from a user thread: the work queue is started,
 * the queue thread is granted access to the objects its handlers touch, and
 * user work items are initialized, submitted and run to completion. The
 * submission error paths are exercised as well: an item still pending on the
 * queue cannot be queued twice, and submission fails when the thread resource
 * pool is exhausted.
 *
 * The steps share one work queue and must run in this order, which is why they
 * are a single test case rather than several.
 *
 * Test steps:
 * - Verify K_WORK_USER_DEFINE() leaves the item bound to its handler and not
 *   pending.
 * - Start a user-mode work queue with k_work_user_queue_start().
 * - Grant the work queue thread access to a kernel object and verify a
 *   handler running on the queue can use it.
 * - Initialize and submit user work items and wait for their handlers to
 *   run.
 * - Resubmit an item that is still pending and verify it is not queued
 *   twice.
 * - Exhaust the thread resource pool and verify submission fails.
 *
 * Expected result:
 * - Submitted user work items execute exactly once, and both error paths
 *   (already pending, out of memory) refuse the submission.
 *
 * @ingroup kernel_workqueue_tests
 * @see K_WORK_USER_DEFINE()
 * @see k_work_user_queue_start()
 * @see k_work_user_init()
 * @see k_work_user_submit_to_queue()
 * @see k_work_user_is_pending()
 * @see k_object_access_grant()
 */
ZTEST_USER(workqueue_api, test_workq_user_mode)
{
	check_work_user_define();

	/* Do not disturb the ordering of these steps */
	start_user_work_queue();
	grant_dummy_sema_to_workq_thread();

	/* End order-important steps */
	submit_work_and_wait();
	check_submit_to_queue_errors();
}

ZTEST_SUITE(workqueue_api, NULL, workq_setup, NULL, NULL, NULL);
