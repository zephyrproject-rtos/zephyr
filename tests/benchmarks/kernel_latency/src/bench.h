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
 * and another takes the second, which only ZTEST_BENCHMARK_MANUAL()
 * can express. Both kinds get their warmup from the framework.
 */

#define BENCH_SAMPLES CONFIG_KERNEL_BENCH_NUM_SAMPLES

#define BENCH_PARTNER_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

/*
 * Record one measured span. The framework decides which samples belong
 * to the warmup and which to the distribution, so every iteration
 * records and the body does not tell the phases apart.
 */
static inline void bench_record_span(timing_t start, timing_t finish)
{
	ztest_benchmark_record_sample(timing_cycles_get(&start, &finish));
}

#endif /* ZEPHYR_TESTS_BENCHMARKS_KERNEL_LATENCY_SRC_BENCH_H_ */
