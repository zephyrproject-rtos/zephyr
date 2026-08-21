/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Thread lifecycle operations, and the context switch itself.
 *
 * The lifecycle operations are timed by the framework: each one is the
 * body, and whatever has to happen around it to make the operation
 * legal is the setup or the teardown, which are not timed. The context
 * switch is measured manually, because it ends in a different thread
 * from the one it starts in.
 */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(thread, NULL, NULL);

static K_THREAD_STACK_DEFINE(target_stack, BENCH_PARTNER_STACK_SIZE);
static struct k_thread target_thread;

/* Never runs: created below the benchmark's priority and left parked */
static void target_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(K_FOREVER);
}

static int target_priority(void)
{
	return k_thread_priority_get(k_current_get()) + 1;
}

static void target_create(void)
{
	k_thread_create(&target_thread, target_stack, BENCH_PARTNER_STACK_SIZE, target_entry,
			NULL, NULL, NULL, target_priority(), 0, K_FOREVER);
}

static void target_create_and_start(void)
{
	target_create();
	k_thread_start(&target_thread);
}

static void target_create_start_suspend(void)
{
	target_create_and_start();
	k_thread_suspend(&target_thread);
}

/*
 * Abort and then wait for the thread to be reaped. Where thread stacks
 * are memory mapped, the mapping is only released once the terminated
 * thread has been cleaned up, and a thousand create/abort cycles that
 * do not wait exhaust the mapping pool.
 */
static void target_abort(void)
{
	k_thread_abort(&target_thread);
	(void)k_thread_join(&target_thread, K_FOREVER);
}

static void target_join(void)
{
	(void)k_thread_join(&target_thread, K_FOREVER);
}

ZTEST_BENCHMARK(thread, create, BENCH_SAMPLES, NULL, target_abort)
{
	target_create();
}

ZTEST_BENCHMARK(thread, start, BENCH_SAMPLES, target_create, target_abort)
{
	k_thread_start(&target_thread);
}

ZTEST_BENCHMARK(thread, suspend, BENCH_SAMPLES, target_create_and_start, target_abort)
{
	k_thread_suspend(&target_thread);
}

ZTEST_BENCHMARK(thread, resume, BENCH_SAMPLES, target_create_start_suspend, target_abort)
{
	k_thread_resume(&target_thread);
}

ZTEST_BENCHMARK(thread, abort, BENCH_SAMPLES, target_create_and_start, target_join)
{
	k_thread_abort(&target_thread);
}

/*
 * Context switch through k_yield(), measured between two threads of
 * equal priority that hand the CPU back and forth. The partner takes
 * the second timestamp, so the span covers the switch itself rather
 * than the cost of calling k_yield().
 */
static K_THREAD_STACK_DEFINE(yield_stack, BENCH_PARTNER_STACK_SIZE);
static struct k_thread yield_thread;
static volatile timing_t yield_start;

static void yield_partner(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (size_t i = 0U; i < ztest_benchmark_iterations(); i++) {
		timing_t finish = timing_counter_get();

		bench_record_span(yield_start, finish);
		k_yield();
	}
}

static void yield_measure(bool cooperative)
{
	int priority = cooperative ? K_PRIO_COOP(2) : k_thread_priority_get(k_current_get());
	int saved = k_thread_priority_get(k_current_get());

	if (cooperative) {
		k_thread_priority_set(k_current_get(), priority);
	}

	k_thread_create(&yield_thread, yield_stack, BENCH_PARTNER_STACK_SIZE, yield_partner,
			NULL, NULL, NULL, priority, 0, K_NO_WAIT);

	for (size_t i = 0U; i < ztest_benchmark_iterations(); i++) {
		yield_start = timing_counter_get();
		k_yield();
	}

	k_thread_join(&yield_thread, K_FOREVER);
	k_thread_priority_set(k_current_get(), saved);
}

ZTEST_BENCHMARK_MANUAL(thread, yield_preemptive, BENCH_SAMPLES, NULL, NULL)
{
	yield_measure(false);
}

ZTEST_BENCHMARK_MANUAL(thread, yield_cooperative, BENCH_SAMPLES, NULL, NULL)
{
	yield_measure(true);
}
