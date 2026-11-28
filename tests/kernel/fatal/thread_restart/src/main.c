/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_restart.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define STACK_SIZE   (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_PRIO  K_PRIO_PREEMPT(5)
#define THREAD_NAME  "restartable"
#define WAIT_TIMEOUT K_SECONDS(5)
#define POLL_SLOW_MS 10000U

static K_THREAD_STACK_DEFINE(test_stack, STACK_SIZE);
static struct k_thread test_thread;

/* Resource pool for test_restart_resources; K_HEAP_DEFINE() cannot be used in a function */
static K_HEAP_DEFINE(restart_heap, 256);

/* Shared with k_sys_fatal_error_handler() */
static struct k_thread_restart *volatile active_rst;
static volatile int last_schedule_ret;
static atomic_t fault_count;
static atomic_t unexpected_faults;

/* Descriptor for the ztest thread itself, used by tests that schedule a restart without
 * faulting. Shared with thread_restart_after(), which cancels it so a failed test does not
 * leave a restart of the ztest thread scheduled.
 */
static struct k_thread_restart self_rst;

struct run_ctx {
	/* Number of times the thread entry point ran */
	atomic_t runs;

	/* Number of runs that fault before one completes, -1 to always fault */
	int32_t fault_runs;

	/* Fault with a CPU exception instead of k_oops() */
	bool cpu_exception;

	/* First run schedules its own restart outside the fatal path and exits after a delay
	 * without faulting, so the restart work polls (with backoff) until the thread has exited.
	 */
	bool self_schedule;

	/* Uptime when the self-scheduling first run exited */
	int64_t first_exit_ms;

	/* Uptime when the last run started */
	int64_t last_run_ms;

	/* Resource pool of the first run */
	struct k_heap *first_pool;

	/* Resource pool of the last run */
	struct k_heap *last_pool;

	/* Memory domain of the first run */
	struct k_mem_domain *first_domain;

	/* Memory domain of the last run */
	struct k_mem_domain *last_domain;

	/* Priority of the last run */
	int last_prio;

	/* Number of restart callback calls */
	atomic_t cb_calls;

	/* Value the restart callback returns */
	int cb_ret;

	/* Thread passed to the restart callback */
	k_tid_t cb_thread;

	/* Number of runs when the restart callback was called */
	atomic_val_t cb_runs;

	/* Given by the run that completes */
	struct k_sem done;
};

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	struct k_thread_restart *rst = active_rst;

	ARG_UNUSED(esf);

	if (reason == K_ERR_KERNEL_PANIC) {
		k_fatal_halt(reason);
	}

	if ((rst == NULL) || (rst->thread != k_current_get())) {
		atomic_inc(&unexpected_faults);
		return;
	}

	last_schedule_ret = k_thread_restart_schedule(rst);
	atomic_inc(&fault_count);
}

static void trigger_cpu_exception(void)
{
#if defined(CONFIG_X86)
	__asm__ volatile("ud2");
#elif defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_ARM64)
	__asm__ volatile("udf #0");
#else
	k_oops();
#endif
}

static void test_entry(void *p1, void *p2, void *p3)
{
	struct run_ctx *ctx = p1;
	atomic_val_t run = atomic_inc(&ctx->runs);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ctx->last_run_ms = k_uptime_get();
	ctx->last_prio = k_thread_priority_get(k_current_get());
	ctx->last_pool = k_current_get()->resource_pool;
#ifdef CONFIG_USERSPACE
	ctx->last_domain = k_current_get()->mem_domain_info.mem_domain;
#endif
	if (run == 0) {
		ctx->first_pool = ctx->last_pool;
		ctx->first_domain = ctx->last_domain;
	}

	if (ctx->self_schedule && (run == 0)) {
		last_schedule_ret = k_thread_restart_schedule(active_rst);
		/* The restart work polls until this thread has exited */
		k_msleep(50);
		ctx->first_exit_ms = k_uptime_get();
		return;
	}

	if ((ctx->fault_runs < 0) || (run < ctx->fault_runs)) {
		if (ctx->cpu_exception) {
			trigger_cpu_exception();
		} else {
			k_oops();
		}

		/* Not reached: the fatal path aborts the thread */
		ztest_test_fail();
	}

	k_sem_give(&ctx->done);
}

