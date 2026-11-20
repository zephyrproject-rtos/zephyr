/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/offloader/engine.h>

#define WORKER_STACK_SIZE 2048
#define SAMPLES           1000U

#define WORK_NONE  0U
#define WORK_SHORT 1024U
#define WORK_LONG  16384U

OFFLOADER_ENGINE_DEFINE(bench_engine, 1, WORKER_STACK_SIZE);

static volatile uint32_t sink;
static k_tid_t work_tid;

static __noinline void work(void *args)
{
	uint32_t n = (uint32_t)(uintptr_t)args;
	uint32_t acc = 0;

	while (n-- > 0U) {
		acc += n;
	}

	sink = acc;
}

static offloader_work_fn_t volatile direct_fn = work;

ZTEST_BENCHMARK_SUITE(offloader_engine, NULL, NULL);

ZTEST_BENCHMARK(offloader_engine, direct_work_none, SAMPLES, NULL, NULL)
{
	direct_fn((void *)(uintptr_t)WORK_NONE);
}

ZTEST_BENCHMARK(offloader_engine, dispatched_work_none, SAMPLES, NULL, NULL)
{
	offloader_dispatch(&bench_engine, work, (void *)(uintptr_t)WORK_NONE);
}

ZTEST_BENCHMARK(offloader_engine, direct_work_short, SAMPLES, NULL, NULL)
{
	direct_fn((void *)(uintptr_t)WORK_SHORT);
}

ZTEST_BENCHMARK(offloader_engine, dispatched_work_short, SAMPLES, NULL, NULL)
{
	offloader_dispatch(&bench_engine, work, (void *)(uintptr_t)WORK_SHORT);
}

ZTEST_BENCHMARK(offloader_engine, direct_work_long, SAMPLES, NULL, NULL)
{
	direct_fn((void *)(uintptr_t)WORK_LONG);
}

ZTEST_BENCHMARK(offloader_engine, dispatched_work_long, SAMPLES, NULL, NULL)
{
	offloader_dispatch(&bench_engine, work, (void *)(uintptr_t)WORK_LONG);
}

static void mark_arrival(void *args)
{
	ARG_UNUSED(args);

	ztest_benchmark_end();
}

static void mark_departure(void *args)
{
	ARG_UNUSED(args);

	ztest_benchmark_start();
}

ZTEST_BENCHMARK_MANUAL(offloader_engine, dispatch_to_worker, SAMPLES, NULL, NULL)
{
	ztest_benchmark_start();
	offloader_dispatch(&bench_engine, mark_arrival, NULL);
}

ZTEST_BENCHMARK_MANUAL(offloader_engine, worker_to_caller, SAMPLES, NULL, NULL)
{
	offloader_dispatch(&bench_engine, mark_departure, NULL);
	ztest_benchmark_end();
}

static void record_tid(void *args)
{
	ARG_UNUSED(args);

	work_tid = k_current_get();
}

ZTEST(offloader_engine_sanity, test_dispatch_runs_on_a_worker)
{
	k_tid_t self = k_current_get();

	work_tid = NULL;
	record_tid(NULL);
	zassert_true(work_tid == self, "the direct call did not run on the caller");

	work_tid = NULL;
	offloader_dispatch(&bench_engine, record_tid, NULL);
	zassert_not_null(work_tid, "the dispatched work did not run");
	zassert_true(work_tid != self, "the dispatched work ran on the caller");
}

ZTEST(offloader_engine_sanity, test_work_scales_with_its_argument)
{
	sink = 0;
	work((void *)(uintptr_t)WORK_NONE);
	zassert_equal(sink, 0, "the empty work function did not run");

	sink = 0;
	work((void *)(uintptr_t)WORK_SHORT);
	zassert_equal(sink, (WORK_SHORT * (WORK_SHORT - 1U)) / 2U,
		      "the work function did not execute every iteration");
}

ZTEST_SUITE(offloader_engine_sanity, NULL, NULL, NULL, NULL, NULL);
