/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This test covers deprecated API.  Avoid inappropriate diagnostics
 * about the use of that API.
 */
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define COOPHI_PRIORITY K_PRIO_COOP(0) /* = -4 */
/* SYSTEM_WORKQUEUE_PRIORITY = -3 */
/* ZTEST_THREAD_PRIORITY = -2 */
#define COOPLO_PRIORITY K_PRIO_COOP(3) /* = -1 */
#define PREEMPT_PRIORITY K_PRIO_PREEMPT(1) /* = 1 */

#define DELAY_MS 100
#define DELAY_TIMEOUT K_MSEC(DELAY_MS)
#define DELAY_TOLERANCE_TICKS (IS_ENABLED(CONFIG_BOARD_QEMU_CORTEX_A9) ? 10U : 1U)

static uint32_t delay_max_ms(void)
{
	return k_ticks_to_ms_ceil32(DELAY_TOLERANCE_TICKS + k_ms_to_ticks_ceil32(DELAY_MS));
}

BUILD_ASSERT(COOPHI_PRIORITY < CONFIG_SYSTEM_WORKQUEUE_PRIORITY,
	     "COOPHI not higher priority than system workqueue");
BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < CONFIG_ZTEST_THREAD_PRIORITY,
	     "System workqueue not higher priority than ZTEST");
BUILD_ASSERT(CONFIG_ZTEST_THREAD_PRIORITY < COOPLO_PRIORITY,
	     "ZTEST not higher priority than COOPLO");
BUILD_ASSERT(COOPLO_PRIORITY < 0,
	     "COOPLO not cooperative");

/* Given by work thread to signal completion. */
static struct k_sem sync_sem;

static bool run_flag = true;

/* Given by test thread to release a work item. */
static struct k_sem rel_sem;

/* Common work structures, to avoid dead references to stack objects
 * if a test fails.
 */
static struct k_work common_work;
static struct k_work common_work1;
static struct k_work_delayable dwork;

/* Work synchronization objects must be in cache-coherent memory,
 * which excludes stacks on some architectures.
 */
static struct k_work_sync work_sync;

static struct k_thread *main_thread;

/* We have these threads, in strictly decreasing order of priority:
 * * coophi: a high priority cooperative work queue
 * * system: the standard system work queue
 * * ztest thread: priority for threads running tests
 * * cooplo : a low-priority cooperative work queue
 * * preempt: a preemptible work queue
 *
 * The test infrastructure records the number of times each work queue
 * executes in a counter.
 *
 * The common work handler also supports internal re-submission if
 * configured to do so.
 *
 * There are three core handlers:
 * * The basic one (counter_handler) increments the count of handler
 *   invocations by work queue thread, optionally resubmits, then
 *   releases the semaphore the test is waiting for.
 * * The blocking one (rel_handler) waits until something invokes
 *   handler_release() to allow it to complete by invoking
 *   counter_handler().  This makes a work queue busy for arbitrary
 *   periods, but requires something external to trigger the release.
 * * The delaying one (delay_handler) waits for K_MSEC(DELAY_MS) before
 *   invoking counter_handler().
 */
static atomic_t resubmits_left;

/* k_uptime_get32() on the last invocation of the core handler. */
static uint32_t volatile last_handle_ms;

static K_THREAD_STACK_DEFINE(coophi_stack, STACK_SIZE);
static struct k_work_q coophi_queue;
static struct k_work_q not_start_queue;
static atomic_t coophi_ctr;
static inline int coophi_counter(void)
{
	return atomic_get(&coophi_ctr);
}

static K_THREAD_STACK_DEFINE(cooplo_stack, STACK_SIZE);
static struct k_thread cooplo_thread;
static struct k_work_q cooplo_queue;
static atomic_t cooplo_ctr;
static inline int cooplo_counter(void)
{
	return atomic_get(&cooplo_ctr);
}

static inline int coop_counter(struct k_work_q *wq)
{
	return (wq == &coophi_queue) ? coophi_counter()
		: (wq == &cooplo_queue) ? cooplo_counter()
		: -1;
}

static K_THREAD_STACK_DEFINE(preempt_stack, STACK_SIZE);
static struct k_work_q preempt_queue;
static atomic_t preempt_ctr;
static inline int preempt_counter(void)
{
	return atomic_get(&preempt_ctr);
}

static K_THREAD_STACK_DEFINE(invalid_test_stack, STACK_SIZE);
static struct k_work_q invalid_test_queue;

static atomic_t system_ctr;
static inline int system_counter(void)
{
	return atomic_get(&system_ctr);
}

static inline void reset_counters(void)
{
	/* If this fails the previous test didn't clean up */
	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), -EBUSY);
	last_handle_ms = UINT32_MAX;
	atomic_set(&resubmits_left, 0);
	atomic_set(&coophi_ctr, 0);
	atomic_set(&system_ctr, 0);
	atomic_set(&cooplo_ctr, 0);
	atomic_set(&preempt_ctr, 0);
}

static void counter_handler(struct k_work *work)
{
	last_handle_ms = k_uptime_get_32();
	if (k_current_get() == coophi_queue.thread_id) {
		atomic_inc(&coophi_ctr);
	} else if (k_current_get() == k_sys_work_q.thread_id) {
		atomic_inc(&system_ctr);
	} else if (k_current_get() == cooplo_queue.thread_id) {
		atomic_inc(&cooplo_ctr);
	} else if (k_current_get() == preempt_queue.thread_id) {
		atomic_inc(&preempt_ctr);
	}
	if (atomic_dec(&resubmits_left) > 0) {
		(void)k_work_submit_to_queue(NULL, work);
	} else {
		k_sem_give(&sync_sem);
	}
}

static inline void handler_release(void)
{
	k_sem_give(&rel_sem);
}

static void async_release_cb(struct k_timer *timer)
{
	handler_release();
}

static K_TIMER_DEFINE(async_releaser, async_release_cb, NULL);

static inline void async_release(void)
{
	k_timer_start(&async_releaser, K_TICKS(1), K_NO_WAIT);
}

static void rel_handler(struct k_work *work)
{
	(void)k_sem_take(&rel_sem, K_FOREVER);
	counter_handler(work);
}

static void delay_handler(struct k_work *work)
{
	k_sleep(K_MSEC(DELAY_MS));
	counter_handler(work);
}

/**
 * @brief Verify K_WORK_DEFINE() and k_work_init() produce the same work item.
 *
 * @details
 * A work item can be defined statically or initialized at run time, and the two
 * forms must be interchangeable, so code that uses one is not subtly different
 * from code that uses the other.
 *
 * Test steps:
 * - Define a work item statically with K_WORK_DEFINE().
 * - Initialize a second item with k_work_init() using the same handler.
 * - Compare the two structures.
 *
 * Expected result:
 * - The statically defined and run-time initialized items are identical.
 *
 * @ingroup kernel_workqueue_tests
 * @see K_WORK_DEFINE()
 * @see k_work_init()
 */
ZTEST(work, test_workq_work_define_matches_init)
{
	static K_WORK_DEFINE(fnstat, counter_handler);

	static struct k_work stack;

	k_work_init(&stack, counter_handler);
	zassert_mem_equal(&stack, &fnstat, sizeof(stack),
			  NULL);
}

/**
 * @brief Verify K_WORK_DELAYABLE_DEFINE() and k_work_init_delayable() produce
 * the same delayable work item.
 *
 * @details
 * As for plain work items, the static and run-time forms of initializing a
 * delayable work item must be interchangeable.
 *
 * Test steps:
 * - Define a delayable work item statically with K_WORK_DELAYABLE_DEFINE().
 * - Initialize a second item with k_work_init_delayable() using the same
 *   handler.
 * - Compare the two structures.
 *
 * Expected result:
 * - The statically defined and run-time initialized items are identical.
 *
 * @ingroup kernel_workqueue_tests
 * @see K_WORK_DELAYABLE_DEFINE()
 * @see k_work_init_delayable()
 */