static void start_thread_with(const struct k_thread_restart *rst, struct k_heap *pool,
			      struct k_mem_domain *domain)
{
	k_tid_t tid = k_thread_create(rst->thread, rst->stack, rst->stack_size, rst->entry, rst->p1,
				      rst->p2, rst->p3, rst->prio, rst->options, K_FOREVER);

	if (rst->name != NULL) {
		zassert_ok(k_thread_name_set(tid, rst->name));
	}
	if (pool != NULL) {
		k_thread_heap_assign(tid, pool);
	}
	if (IS_ENABLED(CONFIG_USERSPACE) && (domain != NULL)) {
		zassert_ok(k_mem_domain_add_thread(domain, tid));
	}
	k_thread_start(tid);
}

static void start_thread(const struct k_thread_restart *rst)
{
	start_thread_with(rst, NULL, NULL);
}

static void run_restart_test_with(struct k_thread_restart *rst, struct run_ctx *ctx,
				  uint32_t expected_restarts, struct k_heap *pool,
				  struct k_mem_domain *domain)
{
	k_sem_init(&ctx->done, 0, 1);
	active_rst = rst;

	start_thread_with(rst, pool, domain);

	zassert_ok(k_sem_take(&ctx->done, WAIT_TIMEOUT), "thread did not complete");
	zassert_ok(k_thread_join(&test_thread, WAIT_TIMEOUT));

	zassert_equal(last_schedule_ret, 0);
	zassert_equal((uint32_t)atomic_get(&fault_count), expected_restarts);
	zassert_equal((uint32_t)atomic_get(&ctx->runs), expected_restarts + 1U);
	/* The restarted thread keeps the resource pool and memory domain of the first run */
	zassert_equal_ptr(ctx->last_pool, ctx->first_pool);
	zassert_equal_ptr(ctx->last_domain, ctx->first_domain);
	zassert_equal(k_thread_restart_count_get(rst), expected_restarts);
	zassert_str_equal(k_thread_name_get(&test_thread), THREAD_NAME);
}

static void run_restart_test(struct k_thread_restart *rst, struct run_ctx *ctx,
			     uint32_t expected_restarts)
{
	run_restart_test_with(rst, ctx, expected_restarts, NULL, NULL);
}

ZTEST(thread_restart, test_restart_after_oops)
{
	static struct run_ctx ctx = {
		.fault_runs = 3,
	};
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, NULL);

	run_restart_test(&rst, &ctx, 3U);
}

ZTEST(thread_restart, test_restart_after_cpu_exception)
{
	static struct run_ctx ctx = {
		.fault_runs = 1,
		.cpu_exception = true,
	};
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, NULL);

	if (!IS_ENABLED(CONFIG_X86) && !IS_ENABLED(CONFIG_CPU_CORTEX_M) &&
	    !IS_ENABLED(CONFIG_ARM64)) {
		ztest_test_skip();
	}

	run_restart_test(&rst, &ctx, 1U);
}

ZTEST(thread_restart, test_restart_resources)
{
	static struct run_ctx ctx = {
		.fault_runs = 1,
	};
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, NULL);

	struct k_mem_domain *domain = NULL;

#ifdef CONFIG_USERSPACE
	/* struct k_mem_domain is only defined with CONFIG_USERSPACE */
	static struct k_mem_domain restart_domain;
	static bool restart_domain_ready;

	/* Initialize once: each initialization takes resources, e.g. page tables on x86 */
	if (!restart_domain_ready) {
		zassert_ok(k_mem_domain_init(&restart_domain, 0, NULL));
		restart_domain_ready = true;
	}
	domain = &restart_domain;
#endif

	/* Neither matches the system workqueue thread's unless taken from the faulting thread */
	run_restart_test_with(&rst, &ctx, 1U, &restart_heap, domain);
	zassert_equal_ptr(ctx.last_pool, &restart_heap);
	zassert_equal_ptr(ctx.last_domain, domain);
}

