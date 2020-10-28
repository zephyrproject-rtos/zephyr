/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)
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

/* Given by test thread to release a work item. */
static struct k_sem rel_sem;

/* Common work structures, to avoid dead references to stack objects
 * if a test fails.
 */
static struct k_work work;
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

static atomic_t system_ctr;
static inline int system_counter(void)
{
	return atomic_get(&system_ctr);
}

static inline void reset_counters(void)
{
	/* If this fails the previous test didn't clean up */
	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), -EBUSY, NULL);
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
	k_sem_take(&rel_sem, K_FOREVER);
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

	struct k_work stack;

	k_work_init(&stack, counter_handler);
	zassert_mem_equal(&stack, &fnstat, sizeof(stack),
			  NULL);
}

static void test_delayable_init(void)
{
	static K_WORK_DELAYABLE_DEFINE(fnstat, counter_handler);

	struct k_work_delayable stack;

	k_work_init_delayable(&stack, counter_handler);
	zassert_mem_equal(&stack, &fnstat, sizeof(stack),
			  NULL);
}

static void test_legacy_delayed_init(void)
{
	static K_DELAYED_WORK_DEFINE(fnstat, counter_handler);

	struct k_delayed_work stack;

	k_delayed_work_init(&stack, counter_handler);
	zassert_mem_equal(&stack, &fnstat, sizeof(stack),
			  NULL);
}

/* Check that submission to an unstarted queue is diagnosed. */
static void test_unstarted(void)
{
	int rc;

	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0, NULL);

	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, -ENODEV, NULL);
}

static void test_queue_start(void)
{
	struct k_work_queue_config cfg = {
		.name = "wq.preempt",
	};

	zassert_equal(preempt_queue.flags, 0, NULL);
	k_work_queue_start(&preempt_queue, preempt_stack, STACK_SIZE,
			    PREEMPT_PRIORITY, &cfg);
	zassert_equal(preempt_queue.flags, K_WORK_QUEUE_STARTED, NULL);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		const char *tn = k_thread_name_get(&preempt_queue.thread);

		zassert_true(tn != cfg.name, NULL);
		zassert_true(tn != NULL, NULL);
		zassert_equal(strcmp(tn, cfg.name), 0, NULL);
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
static void test_null_queue(void)
{
	int rc;

	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0, NULL);

	rc = k_work_submit_to_queue(NULL, &work);
	zassert_equal(rc, -EINVAL, NULL);
}

/* Basic single-CPU check submitting with a non-blocking handler. */
static void test_1cpu_simple_queue(void)
{
	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0, NULL);
	zassert_equal(k_work_is_pending(&work), false, NULL);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED, NULL);
	zassert_equal(k_work_is_pending(&work), true, NULL);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0, NULL);

	/* Let it run, then check it finished. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 1, NULL);
	zassert_equal(k_work_busy_get(&work), 0, NULL);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
}

/* Basic SMP check submitting with a non-blocking handler. */
static void test_smp_simple_queue(void)
{
	if (!IS_ENABLED(CONFIG_SMP)) {
		ztest_test_skip();
		return;
	}

	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0, NULL);
	zassert_equal(k_work_is_pending(&work), false, NULL);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);

	/* It should run and finish without this thread yielding. */
	int64_t ts0 = k_uptime_ticks();
	uint32_t delay;

	do {
		delay = k_ticks_to_ms_floor32(k_uptime_ticks() - ts0);
	} while (k_work_is_pending(&work) && (delay < DELAY_MS));

	zassert_equal(k_work_busy_get(&work), 0, NULL);
	zassert_equal(coophi_counter(), 1, NULL);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
}

/* Basic single-CPU check submitting with a blocking handler */
static void test_1cpu_sync_queue(void)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);
	zassert_equal(k_work_busy_get(&work), 0, NULL);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED, NULL);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0, NULL);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0, NULL);
	zassert_equal(k_work_busy_get(&work), K_WORK_RUNNING, NULL);

	/* Make it ready so it can finish when this thread yields. */
	handler_release();
	zassert_equal(coophi_counter(), 0, NULL);

	/* Wait for then verify finish */
	k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(coophi_counter(), 1, NULL);
}

