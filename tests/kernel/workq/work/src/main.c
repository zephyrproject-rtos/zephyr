/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This test covers deprecated API.  Avoid inappropriate diagnostics
 * about the use of that API.
 */
#include <zephyr/toolchain.h>
#undef __deprecated
#define __deprecated
#undef __DEPRECATED_MACRO
#define __DEPRECATED_MACRO

#include <zephyr/ztest.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define COOPHI_PRIORITY K_PRIO_COOP(0) /* = -4 */
/* SYSTEM_WORKQUEUE_PRIORITY = -3 */
/* ZTEST_THREAD_PRIORITY = -2 */
#define COOPLO_PRIORITY K_PRIO_COOP(3) /* = -1 */
#define PREEMPT_PRIORITY K_PRIO_PREEMPT(1) /* = 1 */

#define DELAY_MS 100
#define DELAY_TIMEOUT K_MSEC(DELAY_MS)

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
static struct k_work work;
static struct k_work work1;
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
	if (k_current_get() == &coophi_queue.thread) {
		atomic_inc(&coophi_ctr);
	} else if (k_current_get() == &k_sys_work_q.thread) {
		atomic_inc(&system_ctr);
	} else if (k_current_get() == &cooplo_queue.thread) {
		atomic_inc(&cooplo_ctr);
	} else if (k_current_get() == &preempt_queue.thread) {
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

/* Check that standard initializations result in expected content. */
static void test_work_init(void)
{
	static K_WORK_DEFINE(fnstat, counter_handler);

	static struct k_work stack;

	k_work_init(&stack, counter_handler);
	zassert_mem_equal(&stack, &fnstat, sizeof(stack),
			  NULL);
}

static void test_delayable_init(void)
{
	static K_WORK_DELAYABLE_DEFINE(fnstat, counter_handler);

	static struct k_work_delayable stack;

	k_work_init_delayable(&stack, counter_handler);
	zassert_mem_equal(&stack, &fnstat, sizeof(stack),
			  NULL);
}

/* Check that submission to an unstarted queue is diagnosed. */
ZTEST(work, test_unstarted)
{
	int rc;

	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0);

	rc = k_work_submit_to_queue(&not_start_queue, &work);
	zassert_equal(rc, -ENODEV);
}

static void test_queue_start(void)
{
	struct k_work_queue_config cfg = {
		.name = "wq.preempt",
	};
	k_work_queue_init(&preempt_queue);
	zassert_equal(preempt_queue.flags, 0);
	k_work_queue_start(&preempt_queue, preempt_stack, STACK_SIZE,
			    PREEMPT_PRIORITY, &cfg);
	zassert_equal(preempt_queue.flags, K_WORK_QUEUE_STARTED);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		const char *tn = k_thread_name_get(&preempt_queue.thread);

		zassert_true(tn != cfg.name);
		zassert_true(tn != NULL);
		zassert_equal(strcmp(tn, cfg.name), 0);
	}

	cfg.name = NULL;
	zassert_equal(invalid_test_queue.flags, 0);
	k_work_queue_start(&invalid_test_queue, invalid_test_stack, STACK_SIZE,
			    PREEMPT_PRIORITY, &cfg);
	zassert_equal(invalid_test_queue.flags, K_WORK_QUEUE_STARTED);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		const char *tn = k_thread_name_get(&invalid_test_queue.thread);

		zassert_true(tn != cfg.name);
		zassert_true(tn != NULL);
		zassert_equal(strcmp(tn, ""), 0);
	}

	cfg.name = "wq.coophi";
	cfg.no_yield = true;
	k_work_queue_start(&coophi_queue, coophi_stack, STACK_SIZE,
			    COOPHI_PRIORITY, &cfg);
	zassert_equal(coophi_queue.flags,
		      K_WORK_QUEUE_STARTED | K_WORK_QUEUE_NO_YIELD, NULL);

	cfg.name = "wq.cooplo";
	cfg.no_yield = true;
	k_work_queue_start(&cooplo_queue, cooplo_stack, STACK_SIZE,
			    COOPLO_PRIORITY, &cfg);
	zassert_equal(cooplo_queue.flags,
		      K_WORK_QUEUE_STARTED | K_WORK_QUEUE_NO_YIELD, NULL);
}

/* Check validation of submission without a destination queue. */
ZTEST(work, test_null_queue)
{
	int rc;

	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0);

	rc = k_work_submit_to_queue(NULL, &work);
	zassert_equal(rc, -EINVAL);
}