/* Context reached from the descriptor, lowers the priority of the restarted thread */
static int restart_cb(struct k_thread_restart *rst, k_tid_t thread)
{
	struct run_ctx *ctx = rst->p1;

	ctx->cb_thread = thread;
	ctx->cb_runs = atomic_get(&ctx->runs);
	atomic_inc(&ctx->cb_calls);

	if (ctx->cb_ret == 0) {
		k_thread_priority_set(thread, THREAD_PRIO + 1);
	}

	return ctx->cb_ret;
}

ZTEST(thread_restart, test_restart_callback)
{
	static struct run_ctx ctx = {
		.fault_runs = 2,
	};
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, restart_cb);

	run_restart_test(&rst, &ctx, 2U);

	/* Called for each restart, before the thread ran again */
	zassert_equal(atomic_get(&ctx.cb_calls), 2);
	zassert_equal_ptr(ctx.cb_thread, &test_thread);
	zassert_equal(ctx.cb_runs, 2);
	zassert_equal(ctx.last_prio, THREAD_PRIO + 1);
}

ZTEST(thread_restart, test_restart_callback_veto)
{
	static struct run_ctx ctx = {
		.fault_runs = -1,
		.cb_ret = -ECANCELED,
	};
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, restart_cb);
	int64_t start = k_uptime_get();

	active_rst = &rst;

	start_thread(&rst);

	while ((atomic_get(&ctx.cb_calls) == 0) && ((k_uptime_get() - start) < 5000)) {
		k_msleep(10);
	}

	/* The vetoed thread is aborted, not started */
	zassert_ok(k_thread_join(&test_thread, WAIT_TIMEOUT));
	zassert_equal(atomic_get(&ctx.cb_calls), 1);
	zassert_equal(last_schedule_ret, 0);
	zassert_equal(k_thread_restart_count_get(&rst), 1U);

	k_msleep(50);
	zassert_equal(atomic_get(&ctx.runs), 1);
	zassert_equal(atomic_get(&fault_count), 1);
}

ZTEST(thread_restart, test_restart_waits_for_exit)
{
	static struct run_ctx ctx = {
		.self_schedule = true,
	};
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, NULL);

	zassert_ok(k_thread_restart_poll_set(&rst, 1U, 8U));

	k_sem_init(&ctx.done, 0, 1);
	active_rst = &rst;

	start_thread(&rst);

	zassert_ok(k_sem_take(&ctx.done, WAIT_TIMEOUT), "thread was not restarted");
	zassert_ok(k_thread_join(&test_thread, WAIT_TIMEOUT));

	zassert_equal(last_schedule_ret, 0);
	zassert_equal(atomic_get(&fault_count), 0);
	zassert_equal(atomic_get(&ctx.runs), 2);
	zassert_equal(k_thread_restart_count_get(&rst), 1U);
	zassert_true(ctx.last_run_ms >= ctx.first_exit_ms, "restarted before exit");

	/* Polling backed off up to the 8 ms ceiling while the thread slept. poll_ticks is a
	 * private member, checked here because the backoff has no other observable effect.
	 */
	zassert_equal(rst.poll_ticks, (k_ticks_t)k_ms_to_ticks_ceil64(8U));
}

ZTEST(thread_restart, test_restart_limit)
{
	static struct run_ctx ctx = {
		.fault_runs = -1,
	};
	/* Runtime-initialized descriptor without a thread name */
	static struct k_thread_restart rst;
	int64_t start = k_uptime_get();

	k_thread_restart_init(&rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx, NULL,
			      NULL, THREAD_PRIO, 0U, NULL, 2U, NULL);

	k_sem_init(&ctx.done, 0, 1);
	active_rst = &rst;

	start_thread(&rst);

	while ((atomic_get(&fault_count) < 3) && ((k_uptime_get() - start) < 5000)) {
		k_msleep(10);
	}

	zassert_equal(atomic_get(&fault_count), 3);
	zassert_ok(k_thread_join(&test_thread, WAIT_TIMEOUT));
	zassert_equal(last_schedule_ret, -ENOSPC);

	/* No restart after the limit */
	k_msleep(100);
	zassert_equal(atomic_get(&ctx.runs), 3);
	zassert_equal(k_thread_restart_count_get(&rst), 2U);
}