/* Verify that if a work item is submitted while it is being run by a
 * queue thread it gets submitted to the queue it's running on, to
 * prevent reentrant invocation, at least on a single CPU.
 */
static void test_1cpu_reentrant_queue(void)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Release it so it's running and can be rescheduled. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0, NULL);

	/* Resubmit to a different queue. */
	rc = k_work_submit_to_queue(&preempt_queue, &work);
	zassert_equal(rc, 2, NULL);

	/* Release the first submission. */
	handler_release();
	k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(coophi_counter(), 1, NULL);

	/* Confirm the second submission was redirected to the running
	 * queue to avoid re-entrancy problems.
	 */
	handler_release();
	k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(coophi_counter(), 2, NULL);
}

/* Single CPU submit a work item and wait for flush before it gets started.
 */
static void test_1cpu_queued_flush(void)
{
	int rc;

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&work, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Confirm that it's still in the queue, then wait for completion.
	 * This should wait.
	 */
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED, NULL);
	zassert_true(k_work_flush(&work, &work_sync), NULL);

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1, NULL);
	zassert_true(!k_work_is_pending(&work), NULL);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);

	/* After completion flush should be a no-op */
	zassert_false(k_work_flush(&work, &work_sync), NULL);
}

/* Single CPU submit a work item and wait for flush after it's started.
 */
static void test_1cpu_running_flush(void)
{
	int rc;

	/* Reset state and use the delaying handler */
	reset_counters();
	k_work_init(&work, delay_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED, NULL);

	/* Release it so it's running. */
	k_sleep(K_TICKS(1));
	zassert_equal(k_work_busy_get(&work), K_WORK_RUNNING, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Wait for completion.  This should be released by the delay
	 * handler.
	 */
	zassert_true(k_work_flush(&work, &work_sync), NULL);

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1, NULL);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
}

/* Single CPU schedule a work item and wait for flush. */
static void test_1cpu_delayed_flush(void)
{
	int rc;
	uint32_t flush_ms;
	uint32_t wait_ms;

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);

	/* Unscheduled completes immediately. */
	zassert_false(k_work_flush_delayable(&dwork, &work_sync), NULL);

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Align to tick then flush. */
	k_sleep(K_TICKS(1));
	flush_ms = k_uptime_get_32();
	zassert_true(k_work_flush_delayable(&dwork, &work_sync), NULL);
	wait_ms = last_handle_ms - flush_ms;
	zassert_true(wait_ms <= 1, "waited %u", wait_ms);

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1, NULL);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
}

/* Single CPU cancel before work item is unqueued should complete
 * immediately.
 */
static void test_1cpu_queued_cancel(void)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Cancellation should complete immediately. */
	zassert_equal(k_work_cancel(&work), 0, NULL);

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0, NULL);
}

/* Single CPU cancel before work item is unqueued should not wait. */
static void test_1cpu_queued_cancel_sync(void)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(&work, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Cancellation should complete immediately. */
	zassert_false(k_work_cancel_sync(&work, &work_sync), NULL);

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0, NULL);
}

/* Single CPU cancel before scheduled work item is queued should
 * complete immediately.
 */
static void test_1cpu_delayed_cancel(void)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Cancellation should complete immediately. */
	zassert_equal(k_work_cancel_delayable(&dwork), 0, NULL);

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0, NULL);
}


/* Single CPU cancel before scheduled work item is queued should not wait. */
static void test_1cpu_delayed_cancel_sync(void)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Cancellation should complete immediately. */
	zassert_false(k_work_cancel_delayable_sync(&dwork, &work_sync), NULL);

	/* Shouldn't have run. */
	zassert_equal(coophi_counter(), 0, NULL);
}