ZTEST(work, test_workq_delayable_define_matches_init)
{
	static K_WORK_DELAYABLE_DEFINE(fnstat, counter_handler);

	static struct k_work_delayable stack;

	k_work_init_delayable(&stack, counter_handler);
	zassert_mem_equal(&stack, &fnstat, sizeof(stack),
			  NULL);
}

/**
 * @brief Verify submitting to a work queue that was never started fails.
 *
 * @details
 * A work queue that has never been started has no thread to run handlers, so a
 * submission must be rejected outright rather than accepted and never run.
 *
 * Test steps:
 * - Initialize a work item and confirm it reports no busy flags.
 * - Submit it to a work queue that was never started.
 *
 * Expected result:
 * - k_work_submit_to_queue() returns -ENODEV.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_busy_get()
 * @see k_work_submit_to_queue()
 */
ZTEST(work, test_workq_submit_unstarted)
{
	int rc;

	k_work_init(&common_work, counter_handler);
	zassert_equal(k_work_busy_get(&common_work), 0);

	rc = k_work_submit_to_queue(&not_start_queue, &common_work);
	zassert_equal(rc, -ENODEV);
}

static void cooplo_main(void *workq_ptr, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct k_work_q *queue = (struct k_work_q *)workq_ptr;

	struct k_work_queue_config cfg = {
		.name = "wq.cooplo",
		.no_yield = true,
	};

	k_work_queue_run(queue, &cfg);
}

/* Start the work queues the test cases submit to. This has to happen in the
 * suite setup because nearly every case needs the queues already running; the
 * observable results of starting them are checked by
 * test_workq_queue_start_config().
 */
static void start_test_queues(void)
{
	struct k_work_queue_config cfg = {
		.name = "wq.preempt",
	};
	k_work_queue_init(&preempt_queue);
	zassert_equal(preempt_queue.flags, 0);
	k_work_queue_start(&preempt_queue, preempt_stack, STACK_SIZE,
			    PREEMPT_PRIORITY, &cfg);

	cfg.name = "wq.coophi";
	cfg.no_yield = true;
	k_work_queue_start(&coophi_queue, coophi_stack, STACK_SIZE,
			    COOPHI_PRIORITY, &cfg);
	zassert_equal(coophi_queue.flags,
		      K_WORK_QUEUE_STARTED | K_WORK_QUEUE_NO_YIELD, NULL);

	(void)k_thread_create(&cooplo_thread, cooplo_stack, STACK_SIZE, cooplo_main, &cooplo_queue,
			      NULL, NULL, COOPLO_PRIORITY, 0, K_FOREVER);

	k_thread_start(&cooplo_thread);

	/* Be sure the cooplo_thread has a chance to start running */
	k_msleep(1);

	zassert_equal(cooplo_queue.flags,
		      K_WORK_QUEUE_STARTED | K_WORK_QUEUE_NO_YIELD, NULL);
}

/**
 * @brief Verify a started work queue reflects its start configuration.
 *
 * @details
 * k_work_queue_start() records the requested configuration on the queue: the
 * started flag, the no-yield option, and the thread name. A NULL name in the
 * configuration is not an error and must leave the queue thread with an empty
 * name rather than a dangling pointer into the caller's configuration.
 *
 * Test steps:
 * - Check the flags and thread name of the queues started by the suite setup,
 *   both with and without the no-yield option.
 * - Initialize a further queue and confirm k_work_queue_init() leaves its flags
 *   clear.
 * - Start that queue with a NULL name in its configuration.
 *
 * Expected result:
 * - Every started queue reports K_WORK_QUEUE_STARTED, queues started with the
 *   no-yield option also report K_WORK_QUEUE_NO_YIELD, a named queue's thread
 *   carries a copy of the name rather than the caller's pointer, and the queue
 *   started with a NULL name has an empty thread name.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_init()
 * @see k_work_queue_start()
 * @see k_thread_name_get()
 */
ZTEST(work, test_workq_queue_start_config)
{
	struct k_work_queue_config cfg = {
		.name = NULL,
	};

	zassert_equal(preempt_queue.flags, K_WORK_QUEUE_STARTED);
	zassert_equal(coophi_queue.flags,
		      K_WORK_QUEUE_STARTED | K_WORK_QUEUE_NO_YIELD, NULL);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		const char *tn = k_thread_name_get(preempt_queue.thread_id);

		zassert_true(tn != NULL);
		zassert_str_equal(tn, "wq.preempt");
	}

	/* A NULL name is accepted and leaves the thread name empty. */
	zassert_equal(invalid_test_queue.flags, 0);
	k_work_queue_start(&invalid_test_queue, invalid_test_stack, STACK_SIZE,
			    PREEMPT_PRIORITY, &cfg);
	zassert_equal(invalid_test_queue.flags, K_WORK_QUEUE_STARTED);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		const char *tn = k_thread_name_get(invalid_test_queue.thread_id);

		zassert_true(tn != cfg.name);
		zassert_true(tn != NULL);
		zassert_str_equal(tn, "");
	}
}

/**
 * @brief Verify submitting to a NULL work queue is rejected.
 *
 * @details
 * k_work_submit_to_queue() names the queue explicitly, and passing no queue at
 * all is a caller error that must be reported rather than dereferenced.
 *
 * Test steps:
 * - Initialize a work item and confirm it reports no busy flags.
 * - Submit it with a NULL queue pointer.
 *
 * Expected result:
 * - k_work_submit_to_queue() returns -EINVAL.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_busy_get()
 * @see k_work_submit_to_queue()
 */
ZTEST(work, test_workq_submit_null_queue)
{
	int rc;

	k_work_init(&common_work, counter_handler);
	zassert_equal(k_work_busy_get(&common_work), 0);

	rc = k_work_submit_to_queue(NULL, &common_work);
	zassert_equal(rc, -EINVAL);
}

/**
 * @brief Verify a work item submitted to a started queue runs when the
 * submitting thread yields.
 *
 * @details
 * The test thread is cooperative and higher priority than the queue thread, so
 * a submitted item must stay queued until the test thread blocks, and then run
 * exactly once and return the item to idle.
 *
 * Test steps:
 * - Initialize a work item with a non-blocking handler and submit it to the
 *   cooperative queue.
 * - Check the item is reported queued and pending, and has not run.
 * - Sleep to let the queue thread run.
 *
 * Expected result:
 * - The handler runs exactly once while the test thread sleeps, and the item
 *   afterwards reports no busy flags.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_busy_get()
 * @see k_work_is_pending()
 * @see k_work_submit_to_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_simple_queue)
{
	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&common_work, counter_handler);
	zassert_equal(k_work_busy_get(&common_work), 0);
	zassert_equal(k_work_is_pending(&common_work), false);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(&common_work), K_WORK_QUEUED);
	zassert_equal(k_work_is_pending(&common_work), true);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0);

	/* Let it run, then check it finished. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 1);
	zassert_equal(k_work_busy_get(&common_work), 0);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/**
 * @brief Verify work submitted while the queue thread is suspended runs on
 * resume.
 *
 * @details
 * Work submitted while the work queue's thread is suspended must be accepted
 * and stay queued, not lost or rejected, and must run as soon as the thread
 * is resumed.
 *
 * Test steps:
 * - Look up the queue's thread with k_work_queue_thread_get() and suspend it.
 * - Submit a work item and verify it is accepted and reported as queued.
 * - Sleep and verify the item has not run and is still pending.
 * - Resume the queue thread.
 *
 * Expected result:
 * - The handler runs exactly once after the resume, and the item ends idle.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_submit_to_queue()
 * @see k_work_queue_thread_get()
 * @see k_thread_suspend()
 * @see k_thread_resume()
 */