ZTEST(thread_restart, test_reject_not_current)
{
	static struct run_ctx ctx;

	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, NULL);
	static K_THREAD_RESTART_DEFINE(no_thread, NULL, test_stack, STACK_SIZE, test_entry, &ctx,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, NULL);

	zassert_equal(k_thread_restart_schedule(NULL), -EINVAL);
	zassert_equal(k_thread_restart_schedule(&no_thread), -EINVAL);
	zassert_equal(k_thread_restart_schedule(&rst), -EINVAL);
	zassert_equal(k_thread_restart_count_get(&rst), 0U);
}

ZTEST(thread_restart, test_reject_options)
{
	static struct k_thread_restart rst;

	k_thread_restart_init(&rst, k_current_get(), NULL, 0U, NULL, NULL, NULL, NULL, 0, 0U, NULL,
			      0U, NULL);

	rst.options = K_USER;
	zassert_equal(k_thread_restart_schedule(&rst), -ENOTSUP);

	rst.options = K_ESSENTIAL;
	zassert_equal(k_thread_restart_schedule(&rst), -ENOTSUP);

	/* Must not inherit object permissions from the workqueue thread */
	rst.options = K_INHERIT_PERMS;
	zassert_equal(k_thread_restart_schedule(&rst), -EINVAL);

	zassert_equal(k_thread_restart_count_get(&rst), 0U);
}

static void user_oops_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_oops();
}

static void enter_user_mode_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_thread_user_mode_enter(user_oops_entry, NULL, NULL, NULL);
}

ZTEST(thread_restart, test_reject_user_mode_thread)
{
	/* Supervisor options, but the thread drops to user mode before faulting */
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE,
				       enter_user_mode_entry, NULL, NULL, NULL, THREAD_PRIO, 0,
				       THREAD_NAME, 0, NULL);

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	active_rst = &rst;

	start_thread(&rst);

	zassert_ok(k_thread_join(&test_thread, WAIT_TIMEOUT));
	zassert_equal(atomic_get(&fault_count), 1);
	zassert_equal(last_schedule_ret, -ENOTSUP);

	/* No restart */
	k_msleep(50);
	zassert_equal(k_thread_restart_count_get(&rst), 0U);
	zassert_equal(atomic_get(&fault_count), 1);
}

static int sysworkq_ret;

static void sysworkq_handler(struct k_work *work)
{
	static struct k_thread_restart rst;

	ARG_UNUSED(work);

	k_thread_restart_init(&rst, k_current_get(), NULL, 0U, NULL, NULL, NULL, NULL, 0, 0U, NULL,
			      0U, NULL);

	sysworkq_ret = k_thread_restart_schedule(&rst);
}

ZTEST(thread_restart, test_reject_sysworkq)
{
	static K_WORK_DEFINE(work, sysworkq_handler);
	static struct k_work_sync sync;

	zassert_true(k_work_submit(&work) >= 0);

	/* The item may already have completed, so the result is not checked */
	(void)k_work_flush(&work, &sync);
	zassert_equal(sysworkq_ret, -ENOTSUP);
}

/* Wait until the restart work has run once after a self-schedule, found the ztest thread alive
 * and is waiting for the poll interval. Otherwise a failed assertion aborts the ztest thread
 * while the work is still queued, and the work would re-create the thread from a descriptor
 * without stack or entry point before thread_restart_after() can cancel it. Polling the work
 * state works whichever CPU and priority the system workqueue runs at. A timeout is not an
 * assertion: failing here would abort the ztest thread with the work still queued.
 */
static void wait_for_restart_poll(struct k_thread_restart *rst)
{
	int64_t start = k_uptime_get();

	while ((k_work_delayable_busy_get(&rst->work) != K_WORK_DELAYED) &&
	       ((k_uptime_get() - start) < 1000)) {
		k_msleep(1);
	}
}