/* Single CPU cancel after delayable item starts should wait. */
static void test_1cpu_delayed_cancel_sync_wait(void)
{
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_QUEUED, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Get it to running, where it will block. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_RUNNING, NULL);

	/* Schedule to release, then cancel should delay. */
	async_release();
	zassert_true(k_work_cancel_delayable_sync(&dwork, &work_sync), NULL);

	/* Verify completion. */
	zassert_equal(coophi_counter(), 1, NULL);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
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
static void test_1cpu_running_cancel(void)
{
	struct test_running_cancel_timer *ctx = &test_running_cancel_ctx;
	struct k_work *wp = &ctx->work;
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(wp, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, wp);
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Release it so it's running. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0, NULL);

	/* Schedule the async process to capture state and release work. */
	ctx->submit_rc = INT_MAX;
	ctx->busy_rc = INT_MAX;
	k_timer_init(&ctx->timer, test_running_cancel_cb, NULL);
	k_timer_start(&ctx->timer, K_TICKS(1), K_NO_WAIT);

	/* Cancellation should not complete. */
	zassert_equal(k_work_cancel(wp), K_WORK_RUNNING | K_WORK_CANCELING,
		      NULL);

	/* Handler should not have run. */
	zassert_equal(coophi_counter(), 0, NULL);

	/* Wait for cancellation to complete. */
	zassert_true(k_work_cancel_sync(wp, &work_sync), NULL);

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);

	/* Handler should have detected running and canceling. */
	zassert_equal(ctx->busy_rc, K_WORK_RUNNING | K_WORK_CANCELING,
		      NULL);

	/* Attempt to submit while cancelling should have been
	 * rejected.
	 */
	zassert_equal(ctx->submit_rc, -EBUSY, NULL);

	/* Post-cancellation should have no flags. */
	rc = k_work_busy_get(wp);
	zassert_equal(rc, 0, "bad: %d", rc);
}

/* Single CPU test wait-for-cancellation after the work item has
 * started running.
 */
static void test_1cpu_running_cancel_sync(void)
{
	struct test_running_cancel_timer *ctx = &test_running_cancel_ctx;
	struct k_work *wp = &ctx->work;
	int rc;

	/* Reset state and use the blocking handler */
	reset_counters();
	k_work_init(wp, rel_handler);

	/* Submit to the cooperative queue. */
	rc = k_work_submit_to_queue(&coophi_queue, wp);
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Release it so it's running. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0, NULL);

	/* Schedule the async process to capture state and release work. */
	ctx->submit_rc = INT_MAX;
	ctx->busy_rc = INT_MAX;
	k_timer_init(&ctx->timer, test_running_cancel_cb, NULL);
	k_timer_start(&ctx->timer, K_TICKS(1), K_NO_WAIT);

	/* Cancellation should wait. */
	zassert_true(k_work_cancel_sync(wp, &work_sync), NULL);

	/* Handler should have run. */
	zassert_equal(coophi_counter(), 1, NULL);

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);

	/* Handler should have detected running and canceling. */
	zassert_equal(ctx->busy_rc, K_WORK_RUNNING | K_WORK_CANCELING,
		      NULL);

	/* Attempt to submit while cancelling should have been
	 * rejected.
	 */
	zassert_equal(ctx->submit_rc, -EBUSY, NULL);

	/* Post-cancellation should have no flags. */
	rc = k_work_busy_get(wp);
	zassert_equal(rc, 0, "bad: %d", rc);
}

/* SMP cancel after work item is started should succeed but require
 * wait.
 */
static void test_smp_running_cancel(void)
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
	zassert_equal(rc, 1, NULL);

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
	zassert_equal(k_work_cancel_sync(&work, &work_sync), true, NULL);

	/* Should have completed. */
	zassert_equal(coophi_counter(), 1, NULL);
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
}

/* Drain with no active workers completes immediately. */
static void test_drain_empty(void)
{
	int rc;

	rc = k_work_queue_drain(&coophi_queue, false);
	zassert_equal(rc, 0, NULL);
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
static void test_1cpu_drain_wait(void)
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
	zassert_equal(rc, 1, NULL);
	zassert_equal(coophi_counter(), 0, NULL);

	/* Schedule the async process to capture submission state
	 * while draining.
	 */
	ctx->submit_rc = INT_MAX;
	k_timer_init(&ctx->timer, test_drain_wait_cb, NULL);
	k_timer_start(&ctx->timer, K_TICKS(1), K_NO_WAIT);

	/* Wait to drain */
	rc = k_work_queue_drain(&coophi_queue, false);
	zassert_equal(rc, 1, NULL);

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);

	/* Confirm that chained submission worked, and non-chained
	 * submission failed.
	 */
	zassert_equal(coophi_counter(), 2, NULL);
	zassert_equal(ctx->submit_rc, -EBUSY, NULL);
}

