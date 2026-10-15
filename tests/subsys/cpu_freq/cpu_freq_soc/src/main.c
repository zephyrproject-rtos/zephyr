/*
 * Copyright (c) 2025 Analog Devices, Inc.
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/irq.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/cpu_freq.h>

#define NUM_THREADS (2 * (CONFIG_MP_MAX_NUM_CPUS - 1))

/*
 * Iterations of the benchmark loop. Sized so that the loop takes roughly ten
 * milliseconds on a 1 GHz class core: long enough to swamp the measurement
 * overhead, short enough that the interrupt lock below is not held for a
 * disruptive amount of time and that a 24-bit system counter cannot wrap.
 */
#define BENCH_ITERATIONS 2000000U

#define BENCH_STACK_SIZE 2048

LOG_MODULE_REGISTER(cpu_freq_soc_test, LOG_LEVEL_INF);

const struct pstate *soc_pstates_dt[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))};

#define NUM_PSTATES ARRAY_SIZE(soc_pstates_dt)

/*
 * One P-state sweep, as measured by pstate_benchmark_run(). The benchmark runs
 * on a thread of its own, where ztest's assertion macros are not reliable: a
 * failing assertion aborts that thread only, so k_thread_join() still succeeds
 * and the failure is never attributed to the test case. The benchmark therefore
 * only records what it observed and the test thread does the checking.
 */
struct bench_result {
	unsigned int cpu;
	/* cpu_freq_pstate_set() return value, per P-state. */
	int ret[NUM_PSTATES];
	/* Duration of the measured window, 0 if the P-state was not set. */
	uint64_t cycles[NUM_PSTATES];
	/* cpu_freq_pstate_set() return value for the P-state restored at the end. */
	int restore_ret;
};

/* One entry per CPU; a single CPU build only uses index 0. */
static struct bench_result bench_results[CONFIG_MP_MAX_NUM_CPUS];

/* Written by the benchmark so that the loop is not optimized away. */
static volatile uint32_t bench_sink;

#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_CPU_MASK)
static K_THREAD_STACK_ARRAY_DEFINE(bench_stacks, CONFIG_MP_MAX_NUM_CPUS, BENCH_STACK_SIZE);
static struct k_thread bench_threads[CONFIG_MP_MAX_NUM_CPUS];
#endif

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
static K_THREAD_STACK_ARRAY_DEFINE(busy_thread_stacks, NUM_THREADS, 1024);
static struct k_thread busy_threads[NUM_THREADS];
static struct k_sem busy_thread_sem[NUM_THREADS];

/**
 * For each extra CPU, two threads are created that ping-pong giving and taking
 * semaphores. Not only does this keep the CPUs busy, it generates scheduling
 * point IPIs which can be used to validate a test environment assumption--
 * that the current schedule lock will be respected.
 */
static void give_take_helper(void *p1, void *p2, void *p3)
{
	int order = (int)(intptr_t)p1;
	struct k_sem *sem1 = (struct k_sem *)p2;
	struct k_sem *sem2 = (struct k_sem *)p3;

	while (1) {
		if ((order & 1) == 0) {
			k_sem_give(sem1);
			k_sem_take(sem2, K_FOREVER);
		} else {
			k_sem_give(sem2);
			k_sem_take(sem1, K_FOREVER);
		}
	}
}
#endif

/*
 * Test SoC integration of CPU Freq
 */
ZTEST(cpu_freq_soc, test_soc_pstates)
{
	int ret;
	int i;

	zassert_true(ARRAY_SIZE(soc_pstates_dt) > 0, "No P-states defined in devicetree");

	LOG_INF("%zu P-states defined for %s", ARRAY_SIZE(soc_pstates_dt), CONFIG_BOARD_TARGET);

	zassert_equal(cpu_freq_pstate_set(NULL), -EINVAL, "Expected -EINVAL for NULL pstate");

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	int priority;
	unsigned int id1;
	unsigned int id2;

	priority = k_thread_priority_get(k_current_get());

	for (i = 0; i < NUM_THREADS; i++) {
		k_sem_init(&busy_thread_sem[i], 0, 1);
	}

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&busy_threads[i], busy_thread_stacks[i],
				K_THREAD_STACK_SIZEOF(busy_thread_stacks[i]),
				(k_thread_entry_t)give_take_helper,
				(void *)(intptr_t)i,
				&busy_thread_sem[i / 2],
				&busy_thread_sem[1 + (i / 2)],
				priority, 0, K_NO_WAIT);
	}
#endif

	for (i = 0; i < ARRAY_SIZE(soc_pstates_dt); i++) {
		const struct pstate *state = soc_pstates_dt[i];

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
		/*
		 * Lock the scheduler to ensure that the current thread
		 * does not migrate to another CPU.
		 */
		k_sched_lock();

		id1 = arch_curr_cpu()->id;

		/*
		 * Validate the assumption that the current thread does not
		 * migrate across CPUs before calling cpu_freq_pstate_set().
		 */

		for (int j = 0; j < 10; j++) {
			k_busy_wait(10000);
			id2 = arch_curr_cpu()->id;
			zassert_equal(id1, id2,
				      "Current CPU changed while scheduler locked");
		}
#endif

		/* Set performance state using pstate driver */
		ret = cpu_freq_pstate_set(state);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
		/*
		 * Validate the assumption that the current thread does not
		 * migrate after calling cpu_freq_pstate_set().
		 */
		for (int j = 0; j < 10; j++) {
			k_busy_wait(10000);
			id2 = arch_curr_cpu()->id;
			zassert_equal(id1, id2,
				      "Current CPU changed while scheduler locked");
		}

		k_sched_unlock();
#endif
		zassert_equal(ret, 0, "Failed to set P-state %d", i);
	}

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(&busy_threads[i]);
	}
