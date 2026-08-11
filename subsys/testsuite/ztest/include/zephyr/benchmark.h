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
#include <zephyr/timing/timing.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	ztest_benchmark_fn_t setup;
	ztest_benchmark_fn_t run;
	ztest_benchmark_fn_t teardown;
	struct ztest_benchmark_stats stats;
	const struct ztest_benchmark_suite *suite;
};

struct ztest_benchmark_manual {
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
#define ZTEST_BENCHMARK(suite_name, benchmark, samples, setup_fn, teardown_fn)			\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void);		\
	static const STRUCT_SECTION_ITERABLE(ztest_benchmark,					\
					     Z_ZTEST_BENCHMARK_NODE(suite_name, benchmark)) =	\
	{											\
		.name = #benchmark,								\
		.iterations = samples,								\
		.setup = setup_fn,								\
		.run = Z_ZTEST_BENCHMARK_FN(suite_name, benchmark),				\
		.teardown = teardown_fn,							\
		.suite = &Z_ZTEST_BENCHMARK_SUITE_NODE(suite_name),				\
	};											\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void)


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
		.duration_ms = duration,							\
		.name = #benchmark,								\
		.setup = setup_fn,								\
		.run = Z_ZTEST_BENCHMARK_FN(testsuite, benchmark),				\
		.teardown = teardown_fn,							\
		.suite = &Z_ZTEST_BENCHMARK_SUITE_NODE(testsuite),				\
	};											\
	static __noinline void Z_ZTEST_BENCHMARK_FN(testsuite, benchmark)(void)

/**
 * @brief Define a manually sampled benchmark
 *
 * The framework runs the body once per iteration exactly as it does for
 * ZTEST_BENCHMARK(), but the body decides which part of itself is measured
 * by bracketing it with ztest_benchmark_start() and
 * ztest_benchmark_end():
 *
 * @code
 * ZTEST_BENCHMARK_MANUAL(my_suite, my_benchmark, 1000, NULL, NULL)
 * {
 *         prepare();
 *         ztest_benchmark_start();
 *         operation_under_test();
 *         ztest_benchmark_end();
 * }
 * @endcode
 *
 * That covers a span the framework cannot time by itself because it does
 * not start and end where the body does. When the span does not even begin
 * and end in the same execution context - the latency from an interrupt to
 * the thread it wakes, say - the endpoint captured elsewhere is handed over
 * with ztest_benchmark_start_at() or ztest_benchmark_end_at().
 *
 * A body that records no span contributes no sample for that iteration.
 *
 * @param suite_name Name of the suite the benchmark belongs to
 * @param benchmark Name of the benchmark
 * @param samples Number of iterations to run the benchmark
 * @param setup_fn Function to run before each iteration
 * @param teardown_fn Function to run after each iteration
 */
#define ZTEST_BENCHMARK_MANUAL(suite_name, benchmark, samples, setup_fn, teardown_fn)		\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void);		\
	static STRUCT_SECTION_ITERABLE(ztest_benchmark_manual,					\
				       Z_ZTEST_BENCHMARK_MANUAL_NODE(suite_name, benchmark)) =	\
	{											\
		.name = #benchmark,								\
		.iterations = samples,								\
		.setup = setup_fn,								\
		.run = Z_ZTEST_BENCHMARK_FN(suite_name, benchmark),				\
		.teardown = teardown_fn,							\
		.suite = &Z_ZTEST_BENCHMARK_SUITE_NODE(suite_name),				\
	};											\
	static __noinline void Z_ZTEST_BENCHMARK_FN(suite_name, benchmark)(void)

/**
 * @brief Begin the measured span of a manual benchmark iteration
 *
 * Timestamps now. Anything the body did before this call is excluded from
 * the measurement.
 *
 * Calls made outside a ZTEST_BENCHMARK_MANUAL() body are ignored.
 */
void ztest_benchmark_start(void);

/**
 * @brief End the measured span of a manual benchmark iteration
 *
 * Timestamps now and hands the span to the framework as one sample.
 * Without a preceding ztest_benchmark_start() there is no span, and the
 * call is ignored.
 *
 * Must be called from thread context: recording a sample performs floating
 * point math, which is not safe in ISRs on all architectures.
 */
void ztest_benchmark_end(void);

/**
 * @brief Begin the measured span at a timestamp taken elsewhere
 *
 * As ztest_benchmark_start(), for a span that began in another execution
 * context. An ISR captures timing_counter_get() and the body hands the
 * result over once it runs.
 *
 * @param start Timestamp the measured span began at
 */
void ztest_benchmark_start_at(timing_t start);

/**
 * @brief End the measured span at a timestamp taken elsewhere
 *
 * As ztest_benchmark_end(), for a span whose end was captured in another
 * execution context.
 *
 * Must be called from thread context, for the same reason as
 * ztest_benchmark_end().
 *
 * @param end Timestamp the measured span ended at
 */
void ztest_benchmark_end_at(timing_t end);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZTEST_BENCHMARK_H */