ZTEST(work_1cpu, test_workq_1cpu_suspend_resume_queue)
{
	int rc;
	k_tid_t wq_tid = k_work_queue_thread_get(&coophi_queue);

	zassert_not_null(wq_tid);

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&common_work, counter_handler);
	zassert_equal(k_work_busy_get(&common_work), 0);

	/* Suspend the workqueue's runner thread. */
	k_thread_suspend(wq_tid);

	/* Submission should still succeed and the work should be queued. */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(&common_work), K_WORK_QUEUED);
	zassert_equal(k_work_is_pending(&common_work), true);

	/* Sleeping must not let the work run while the runner is suspended. */
	k_sleep(K_MSEC(DELAY_MS));
	zassert_equal(coophi_counter(), 0);
	zassert_equal(k_work_busy_get(&common_work), K_WORK_QUEUED);
	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), -EBUSY);

	/* Resuming the runner should drain the pending work. */
	k_thread_resume(wq_tid);

	rc = k_sem_take(&sync_sem, DELAY_TIMEOUT);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 1);
	zassert_equal(k_work_busy_get(&common_work), 0);
}

/**
 * @brief Verify work submitted on SMP runs without the submitter yielding.
 *
 * @details
 * With more than one CPU the queue thread can run the handler concurrently with
 * the submitting thread, so the item must complete even though the test thread
 * never blocks. Skipped when the configuration has a single CPU.
 *
 * Test steps:
 * - Initialize a work item with a non-blocking handler and submit it to the
 *   cooperative queue.
 * - Busy-poll the item's pending state without sleeping.
 *
 * Expected result:
 * - The handler runs exactly once and the item reports no busy flags, without
 *   the test thread yielding.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_busy_get()
 * @see k_work_is_pending()
 * @see k_work_submit_to_queue()
 */
ZTEST(work, test_workq_smp_simple_queue)
{
	if (!IS_ENABLED(CONFIG_SMP) || (CONFIG_MP_MAX_NUM_CPUS == 1)) {
		ztest_test_skip();
		return;
	}

	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&common_work, counter_handler);
	zassert_equal(k_work_busy_get(&common_work), 0);
	zassert_equal(k_work_is_pending(&common_work), false);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);

	/* It should run and finish without this thread yielding. */
	int64_t ts0 = k_uptime_ticks();
	uint32_t delay;

	do {
		delay = k_ticks_to_ms_floor32(k_uptime_ticks() - ts0);
	} while (k_work_is_pending(&common_work) && (delay < DELAY_MS));

	zassert_equal(k_work_busy_get(&common_work), 0);
	zassert_equal(coophi_counter(), 1);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/**
 * @brief Verify the busy state of a work item whose handler blocks.
 *
 * @details
 * A work item reports K_WORK_QUEUED while it waits for the queue thread and
 * K_WORK_RUNNING once its handler has started, so a handler that blocks leaves
 * the item running until something releases it.
 *
 * Test steps:
 * - Submit a work item whose handler blocks on a semaphore.
 * - Check it reports queued, then sleep and check it reports running without
 *   having completed.
 * - Release the handler and wait for completion.
 *
 * Expected result:
 * - The item is queued before the queue thread runs, running while its handler
 *   is blocked, and its handler completes exactly once after the release.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_busy_get()
 * @see k_work_submit_to_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_sync_queue)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&common_work, rel_handler);
	zassert_equal(k_work_busy_get(&common_work), 0);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(&common_work), K_WORK_QUEUED);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);
	zassert_equal(k_work_busy_get(&common_work), K_WORK_RUNNING);

	/* Make it ready so it can finish when this thread yields. */
	handler_release();
	zassert_equal(coophi_counter(), 0);

	/* Wait for then verify finish */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 1);
}

/**
 * @brief Verify resubmitting a running item diverts it to the queue it runs on.
 *
 * @details
 * A handler must never be entered twice concurrently. If an item that is
 * already running is submitted to a different queue, the submission is
 * redirected to the queue currently running it, so the second invocation
 * happens after the first one returns instead of in parallel.
 *
 * Test steps:
 * - Submit a blocking work item to the cooperative queue and let it start.
 * - Submit the same item to the preemptible queue while it is running.
 * - Release the handler twice and count where the invocations ran.
 *
 * Expected result:
 * - The second submission is accepted and both invocations run on the
 *   cooperative queue, never on the preemptible one.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_reentrant_queue)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&common_work, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Release it so it's running and can be rescheduled. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);

	/* Resubmit to a different queue. */
	rc = k_work_submit_to_queue(&preempt_queue, &common_work);
	zassert_equal(rc, 2);

	/* Release the first submission. */
	handler_release();
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 1);

	/* Confirm the second submission was redirected to the running
	 * queue to avoid re-entrancy problems.
	 */
	handler_release();
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 2);
}

/**
 * @brief Verify k_work_flush() waits for a queued item to complete.
 *
 * @details
 * Two items with delaying handlers are submitted to a cooperative queue so
 * neither has started when the test flushes the second one. The flush must
 * block until that item's handler has run, which also implies the item
 * queued ahead of it completed first.
 *
 * Test steps:
 * - Submit two work items with delaying handlers to the cooperative queue.
 * - Verify both are still queued and no handler has run.
 * - Call k_work_flush() on the second item.
 *
 * Expected result:
 * - The flush returns only after both handlers have completed, in
 *   submission order.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_flush()
 * @see k_work_submit_to_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_queued_flush)
{
	int rc;

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&common_work, delay_handler);
	k_work_init(&common_work1, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work1);
	zassert_equal(rc, 1);
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Confirm that it's still in the queue, then wait for completion.
	 * This should wait.
	 */
	zassert_equal(k_work_busy_get(&common_work), K_WORK_QUEUED);
	zassert_equal(k_work_busy_get(&common_work1), K_WORK_QUEUED);
	zassert_true(k_work_flush(&common_work, &work_sync));
	zassert_false(k_work_flush(&common_work1, &work_sync));

	/* Verify completion. */
	zassert_equal(coophi_counter(), 2);
	zassert_true(!k_work_is_pending(&common_work));
	zassert_true(!k_work_is_pending(&common_work1));
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* After completion flush should be a no-op */
	zassert_false(k_work_flush(&common_work, &work_sync));
	zassert_false(k_work_flush(&common_work1, &work_sync));
}

/**
 * @brief Verify flushing a work item whose handler is already running waits.
 *
 * @details
 * k_work_flush() must block until the handler has returned, whether the item is
 * still queued or already running. Here the handler is started first and sleeps,
 * so the flush cannot be satisfied immediately.
 *
 * Test steps:
 * - Submit a work item with a delaying handler and let it start running.
 * - Confirm the item reports running and has not completed.
 * - Call k_work_flush() on it.
 *
 * Expected result:
 * - The flush reports that waiting was required and returns only after the
 *   handler has completed.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_work_busy_get()
 * @see k_work_flush()
 */
ZTEST(work_1cpu, test_workq_1cpu_running_flush)
{
	int rc;

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&common_work, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);
	zassert_equal(k_work_busy_get(&common_work), K_WORK_QUEUED);

	/* Release it so it's running. */
	k_sleep(K_TICKS(1));
	zassert_equal(k_work_busy_get(&common_work), K_WORK_RUNNING);
	zassert_equal(coophi_counter(), 0);

	/* Wait for completion.  This should be released by the delay
	 * handler.
	 */
	zassert_true(k_work_flush(&common_work, &work_sync));

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/**
 * @brief Verify flushing a delayable work item runs it without its delay.
 *
 * @details
 * k_work_flush_delayable() completes immediately for an item that is not
 * scheduled. For an item waiting on a delay it must submit the item right away
 * and wait for the handler, rather than blocking for the remaining delay.
 *
 * Test steps:
 * - Flush an unscheduled delayable work item.
 * - Schedule the item for a future delay, then flush it and record when its
 *   handler ran relative to the flush.
 *
 * Expected result:
 * - Flushing the unscheduled item reports no wait, and flushing the scheduled
 *   item runs the handler immediately rather than after the remaining delay.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_flush_delayable()
 * @see k_work_schedule_for_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_delayed_flush)
{
	int rc;
	uint32_t flush_ms;
	uint32_t wait_ms;

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);

	/* Unscheduled completes immediately. */
	zassert_false(k_work_flush_delayable(&dwork, &work_sync));

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Align to tick then flush. */
	k_sleep(K_TICKS(1));
	flush_ms = k_uptime_get_32();
	zassert_true(k_work_flush_delayable(&dwork, &work_sync));
	wait_ms = last_handle_ms - flush_ms;
	zassert_true(wait_ms <= 1, "waited %u", wait_ms);

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/**
 * @brief Verify cancelling a work item before the queue thread dequeues it.
 *
 * @details
 * An item that is still waiting in the queue has not started, so cancelling it
 * can take effect at once: the cancel reports nothing left busy and the handler
 * never runs.
 *
 * Test steps:
 * - Submit a work item to the cooperative queue without letting it run.
 * - Call k_work_cancel() on it.
 *
 * Expected result:
 * - The cancel returns 0 (nothing still busy) and the handler is never invoked.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_work_cancel()
 */
