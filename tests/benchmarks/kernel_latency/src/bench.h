/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BENCHMARKS_KERNEL_LATENCY_SRC_BENCH_H_
#define ZEPHYR_TESTS_BENCHMARKS_KERNEL_LATENCY_SRC_BENCH_H_

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/timing/timing.h>

/*
 * Shared helpers for the manually sampled benchmarks.
 *
 * Operations that begin and end in the same thread are plain
 * ZTEST_BENCHMARK() bodies, which the framework times itself. The ones
 * here span a context switch, so one thread takes the first timestamp
 * and the partner thread takes the second, which only
 * ZTEST_BENCHMARK_MANUAL() can express.
 *
 * The framework runs the body once per iteration and handles the warmup
 * either way, so a body performs a single handshake: the partner is
 * created by the setup function, does its half, and is joined by the
 * teardown function.
 */

#define BENCH_SAMPLES CONFIG_KERNEL_BENCH_NUM_SAMPLES

#define BENCH_PARTNER_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

/*
 * Hand the framework a span whose endpoints were taken in two different
 * threads. Called from the benchmark body once the handshake has
 * completed, so both timestamps are settled by then.
 */
static inline void bench_span(timing_t start, timing_t finish)
{
	ztest_benchmark_start_at(start);
	ztest_benchmark_end_at(finish);
}

#endif /* ZEPHYR_TESTS_BENCHMARKS_KERNEL_LATENCY_SRC_BENCH_H_ */