/* Single CPU submit item, drain with plug, test, then unplug. */
static void test_1cpu_plugged_drain(void)
{
	int rc;

	/* Reset state and use the delaying handler. */
	reset_counters();
	k_work_init(&work, delay_handler);

	/* Submit to the cooperative queue */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);

	/* Wait to drain, and plug. */
	rc = k_work_queue_drain(&coophi_queue, true);
	zassert_equal(rc, 1, NULL);

	/* Verify completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
	zassert_equal(coophi_counter(), 1, NULL);

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
	zassert_equal(rc, -EBUSY, NULL);

	/* Unplug the queue */
	rc = k_work_queue_unplug(&coophi_queue);
	zassert_equal(rc, 0, NULL);
	zassert_equal(coophi_queue.flags,
		      K_WORK_QUEUE_STARTED | K_WORK_QUEUE_NO_YIELD,
		      NULL);

	/* Resubmission should succeed and complete */
	rc = k_work_submit_to_queue(&coophi_queue, &work);
	zassert_equal(rc, 1, NULL);

	/* Flush the sync state and verify completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);
	zassert_equal(coophi_counter(), 2, NULL);
}

/* Single CPU test delayed submission */
static void test_1cpu_basic_schedule(void)
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
	zassert_equal(k_work_busy_get(wp), 0, NULL);
	zassert_equal(wp->flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Align to tick, then schedule after normal delay. */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1, NULL);
	rc = k_work_busy_get(wp);
	zassert_equal(rc, K_WORK_DELAYED, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), rc, NULL);
	zassert_equal(k_work_delayable_is_pending(&dwork), true, NULL);

	/* Scheduling again does nothing. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Make sure it ran and is now idle */
	zassert_equal(coophi_counter(), 1, NULL);
	zassert_equal(k_work_busy_get(wp), 0, NULL);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

/* Single CPU test schedule without delay is queued immediately. */
static void test_1cpu_immed_schedule(void)
{
	int rc;
	struct k_work *wp = &dwork.work; /* whitebox testing */

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init_delayable(&dwork, counter_handler);
	zassert_equal(k_work_busy_get(wp), 0, NULL);

	/* Submit to the cooperative queue */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 1, NULL);
	rc = k_work_busy_get(wp);
	zassert_equal(rc, K_WORK_QUEUED, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), rc, NULL);
	zassert_equal(k_work_delayable_is_pending(&dwork), true, NULL);

	/* Scheduling again does nothing. */
	rc = k_work_schedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0, NULL);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 1, NULL);
	zassert_equal(k_work_busy_get(wp), 0, NULL);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
}