ZTEST(work_1cpu, test_workq_1cpu_queued_cancel)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&common_work, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Cancellation should complete immediately. */
	zassert_equal(k_work_cancel(&common_work), 0);

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0);
}

/**
 * @brief Verify a synchronous cancel of a queued item does not wait.
 *
 * @details
 * k_work_cancel_sync() reports whether the item was pending and had to be
 * waited for. For an item that never started there is nothing to wait for, and
 * for an item that was never submitted there is nothing to cancel at all.
 *
 * Test steps:
 * - Call k_work_cancel_sync() on an item that was never submitted.
 * - Submit the item to the cooperative queue without letting it run.
 * - Call k_work_cancel_sync() again.
 *
 * Expected result:
 * - The first call reports the item was not pending, the second reports it was,
 *   and the handler never runs.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_cancel_sync()
 * @see k_work_submit_to_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_queued_cancel_sync)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&common_work, rel_handler);

	/* Cancel an unqueued work item should not affect the work
	 * and return false.
	 */
	zassert_false(k_work_cancel_sync(&common_work, &work_sync));

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Cancellation should complete immediately, indicating that
	 * work was pending.
	 */
	zassert_true(k_work_cancel_sync(&common_work, &work_sync));

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0);
}

/**
 * @brief Verify cancelling a delayable work item before its delay elapses.
 *
 * @details
 * A scheduled item that has not yet been submitted to its queue can be
 * cancelled outright: the timeout is stopped, nothing is left busy and the
 * handler never runs.
 *
 * Test steps:
 * - Schedule a delayable work item for a future delay.
 * - Call k_work_cancel_delayable() before the delay elapses.
 *
 * Expected result:
 * - The cancel returns 0 (nothing still busy) and the handler is never invoked.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_schedule_for_queue()
 * @see k_work_cancel_delayable()
 */
ZTEST(work_1cpu, test_workq_1cpu_delayed_cancel)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Cancellation should complete immediately. */
	zassert_equal(k_work_cancel_delayable(&dwork), 0);

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0);
}


/**
 * @brief Verify a synchronous cancel of a scheduled delayable item.
 *
 * @details
 * k_work_cancel_delayable_sync() reports whether the item was pending. An item
 * that was never scheduled is not pending, while one waiting on a delay is, and
 * neither case has a running handler to wait for.
 *
 * Test steps:
 * - Call k_work_cancel_delayable_sync() on an item that was never scheduled.
 * - Schedule the item for a future delay.
 * - Call k_work_cancel_delayable_sync() again.
 *
 * Expected result:
 * - The first call reports the item was not pending, the second reports it was,
 *   and the handler never runs.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_cancel_delayable_sync()
 * @see k_work_schedule_for_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_delayed_cancel_sync)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, rel_handler);

	/* Cancel an unqueued delayable work item should not affect the work
	 * and return false.
	 */
	zassert_false(k_work_cancel_delayable_sync(&dwork, &work_sync));

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Cancellation should complete immediately, indicating that
	 * work was pending.
	 */
	zassert_true(k_work_cancel_delayable_sync(&dwork, &work_sync));

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0);
}

/**
 * @brief Verify a synchronous cancel waits for a running delayable item.
 *
 * @details
 * Once a delayable item's handler has started, cancelling it cannot unschedule
 * anything: the synchronous cancel must block until that handler returns, so
 * the caller knows the handler is no longer executing.
 *
 * Test steps:
 * - Schedule a delayable item with no delay and let its blocking handler start.
 * - Confirm the item reports running.
 * - Arrange for the handler to be released asynchronously, then call
 *   k_work_cancel_delayable_sync().
 *
 * Expected result:
 * - The cancel reports that waiting was required and returns only after the
 *   handler has completed.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_schedule_for_queue()
 * @see k_work_delayable_busy_get()
 * @see k_work_cancel_delayable_sync()
 */
ZTEST(work_1cpu, test_workq_1cpu_delayed_cancel_sync_wait)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_QUEUED);
	zassert_equal(coophi_counter(), 0);

	/* Get it to running, where it will block. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_RUNNING);

	/* Schedule to release, then cancel should delay. */
	async_release();
	zassert_true(k_work_cancel_delayable_sync(&dwork, &work_sync));

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/* Infrastructure to capture behavior of work item that's being
 * cancelled.
 */
struct test_running_cancel_timer {
	struct k_timer timer;
	struct k_work work;
	int submit_rc;
	int busy_rc;
};

static struct test_running_cancel_timer test_running_cancel_ctx;

static void running_cancel_timer_cb(struct k_timer *timer)
{
	struct test_running_cancel_timer *ctx =
		CONTAINER_OF(timer, struct test_running_cancel_timer, timer);

	ctx->busy_rc = k_work_busy_get(&ctx->work);
	ctx->submit_rc = k_work_submit_to_queue(&coophi_queue, &ctx->work);
	handler_release();
}

/**
 * @brief Verify a non-blocking cancel of a running item reports it as
 * cancelling.
 *
 * @details
 * k_work_cancel() never waits. If the handler is already running the item
 * enters the cancelling state, which the handler itself can observe, and any
 * submission attempted while cancellation is in progress is refused.
 *
 * Test steps:
 * - Submit a work item with a blocking handler and let it start running.
 * - Call k_work_cancel() and check the returned busy state.
 * - From a timer callback, read the item's busy state, attempt to resubmit it,
 *   and release the handler.
 * - Wait out the cancellation with k_work_cancel_sync().
 *
 * Expected result:
 * - k_work_cancel() reports the item running and cancelling, the resubmission
 *   is refused with -EBUSY, and once cancellation completes the item reports no
 *   busy flags.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_work_cancel()
 * @see k_work_cancel_sync()
 */
ZTEST(work_1cpu, test_workq_1cpu_running_cancel)
{
	struct test_running_cancel_timer *ctx = &test_running_cancel_ctx;
	struct k_work *wp = &ctx->work;
	static const uint32_t ms_timeout = 10;
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(wp, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, wp);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Release it so it's running. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);

	/* Schedule the async process to capture state and release work. */
	ctx->submit_rc = INT_MAX;
	ctx->busy_rc = INT_MAX;
	k_timer_init(&ctx->timer, running_cancel_timer_cb, NULL);
	k_timer_start(&ctx->timer, K_MSEC(ms_timeout), K_NO_WAIT);

	/* Cancellation should not complete. */
	zassert_equal(k_work_cancel(wp), K_WORK_RUNNING | K_WORK_CANCELING,
		      NULL);

	/* Handler should not have run. */
	zassert_equal(coophi_counter(), 0);

	/* Busy wait until timer expires. Thread context is blocked so cancelling
	 * of work won't be completed. Add one tick of slack on top of the
	 * nominal timeout for the +1 round-up inside z_add_timeout() plus
	 * 1 ms for general measurement jitter.
	 */
	k_busy_wait(1000 * ms_timeout + k_ticks_to_us_ceil32(1) + 1000);

	zassert_equal(k_timer_status_get(&ctx->timer), 1);

	/* Wait for cancellation to complete. */
	zassert_true(k_work_cancel_sync(wp, &work_sync));

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* Handler should have detected running and canceling. */
	zassert_equal(ctx->busy_rc, K_WORK_RUNNING | K_WORK_CANCELING);

	/* Attempt to submit while cancelling should have been
	 * rejected.
	 */
	zassert_equal(ctx->submit_rc, -EBUSY);

	/* Post-cancellation should have no flags. */
	rc = k_work_busy_get(wp);
	zassert_equal(rc, 0, "bad: %d", rc);
}

