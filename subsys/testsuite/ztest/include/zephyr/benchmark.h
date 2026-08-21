/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr Benchmarking Framework
 */
#ifndef ZTEST_BENCHMARK_H
#define ZTEST_BENCHMARK_H
#include <stdint.h>
#include <stddef.h>

/** @cond INTERNAL_HIDDEN */
/*
 * Identifier builders for the linker-visible symbols that the ZTEST_BENCHMARK*()
 * macros generate. Use when defining a symbol or when taking its reference.
 */
#define Z_ZTEST_BENCHMARK_SUITE_NODE(suite)         z_ztest_benchmark_suite_##suite
#define Z_ZTEST_BENCHMARK_NODE(suite, bench)        z_ztest_benchmark_##suite##_##bench
#define Z_ZTEST_BENCHMARK_TIMED_NODE(suite, bench)  z_ztest_benchmark_timed_##suite##_##bench
#define Z_ZTEST_BENCHMARK_MANUAL_NODE(suite, bench) z_ztest_benchmark_manual_##suite##_##bench
#define Z_ZTEST_BENCHMARK_FN(suite, bench)          z_ztest_benchmark_##suite##_##bench##_fn

typedef void (*ztest_benchmark_fn_t)(void);
struct ztest_benchmark_suite {
	const char *name;
	ztest_benchmark_fn_t setup;
	ztest_benchmark_fn_t teardown;
};

struct ztest_extreme_value {
	uint64_t value;
	uint64_t sample;
};

struct ztest_benchmark_stats {
	double m2;
	double mean;
	uint64_t total;
	uint64_t samples;
	struct ztest_extreme_value min;
	struct ztest_extreme_value max;
};

struct ztest_benchmark {
	const char *name;
	size_t iterations;
	size_t warmup;
	ztest_benchmark_fn_t setup;
	ztest_benchmark_fn_t run;
	ztest_benchmark_fn_t teardown;
	struct ztest_benchmark_stats stats;
	uint64_t cold;
	const struct ztest_benchmark_suite *suite;
};

struct ztest_benchmark_manual {
	const char *name;
	size_t iterations;
	size_t warmup;
	ztest_benchmark_fn_t setup;
	ztest_benchmark_fn_t run;
	ztest_benchmark_fn_t teardown;
	struct ztest_benchmark_stats stats;
	uint64_t cold;
	const struct ztest_benchmark_suite *suite;
};

struct ztest_benchmark_timed_stats {
	uint64_t iterations;
	uint64_t duration_cycles;
};

struct ztest_benchmark_timed {
	size_t duration_ms;
	const char *name;
	ztest_benchmark_fn_t setup;
	ztest_benchmark_fn_t run;
	ztest_benchmark_fn_t teardown;
	const struct ztest_benchmark_suite *suite;
	struct ztest_benchmark_timed_stats stats;
};

void benchmark_main(void);
/** @endcond */

/**
 * @defgroup ztest_benchmark Zephyr Benchmarking Framework
 * @ingroup testing
 * @{
 */

/**
 * @brief Define a benchmark suite
 *
 * @param suite Name of the suite
 * @param setup_fn Function to run before the suite
 * @param teardown_fn Function to run after the suite
 */
#define ZTEST_BENCHMARK_SUITE(suite, setup_fn, teardown_fn)				\
	static const STRUCT_SECTION_ITERABLE(ztest_benchmark_suite,			\
					     Z_ZTEST_BENCHMARK_SUITE_NODE(suite)) =	\
	{										\
		.name = #suite,								\
		.setup = setup_fn,							\
		.teardown = teardown_fn,						\
	}

/**
 * @brief Define a benchmark with setup and teardown functions
 *
 * @param suite_name Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param samples Number of iterations to run the benchmark
 * @param setup_fn Function to run before the benchmark
 * @param teardown_fn Function to run after the benchmark
 */
#define ZTEST_BENCHMARK_WARMUP(suite_name, benchmark, samples, warmup_count, setup_fn,		\
			       teardown_fn)							\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void);		\
	static STRUCT_SECTION_ITERABLE(ztest_benchmark,						\
				       Z_ZTEST_BENCHMARK_NODE(suite_name, benchmark)) =		\
	{											\
		.name = #benchmark,								\
		.iterations = samples,								\
		.warmup = warmup_count,								\
		.setup = setup_fn,								\
		.teardown = teardown_fn,							\
		.run = Z_ZTEST_BENCHMARK_FN(suite_name, benchmark),				\
		.suite = &Z_ZTEST_BENCHMARK_SUITE_NODE(suite_name),				\
	};											\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void)

/**
 * @brief Define a benchmark with the default warmup
 *
 * As ZTEST_BENCHMARK_WARMUP(), with the warmup taken from
 * CONFIG_ZTEST_BENCHMARK_WARMUP.
 *
 * @param suite_name Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param samples Number of measured iterations
 * @param setup_fn Function to run before each iteration
 * @param teardown_fn Function to run after each iteration
 */