/* Single CPU test that delayed work can be rescheduled. */
static void test_1cpu_basic_reschedule(void)
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
	zassert_equal(k_work_busy_get(wp), 0, NULL);
	zassert_equal(wp->flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Schedule to the preempt queue after twice the standard
	 * delay.
	 */
	rc = k_work_reschedule_for_queue(&preempt_queue, &dwork,
					  K_MSEC(2U * DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED, NULL);

	/* Align to tick then reschedule on the cooperative queue for
	 * the standard delay.
	 */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_reschedule_for_queue(&coophi_queue, &dwork,
					  K_MSEC(DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED, NULL);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Make sure it ran on the coop queue and is now idle */
	zassert_equal(coophi_counter(), 1, NULL);
	zassert_equal(k_work_busy_get(wp), 0, NULL);

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
static void test_1cpu_immed_reschedule(void)
{
	int rc;
	struct k_work *wp = &dwork.work; /* whitebox testing */

	/* Reset state and use the delay handler */
	reset_counters();
	k_work_init_delayable(&dwork, delay_handler);
	zassert_equal(k_work_busy_get(wp), 0, NULL);

	/* Schedule immediately to the cooperative queue */
	rc = k_work_reschedule_for_queue(&coophi_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_QUEUED, NULL);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(coophi_counter(), 0, NULL);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(coophi_counter(), 0, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_RUNNING, NULL);

	/* Schedule immediately to the preemptive queue (will divert
	 * to coop since running).
	 */
	rc = k_work_reschedule_for_queue(&preempt_queue, &dwork, K_NO_WAIT);
	zassert_equal(rc, 2, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_QUEUED | K_WORK_RUNNING,
		      NULL);

	/* Schedule after 3x the delay to the preemptive queue
	 * (will not divert since previous submissions will have
	 * completed).
	 */
	rc = k_work_reschedule_for_queue(&preempt_queue, &dwork,
					  K_MSEC(3 * DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_busy_get(wp),
		      K_WORK_DELAYED | K_WORK_QUEUED | K_WORK_RUNNING,
		      NULL);

	/* Wait for the original no-wait submission (total 1 delay)) */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Check that coop ran once, and work is still delayed and
	 * also running.
	 */
	zassert_equal(coophi_counter(), 1, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED | K_WORK_RUNNING,
		      NULL);

	/* Wait for the queued no-wait submission (total 2 delay) */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Check that got diverted to coop and ran, and work is still
	 * delayed.
	 */
	zassert_equal(coophi_counter(), 2, NULL);
	zassert_equal(preempt_counter(), 0, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_DELAYED,
		      NULL);

	/* Wait for the delayed submission (total 3 delay) */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Check that ran on preempt.  In fact we're here because the
	 * test thread is higher priority, so the work will still be
	 * marked running.
	 */
	zassert_equal(coophi_counter(), 2, NULL);
	zassert_equal(preempt_counter(), 1, NULL);
	zassert_equal(k_work_busy_get(wp), K_WORK_RUNNING,
		      NULL);

	/* Wait for preempt to drain */
	rc = k_work_queue_drain(&preempt_queue, false);
	zassert_equal(rc, 1, NULL);
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
	zassert_equal(rc, 1, NULL);
	rc = k_work_schedule_for_queue(wq, &dwork, K_NO_WAIT);
	zassert_equal(rc, 1, NULL);

	/* Wait for completion */
	zassert_equal(k_work_is_pending(&work), true, NULL);
	zassert_equal(k_work_delayable_is_pending(&dwork), true, NULL);
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Because there was no yield both should have run, and
	 * another yield won't cause anything to happen.
	 */
	zassert_equal(coop_counter(wq), 2, NULL);
	zassert_equal(k_work_is_pending(&work), false, NULL);
	zassert_equal(k_work_delayable_is_pending(&dwork), false, NULL);

	/* The first give unblocked this thread; we need to consume
	 * the give from the second work task.
	 */
	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), 0, NULL);

	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), -EBUSY, NULL);

	return is_high;
}

/* Verify that no-yield policy works */
static void test_1cpu_queue_no_yield(void)
{
	zassert_equal(try_queue_no_yield(&coophi_queue), true, NULL);
	zassert_equal(try_queue_no_yield(&cooplo_queue), false, NULL);
}

/* Basic functionality with the system work queue. */
static void test_1cpu_system_queue(void)
{
	int rc;

	/* Reset state and use the non-blocking handler */
	reset_counters();
	k_work_init(&work, counter_handler);
	zassert_equal(k_work_busy_get(&work), 0, NULL);

	/* Submit to the system queue */
	rc = k_work_submit(&work);
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_busy_get(&work), K_WORK_QUEUED, NULL);

	/* Shouldn't have been started since test thread is
	 * cooperative.
	 */
	zassert_equal(system_counter(), 0, NULL);

	/* Let it run, then check it didn't finish. */
	k_sleep(K_TICKS(1));
	zassert_equal(system_counter(), 1, NULL);
	zassert_equal(k_work_busy_get(&work), 0, NULL);

	/* Flush the sync state from completion */
	rc = k_sem_take(&sync_sem, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);
}