/* Basic single-CPU check submitting with a non-blocking handler. */
ZTEST(work_1cpu, test_1cpu_simple_queue)
{
	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0);
	zassert_equal(k_work_is_pending(&work), false);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED);
	zassert_equal(k_work_is_pending(&work), true);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0);

	/* Let it run, then check it finished. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 1);
	zassert_equal(k_work_busy_get(&work), 0);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/* Basic SMP check submitting with a non-blocking handler. */
ZTEST(work, test_smp_simple_queue)
{
	if (!IS_ENABLED(CONFIG_SMP)) {
		ztest_test_skip();
		return;
	}

	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0);
	zassert_equal(k_work_is_pending(&work), false);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);

	/* It should run and finish without this thread yielding. */
	int64_t ts0 = k_uptime_ticks();
	uint32_t delay;

	do {
		delay = k_ticks_to_ms_floor32(k_uptime_ticks() - ts0);
	} while (k_work_is_pending(&work) && (delay < DELAY_MS));

	zassert_equal(k_work_busy_get(&work), 0);
	zassert_equal(coophi_counter(), 1);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/* Basic single-CPU check submitting with a blocking handler */
ZTEST(work_1cpu, test_1cpu_sync_queue)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);
	zassert_equal(k_work_busy_get(&work), 0);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);
	zassert_equal(k_work_busy_get(&work), K_WORK_RUNNING);

	/* Make it ready so it can finish when this thread yields. */
	handler_release();
	zassert_equal(coophi_counter(), 0);

	/* Wait for then verify finish */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 1);
}

/* Verify that if a work item is submitted while it is being run by a
 * queue thread it gets submitted to the queue it's running on, to
 * prevent reentrant invocation, at least on a single CPU.
 */
ZTEST(work_1cpu, test_1cpu_reentrant_queue)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Release it so it's running and can be rescheduled. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0);

	/* Resubmit to a different queue. */
	rc = k_work_submit_to_queue(&preempt_queue, &work);
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

/* Single CPU submit two work items and wait for flush in order
 * before they get started.
 */
ZTEST(work_1cpu, test_1cpu_queued_flush)
{
	int rc;

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&work, delay_handler);
	k_work_init(&work1, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work1);
	zassert_equal(rc, 1);
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Confirm that it's still in the queue, then wait for completion.
	 * This should wait.
	 */
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED);
	zassert_equal(k_work_busy_get(&work1), K_WORK_QUEUED);
	zassert_true(k_work_flush(&work, &work_sync));
	zassert_false(k_work_flush(&work1, &work_sync));

	/* Verify completion. */
	zassert_equal(coophi_counter(), 2);
	zassert_true(!k_work_is_pending(&work));
	zassert_true(!k_work_is_pending(&work1));
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);

	/* After completion flush should be a no-op */
	zassert_false(k_work_flush(&work, &work_sync));
	zassert_false(k_work_flush(&work1, &work_sync));
}

/* Single CPU submit a work item and wait for flush after it's started.
 */
ZTEST(work_1cpu, test_1cpu_running_flush)
{
	int rc;

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&work, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED);

	/* Release it so it's running. */
	k_sleep(K_TICKS(1));
	zassert_equal(k_work_busy_get(&work), K_WORK_RUNNING);
	zassert_equal(coophi_counter(), 0);

	/* Wait for completion.  This should be released by the delay
	 * handler.
	 */
	zassert_true(k_work_flush(&work, &work_sync));

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/* Single CPU schedule a work item and wait for flush. */
ZTEST(work_1cpu, test_1cpu_delayed_flush)
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

/* Single CPU cancel before work item is unqueued should complete
 * immediately.
 */
ZTEST(work_1cpu, test_1cpu_queued_cancel)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Cancellation should complete immediately. */
	zassert_equal(k_work_cancel(&work), 0);

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0);
}

/* Single CPU cancel before work item is unqueued should not wait. */
ZTEST(work_1cpu, test_1cpu_queued_cancel_sync)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);

	/* Cancel an unqueued work item should not affect the work
	 * and return false.
	 */
	zassert_false(k_work_cancel_sync(&work, &work_sync));

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Cancellation should complete immediately, indicating that
	 * work was pending.
	 */
	zassert_true(k_work_cancel_sync(&work, &work_sync));

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0);
}

/* Single CPU cancel before scheduled work item is queued should
 * complete immediately.
 */
ZTEST(work_1cpu, test_1cpu_delayed_cancel)
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