/**
 * @brief Verify k_work_cancel_sync() waits for a running item to finish.
 *
 * @details
 * A work item whose handler blocks is started on a cooperative queue, then
 * cancelled synchronously. Because the handler is already running the
 * cancellation cannot take effect immediately: the call must report that
 * waiting was required and return only after the handler has completed,
 * while resubmission during cancellation is rejected.
 *
 * Test steps:
 * - Submit a work item with a blocking handler and let it start running.
 * - Call k_work_cancel_sync() on the running item from a timer-driven
 *   context that also releases the handler.
 * - Attempt to resubmit the item while cancellation is in progress.
 *
 * Expected result:
 * - k_work_cancel_sync() returns true (waiting was required), the
 *   resubmission is rejected with -EBUSY, and the item ends with no busy
 *   flags set.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_cancel_sync()
 * @see k_work_busy_get()
 */
ZTEST(work_1cpu, test_workq_1cpu_running_cancel_sync)
{
	struct test_running_cancel_timer *ctx = &test_running_cancel_ctx;
	struct k_work *wp = &ctx->work;
	static const uint32_t ms_timeout = 10;
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(wp, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, wp);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Release it so it's running. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);

	/* Schedule the async process to capture state and release work. */
	ctx->submit_rc = INT_MAX;
	ctx->busy_rc = INT_MAX;
	k_timer_init(&ctx->timer, running_cancel_timer_cb, NULL);
	k_timer_start(&ctx->timer, K_MSEC(ms_timeout), K_NO_WAIT);

	/* Cancellation should wait. */
	zassert_true(k_work_cancel_sync(wp, &work_sync));

	/* Handler should have run. */
	zassert_equal(coophi_counter(), 1);

	/* Busy wait until timer expires. Thread context is blocked so cancelling
	 * of work won't be completed. Add one tick of slack on top of the
	 * nominal timeout for the +1 round-up inside z_add_timeout() plus
	 * 1 ms for general measurement jitter.
	 */
	k_busy_wait(1000 * ms_timeout + k_ticks_to_us_ceil32(1) + 1000);

	zassert_equal(k_timer_status_get(&ctx->timer), 1);

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* Handler should have detected running and canceling. */
	zassert_equal(ctx->busy_rc, K_WORK_RUNNING | K_WORK_CANCELING,
		      NULL);

	/* Attempt to submit while cancelling should have been
	 * rejected.
	 */
	zassert_equal(ctx->submit_rc, -EBUSY);

	/* Post-cancellation should have no flags. */
	rc = k_work_busy_get(wp);
	zassert_equal(rc, 0, "bad: %d", rc);
}

/**
 * @brief Verify cancelling a running item on SMP requires a synchronous wait.
 *
 * @details
 * On SMP the work queue runs the handler concurrently with the test thread.
 * Once the item is observed running, a plain k_work_cancel() must report the
 * item still busy, and k_work_cancel_sync() must wait for the handler to
 * complete before returning.
 *
 * Test steps:
 * - Submit a work item with a delaying handler and busy-wait until it is
 *   reported running on another CPU.
 * - Call k_work_cancel() and check the returned busy state.
 * - Call k_work_cancel_sync() and verify it reports having waited.
 *
 * Expected result:
 * - The running item cannot be cancelled without waiting; after the
 *   synchronous cancel completes the item reports no busy flags.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_cancel()
 * @see k_work_cancel_sync()
 * @see k_work_busy_get()
 */
ZTEST(work, test_workq_smp_running_cancel)
{
	int rc;

	if (!IS_ENABLED(CONFIG_SMP) || (CONFIG_MP_MAX_NUM_CPUS == 1)) {
		ztest_test_skip();
		return;
	}

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&common_work, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);

	/* It should advance to running without this thread yielding. */
	int64_t ts0 = k_uptime_ticks();
	uint32_t delay;

	do {
		delay = k_ticks_to_ms_floor32(k_uptime_ticks() - ts0);
	} while ((k_work_busy_get(&common_work) != K_WORK_RUNNING)
		 && (delay < DELAY_MS));

	/* Cancellation should not succeed immediately because the
	 * work is running.
	 */
	rc = k_work_cancel(&common_work);
	zassert_equal(rc, K_WORK_RUNNING | K_WORK_CANCELING, "rc %x", rc);

	/* Sync should wait. */
	zassert_equal(k_work_cancel_sync(&common_work, &work_sync), true);

	/* Should have completed. */
	zassert_equal(coophi_counter(), 1);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/**
 * @brief Verify draining a work queue with nothing queued returns at once.
 *
 * @details
 * Draining waits for the queue to become empty. A queue that is already empty
 * has nothing to wait for, so the call must return immediately and report that
 * no items were pending.
 *
 * Test steps:
 * - Call k_work_queue_drain() on a queue with no queued or running items.
 *
 * Expected result:
 * - The drain returns 0, meaning nothing was pending.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_drain()
 */
ZTEST(work, test_workq_drain_empty)
{
	int rc;

	rc = k_work_queue_drain(&coophi_queue, false);
	zassert_equal(rc, 0);
}

struct test_drain_wait_timer {
	struct k_timer timer;
	struct k_work work;
	int submit_rc;
};

static struct test_drain_wait_timer test_drain_wait_ctx;

static void drain_wait_timer_cb(struct k_timer *timer)
{
	struct test_drain_wait_timer *ctx =
		CONTAINER_OF(timer, struct test_drain_wait_timer, timer);

	ctx->submit_rc = k_work_submit_to_queue(&coophi_queue, &ctx->work);
}

/**
 * @brief Verify draining waits for queued work and blocks new submissions.
 *
 * @details
 * While a drain is in progress the queue keeps processing what it already has,
 * including items a handler chains onto itself, but submissions from outside
 * are refused so the queue can actually reach empty.
 *
 * Test steps:
 * - Submit an item whose handler resubmits itself once, then start a drain.
 * - From a timer callback, attempt to submit another item while the drain is
 *   running.
 *
 * Expected result:
 * - The drain reports one item pending and returns after both chained
 *   invocations have run, while the external submission is refused with
 *   -EBUSY.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_work_queue_drain()
 */
ZTEST(work_1cpu, test_workq_1cpu_drain_wait)
{
	struct test_drain_wait_timer *ctx = &test_drain_wait_ctx;
	struct k_work *wp = &ctx->work;
	int rc;

	/* Reset state, allow one re-submission, and use the delaying
	 * handler.
	 */
	reset_counters();
	atomic_set(&resubmits_left, 1);
	k_work_init(wp, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, wp);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Schedule the async process to capture submission state
	 * while draining.
	 */
	ctx->submit_rc = INT_MAX;
	k_timer_init(&ctx->timer, drain_wait_timer_cb, NULL);
	k_timer_start(&ctx->timer, K_MSEC(10), K_NO_WAIT);

	/* Wait to drain */
	rc = k_work_queue_drain(&coophi_queue, false);
	zassert_equal(rc, 1);

	/* Wait until timer expires. */
	(void)k_timer_status_sync(&ctx->timer);

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* Confirm that chained submission worked, and non-chained
	 * submission failed.
	 */
	zassert_equal(coophi_counter(), 2);
	zassert_equal(ctx->submit_rc, -EBUSY);
}

/**
 * @brief Verify a queue plugged by a drain refuses work until it is unplugged.
 *
 * @details
 * Draining with the plug option leaves the queue closed to new work once it is
 * empty, so a caller can be sure nothing else starts running before it acts.
 * The queue accepts work again only after an explicit unplug.
 *
 * Test steps:
 * - Submit an item, then drain the queue with the plug option.
 * - Check the queue's plugged state and try to submit another item.
 * - Unplug the queue, unplug it a second time, and submit again.
 *
 * Expected result:
 * - Submission to the plugged queue is refused with -EBUSY, the first unplug
 *   succeeds and the redundant one returns -EALREADY, and submission succeeds
 *   and completes once the queue is unplugged.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_work_queue_drain()
 * @see k_work_queue_unplug()
 */