static void test_1cpu_system_schedule(void)
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
	zassert_equal(k_work_delayable_busy_get(&dwork), 0, NULL);
	zassert_equal(dwork.work.flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Align to tick, then schedule after normal delay. */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_schedule(&dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_DELAYED, NULL);

	/* Scheduling again does nothing. */
	rc = k_work_schedule(&dwork, K_NO_WAIT);
	zassert_equal(rc, 0, NULL);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Make sure it ran and is now idle */
	zassert_equal(system_counter(), 1, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), 0, NULL);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

static void test_1cpu_system_reschedule(void)
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
	zassert_equal(k_work_delayable_busy_get(&dwork), 0, NULL);
	zassert_equal(dwork.work.flags & K_WORK_DELAYABLE, K_WORK_DELAYABLE,
		       NULL);

	/* Schedule to the preempt queue after twice the standard
	 * delay.
	 */
	rc = k_work_reschedule(&dwork, K_MSEC(2U * DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_DELAYED, NULL);

	/* Align to tick then reschedule on the system queue for
	 * the standard delay.
	 */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_work_reschedule(&dwork, K_MSEC(DELAY_MS));
	zassert_equal(rc, 1, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), K_WORK_DELAYED, NULL);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Make sure it ran on the system queue and is now idle */
	zassert_equal(system_counter(), 1, NULL);
	zassert_equal(k_work_delayable_busy_get(&dwork), 0, NULL);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

/* Single CPU test legacy delayed API */
static void test_1cpu_legacy_delayed_submit(void)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = k_ticks_to_ms_ceil32(1U
				+ k_ms_to_ticks_ceil32(DELAY_MS));
	uint32_t elapsed_ms;
	struct k_delayed_work lwork;

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_delayed_work_init(&lwork, counter_handler);

	/* Verify that work is not pending */
	zassert_false(k_delayed_work_pending(&lwork), NULL);

	/* Align to tick, then schedule after normal delay. */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_delayed_work_submit_to_queue(&coophi_queue, &lwork,
					    K_MSEC(DELAY_MS));
	zassert_equal(rc, 0, NULL);
	zassert_true(k_delayed_work_pending(&lwork), NULL);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Make sure it ran and is now idle */
	zassert_equal(coophi_counter(), 1, NULL);
	rc = k_work_delayable_busy_get(&lwork.work);
	zassert_false(k_delayed_work_pending(&lwork), "rc %d", rc);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

/* Single CPU test legacy delayed API resubmit */
static void test_1cpu_legacy_delayed_resubmit(void)
{
	int rc;
	uint32_t sched_ms;
	uint32_t max_ms = k_ticks_to_ms_ceil32(1U
				+ k_ms_to_ticks_ceil32(DELAY_MS));
	uint32_t elapsed_ms;
	struct k_delayed_work lwork;

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_delayed_work_init(&lwork, counter_handler);

	/* Verify that work is not pending */
	zassert_false(k_delayed_work_pending(&lwork), NULL);

	/* Schedule to the preempt queue after twice the standard
	 * delay.
	 */
	rc = k_delayed_work_submit_to_queue(&preempt_queue, &lwork,
					    K_MSEC(2 * DELAY_MS));
	zassert_equal(rc, 0, NULL);
	zassert_true(k_delayed_work_pending(&lwork), NULL);

	/* Align to tick then schedule after standard delay */
	k_sleep(K_TICKS(1));
	sched_ms = k_uptime_get_32();
	rc = k_delayed_work_submit_to_queue(&coophi_queue, &lwork,
					    K_MSEC(DELAY_MS));
	zassert_equal(rc, 0, NULL);
	zassert_true(k_delayed_work_pending(&lwork), NULL);

	/* Wait for completion */
	rc = k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(rc, 0, NULL);

	/* Make sure it ran and is now idle */
	rc = k_work_delayable_busy_get(&lwork.work);
	zassert_false(k_delayed_work_pending(&lwork), "rc %d", rc);
	zassert_equal(coophi_counter(), 1, NULL);

	/* Check that the delay is within the expected range. */
	elapsed_ms = last_handle_ms - sched_ms;
	zassert_true(elapsed_ms >= DELAY_MS,
		     "short %u < %u\n", elapsed_ms, DELAY_MS);
	zassert_true(elapsed_ms <= max_ms,
		     "long %u > %u\n", elapsed_ms, max_ms);
}