/* Single CPU cancel before scheduled work item is queued should not wait. */
ZTEST(work_1cpu, test_1cpu_delayed_cancel_sync)
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

/* Single CPU cancel after delayable item starts should wait. */
ZTEST(work_1cpu, test_1cpu_delayed_cancel_sync_wait)
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

static void test_running_cancel_cb(struct k_timer *timer)
{
	struct test_running_cancel_timer *ctx =
		CONTAINER_OF(timer, struct test_running_cancel_timer, timer);

	ctx->busy_rc = k_work_busy_get(&ctx->work);
	ctx->submit_rc = k_work_submit_to_queue(&coophi_queue, &ctx->work);
	handler_release();
}

/* Single CPU test cancellation after work starts. */
ZTEST(work_1cpu, test_1cpu_running_cancel)
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
	k_timer_init(&ctx->timer, test_running_cancel_cb, NULL);
	k_timer_start(&ctx->timer, K_MSEC(ms_timeout), K_NO_WAIT);

	/* Cancellation should not complete. */
	zassert_equal(k_work_cancel(wp), K_WORK_RUNNING | K_WORK_CANCELING,
		      NULL);

	/* Handler should not have run. */
	zassert_equal(coophi_counter(), 0);

	/* Busy wait until timer expires. Thread context is blocked so cancelling
	 * of work won't be completed.
	 */
	k_busy_wait(1000 * (ms_timeout + 1));

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

/* Single CPU test wait-for-cancellation after the work item has
 * started running.
 */
ZTEST(work_1cpu, test_1cpu_running_cancel_sync)
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
	k_timer_init(&ctx->timer, test_running_cancel_cb, NULL);
	k_timer_start(&ctx->timer, K_MSEC(ms_timeout), K_NO_WAIT);

	/* Cancellation should wait. */
	zassert_true(k_work_cancel_sync(wp, &work_sync));

	/* Handler should have run. */
	zassert_equal(coophi_counter(), 1);

	/* Busy wait until timer expires. Thread context is blocked so cancelling
	 * of work won't be completed.
	 */
	k_busy_wait(1000 * (ms_timeout + 1));

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

/* SMP cancel after work item is started should succeed but require
 * wait.
 */
ZTEST(work, test_smp_running_cancel)
{
	int rc;

	if (!IS_ENABLED(CONFIG_SMP)) {
		ztest_test_skip();
		return;
	}

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&work, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);

	/* It should advance to running without this thread yielding. */
	int64_t ts0 = k_uptime_ticks();
	uint32_t delay;

	do {
		delay = k_ticks_to_ms_floor32(k_uptime_ticks() - ts0);
	} while ((k_work_busy_get(&work) != K_WORK_RUNNING)
		 && (delay < DELAY_MS));

	/* Cancellation should not succeed immediately because the
	 * work is running.
	 */
	rc = k_work_cancel(&work);
	zassert_equal(rc, K_WORK_RUNNING | K_WORK_CANCELING, "rc %x", rc);

	/* Sync should wait. */
	zassert_equal(k_work_cancel_sync(&work, &work_sync), true);

	/* Should have completed. */
	zassert_equal(coophi_counter(), 1);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

/* Drain with no active workers completes immediately. */
ZTEST(work, test_drain_empty)
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

static void test_drain_wait_cb(struct k_timer *timer)
{
	struct test_drain_wait_timer *ctx =
		CONTAINER_OF(timer, struct test_drain_wait_timer, timer);

	ctx->submit_rc = k_work_submit_to_queue(&coophi_queue, &ctx->work);
}