ZTEST(work_1cpu, test_workq_1cpu_plugged_drain)
{
	int rc;

	/* Reset state and use the delaying handler. */
	reset_counters();
	k_work_init(&common_work, delay_handler);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);

	/* Wait to drain, and plug. */
	rc = k_work_queue_drain(&coophi_queue, true);
	zassert_equal(rc, 1);

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 1);

	/* Queue should be plugged */
	zassert_equal(coophi_queue.flags,
		      K_WORK_QUEUE_STARTED
		      | K_WORK_QUEUE_PLUGGED
		      | K_WORK_QUEUE_NO_YIELD,
		      NULL);

	/* Switch to the non-blocking handler. */
	k_work_init(&common_work, counter_handler);

	/* Resubmission should fail because queue is plugged */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, -EBUSY);

	/* Unplug the queue */
	rc = k_work_queue_unplug(&coophi_queue);
	zassert_equal(rc, 0);

	/* Unplug the unplugged queue should not affect the queue */
	rc = k_work_queue_unplug(&coophi_queue);
	zassert_equal(rc, -EALREADY);
	zassert_equal(coophi_queue.flags,
		      K_WORK_QUEUE_STARTED | K_WORK_QUEUE_NO_YIELD,
		      NULL);

	/* Resubmission should succeed and complete */
	rc = k_work_submit_to_queue(&coophi_queue, &common_work);
	zassert_equal(rc, 1);

	/* Flush the sync state and verify completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 2);
}

/**
 * @brief Verify a scheduled delayable item runs only after its delay elapses.
 *
 * @details
 * Scheduling a delayable item starts a timeout rather than queueing it, so the
 * item reports the delayed state until the timeout fires, and the handler runs
 * no earlier than the requested delay. A second schedule while the first is
 * still waiting must not change the pending timeout.
 *
 * Test steps:
 * - Schedule an idle delayable item for a known delay and check it reports
 *   delayed and pending.
 * - Schedule it again with no delay while it is still waiting.
 * - Wait for the handler and measure the elapsed time since scheduling.
 *
 * Expected result:
 * - The second schedule is a no-op, the handler runs once after at least the
 *   requested delay and within the accepted tolerance, and the item ends idle.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_busy_get()
 * @see k_work_schedule_for_queue()
 * @see k_work_delayable_busy_get()
 */
ZTEST(work_1cpu, test_workq_1cpu_basic_schedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = delay_max_ms();
	uint32_t elapsed_ms;
	struct k_work *wp = &dwork.work; /* whitebox testing */

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);

	/* Verify that work is idle and marked delayable. */
	zassert_equal(k_work_busy_get(wp), 0);
	zassert_equal(wp->flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Align to tick, then schedule after normal delay. */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);
	rc = k_work_busy_get(wp);
	zassert_equal(rc, K_WORK_DELAYED);
	zassert_equal(k_work_delayable_busy_get(&dwork), rc);
	zassert_equal(k_work_delayable_is_pending(&dwork), true);

	/* Scheduling again does nothing. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Make sure it ran and is now idle */
	zassert_equal(coophi_counter(), 1);
	zassert_equal(k_work_busy_get(wp), 0);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

struct state_1cpu_basic_schedule_running {
	struct k_work_delayable dwork;
	int schedule_res;
};

static void handle_1cpu_basic_schedule_running(struct k_work *work)
{
	struct k_work_delayable *one_dwork = k_work_delayable_from_work(work);
	struct state_1cpu_basic_schedule_running *state
		= CONTAINER_OF(one_dwork, struct state_1cpu_basic_schedule_running,
			       dwork);

	/* Co-opt the resubmits so we can test the schedule API
	 * explicitly.
	 */
	if (atomic_dec(&resubmits_left) > 0) {
		/* Schedule again on current queue */
		state->schedule_res = k_work_schedule_for_queue(one_dwork->work.queue, one_dwork,
								K_MSEC(DELAY_MS));
	} else {
		/* Flag that it didn't schedule */
		state->schedule_res = -EALREADY;
	}

	counter_handler(work);
}

/**
 * @brief Verify a delayable work handler can schedule itself again.
 *
 * @details
 * A handler running on a queue may schedule its own item for a later delay, so
 * periodic work can keep itself going. The scheduling call has to be accepted
 * while the item is still running.
 *
 * Test steps:
 * - Schedule a delayable item whose handler schedules the same item once more.
 * - Wait for the first invocation and record the result of the scheduling call
 *   made from inside the handler.
 * - Wait for the second invocation.
 *
 * Expected result:
 * - The scheduling call from the handler succeeds, and the handler runs twice
 *   on the same queue.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_schedule_for_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_basic_schedule_running)
{
	int rc;
	static struct state_1cpu_basic_schedule_running state = {
		.schedule_res = -1,
	};

	/* Reset state and set for one resubmit.  Use a test-specific
	 * handler.
	 */
	reset_counters();
	atomic_set(&resubmits_left, 1);
	k_work_init_delayable(&state.dwork, handle_1cpu_basic_schedule_running);

	zassert_equal(state.schedule_res, -1);

	rc = k_work_schedule_for_queue(&coophi_queue, &state.dwork,
				       K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);

	zassert_equal(coop_counter(&coophi_queue), 0);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(state.schedule_res, 1);
	zassert_equal(coop_counter(&coophi_queue), 1);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(state.schedule_res, -EALREADY);
	zassert_equal(coop_counter(&coophi_queue), 2);
}

/**
 * @brief Verify scheduling a delayable item with no delay queues it directly.
 *
 * @details
 * K_NO_WAIT means there is no timeout to wait for, so the item must go straight
 * into the queue and report the queued rather than the delayed state, while a
 * repeated schedule is still a no-op.
 *
 * Test steps:
 * - Schedule an idle delayable item with K_NO_WAIT.
 * - Check it reports queued and pending, and schedule it again.
 * - Sleep to let the queue thread run.
 *
 * Expected result:
 * - The item is queued immediately, the second schedule does nothing, and the
 *   handler runs once and leaves the item idle.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_busy_get()
 * @see k_work_schedule_for_queue()
 * @see k_work_delayable_busy_get()
 */
