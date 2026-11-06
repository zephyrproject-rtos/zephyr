/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BENCHMARKS_KERNEL_LATENCY_SRC_BENCH_H_
#define ZEPHYR_TESTS_BENCHMARKS_KERNEL_LATENCY_SRC_BENCH_H_

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * Shared helpers for the manually sampled benchmarks.
 *
 * Operations that begin and end in the same thread are plain
 * ZTEST_BENCHMARK() bodies, which the framework times itself. The ones
 * here span a context switch, so the body marks the start of the span
 * and the partner thread marks its end, which only
 * ZTEST_BENCHMARK_MANUAL() can express.
 *
 * The framework runs the body once per iteration and handles the warmup
 * either way, so a body performs a single handshake: the partner is
 * created by the setup function, does its half, and is joined by the
 * teardown function.
 */

#define BENCH_SAMPLES CONFIG_KERNEL_BENCH_NUM_SAMPLES

#define BENCH_PARTNER_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#endif /* ZEPHYR_TESTS_BENCHMARKS_KERNEL_LATENCY_SRC_BENCH_H_ */