/* Single CPU submit an item and wait for it to drain. */
ZTEST(work_1cpu, test_1cpu_drain_wait)
{
	struct test_drain_wait_timer *ctx = &test_drain_wait_ctx;
	int rc;

	/* Reset state, allow one re-submission, and use the delaying
	 * handler.
	 */
	reset_counters();
	atomic_set(&resubmits_left, 1);
	k_work_init(&work, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);
	zassert_equal(coophi_counter(), 0);

	/* Schedule the async process to capture submission state
	 * while draining.
	 */
	ctx->submit_rc = INT_MAX;
	k_timer_init(&ctx->timer, test_drain_wait_cb, NULL);
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

/* Single CPU submit item, drain with plug, test, then unplug. */
ZTEST(work_1cpu, test_1cpu_plugged_drain)
{
	int rc;

	/* Reset state and use the delaying handler. */
	reset_counters();
	k_work_init(&work, delay_handler);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
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
	k_work_init(&work, counter_handler);

	/* Resubmission should fail because queue is plugged */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
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
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1);

	/* Flush the sync state and verify completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);
	zassert_equal(coophi_counter(), 2);
}

/* Single CPU test delayed submission */
ZTEST(work_1cpu, test_1cpu_basic_schedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = k_ticks_to_ms_ceil32(1U
				+ k_ms_to_ticks_ceil32(DELAY_MS));
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
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct state_1cpu_basic_schedule_running *state
		= CONTAINER_OF(dwork, struct state_1cpu_basic_schedule_running,
			       dwork);

	/* Co-opt the resubmits so we can test the schedule API
	 * explicitly.
	 */
	if (atomic_dec(&resubmits_left) > 0) {
		/* Schedule again on current queue */
		state->schedule_res = k_work_schedule_for_queue(NULL, dwork,
								K_MSEC(DELAY_MS));
	} else {
		/* Flag that it didn't schedule */
		state->schedule_res = -EALREADY;
	}

	counter_handler(work);
}

/* Single CPU test that schedules when running */
ZTEST(work_1cpu, test_1cpu_basic_schedule_running)
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

/* Single CPU test schedule without delay is queued immediately. */
ZTEST(work_1cpu, test_1cpu_immed_schedule)
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

/* Single CPU test that delayed work can be rescheduled. */
ZTEST(work_1cpu, test_1cpu_basic_reschedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = k_ticks_to_ms_ceil32(1U
				+ k_ms_to_ticks_ceil32(DELAY_MS));
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

/* Single CPU test that delayed work can be immediately queued by
 * reschedule API.
 */
ZTEST(work_1cpu, test_1cpu_immed_reschedule)
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

/* Test no-yield behavior, returns true iff work queue priority is
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

	k_work_init(&work, counter_handler);
	k_work_init_delayable(&dwork, counter_handler);

	rc = k_work_submit_to_queue(wq, &work);
	zassert_equal(rc, 1);
	rc = k_work_schedule_for_queue(wq, &dwork, K_NO_WAIT);
	zassert_equal(rc, 1);

	/* Wait for completion */
	zassert_equal(k_work_is_pending(&work), true);
	zassert_equal(k_work_delayable_is_pending(&dwork), true);
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0);

	/* Because there was no yield both should have run, and
	 * another yield won't cause anything to happen.
	 */
	zassert_equal(coop_counter(wq), 2);
	zassert_equal(k_work_is_pending(&work), false);
	zassert_equal(k_work_delayable_is_pending(&dwork), false);

	/* The first give unblocked this thread; we need to consume
	 * the give from the second work task.
	 */
	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), 0);

	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), -EBUSY);

	return is_high;
}

/* Verify that no-yield policy works */
ZTEST(work_1cpu, test_1cpu_queue_no_yield)
{
	/* This test needs two slots available in the sem! */
	k_sem_init(&sync_sem, 0, 2);
	zassert_equal(try_queue_no_yield(&coophi_queue), true);
	zassert_equal(try_queue_no_yield(&cooplo_queue), false);
	k_sem_init(&sync_sem, 0, 1);
}

/* Basic functionality with the system work queue. */
ZTEST(work_1cpu, test_1cpu_system_queue)
{
	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0);

	/* Submit to the system queue */
	rc = k_work_submit(&work);
	zassert_equal(rc, 1);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(system_counter(), 0);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(system_counter(), 1);
	zassert_equal(k_work_busy_get(&work), 0);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0);
}

ZTEST(work_1cpu, test_1cpu_system_schedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = k_ticks_to_ms_ceil32(1U
				+ k_ms_to_ticks_ceil32(DELAY_MS));
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

ZTEST(work_1cpu, test_1cpu_system_reschedule)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = k_ticks_to_ms_ceil32(1U
				+ k_ms_to_ticks_ceil32(DELAY_MS));
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

ZTEST(work, test_nop)
{
	ztest_test_skip();
}

void *workq_setup(void)
{
	main_thread = k_current_get();
	k_sem_init(&sync_sem, 0, 1);
	k_sem_init(&rel_sem, 0, 1);

	test_work_init();
	test_delayable_init();

	if (run_flag) {
		test_queue_start();
		run_flag = false;
	}

	return NULL;
}

ZTEST_SUITE(work_1cpu, NULL, workq_setup, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
ZTEST_SUITE(work, NULL, workq_setup, NULL, NULL, NULL);
