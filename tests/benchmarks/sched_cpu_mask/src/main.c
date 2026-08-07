/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SMP CPU Mask Benchmark
 *
 * Measures two performance aspects of CONFIG_SCHED_CPU_MASK:
 *
 *   1. cpu_mask_api  - Direct API call latency for k_thread_cpu_pin,
 *      k_thread_cpu_mask_clear, k_thread_cpu_mask_enable_all,
 *      k_thread_cpu_mask_enable and k_thread_cpu_mask_disable, each
 *      applied to a pre-start (prevented-from-running) thread.
 *
 *   2. cpu_mask_wakeup - Thread wakeup round-trip latency as a
 *      function of run-queue depth.  N cooperative threads are all
 *      pinned to CPU 1 and kept runnable; a preemptive target thread
 *      is pinned to CPU 0.  The benchmark thread (cooperative, on
 *      CPU 0) ping-pongs a semaphore with the target so every
 *      iteration forces two scheduler best-thread searches that must
 *      walk past all N - 1 queued noise threads before finding the
 *      eligible one.  Varying N from 0 to 16 directly exposes the
 *      O(N) scan cost for SCHED_SIMPLE and SCHED_SCALABLE, and the
 *      per-bucket list cost for SCHED_MULTIQ.
 *
 * Priority layout used by the wakeup suite:
 *
 *   K_PRIO_COOP(0)   = -NUM_COOP_PRIORITIES  (NOISE threads, highest)
 *   K_PRIO_COOP(...)                          (ztest runner default = -1)
 *   K_PRIO_PREEMPT(0) = 0                     (TARGET thread)
 *
 * Noise threads are pinned to CPU 1 so they never run on CPU 0 but
 * their TCBs remain in the run queue, forcing the scheduler to scan
 * past them on every context switch.  CPU 1 can only execute one
 * noise thread at a time; the remaining N - 1 accumulate in the
 * ready queue and constitute the scan depth being measured.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS >= 2,
	     "This benchmark requires at least 2 CPUs (CONFIG_MP_MAX_NUM_CPUS >= 2)");

/* ------------------------------------------------------------------ */
/* API latency suite                                                   */
/* ------------------------------------------------------------------ */

#define API_STACK_SIZE  256U

static struct k_thread api_thread;
static K_THREAD_STACK_DEFINE(api_stack, API_STACK_SIZE);
static k_tid_t api_tid;

/*
 * Dummy entry – never actually executed.  The thread is created with
 * K_FOREVER so it remains in _THREAD_PRESTART state, satisfying
 * z_is_thread_prevented_from_running() throughout the suite.
 */
static void api_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
}

static void api_suite_setup(void)
{
	api_tid = k_thread_create(&api_thread, api_stack,
				  API_STACK_SIZE, api_thread_fn,
				  NULL, NULL, NULL,
				  K_PRIO_PREEMPT(4), 0U, K_FOREVER);
}

static void api_suite_teardown(void)
{
	k_thread_abort(api_tid);
}

ZTEST_BENCHMARK_SUITE(cpu_mask_api, api_suite_setup, api_suite_teardown);

/*
 * k_thread_cpu_pin: valid in all modes including PIN_ONLY.
 * Repeated calls are idempotent (mask stays BIT(0)).
 */
ZTEST_BENCHMARK(cpu_mask_api, cpu_pin, 10000U, NULL, NULL)
{
	(void)k_thread_cpu_pin(api_tid, 0);
}

#if !defined(CONFIG_SCHED_CPU_MASK_PIN_ONLY)
/*
 * Restore all CPU bits before benchmarking a clearing operation so
 * that the clear has real work to do each iteration.
 */
static void restore_all_cpus(void)
{
	(void)k_thread_cpu_mask_enable_all(api_tid);
}

/*
 * Restore to cleared state before benchmarking an enable operation.
 */
static void restore_cleared(void)
{
	(void)k_thread_cpu_mask_clear(api_tid);
}

/*
 * Restore to a two-bit mask (CPUs 0 and 1) before benchmarking a
 * single-CPU disable so there is always a bit to remove.
 */
static void restore_two_bits(void)
{
	(void)k_thread_cpu_mask_clear(api_tid);
	(void)k_thread_cpu_mask_enable(api_tid, 0);
	(void)k_thread_cpu_mask_enable(api_tid, 1);
}

ZTEST_BENCHMARK(cpu_mask_api, cpu_mask_clear, 10000U, restore_all_cpus, NULL)
{
	(void)k_thread_cpu_mask_clear(api_tid);
}

ZTEST_BENCHMARK(cpu_mask_api, cpu_mask_enable_all, 10000U, restore_cleared, NULL)
{
	(void)k_thread_cpu_mask_enable_all(api_tid);
}

ZTEST_BENCHMARK(cpu_mask_api, cpu_mask_enable, 10000U, restore_cleared, NULL)
{
	(void)k_thread_cpu_mask_enable(api_tid, 0);
}

ZTEST_BENCHMARK(cpu_mask_api, cpu_mask_disable, 10000U, restore_two_bits, NULL)
{
	(void)k_thread_cpu_mask_disable(api_tid, 0);
}
#endif /* !CONFIG_SCHED_CPU_MASK_PIN_ONLY */

/* ------------------------------------------------------------------ */
/* Wakeup latency suite                                                */
/* ------------------------------------------------------------------ */

