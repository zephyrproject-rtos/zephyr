/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Validation for ZTEST_BENCHMARK_MANUAL(): one benchmark measuring a
 * span with a known duration (k_busy_wait) and one measuring a span
 * whose starting timestamp is captured in a different execution
 * context (an ISR entered via irq_offload), which framework-timed
 * benchmarks cannot express.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/timing/timing.h>

#define NUM_SAMPLES  100
#define BUSY_WAIT_US 100

ZTEST_BENCHMARK_SUITE(benchmark_manual, NULL, NULL);

ZTEST_BENCHMARK_MANUAL(benchmark_manual, busy_wait_span, NULL, NULL)
{
	timing_t start;
	timing_t finish;

	for (uint32_t i = 0U; i < NUM_SAMPLES; i++) {
		start = timing_counter_get();
		k_busy_wait(BUSY_WAIT_US);
		finish = timing_counter_get();

		ztest_benchmark_record_sample(timing_cycles_get(&start, &finish));
	}
}

static volatile timing_t isr_timestamp;

static void offload_isr(const void *arg)
{
	ARG_UNUSED(arg);

	isr_timestamp = timing_counter_get();
}

ZTEST_BENCHMARK_MANUAL(benchmark_manual, isr_exit_span, NULL, NULL)
{
	timing_t start;
	timing_t finish;

	for (uint32_t i = 0U; i < NUM_SAMPLES; i++) {
		irq_offload(offload_isr, NULL);
		finish = timing_counter_get();
		start = isr_timestamp;

		ztest_benchmark_record_sample(timing_cycles_get(&start, &finish));
	}
}
