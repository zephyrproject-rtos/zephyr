/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Validation for ZTEST_BENCHMARK_MANUAL(): one benchmark measuring a
 * span of a known duration (k_busy_wait) that is only part of the body,
 * and one measuring a span that starts in a different execution context
 * (an ISR entered via irq_offload marks it) and ends in the thread,
 * which framework-timed benchmarks cannot express.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>

#define NUM_SAMPLES  100
#define BUSY_WAIT_US 100

ZTEST_BENCHMARK_SUITE(benchmark_manual, NULL, NULL);

ZTEST_BENCHMARK_MANUAL(benchmark_manual, busy_wait_span, NUM_SAMPLES, NULL, NULL)
{
	/* Excluded from the measurement, to show that the brackets pick
	 * out part of the body rather than all of it.
	 */
	k_busy_wait(BUSY_WAIT_US);

	ztest_benchmark_start();
	k_busy_wait(BUSY_WAIT_US);
	ztest_benchmark_end();
}

static void offload_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Marking the span from interrupt context is the point here. */
	ztest_benchmark_start();
}

ZTEST_BENCHMARK_MANUAL(benchmark_manual, isr_exit_span, NUM_SAMPLES, NULL, NULL)
{
	irq_offload(offload_isr, NULL);

	ztest_benchmark_end();
}