ZTEST(thread_restart, test_already_pending)
{
	static struct k_work_sync sync;
	struct k_thread_restart *rst = &self_rst;

	k_thread_restart_init(rst, k_current_get(), NULL, 0U, NULL, NULL, NULL, NULL, 0, 0U, NULL,
			      0U, NULL);

	zassert_ok(k_thread_restart_poll_set(rst, POLL_SLOW_MS, POLL_SLOW_MS));

	zassert_ok(k_thread_restart_schedule(rst));
	wait_for_restart_poll(rst);

	zassert_equal(k_thread_restart_schedule(rst), -EALREADY);
	zassert_equal(k_thread_restart_poll_set(rst, 1U, 10U), -EBUSY);
	zassert_equal(k_thread_restart_count_get(rst), 1U);

	/* Cancel while the thread is alive, so the restart work never finds it exited */
	(void)k_work_cancel_delayable_sync(&rst->work, &sync);
}

ZTEST(thread_restart, test_queue_plugged)
{
	static struct k_work_sync sync;
	struct k_thread_restart *rst = &self_rst;
	int64_t start;

	k_thread_restart_init(rst, k_current_get(), NULL, 0U, NULL, NULL, NULL, NULL, 0, 0U, NULL,
			      0U, NULL);
	zassert_ok(k_thread_restart_poll_set(rst, 10U, 10U));

	/* The thread is alive, so the restart work keeps polling for it to exit */
	zassert_ok(k_thread_restart_schedule(rst));
	wait_for_restart_poll(rst);

	/* Other submissions to the system workqueue are refused until the queue is unplugged,
	 * which is harmless on the emulated test platforms. The next poll is then dropped.
	 */
	zassert_true(k_work_queue_drain(&k_sys_work_q, true) >= 0);

	start = k_uptime_get();
	while (k_work_delayable_is_pending(&rst->work) && ((k_uptime_get() - start) < 1000)) {
		k_msleep(10);
	}
	zassert_false(k_work_delayable_is_pending(&rst->work));

	/* Immediate submission is refused and not counted */
	zassert_equal(k_thread_restart_schedule(rst), -EBUSY);
	zassert_equal(k_thread_restart_count_get(rst), 1U);

	/* A dropped restart does not block new ones */
	zassert_ok(k_work_queue_unplug(&k_sys_work_q));
	zassert_ok(k_thread_restart_poll_set(rst, POLL_SLOW_MS, POLL_SLOW_MS));
	zassert_ok(k_thread_restart_schedule(rst));
	wait_for_restart_poll(rst);
	zassert_equal(k_thread_restart_count_get(rst), 2U);

	/* Cancel while the thread is alive, as in test_already_pending */
	(void)k_work_cancel_delayable_sync(&rst->work, &sync);
}

ZTEST(thread_restart, test_poll_set)
{
	static K_THREAD_RESTART_DEFINE(rst, &test_thread, test_stack, STACK_SIZE, test_entry, NULL,
				       NULL, NULL, THREAD_PRIO, 0, THREAD_NAME, 0, NULL);

	zassert_equal(rst.poll_min_ms, (uint32_t)CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MIN_MS);
	zassert_equal(rst.poll_max_ms, (uint32_t)CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MAX_MS);

	zassert_equal(k_thread_restart_poll_set(NULL, 1U, 2U), -EINVAL);
	zassert_equal(k_thread_restart_poll_set(&rst, 0U, 2U), -EINVAL);
	zassert_equal(k_thread_restart_poll_set(&rst, 3U, 2U), -EINVAL);

	zassert_ok(k_thread_restart_poll_set(&rst, 5U, 50U));
	zassert_equal(rst.poll_min_ms, 5U);
	zassert_equal(rst.poll_max_ms, 50U);
}

static void thread_restart_before(void *fixture)
{
	ARG_UNUSED(fixture);

	active_rst = NULL;
	last_schedule_ret = INT_MIN;
	atomic_clear(&fault_count);
	atomic_clear(&unexpected_faults);
}

static void thread_restart_after(void *fixture)
{
	static struct k_work_sync sync;

	ARG_UNUSED(fixture);

	/* Undo what a test may have left behind, also when it failed: a scheduled restart of
	 * the ztest thread and a plugged system workqueue.
	 */
	(void)k_work_cancel_delayable_sync(&self_rst.work, &sync);
	(void)k_work_queue_unplug(&k_sys_work_q);

	active_rst = NULL;
	zassert_equal(atomic_get(&unexpected_faults), 0, "unexpected fatal error");
}

ZTEST_SUITE(thread_restart, NULL, NULL, thread_restart_before, thread_restart_after, NULL);
