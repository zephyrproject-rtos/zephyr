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
#define STATIC_WQ_NOCFG_PRIO K_PRIO_PREEMPT(5)

static const struct k_work_queue_config static_wq_cfg = {
	.name = "static_wq",
	.no_yield = true,
	.essential = true,
};

K_WORK_QUEUE_DEFINE(static_work_q, STATIC_WQ_STACK_SIZE, STATIC_WQ_PRIO, &static_wq_cfg);
K_WORK_QUEUE_DEFINE(static_work_q_nocfg, STATIC_WQ_STACK_SIZE, STATIC_WQ_NOCFG_PRIO, NULL);

static K_SEM_DEFINE(init_work_sem, 0, 1);
static K_SEM_DEFINE(nocfg_work_sem, 0, 1);

static int init_submit_rc;

static void init_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_sem_give(&init_work_sem);
}

static void nocfg_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_sem_give(&nocfg_work_sem);
}

static K_WORK_DEFINE(init_work, init_work_handler);
static K_WORK_DEFINE(nocfg_work, nocfg_work_handler);

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
 * submitted from a POST_KERNEL init function is accepted and processed, and
 * that the thread carries the requested priority, name and essential flag.
 *
 * Test steps:
 * - Submit a work item to the statically defined work queue from a
 *   SYS_INIT() function at POST_KERNEL level, priority 0.
 * - Wait for the work item to be processed.
 * - Read the work queue thread priority and name.
 * - Drain and plug the essential queue, then attempt to stop it.
 * - Submit and flush a work item on a queue defined without configuration.
 *
 * Expected result:
 * - The submission from the init function returns 1 and the handler runs.
 * - The thread priority is the one given to K_WORK_QUEUE_DEFINE(), the thread
 *   name is the configured one, or the work queue name when no configuration
 *   is given.
 * - k_work_queue_stop() returns -ENOTSUP on the essential queue.
 * - The queue defined without configuration processes work.
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
		zassert_str_equal(k_thread_name_get(tid), "static_wq", "Wrong thread name");
	}

	tid = k_work_queue_thread_get(&static_work_q_nocfg);
	zassert_not_null(tid, "Static work queue has no thread");
	zassert_equal(k_thread_priority_get(tid), STATIC_WQ_NOCFG_PRIO, "Wrong thread priority");
	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		zassert_str_equal(k_thread_name_get(tid), "static_work_q_nocfg",
				  "Wrong thread name");
	}

	zassert_true(k_work_queue_drain(&static_work_q, true) >= 0,
		     "Failed to drain & plug work queue");
	zassert_equal(k_work_queue_stop(&static_work_q, K_FOREVER), -ENOTSUP,
		      "Stopped an essential work queue");
	zassert_ok(k_work_queue_unplug(&static_work_q), "Failed to unplug work queue");

	zassert_equal(k_work_submit_to_queue(&static_work_q_nocfg, &nocfg_work), 1,
		      "Failed to submit work item");
	zassert_ok(k_sem_take(&nocfg_work_sem, K_MSEC(100)), "Work item was not processed");
}
