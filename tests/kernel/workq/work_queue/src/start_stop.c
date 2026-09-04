/*
 * Copyright (c) 2024 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define NUM_TEST_ITEMS 10
/* In fact, each work item could take up to this value */
#define WORK_ITEM_WAIT_ALIGNED                                                                     \
	k_ticks_to_ms_floor64(k_ms_to_ticks_ceil32(CONFIG_TEST_WORK_ITEM_WAIT_MS) + _TICK_ALIGN)
#define CHECK_WAIT ((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT_ALIGNED)

static K_THREAD_STACK_DEFINE(work_q_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);

static void work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_msleep(CONFIG_TEST_WORK_ITEM_WAIT_MS);
}

/**
 * @brief Verify a work queue's dedicated thread can be stopped once drained.
 *
 * @details
 * A work queue thread may only be torn down when the queue is drained and
 * plugged, so no work item is lost and no submission races the teardown. This
 * test walks the queue through its whole lifecycle and checks the error
 * reported at each point where stopping is not allowed.
 *
 * Test steps:
 * - Call k_work_queue_stop() on a queue that was never started.
 * - Start the queue, submit work items and wait for them to complete.
 * - Call k_work_queue_stop() while the queue is running and unplugged.
 * - Drain and plug the queue, then stop it.
 * - Submit a work item to the stopped queue.
 *
 * Expected result:
 * - Stopping an unstarted queue returns -EALREADY, stopping a running unplugged
 *   queue returns -EBUSY, stopping a drained and plugged queue succeeds, and
 *   submitting to the stopped queue returns -ENODEV.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_start()
 * @see k_work_queue_stop()
 * @see k_work_queue_drain()
 * @see k_work_submit_to_queue()
 */
ZTEST(workqueue_api, test_workq_start_stop)
{
	size_t i;
	struct k_work work;
	struct k_work_q work_q = {};
	struct k_work works[NUM_TEST_ITEMS];
	struct k_work_queue_config cfg = {
		.name = "test_work_q",
		.no_yield = true,
	};

	zassert_equal(k_work_queue_stop(&work_q, K_FOREVER), -EALREADY,
		      "Succeeded to stop work queue on non-initialized work queue");
	k_work_queue_start(&work_q, work_q_stack, K_THREAD_STACK_SIZEOF(work_q_stack),
			   K_PRIO_PREEMPT(4), &cfg);

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		k_work_init(&works[i], work_handler);
		zassert_equal(k_work_submit_to_queue(&work_q, &works[i]), 1,
			      "Failed to submit work item");
	}

	/* Wait for the work item to complete */
	k_sleep(K_MSEC(CHECK_WAIT));

	zassert_equal(k_work_queue_stop(&work_q, K_FOREVER), -EBUSY,
		      "Succeeded to stop work queue while it is running & not plugged");
	zassert_true(k_work_queue_drain(&work_q, true) >= 0, "Failed to drain & plug work queue");
	zassert_ok(k_work_queue_stop(&work_q, K_FOREVER), "Failed to stop work queue");

	k_work_init(&work, work_handler);
	zassert_equal(k_work_submit_to_queue(&work_q, &work), -ENODEV,
		      "Succeeded to submit work item to non-initialized work queue");
}

/**
 * @brief Verify an essential work queue thread cannot be stopped.
 *
 * @details
 * A work queue started with the essential configuration flag runs an essential
 * thread that must not be torn down. This test confirms k_work_queue_stop()
 * refuses to stop such a queue even after it has been drained and plugged.
 *
 * Test steps:
 * - Start a work queue configured as essential.
 * - Drain and plug the queue.
 * - Attempt to stop the queue with k_work_queue_stop().
 *
 * Expected result:
 * - k_work_queue_stop() returns -ENOTSUP.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_start()
 * @see k_work_queue_stop()
 */
ZTEST(workqueue_api, test_workq_stop_essential)
{
	struct k_work_q work_q = {};
	struct k_work_queue_config cfg = {
		.name = "test_work_q",
		.no_yield = true,
		.essential = true,
	};

	k_work_queue_start(&work_q, work_q_stack, K_THREAD_STACK_SIZEOF(work_q_stack),
			   K_PRIO_PREEMPT(4), &cfg);

	zassert_true(k_work_queue_drain(&work_q, true) >= 0, "Failed to drain & plug work queue");
	zassert_equal(-ENOTSUP, k_work_queue_stop(&work_q, K_FOREVER), "Failed to stop work queue");
}


#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_DEFINE(run_stack, STACK_SIZE);

static void run_q_main(void *workq_ptr, void *sem_ptr, void *p3)
{
	ARG_UNUSED(p3);

	struct k_work_q *queue = (struct k_work_q *)workq_ptr;
	struct k_sem *sem = (struct k_sem *)sem_ptr;

	struct k_work_queue_config cfg = {
		.name = "wq.run_q",
		.no_yield = true,
	};

	k_work_queue_run(queue, &cfg);

	k_sem_give(sem);
}

/**
 * @brief Verify a work queue served by k_work_queue_run() can be stopped.
 *
 * @details
 * k_work_queue_run() turns the calling thread into a work queue's server thread.
 * This test runs a queue from a dedicated thread, submits and drains work, and
 * confirms the queue can then be stopped (and that stop is rejected while the
 * queue is running and unplugged, and that submitting to a stopped queue fails).
 *
 * Test steps:
 * - Start a thread that serves a work queue via k_work_queue_run().
 * - Submit work items and wait for them to complete.
 * - Confirm k_work_queue_stop() returns -EBUSY while running and unplugged.
 * - Drain and plug the queue, then stop it.
 * - Confirm submitting to the stopped queue and the server thread exiting both
 *   behave as expected.
 *
 * Expected result:
 * - The queue stops cleanly once drained and plugged, submitting afterwards
 *   returns -ENODEV, and the server thread releases its completion semaphore.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_run()
 * @see k_work_queue_stop()
 * @see k_work_queue_drain()
 */