/* Single CPU test legacy delayed cancel */
static void test_1cpu_legacy_delayed_cancel(void)
{
	int rc;
	struct k_delayed_work lwork;

	/* Reset state and use non-blocking handler */
	reset_counters();
	k_delayed_work_init(&lwork, counter_handler);

	/* Verify that work is not pending */
	zassert_false(k_delayed_work_pending(&lwork), NULL);

	/* Cancel should be -EALREADY if not submitted/active; this
	 * does not match original behavior (-EINVAL), but it's what
	 * we can do.
	 */
	rc = k_delayed_work_cancel(&lwork);

	/* Submit after standard delay */
	rc = k_delayed_work_submit_to_queue(&coophi_queue, &lwork,
					    K_MSEC(DELAY_MS));
	zassert_equal(rc, 0, NULL);
	zassert_true(k_delayed_work_pending(&lwork), NULL);

	/* Cancel should succeed */
	rc = k_delayed_work_cancel(&lwork);
	zassert_equal(rc, 0, NULL);
	zassert_false(k_delayed_work_pending(&lwork), NULL);
}


static void test_nop(void)
{
	ztest_test_skip();
}

void test_main(void)
{
	main_thread = k_current_get();
	k_sem_init(&sync_sem, 0, 1);
	k_sem_init(&rel_sem, 0, 1);

	ztest_test_suite(work,
			 ztest_unit_test(test_work_init),
			 ztest_unit_test(test_delayable_init),
			 ztest_unit_test(test_legacy_delayed_init),
			 ztest_unit_test(test_unstarted),
			 ztest_unit_test(test_queue_start),
			 ztest_unit_test(test_null_queue),
			 ztest_1cpu_unit_test(test_1cpu_simple_queue),
			 ztest_unit_test(test_smp_simple_queue),
			 ztest_1cpu_unit_test(test_1cpu_sync_queue),
			 ztest_1cpu_unit_test(test_1cpu_reentrant_queue),
			 ztest_1cpu_unit_test(test_1cpu_queued_flush),
			 ztest_1cpu_unit_test(test_1cpu_running_flush),
			 ztest_1cpu_unit_test(test_1cpu_queued_cancel),
			 ztest_1cpu_unit_test(test_1cpu_queued_cancel_sync),
			 ztest_1cpu_unit_test(test_1cpu_running_cancel),
			 ztest_1cpu_unit_test(test_1cpu_running_cancel_sync),
			 ztest_unit_test(test_smp_running_cancel),
			 ztest_unit_test(test_drain_empty),
			 ztest_1cpu_unit_test(test_1cpu_drain_wait),
			 ztest_1cpu_unit_test(test_1cpu_plugged_drain),
			 ztest_1cpu_unit_test(test_1cpu_basic_schedule),
			 ztest_1cpu_unit_test(test_1cpu_immed_schedule),
			 ztest_1cpu_unit_test(test_1cpu_basic_reschedule),
			 ztest_1cpu_unit_test(test_1cpu_immed_reschedule),
			 ztest_1cpu_unit_test(test_1cpu_delayed_flush),
			 ztest_1cpu_unit_test(test_1cpu_delayed_cancel_sync),
			 ztest_1cpu_unit_test(test_1cpu_delayed_cancel_sync_wait),
			 ztest_1cpu_unit_test(test_1cpu_delayed_cancel),
			 ztest_1cpu_unit_test(test_1cpu_queue_no_yield),
			 ztest_1cpu_unit_test(test_1cpu_system_queue),
			 ztest_1cpu_unit_test(test_1cpu_system_schedule),
			 ztest_1cpu_unit_test(test_1cpu_system_reschedule),
			 ztest_1cpu_unit_test(test_1cpu_legacy_delayed_submit),
			 ztest_1cpu_unit_test(
				 test_1cpu_legacy_delayed_resubmit),
			 ztest_1cpu_unit_test(test_1cpu_legacy_delayed_cancel),
			 ztest_unit_test(test_nop));
	ztest_run_test_suite(work);
}