/*
 * Noise threads run at the highest cooperative priority so they
 * appear at the head of the run queue and force the scheduler to
 * scan past them before finding the preemptive target on CPU 0.
 *
 * TARGET_PRIO is preemptive so the benchmark thread (cooperative)
 * is not preempted when it posts sem_to_target; it continues to
 * k_sem_take, blocks, and the scheduler then performs the scan.
 */
#define WAKEUP_STACK_SIZE  1024U
#define NOISE_PRIO         K_PRIO_COOP(0)
#define TARGET_PRIO        K_PRIO_PREEMPT(0)
#define MAX_NOISE          16U

static struct k_sem sem_to_target;
static struct k_sem sem_from_target;

static struct k_thread target_thread;
static K_THREAD_STACK_DEFINE(target_stack, WAKEUP_STACK_SIZE);

static struct k_thread noise_threads[MAX_NOISE];
static K_THREAD_STACK_ARRAY_DEFINE(noise_stacks, MAX_NOISE, WAKEUP_STACK_SIZE);

static void target_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		(void)k_sem_take(&sem_to_target, K_FOREVER);
		k_sem_give(&sem_from_target);
	}
}

/*
 * Cooperative spin thread pinned to CPU 1.  Never yields and never
 * blocks so it remains runnable, keeping the remaining noise threads
 * in the ready queue as the scan depth to be measured.
 */
static void noise_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		k_busy_wait(1000U);
	}
}

static void wakeup_setup(uint32_t noise_count)
{
	k_tid_t tid;

	k_sem_init(&sem_to_target,   0, 1);
	k_sem_init(&sem_from_target, 0, 1);

	/* Target pinned to CPU 0 – the scheduler must reach this
	 * thread after scanning past all noise entries on CPU 1.
	 */
	tid = k_thread_create(&target_thread, target_stack,
			      WAKEUP_STACK_SIZE, target_fn,
			      NULL, NULL, NULL,
			      TARGET_PRIO, 0U, K_FOREVER);
	(void)k_thread_cpu_pin(tid, 0);
	k_thread_start(tid);

	/* Noise threads pinned to CPU 1.  All run at the same
	 * cooperative priority so they land in the same priority
	 * bucket, maximising the scan depth per bucket in MULTIQ
	 * and forcing a full in-order walk in SIMPLE/SCALABLE.
	 */
	for (uint32_t i = 0U; i < noise_count; i++) {
		tid = k_thread_create(&noise_threads[i], noise_stacks[i],
				      WAKEUP_STACK_SIZE, noise_fn,
				      NULL, NULL, NULL,
				      NOISE_PRIO, 0U, K_FOREVER);
		(void)k_thread_cpu_pin(tid, 1);
		k_thread_start(tid);
	}
}

static void wakeup_teardown(uint32_t noise_count)
{
	k_thread_abort(&target_thread);

	for (uint32_t i = 0U; i < noise_count; i++) {
		k_thread_abort(&noise_threads[i]);
	}
}

/*
 * Single round-trip body reused by every wakeup variant:
 *
 *   1. Post sem_to_target  -> target becomes ready (prio 0).
 *      Benchmark thread is cooperative (prio -1) so it is NOT
 *      preempted; it falls through to k_sem_take and blocks.
 *   2. Scheduler on CPU 0 scans run queue: skips N - 1 noise
 *      entries (all CPU-1-pinned), then switches to target.
 *   3. Target posts sem_from_target.  Benchmark thread (prio -1)
 *      becomes ready; since -1 < 0, target is preempted immediately.
 *   4. k_sem_take returns.  One iteration complete.
 *
 * Total scheduler scans per iteration: 2 x (N - 1) skip steps.
 */
static void wakeup_run_body(void)
{
	k_sem_give(&sem_to_target);
	(void)k_sem_take(&sem_from_target, K_FOREVER);
}

ZTEST_BENCHMARK_SUITE(cpu_mask_wakeup, NULL, NULL);

/* 0 noise threads – baseline: no run-queue scan overhead */
static void setup_0_noise(void)
{
	wakeup_setup(0U);
}

static void teardown_0_noise(void)
{
	wakeup_teardown(0U);
}

ZTEST_BENCHMARK_TIMED(cpu_mask_wakeup, wakeup_0_noise, 1000U,
		      setup_0_noise, teardown_0_noise)
{
	wakeup_run_body();
}

/* 4 noise threads: 3 waiting in the ready queue, 1 running on CPU 1 */
static void setup_4_noise(void)
{
	wakeup_setup(4U);
}

static void teardown_4_noise(void)
{
	wakeup_teardown(4U);
}

ZTEST_BENCHMARK_TIMED(cpu_mask_wakeup, wakeup_4_noise, 1000U,
		      setup_4_noise, teardown_4_noise)
{
	wakeup_run_body();
}

/* 8 noise threads: 7 waiting in the ready queue */
static void setup_8_noise(void)
{
	wakeup_setup(8U);
}

static void teardown_8_noise(void)
{
	wakeup_teardown(8U);
}

ZTEST_BENCHMARK_TIMED(cpu_mask_wakeup, wakeup_8_noise, 1000U,
		      setup_8_noise, teardown_8_noise)
{
	wakeup_run_body();
}

/* 16 noise threads: 15 waiting in the ready queue – maximum depth */
static void setup_16_noise(void)
{
	wakeup_setup(16U);
}

static void teardown_16_noise(void)
{
	wakeup_teardown(16U);
}

ZTEST_BENCHMARK_TIMED(cpu_mask_wakeup, wakeup_16_noise, 1000U,
		      setup_16_noise, teardown_16_noise)
{
	wakeup_run_body();
}