ZTEST(work_1cpu, test_workq_1cpu_immed_schedule)
{
	int rc;
	struct k_work *wp = &dwork.work; /* whitebox testing */

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);
	zassert_equal(k_work_busy_get(wp), 0);

	/* Submit to the cooperative queue */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 1);
	rc = k_work_busy_get(wp);
	zassert_equal(rc, K_WORK_QUEUED);
	zassert_equal(k_work_delayable_busy_get(&dwork), rc);
	zassert_equal(k_work_delayable_is_pending(&dwork), true);

	/* Scheduling again does nothing. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 1);
	zassert_equal(k_work_busy_get(wp), 0);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/**
 * @brief Verify rescheduling replaces a delayable item's pending delay and
 * queue.
 *
 * @details
 * Unlike scheduling, rescheduling an item that is already waiting must take
 * effect: the previous timeout is discarded and the item runs after the new
 * delay, on the queue named by the newer call.
 *
 * Test steps:
 * - Reschedule an idle delayable item onto the preemptible queue with a long
 *   delay.
 * - Reschedule it again onto the cooperative queue with a shorter delay.
 * - Wait for the handler and measure the elapsed time since the second call.
 *
 * Expected result:
 * - The handler runs once, on the cooperative queue, after the second delay and
 *   within the accepted tolerance.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_busy_get()
 * @see k_work_reschedule_for_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_basic_reschedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = delay_max_ms();
	uint32_t elapsed_ms;
	struct k_work *wp = &dwork.work; /* whitebox testing */

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);

	/* Verify that work is idle and marked delayable. */
	zassert_equal(k_work_busy_get(wp), 0);
	zassert_equal(wp->flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Schedule to the preempt queue after twice the standard
	 * delay.
	 */
	rc = k_work_reschedule_for_queue(&preempt_queue, &dwork,
					  K_MSEC(2U * DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED);

	/* Align to tick then reschedule on the cooperative queue for
	 * the standard delay.
	 */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_reschedule_for_queue(&coophi_queue, &dwork,
					  K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Make sure it ran on the coop queue and is now idle */
	zassert_equal(coophi_counter(), 1);
	zassert_equal(k_work_busy_get(wp), 0);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

/**
 * @brief Verify rescheduling with no wait queues delayable work immediately.
 *
 * @details
 * A delayable work item rescheduled with K_NO_WAIT must go straight to the
 * queue instead of starting a delay, and rescheduling it again while it is
 * running must schedule a fresh invocation after the requested delay.
 *
 * Test steps:
 * - Reschedule an idle delayable item with K_NO_WAIT and verify it is
 *   queued immediately.
 * - Let the handler start, then reschedule the running item with a delay.
 * - Measure when the second invocation completes.
 *
 * Expected result:
 * - The first invocation runs without delay and the rescheduled invocation
 *   runs after the requested delay within the expected tolerance.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_reschedule_for_queue()
 * @see k_work_init_delayable()
 */
ZTEST(work_1cpu, test_workq_1cpu_immed_reschedule)
{
	int rc;
	struct k_work *wp = &dwork.work; /* whitebox testing */

	/* Reset state and use the delay handler */
	reset_counters();
	k_work_init_delayable(&dwork, delay_handler);
	zassert_equal(k_work_busy_get(wp), 0);

	/* Schedule immediately to the cooperative queue */
	rc = k_work_reschedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(wp), K_WORK_QUEUED);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);
	zassert_equal(k_work_busy_get(wp), K_WORK_RUNNING);

	/* Schedule immediately to the preemptive queue (will divert
	 * to coop since running).
	 */
	rc = k_work_reschedule_for_queue(&preempt_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 2);
	zassert_equal(k_work_busy_get(wp), K_WORK_QUEUED | K_WORK_RUNNING,
		      NULL);

	/* Schedule after 3x the delay to the preemptive queue
	 * (will not divert since previous submissions will have
	 * completed).
	 */
	rc = k_work_reschedule_for_queue(&preempt_queue, &dwork,
					  K_MSEC(3 * DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(wp),
		      K_WORK_DELAYED | K_WORK_QUEUED | K_WORK_RUNNING,
		      NULL);

	/* Wait for the original no-wait submission (total 1 delay)) */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Check that coop ran once, and work is still delayed and
	 * also running.
	 */
	zassert_equal(coophi_counter(), 1);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED | K_WORK_RUNNING,
		      NULL);

	/* Wait for the queued no-wait submission (total 2 delay) */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Check that got diverted to coop and ran, and work is still
	 * delayed.
	 */
	zassert_equal(coophi_counter(), 2);
	zassert_equal(preempt_counter(), 0);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED,
		      NULL);

	/* Wait for the delayed submission (total 3 delay) */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Check that ran on preempt.  In fact we're here because the
	 * test thread is higher priority, so the work will still be
	 * marked running.
	 */
	zassert_equal(coophi_counter(), 2);
	zassert_equal(preempt_counter(), 1);
	zassert_equal(k_work_busy_get(wp), K_WORK_RUNNING,
		      NULL);

	/* Wait for preempt to drain */
	rc = k_work_queue_drain(&preempt_queue, false);
	zassert_equal(rc, 1);
}

/* Test no-yield behavior, returns true if and only if work queue priority is
 * higher than test thread priority
 */
static bool try_queue_no_yield(struct k_work_q *wq)
{
	int rc;
	bool is_high = (k_thread_priority_get(k_work_queue_thread_get(wq))
			< k_thread_priority_get(k_current_get()));

	TC_PRINT("Testing no-yield on %s-priority queue\n",
		 is_high ? "high" : "low");
	reset_counters();

	/* Submit two work items directly to the cooperative queue. */

	k_work_init(&common_work, counter_handler);
	k_work_init_delayable(&dwork, counter_handler);

	rc = k_work_submit_to_queue(wq, &common_work);
	zassert_equal(rc, 1);
	rc = k_work_schedule_for_queue(wq, &dwork, K_NO_WAIT);
	zassert_equal(rc, 1);

	/* Wait for completion */
	zassert_equal(k_work_is_pending(&common_work), true);
	zassert_equal(k_work_delayable_is_pending(&dwork), true);
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Because there was no yield both should have run, and
	 * another yield won't cause anything to happen.
	 */
	zassert_equal(coop_counter(wq), 2);
	zassert_equal(k_work_is_pending(&common_work), false);
	zassert_equal(k_work_delayable_is_pending(&dwork), false);

	/* The first give unblocked this thread; we need to consume
	 * the give from the second work task.
	 */
	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), 0);

	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), -EBUSY);

	return is_high;
}

/**
 * @brief Verify the no-yield option keeps a queue running between items.
 *
 * @details
 * A queue configured with no_yield does not yield between work items, so once
 * it starts processing it drains everything queued before any equal or lower
 * priority thread runs. Whether that starves the test thread depends on the
 * queue's priority, so the behavior is checked on both a queue above and a
 * queue below the test thread's priority.
 *
 * Test steps:
 * - Submit a work item and a delayable item to a no-yield queue of higher
 *   priority than the test thread, and wait for completion.
 * - Repeat with a no-yield queue of lower priority than the test thread.
 *
 * Expected result:
 * - Both items run on each queue without an intervening yield, and the helper
 *   reports the queue priority relative to the test thread correctly.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_submit_to_queue()
 * @see k_work_schedule_for_queue()
 */
ZTEST(work_1cpu, test_workq_1cpu_queue_no_yield)
{
	/* This test needs two slots available in the sem! */
	k_sem_init(&sync_sem, 0, 2);
	zassert_equal(try_queue_no_yield(&coophi_queue), true);
	zassert_equal(try_queue_no_yield(&cooplo_queue), false);
	k_sem_init(&sync_sem, 0, 1);
}

/**
 * @brief Verify a work item submitted to the system work queue runs there.
 *
 * @details
 * k_work_submit() targets the system work queue rather than a queue the test
 * started, so it must behave like an explicit submission and run the handler on
 * the system queue's thread.
 *
 * Test steps:
 * - Initialize a work item with a non-blocking handler and submit it with
 *   k_work_submit().
 * - Check the item reports queued and has not run.
 * - Sleep to let the system queue thread run.
 *
 * Expected result:
 * - The handler runs exactly once on the system work queue thread and the item
 *   ends idle.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 * @see k_work_busy_get()
 * @see k_work_submit()
 */
ZTEST(work_1cpu, test_workq_1cpu_system_queue)
{
	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&common_work, counter_handler);
	zassert_equal(k_work_busy_get(&common_work), 0);

	/* Submit to the system queue */
	rc = k_work_submit(&common_work);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(&common_work), K_WORK_QUEUED);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(system_counter(), 0);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(system_counter(), 1);
	zassert_equal(k_work_busy_get(&common_work), 0);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/**
 * @brief Verify a delayable item scheduled on the system queue honors its
 * delay.
 *
 * @details
 * k_work_schedule() is the system work queue counterpart of
 * k_work_schedule_for_queue(), and must apply the same delay semantics: the
 * item waits out its timeout, a repeated schedule is a no-op, and the handler
 * then runs on the system queue thread.
 *
 * Test steps:
 * - Schedule an idle delayable item on the system queue for a known delay.
 * - Schedule it again while it is still waiting.
 * - Wait for the handler and measure the elapsed time since scheduling.
 *
 * Expected result:
 * - The second schedule is a no-op and the handler runs once on the system
 *   queue after at least the requested delay and within the tolerance.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_delayable_busy_get()
 * @see k_work_schedule()
 */