#endif
}

/*
 * Fixed integer workload. It touches no memory beyond a register or two, so
 * the time it takes is a function of the core clock and nothing else.
 */
static void cpu_benchmark(void)
{
	uint32_t acc = 1U;

	for (uint32_t i = 0U; i < BENCH_ITERATIONS; i++) {
		acc = (acc * 1664525U) + 1013904223U;
	}

	bench_sink = acc;
}

/*
 * Time the fixed workload once per P-state on the CPU this runs on. The
 * struct bench_result to fill in is passed as p1; it already holds the CPU
 * this thread is pinned to. No assertion is made here, see struct
 * bench_result: the results are checked by bench_result_check() on the test
 * thread.
 *
 * The cycle counter is the system timer, which on an SoC where the P-state
 * only reprograms the core clock keeps running at a fixed rate. A faster core
 * therefore finishes the workload sooner, so the reported durations are
 * expected to grow as the P-state table moves from the highest performance
 * state to the lowest.
 */
static void pstate_benchmark_run(void *p1, void *p2, void *p3)
{
	struct bench_result *res = p1;
	unsigned int key;
	uint64_t start;
	size_t i;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (i = 0; i < NUM_PSTATES; i++) {
		/* Warm up outside the lock: caches, branch predictors, TLB. */
		cpu_benchmark();

		/*
		 * Locking interrupts keeps the current CPU fixed, as
		 * cpu_freq_pstate_set() requires, and keeps interrupt handling
		 * out of the measured window.
		 */
		key = irq_lock();

		res->ret[i] = cpu_freq_pstate_set(soc_pstates_dt[i]);
		if (res->ret[i] == 0) {
			start = k_cycle_get_64();
			cpu_benchmark();
			res->cycles[i] = k_cycle_get_64() - start;
		}

		irq_unlock(key);
	}

	/* Restore the highest P-state. */
	key = irq_lock();
	res->restore_ret = cpu_freq_pstate_set(soc_pstates_dt[0]);
	irq_unlock(key);
}

/*
 * Check and report one sweep. Called on the thread running the test case, so
 * that a failure fails the test case.
 */
static void bench_result_check(const struct bench_result *res)
{
	for (size_t i = 0; i < NUM_PSTATES; i++) {
		zassert_equal(res->ret[i], 0, "CPU %u: failed to set P-state %zu", res->cpu, i);
		zassert_true(res->cycles[i] > 0,
			     "CPU %u: cycle counter did not advance on P-state %zu", res->cpu, i);

		LOG_INF("CPU %u P-state %zu (threshold=%u%%): %u iterations in %" PRIu64 " us",
			res->cpu, i, soc_pstates_dt[i]->load_threshold, BENCH_ITERATIONS,
			k_cyc_to_us_floor64(res->cycles[i]));
	}

	zassert_equal(res->restore_ret, 0, "CPU %u: failed to restore P-state 0", res->cpu);
}

/*
 * Run the P-state benchmark on every CPU.
 *
 * On an SoC where a P-state is a cluster wide setting, every CPU reports the
 * same frequency; the point is that the transition is requested from, and
 * measurable on, each of them.
 */
ZTEST(cpu_freq_soc, test_pstate_benchmark)
{
	if (!IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		/* k_cycle_get_64() asserts and returns 0 without it. */
		ztest_test_skip();
	}

	if (IS_ENABLED(CONFIG_ARCH_POSIX)) {
		/*
		 * Simulated time does not advance while the simulated CPU
		 * computes, so there is nothing to measure.
		 */
		ztest_test_skip();
	}

	zassert_true(ARRAY_SIZE(soc_pstates_dt) > 0, "No P-states defined in devicetree");

#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_CPU_MASK)
	int priority = k_thread_priority_get(k_current_get());

	/*
	 * One CPU at a time: the thread is pinned before it is started, because
	 * a running thread cannot change its CPU mask, and is joined before the
	 * next CPU is measured.
	 */
	for (unsigned int cpu = 0; cpu < arch_num_cpus(); cpu++) {
		struct bench_result *res = &bench_results[cpu];
		k_tid_t tid;
		int ret;

		memset(res, 0, sizeof(*res));
		res->cpu = cpu;

		tid = k_thread_create(&bench_threads[cpu], bench_stacks[cpu],
				      K_THREAD_STACK_SIZEOF(bench_stacks[cpu]),
				      pstate_benchmark_run, res, NULL, NULL, priority, 0,
				      K_FOREVER);

		ret = k_thread_cpu_pin(tid, (int)cpu);
		zassert_equal(ret, 0, "Failed to pin benchmark thread to CPU %u", cpu);

		k_thread_start(tid);

		ret = k_thread_join(tid, K_FOREVER);
		zassert_equal(ret, 0, "Failed to join benchmark thread of CPU %u", cpu);

		bench_result_check(res);
	}
#else
	/*
	 * Without affinity support there is nothing to pin to: run the sweep
	 * once on the CPU that runs the test, which is CPU 0 on a single CPU
	 * build.
	 */
	memset(&bench_results[0], 0, sizeof(bench_results[0]));
	bench_results[0].cpu = 0;

	pstate_benchmark_run(&bench_results[0], NULL, NULL);

	bench_result_check(&bench_results[0]);
#endif
}

ZTEST_SUITE(cpu_freq_soc, NULL, NULL, NULL, NULL, NULL);
