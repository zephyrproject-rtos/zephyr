/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <string.h>

#define STATIC_WQ_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define STATIC_WQ_PRIO       K_PRIO_PREEMPT(3)
#define STATIC_WQ_YIELD_PRIO K_PRIO_PREEMPT(5)

K_WORK_QUEUE_DEFINE(static_work_q, STATIC_WQ_STACK_SIZE, STATIC_WQ_PRIO, true, 0);
K_WORK_QUEUE_DEFINE(static_work_q_yield, STATIC_WQ_STACK_SIZE, STATIC_WQ_YIELD_PRIO, false);

static K_SEM_DEFINE(init_work_sem, 0, 1);
static K_SEM_DEFINE(yield_work_sem, 0, 1);

static int init_submit_rc;

static void init_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_sem_give(&init_work_sem);
}

static void yield_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_sem_give(&yield_work_sem);
}

static K_WORK_DEFINE(init_work, init_work_handler);
static K_WORK_DEFINE(yield_work, yield_work_handler);

/* Runs before any other POST_KERNEL init: the statically defined work queue
 * must already accept work at this point.
 */
static int submit_from_init(void)
{
	init_submit_rc = k_work_submit_to_queue(&static_work_q, &init_work);

	return 0;
}

SYS_INIT(submit_from_init, POST_KERNEL, 0);

/**
 * @brief Verify a statically defined work queue is started by the kernel.
 *
 * @details
 * K_WORK_QUEUE_DEFINE() defines a work queue that the kernel starts once it is
 * up, before POST_KERNEL device initialization. This test checks that work
 * submitted from a POST_KERNEL init function is accepted and processed, that
 * the thread carries the requested priority and the work queue name, and that
 * the thread is essential.
 *
 * Test steps:
 * - Submit a work item to the statically defined work queue from a
 *   SYS_INIT() function at POST_KERNEL level, priority 0.
 * - Wait for the work item to be processed.
 * - Read the work queue thread priority and name.
 * - Drain and plug each queue, then attempt to stop it.
 * - Submit and flush a work item on the second queue.
 *
 * Expected result:
 * - The submission from the init function returns 1 and the handler runs.
 * - The thread priority is the one given to K_WORK_QUEUE_DEFINE() and the
 *   thread name is the work queue name.
 * - k_work_queue_stop() returns -ENOTSUP on both queues.
 * - The second queue processes work.
 *
 * @ingroup kernel_workqueue_tests
 * @see K_WORK_QUEUE_DEFINE()
 * @see k_work_submit_to_queue()
 * @see k_work_queue_thread_get()
 */
ZTEST(workqueue_api, test_workq_static_define)
{
	k_tid_t tid;

	zassert_equal(init_submit_rc, 1, "Failed to submit work from POST_KERNEL init: %d",
		      init_submit_rc);
	zassert_ok(k_sem_take(&init_work_sem, K_MSEC(100)),
		   "Work submitted from POST_KERNEL init was not processed");

	tid = k_work_queue_thread_get(&static_work_q);
	zassert_not_null(tid, "Static work queue has no thread");
	zassert_equal(k_thread_priority_get(tid), STATIC_WQ_PRIO, "Wrong thread priority");
	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		zassert_str_equal(k_thread_name_get(tid), "static_work_q", "Wrong thread name");
	}

	tid = k_work_queue_thread_get(&static_work_q_yield);
	zassert_not_null(tid, "Static work queue has no thread");
	zassert_equal(k_thread_priority_get(tid), STATIC_WQ_YIELD_PRIO, "Wrong thread priority");
	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		zassert_str_equal(k_thread_name_get(tid), "static_work_q_yield",
				  "Wrong thread name");
	}

	zassert_true(k_work_queue_drain(&static_work_q, true) >= 0,
		     "Failed to drain & plug work queue");
	zassert_equal(k_work_queue_stop(&static_work_q, K_FOREVER), -ENOTSUP,
		      "Stopped an essential work queue");
	zassert_ok(k_work_queue_unplug(&static_work_q), "Failed to unplug work queue");

	zassert_true(k_work_queue_drain(&static_work_q_yield, true) >= 0,
		     "Failed to drain & plug work queue");
	zassert_equal(k_work_queue_stop(&static_work_q_yield, K_FOREVER), -ENOTSUP,
		      "Stopped an essential work queue");
	zassert_ok(k_work_queue_unplug(&static_work_q_yield), "Failed to unplug work queue");

	zassert_equal(k_work_submit_to_queue(&static_work_q_yield, &yield_work), 1,
		      "Failed to submit work item");
	zassert_ok(k_sem_take(&yield_work_sem, K_MSEC(100)), "Work item was not processed");
}