ZTEST(work_1cpu, test_workq_1cpu_system_schedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = delay_max_ms();
	uint32_t elapsed_ms;

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);

	/* Verify that work is idle and marked delayable. */
	zassert_equal(k_work_delayable_busy_get(&dwork), 0);
	zassert_equal(dwork.work.flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Align to tick, then schedule after normal delay. */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_schedule(&dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_DELAYED);

	/* Scheduling again does nothing. */
	rc = k_work_schedule(&dwork, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Make sure it ran and is now idle */
	zassert_equal(system_counter(), 1);
	zassert_equal(k_work_delayable_busy_get(&dwork), 0);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

/**
 * @brief Verify rescheduling a delayable item on the system queue replaces its
 * delay.
 *
 * @details
 * k_work_reschedule() must discard a pending timeout on the system work queue
 * just as k_work_reschedule_for_queue() does on a dedicated queue, so the item
 * runs after the most recently requested delay.
 *
 * Test steps:
 * - Reschedule an idle delayable item on the system queue with a long delay.
 * - Reschedule it again with a shorter delay.
 * - Wait for the handler and measure the elapsed time since the second call.
 *
 * Expected result:
 * - The handler runs once on the system queue after the second delay and within
 *   the accepted tolerance.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_init_delayable()
 * @see k_work_delayable_busy_get()
 * @see k_work_reschedule()
 */
ZTEST(work_1cpu, test_workq_1cpu_system_reschedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = delay_max_ms();
	uint32_t elapsed_ms;

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);

	/* Verify that work is idle and marked delayable. */
	zassert_equal(k_work_delayable_busy_get(&dwork), 0);
	zassert_equal(dwork.work.flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Schedule to the preempt queue after twice the standard
	 * delay.
	 */
	rc = k_work_reschedule(&dwork, K_MSEC(2U * DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_DELAYED);

	/* Align to tick then reschedule on the system queue for
	 * the standard delay.
	 */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_reschedule(&dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_DELAYED);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Make sure it ran on the system queue and is now idle */
	zassert_equal(system_counter(), 1);
	zassert_equal(k_work_delayable_busy_get(&dwork), 0);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

/* Cooperative priority below the ztest thread so submitted work stays queued
 * until the test thread blocks, making the processing deterministic on 1 CPU.
 */
#define POLICY_PRIORITY K_PRIO_COOP(3)

struct ordered_work {
	struct k_work work;
	int id;
};

static struct k_work_q order_queue;
static K_THREAD_STACK_DEFINE(order_stack, STACK_SIZE);
static struct ordered_work order_items[3];
static int order_seq[3];
static int order_seq_n;
static struct k_sem order_done_sem;

static void order_handler(struct k_work *work)
{
	struct ordered_work *o = CONTAINER_OF(work, struct ordered_work, work);

	order_seq[order_seq_n++] = o->id;
	if (o->id == 2) {
		k_sem_give(&order_done_sem);
	}
}

/**
 * @brief Verify a work queue processes work items in submission order.
 *
 * @details
 * A work queue is FIFO: items are handled in the order they were accepted. The
 * queue used here runs below the test thread's priority, so all three items are
 * queued before the queue thread gets to run and the resulting order is decided
 * purely by the queue.
 *
 * Test steps:
 * - Start a work queue at a priority below the test thread.
 * - Submit three work items that record their identity when handled.
 * - Wait for the last item to run, then drain and stop the queue.
 *
 * Expected result:
 * - The recorded identities are in submission order.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_start()
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_work_queue_drain()
 */
ZTEST(work_1cpu, test_workq_1cpu_queue_order)
{
	struct k_work_queue_config cfg = {
		.name = "order",
		.no_yield = true,
	};

	order_seq_n = 0;
	k_sem_init(&order_done_sem, 0, 1);

	k_work_queue_start(&order_queue, order_stack, STACK_SIZE, POLICY_PRIORITY, &cfg);

	/* The queue is lower priority than this thread, so all three items are
	 * queued before the queue thread runs.
	 */
	for (int i = 0; i < 3; i++) {
		order_items[i].id = i;
		k_work_init(&order_items[i].work, order_handler);
		zassert_equal(k_work_submit_to_queue(&order_queue, &order_items[i].work), 1,
			      "failed to queue item %d", i);
	}

	zassert_ok(k_sem_take(&order_done_sem, K_FOREVER));

	zassert_equal(order_seq[0], 0, "items not processed in submission order");
	zassert_equal(order_seq[1], 1, "items not processed in submission order");
	zassert_equal(order_seq[2], 2, "items not processed in submission order");

	zassert_true(k_work_queue_drain(&order_queue, true) >= 0, "drain failed");
	zassert_ok(k_work_queue_stop(&order_queue, K_FOREVER), "stop failed");
}

static struct k_work_q yield_queue;
static K_THREAD_STACK_DEFINE(yield_stack, STACK_SIZE);
static struct k_thread yield_competitor;
static K_THREAD_STACK_DEFINE(yield_comp_stack, STACK_SIZE);
static struct k_work yield_w0, yield_w1;
static struct k_sem yield_comp_sem;
static struct k_sem yield_done_sem;
static char yield_seq[4];
static int yield_seq_n;

static void yield_competitor_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Stays unready until the first work item makes us runnable. */
	k_sem_take(&yield_comp_sem, K_FOREVER);
	yield_seq[yield_seq_n++] = 'C';
}

static void yield_w0_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	yield_seq[yield_seq_n++] = '0';
	/* Make the equal-priority competitor runnable while the queue thread is
	 * processing. A yielding queue must let it run before the next item.
	 */
	k_sem_give(&yield_comp_sem);
}

static void yield_w1_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	yield_seq[yield_seq_n++] = '1';
	k_sem_give(&yield_done_sem);
}

/**
 * @brief Verify a work queue yields between successive work items by default.
 *
 * @details
 * Without the no-yield option a cooperative queue yields after each item, so it
 * cannot monopolize the CPU against threads of its own priority. The first
 * handler makes an equal-priority competitor runnable, and the yield is
 * observable as the competitor running before the second item rather than after
 * both.
 *
 * Test steps:
 * - Start a cooperative work queue with the default configuration and create a
 *   competitor thread at the same priority, initially blocked.
 * - Submit two work items; the first unblocks the competitor, and each handler
 *   and the competitor append to a shared sequence.
 * - Wait for both items and the competitor to finish, then drain and stop the
 *   queue.
 *
 * Expected result:
 * - The recorded sequence is "0C1": the competitor ran between the two work
 *   items.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_start()
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_work_queue_drain()
 */
ZTEST(work_1cpu, test_workq_1cpu_queue_yield)
{
	/* Default config: no_yield not set, so the queue yields between items. */
	struct k_work_queue_config cfg = {
		.name = "yield",
	};

	yield_seq_n = 0;
	k_sem_init(&yield_comp_sem, 0, 1);
	k_sem_init(&yield_done_sem, 0, 1);

	k_work_queue_start(&yield_queue, yield_stack, STACK_SIZE, POLICY_PRIORITY, &cfg);

	/* Competitor at the same priority as the queue, initially not runnable. */
	k_thread_create(&yield_competitor, yield_comp_stack, STACK_SIZE,
			yield_competitor_fn, NULL, NULL, NULL,
			POLICY_PRIORITY, 0, K_NO_WAIT);

	k_work_init(&yield_w0, yield_w0_handler);
	k_work_init(&yield_w1, yield_w1_handler);
	zassert_equal(k_work_submit_to_queue(&yield_queue, &yield_w0), 1);
	zassert_equal(k_work_submit_to_queue(&yield_queue, &yield_w1), 1);

	zassert_ok(k_sem_take(&yield_done_sem, K_FOREVER));
	zassert_ok(k_thread_join(&yield_competitor, K_FOREVER));

	yield_seq[yield_seq_n] = '\0';
	zassert_mem_equal(yield_seq, "0C1", 3,
			  "expected competitor to run between items (0C1), got \"%s\"",
			  yield_seq);

	zassert_true(k_work_queue_drain(&yield_queue, true) >= 0, "drain failed");
	zassert_ok(k_work_queue_stop(&yield_queue, K_FOREVER), "stop failed");
}

void *workq_setup(void)
{
	main_thread = k_current_get();
	k_sem_init(&sync_sem, 0, 1);
	k_sem_init(&rel_sem, 0, 1);

	if (run_flag) {
		start_test_queues();
		run_flag = false;
	}

	return NULL;
}

ZTEST_SUITE(work_1cpu, NULL, workq_setup, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
ZTEST_SUITE(work, NULL, workq_setup, NULL, NULL, NULL);
