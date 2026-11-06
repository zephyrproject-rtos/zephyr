/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Yielding against a loaded ready queue.
 *
 * k_yield() puts the running thread back among the threads of equal
 * priority, which a ready queue may do by walking the queue. What that
 * costs therefore depends on how many threads are runnable at the
 * priority, and a single measurement cannot tell an implementation
 * that walks from one that does not. The same yield is measured here
 * against three queue depths, so the three numbers together show
 * whether the cost grows with the depth.
 *
 * The threads that make up the depth are the measurement's partner as
 * well: whichever of them the yield switches to closes the span, so
 * what is measured is one yield and the one context switch it causes,
 * exactly as in thread.yield_*, and not a trip around the queue.
 */

#include "bench.h"

#define FILLER_COUNT       16
#define FILLER_STACK_SIZE  (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

static void fillers_start(void);
static void fillers_stop(void);

ZTEST_BENCHMARK_SUITE(yield_queue, fillers_start, fillers_stop);

static K_THREAD_STACK_ARRAY_DEFINE(filler_stacks, FILLER_COUNT, FILLER_STACK_SIZE);
static struct k_thread filler_threads[FILLER_COUNT];
static volatile bool fillers_stopping;

/*
 * Runnable for as long as the suite lasts, so that the queue the
 * measuring thread yields into is the depth the benchmark asked for.
 * Ending a span that is not open is ignored, so every pass may try.
 */
static void filler_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (!fillers_stopping) {
		ztest_benchmark_end();
		k_yield();
	}
}

static void fillers_start(void)
{
	int priority = k_thread_priority_get(k_current_get());

	fillers_stopping = false;

	for (size_t i = 0; i < FILLER_COUNT; i++) {
		k_thread_create(&filler_threads[i], filler_stacks[i], FILLER_STACK_SIZE,
				filler_entry, NULL, NULL, NULL, priority, 0, K_NO_WAIT);
		k_thread_suspend(&filler_threads[i]);
	}
}

static void fillers_stop(void)
{
	fillers_stopping = true;

	for (size_t i = 0; i < FILLER_COUNT; i++) {
		k_thread_resume(&filler_threads[i]);
	}

	for (size_t i = 0; i < FILLER_COUNT; i++) {
		(void)k_thread_join(&filler_threads[i], K_FOREVER);
	}
}

/*
 * Leave exactly depth filler threads runnable. Runs in the untimed
 * setup, so what it costs is not part of any sample.
 */
static void fillers_set_depth(size_t depth)
{
	for (size_t i = 0; i < FILLER_COUNT; i++) {
		if (i < depth) {
			k_thread_resume(&filler_threads[i]);
		} else {
			k_thread_suspend(&filler_threads[i]);
		}
	}
}

static void depth_1_setup(void)
{
	fillers_set_depth(1);
}

static void depth_4_setup(void)
{
	fillers_set_depth(4);
}

static void depth_16_setup(void)
{
	fillers_set_depth(FILLER_COUNT);
}

ZTEST_BENCHMARK_MANUAL(yield_queue, depth_1, BENCH_SAMPLES, depth_1_setup, NULL)
{
	ztest_benchmark_start();
	k_yield();
}

ZTEST_BENCHMARK_MANUAL(yield_queue, depth_4, BENCH_SAMPLES, depth_4_setup, NULL)
{
	ztest_benchmark_start();
	k_yield();
}

ZTEST_BENCHMARK_MANUAL(yield_queue, depth_16, BENCH_SAMPLES, depth_16_setup, NULL)
{
	ztest_benchmark_start();
	k_yield();
}
