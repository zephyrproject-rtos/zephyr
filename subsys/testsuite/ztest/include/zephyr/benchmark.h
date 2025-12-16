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
	ztest_benchmark_fn_t setup;
	ztest_benchmark_fn_t run;
	ztest_benchmark_fn_t teardown;
	struct ztest_benchmark_stats stats;
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
#define ZTEST_BENCHMARK_SUITE(suite, setup_fn, teardown_fn)			\
	static const STRUCT_SECTION_ITERABLE(ztest_benchmark_suite, suite) =	\
	{									\
		.name = #suite,							\
		.setup = setup_fn,						\
		.teardown = teardown_fn,					\
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
#define ZTEST_BENCHMARK_SETUP_TEARDOWN(suite_name, benchmark, samples, setup_fn, teardown_fn)	\
	static __noinline void benchmark##_fn(void);						\
	static const STRUCT_SECTION_ITERABLE(ztest_benchmark, benchmark) =			\
	{											\
		.name = #benchmark,								\
		.iterations = samples,								\
		.setup = setup_fn,								\
		.run = benchmark##_fn,								\
		.teardown = teardown_fn,							\
		.suite = &suite_name,								\
	};											\
	static __noinline void benchmark##_fn(void)


/**
 * @brief Define a timed benchmark with setup and teardown functions
 *
 * @param testsuite Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param duration Duration in milliseconds to run the benchmark
 * @param setup_fn Function to run before the benchmark
 * @param teardown_fn Function to run after the benchmark
 */
#define ZTEST_BENCHMARK_TIMED_SETUP_TEARDOWN(testsuite, benchmark, duration, setup_fn, teardown_fn)\
	static __noinline void benchmark##_fn(void);						\
	static const STRUCT_SECTION_ITERABLE(ztest_benchmark_timed, benchmark) =		\
	{											\
		.name = #benchmark,								\
		.duration_ms = duration,							\
		.setup = setup_fn,								\
		.run = benchmark##_fn,								\
		.teardown = teardown_fn,							\
		.suite = &testsuite,								\
	};											\
	static __noinline void benchmark##_fn(void)

/**
 * @brief Define a benchmark without setup and teardown functions
 *
 * @param suite Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param samples Number of iterations to run the benchmark
 */
#define ZTEST_BENCHMARK(suite, benchmark, samples) \
	ZTEST_BENCHMARK_SETUP_TEARDOWN(suite, benchmark, samples, NULL, NULL)


/** * @brief Define a timed benchmark without setup and teardown functions
 *
 * @param suite Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param duration Duration in milliseconds to run the benchmark
 */
#define ZTEST_BENCHMARK_TIMED(suite, benchmark, duration) \
	ZTEST_BENCHMARK_TIMED_SETUP_TEARDOWN(suite, benchmark, duration, NULL, NULL)

/**
 * @}
 */
#endif /* ZTEST_BENCHMARK_H */
