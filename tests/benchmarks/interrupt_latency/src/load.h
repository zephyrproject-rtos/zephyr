/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BENCHMARKS_INTERRUPT_LATENCY_SRC_LOAD_H_
#define ZEPHYR_TESTS_BENCHMARKS_INTERRUPT_LATENCY_SRC_LOAD_H_

#include <zephyr/kernel.h>

/*
 * Background system load.
 *
 * Interrupt latency measured on an otherwise idle system is a best
 * case: caches are warm with the benchmark's own working set, no other
 * interrupt is in service and no other code competes for the memory
 * system. These helpers put the system under load so that the reported
 * maximum and standard deviation describe something closer to what an
 * application will see.
 */

#ifdef CONFIG_INT_BENCH_LOAD

/* Start the background load sources. Called once before benchmarking. */
void bench_load_start(void);

/* Stop the background load sources. */
void bench_load_stop(void);

/*
 * Evict the caches and TLBs by walking a working set larger than the
 * benchmark's own. Called between samples, outside the measured span,
 * so that each measured interrupt is served with cold caches.
 */
void bench_load_pollute(void);

/* Describe the enabled load sources, for the benchmark banner. */
const char *bench_load_description(void);

#else

static inline void bench_load_start(void)
{
}

static inline void bench_load_stop(void)
{
}

static inline void bench_load_pollute(void)
{
}

static inline const char *bench_load_description(void)
{
	return "idle";
}

#endif /* CONFIG_INT_BENCH_LOAD */

#endif /* ZEPHYR_TESTS_BENCHMARKS_INTERRUPT_LATENCY_SRC_LOAD_H_ */