#define ZTEST_BENCHMARK(suite_name, benchmark, samples, setup_fn, teardown_fn)			\
	ZTEST_BENCHMARK_WARMUP(suite_name, benchmark, samples, CONFIG_ZTEST_BENCHMARK_WARMUP,	\
			       setup_fn, teardown_fn)


/**
 * @brief Define a timed benchmark with setup and teardown functions
 *
 * @param testsuite Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param duration Duration in milliseconds to run the benchmark
 * @param setup_fn Function to run before the benchmark
 * @param teardown_fn Function to run after the benchmark
 */
#define ZTEST_BENCHMARK_TIMED(testsuite, benchmark, duration, setup_fn, teardown_fn)		\
	static __noinline void Z_ZTEST_BENCHMARK_FN(testsuite, benchmark)(void);		\
	static const STRUCT_SECTION_ITERABLE(ztest_benchmark_timed,				\
					Z_ZTEST_BENCHMARK_TIMED_NODE(testsuite, benchmark)) =	\
	{											\
		.name = #benchmark,								\
		.duration_ms = duration,							\
		.setup = setup_fn,								\
		.run = Z_ZTEST_BENCHMARK_FN(testsuite, benchmark),				\
		.teardown = teardown_fn,							\
		.suite = &Z_ZTEST_BENCHMARK_SUITE_NODE(testsuite),				\
	};											\
	static __noinline void Z_ZTEST_BENCHMARK_FN(testsuite, benchmark)(void)

/**
 * @brief Define a manually sampled benchmark
 *
 * Unlike ZTEST_BENCHMARK(), where the framework takes both timestamps
 * around each invocation of the benchmark body, a manual benchmark body is
 * invoked exactly once and performs its own measurements, reporting each
 * measured span with ztest_benchmark_record_sample(). This allows
 * benchmarking spans whose endpoints are captured in different execution
 * contexts (e.g. a timestamp taken inside an ISR and another taken in a
 * thread), which the framework cannot time by itself.
 *
 * Samples should be raw cycle deltas between two timing_counter_get()
 * calls; the framework applies the same control-measurement noise
 * correction as for standard benchmarks when reporting.
 *
 * @param suite_name Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param setup_fn Function to run before the benchmark
 * @param teardown_fn Function to run after the benchmark
 */
#define ZTEST_BENCHMARK_MANUAL_WARMUP(suite_name, benchmark, samples, warmup_count, setup_fn,	\
				      teardown_fn)						\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void);		\
	static STRUCT_SECTION_ITERABLE(ztest_benchmark_manual,					\
				       Z_ZTEST_BENCHMARK_MANUAL_NODE(suite_name, benchmark)) =	\
	{											\
		.name = #benchmark,								\
		.iterations = samples,								\
		.warmup = warmup_count,								\
		.setup = setup_fn,								\
		.run = Z_ZTEST_BENCHMARK_FN(suite_name, benchmark),				\
		.teardown = teardown_fn,							\
		.suite = &Z_ZTEST_BENCHMARK_SUITE_NODE(suite_name),				\
	};											\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void)

/**
 * @brief Define a manually sampled benchmark with the default warmup
 *
 * As ZTEST_BENCHMARK_MANUAL_WARMUP(), with the warmup taken from
 * CONFIG_ZTEST_BENCHMARK_WARMUP.
 *
 * @param suite_name Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param samples Number of measured samples
 * @param setup_fn Function to run before the benchmark
 * @param teardown_fn Function to run after the benchmark
 */
#define ZTEST_BENCHMARK_MANUAL(suite_name, benchmark, samples, setup_fn, teardown_fn)		\
	ZTEST_BENCHMARK_MANUAL_WARMUP(suite_name, benchmark, samples,				\
				      CONFIG_ZTEST_BENCHMARK_WARMUP, setup_fn, teardown_fn)

/**
 * @brief Record one sample of a manually sampled benchmark
 *
 * Add one measured span, in timing cycles, to the statistics of the
 * currently running ZTEST_BENCHMARK_MANUAL() benchmark. Calls made outside
 * a manual benchmark body are ignored.
 *
 * Must be called from thread context: the statistics update performs
 * floating point math, which is not safe in ISRs on all architectures.
 * ISRs should only capture timestamps and leave computing and recording
 * the delta to the benchmark body.
 *
 * @param cycles Measured span in timing cycles
 */
void ztest_benchmark_record_sample(uint64_t cycles);

/**
 * @brief Number of times a manual benchmark body should run its loop
 *
 * The warmup count plus the measured sample count of the running
 * ZTEST_BENCHMARK_MANUAL() benchmark. The body records a sample on every
 * one of these iterations; the framework drops the warmup ones and keeps
 * the first as the cold cost, so the body does not have to distinguish
 * between the two phases.
 *
 * Returns zero outside a manual benchmark body.
 */
size_t ztest_benchmark_iterations(void);

/**
 * @}
 */
#endif /* ZTEST_BENCHMARK_H */