ZTEST(workqueue_api, test_workq_run_stop)
{
	int rc;
	size_t i;
	struct k_thread thread;
	struct k_work work;
	struct k_work_q work_q = {};
	struct k_work works[NUM_TEST_ITEMS];
	struct k_sem ret_sem;

	k_sem_init(&ret_sem, 0, 1);

	(void)k_thread_create(&thread, run_stack, STACK_SIZE, run_q_main, &work_q, &ret_sem, NULL,
			      K_PRIO_COOP(3), 0, K_FOREVER);

	k_thread_start(&thread);

	k_sleep(K_MSEC(CHECK_WAIT));

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		k_work_init(&works[i], work_handler);
		zassert_equal(k_work_submit_to_queue(&work_q, &works[i]), 1,
			      "Failed to submit work item");
	}

	/* Wait for the work item to complete */
	k_sleep(K_MSEC(CHECK_WAIT));

	zassert_equal(k_work_queue_stop(&work_q, K_FOREVER), -EBUSY,
		      "Succeeded to stop work queue while it is running & not plugged");
	zassert_true(k_work_queue_drain(&work_q, true) >= 0, "Failed to drain & plug work queue");
	zassert_ok(k_work_queue_stop(&work_q, K_FOREVER), "Failed to stop work queue");

	k_work_init(&work, work_handler);
	zassert_equal(k_work_submit_to_queue(&work_q, &work), -ENODEV,
		      "Succeeded to submit work item to non-initialized work queue");

	/* Take the semaphore the other thread released once done running the queue */
	rc = k_sem_take(&ret_sem, K_MSEC(1));
	zassert_equal(rc, 0);
}

/**
 * @brief Verify a work queue's thread runs at the priority given at start.
 *
 * @details
 * The priority passed to k_work_queue_start() decides how the queue's handlers
 * are scheduled against the rest of the system, so it must be applied to the
 * queue's thread rather than silently defaulted.
 *
 * Test steps:
 * - Start a work queue with an explicit preemptible priority.
 * - Read the priority of the thread returned by k_work_queue_thread_get().
 * - Drain, plug and stop the queue.
 *
 * Expected result:
 * - The queue thread's priority equals the priority passed to
 *   k_work_queue_start().
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_start()
 * @see k_work_queue_thread_get()
 */
ZTEST(workqueue_api, test_workq_priority)
{
	struct k_work_q work_q = {};
	struct k_work_queue_config cfg = {
		.name = "prio_work_q",
	};
	const int prio = K_PRIO_PREEMPT(6);

	k_work_queue_start(&work_q, work_q_stack, K_THREAD_STACK_SIZEOF(work_q_stack),
			   prio, &cfg);

	zassert_equal(k_thread_priority_get(k_work_queue_thread_get(&work_q)), prio,
		      "work queue thread priority was not configured at start");

	zassert_true(k_work_queue_drain(&work_q, true) >= 0, "Failed to drain & plug");
	zassert_ok(k_work_queue_stop(&work_q, K_FOREVER), "Failed to stop work queue");
}

static void remaining_noop_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

/**
 * @brief Verify the remaining delay reported for a delayable work item.
 *
 * @details
 * k_work_delayable_remaining_get() reports how long is left before a scheduled
 * item is submitted, and reports no remaining time when the item is not waiting
 * on a delay at all. This test covers an item that was never scheduled, one
 * scheduled for a known delay, and one whose schedule has been cancelled.
 *
 * Test steps:
 * - Initialize a delayable work item and query its remaining time.
 * - Schedule it for a known delay and query the remaining time again.
 * - Cancel the item and query the remaining time once more.
 *
 * Expected result:
 * - An unscheduled item reports 0, a scheduled item reports a non-zero time no
 *   longer than the requested delay, and a cancelled item reports 0 again.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_schedule()
 * @see k_work_delayable_remaining_get()
 * @see k_work_cancel_delayable()
 */
ZTEST(workqueue_api, test_workq_delayable_remaining)
{
	static struct k_work_delayable dwork;
	k_ticks_t remaining;

	k_work_init_delayable(&dwork, remaining_noop_handler);

	/* Not scheduled yet: no remaining time. */
	zassert_equal(k_work_delayable_remaining_get(&dwork), 0,
		      "unscheduled delayable should report no remaining time");

	zassert_true(k_work_schedule(&dwork, K_MSEC(200)) >= 0,
		     "failed to schedule delayable work");

	remaining = k_work_delayable_remaining_get(&dwork);
	zassert_true(remaining > 0 && remaining <= k_ms_to_ticks_ceil32(200) + 1,
		     "remaining time %lld out of the expected range",
		     (long long)remaining);

	zassert_true(k_work_cancel_delayable(&dwork) >= 0, "failed to cancel work");

	/* After cancellation: no remaining time. */
	zassert_equal(k_work_delayable_remaining_get(&dwork), 0,
		      "cancelled delayable should report no remaining time");
}

ZTEST_SUITE(workqueue_api, NULL, NULL, NULL, NULL, NULL);
