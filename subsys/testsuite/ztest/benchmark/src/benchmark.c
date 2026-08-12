/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <math.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/timing/timing.h>

#define RUN_10(x) x; x; x; x; x; x; x; x; x; x;
#define RUN_100(x) RUN_10(x) RUN_10(x) RUN_10(x) RUN_10(x) RUN_10(x) \
			RUN_10(x) RUN_10(x) RUN_10(x) RUN_10(x) RUN_10(x)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ztest_benchmark, LOG_LEVEL_WRN);

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
static void printk_line(const char *fn, char sep_char)
{
	size_t len = strlen(fn);

	if (len) {
		printk("%s ", fn);
		len += 1;
	}
	for (size_t i = 0; i < 60 - len; i++) {
		printk("%c", sep_char);
	}
	printk("\n");
}
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */

/*
 * Subtract the control measurement from a reported statistic.
 *
 * Every statistic has to be corrected by the same amount, or they stop
 * describing one set of samples: truncating the control to an integer
 * for the discrete statistics, as this used to do, left a benchmark
 * whose samples were all identical reporting a mean below its own
 * minimum whenever the control was under one cycle.
 *
 * The correction is a real number, so the corrected values are real
 * numbers too, and generally do not coincide with any single
 * measurement.
 */
static double noise_correction(double value, double ctrl)
{
	return value - ctrl;
}

#ifdef CONFIG_ZTEST_BENCHMARK_PERCENTILES
/*
 * Retained samples, for the percentile report. Only one benchmark runs
 * at a time, so a single buffer serves all of them.
 */
static uint64_t retained[CONFIG_ZTEST_BENCHMARK_MAX_SAMPLES];
static size_t retained_count;
static size_t dropped_count;

static void percentiles_reset(void)
{
	retained_count = 0;
	dropped_count = 0;
}

static void percentiles_record(uint64_t cycles)
{
	if (retained_count < ARRAY_SIZE(retained)) {
		retained[retained_count++] = cycles;
	} else {
		dropped_count++;
	}
}

static void percentiles_sort(void)
{
	/* Shell sort: no allocation, no recursion, good enough here */
	for (size_t gap = retained_count / 2; gap > 0; gap /= 2) {
		for (size_t i = gap; i < retained_count; i++) {
			uint64_t value = retained[i];
			size_t j = i;

			while ((j >= gap) && (retained[j - gap] > value)) {
				retained[j] = retained[j - gap];
				j -= gap;
			}
			retained[j] = value;
		}
	}
}

/*
 * Nearest-rank percentile, with the percentile given in hundredths of a
 * percent so that 99.99 can be expressed: 5000 is the median, 9999 is
 * the 99.99th percentile.
 */
static uint64_t percentile(uint32_t hundredths)
{
	uint64_t rank = ((uint64_t)hundredths * retained_count + 9999U) / 10000U;

	if (rank == 0U) {
		rank = 1U;
	}

	if (rank > retained_count) {
		rank = retained_count;
	}

	return retained[rank - 1U];
}

#if defined(CONFIG_ZTEST_BENCHMARK_OUTLIERS) && defined(CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE)
/*
 * Samples slower than the median.
 *
 * For a kernel latency distribution the median is the baseline: the
 * operation costs the same on almost every run, and what matters is how
 * many runs were disturbed and by how much. Counting them says that
 * directly, where a standard error would only describe how precisely an
 * average of two different populations had been estimated.
 */
static size_t percentiles_outliers(void)
{
	uint64_t median = percentile(5000);
	size_t count = 0;

	while ((count < retained_count) && (retained[retained_count - 1 - count] > median)) {
		count++;
	}

	return count;
}
#endif /* CONFIG_ZTEST_BENCHMARK_OUTLIERS && CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */

static bool percentiles_available(void)
{
	return retained_count != 0;
}

static void percentiles_prepare(const char *suite_name, const char *bench_name)
{
	if (retained_count == 0) {
		return;
	}

	percentiles_sort();

	if (dropped_count != 0) {
		printk("%s %s: percentiles cover the first %zu samples, %zu were dropped; "
		       "raise CONFIG_ZTEST_BENCHMARK_MAX_SAMPLES\n",
		       suite_name, bench_name, retained_count, dropped_count);
	}
}
#else
/*
 * Only the two hooks the runner calls unconditionally need a stub. The
 * rest are referenced solely from code that this option compiles out,
 * and a stub for them would be an unused function, which both
 * compilers reject under the warning flags Zephyr builds with.
 */
static void percentiles_reset(void)
{
}

static void percentiles_record(uint64_t cycles)
{
	ARG_UNUSED(cycles);
}
#endif /* CONFIG_ZTEST_BENCHMARK_PERCENTILES */

/*
 * Report one benchmark.
 *
 * The layout is the one the framework has always printed. The optional
 * additions extend it rather than rearrange it: percentiles and the
 * cold cost are extra lines, and CONFIG_ZTEST_BENCHMARK_OUTLIERS
 * swaps the standard error for the outlier count in place.
 *
 * The CSV output keeps every column it had, including the standard
 * error, so that existing parsers are unaffected whatever is enabled.
 */
static void ztest_benchmark_report(const char *suite_name, const char *bench_name,
				   char record_type, struct ztest_benchmark_stats *stats,
				   size_t warmup, uint64_t cold,
				   struct ztest_benchmark_stats *ctrl_stats)
{
	double ctrl = (double)ctrl_stats->mean;
	double stddev = 0.0;
	double sample_variance;
	double std_error = 0.0;

	if (stats->samples == 0) {
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
		printk("%c,%s,%s\tINCONCLUSIVE\n", record_type, suite_name, bench_name);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
		printk_line(bench_name, '=');
		printk("\tTest inconclusive (no samples recorded)\n");
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */
		return;
	}

	if (stats->samples > 1) {
		sample_variance = stats->m2 / (double)(stats->samples - 1);
		stddev = sqrt(sample_variance);
		std_error = stddev / sqrt(stats->samples);
	}

#ifdef CONFIG_ZTEST_BENCHMARK_PERCENTILES
	percentiles_prepare(suite_name, bench_name);
#endif

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
	printk("%c,%s,%s,%lld,%.3f,%.3f,%.3f,%.3f,%.3f,%lld,%.3f,%lld\n",
		record_type, suite_name, bench_name,
		stats->samples,
		noise_correction((double)stats->total, ctrl * (double)stats->samples),
		noise_correction(stats->mean, ctrl), stddev, std_error,
		noise_correction((double)stats->min.value, ctrl), stats->min.sample,
		noise_correction((double)stats->max.value, ctrl), stats->max.sample);

	if (cold != 0) {
		printk("C,%s,%s,%zu,%.3f\n", suite_name, bench_name, warmup,
			noise_correction((double)cold, ctrl));
	}

#ifdef CONFIG_ZTEST_BENCHMARK_PERCENTILES
	/*
	 * The percentiles go in a row of their own rather than as extra
	 * columns on the row above, so that a parser written against the
	 * original column layout keeps working.
	 *
	 * The extremes come from the retained samples the percentiles were
	 * taken over, which is not the same set as the min and max on the
	 * row above once samples have been dropped.
	 */
	if (percentiles_available()) {
		printk("P,%s,%s,%zu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
			suite_name, bench_name, retained_count,
			noise_correction((double)retained[0], ctrl),
			noise_correction((double)percentile(5000), ctrl),
			noise_correction((double)percentile(9000), ctrl),
			noise_correction((double)percentile(9900), ctrl),
			noise_correction((double)percentile(9990), ctrl),
			noise_correction((double)percentile(9999), ctrl),
			noise_correction((double)retained[retained_count - 1], ctrl));
	}
#endif /* CONFIG_ZTEST_BENCHMARK_PERCENTILES */
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
	printk_line(bench_name, '=');
	printk("\tSample size:%lld, total cycles: %.3f\n", stats->samples,
			noise_correction((double)stats->total, ctrl * (double)stats->samples));

	printk("\tMean(u): %.3f\n", noise_correction(stats->mean, ctrl));
	printk("\tStandard deviation(s): %.3f\n", stddev);
#ifdef CONFIG_ZTEST_BENCHMARK_OUTLIERS
	/*
	 * The standard error describes how precisely the mean of one
	 * stochastic population was estimated. A latency distribution is
	 * usually a baseline plus a few disturbed runs instead, so the
	 * count of samples slower than the median says more about it.
	 */
	if (percentiles_available()) {
		size_t outliers = percentiles_outliers();

		printk("\tOutliers: %zu / %zu (%.3f%%)\n", outliers, retained_count,
			(100.0 * (double)outliers) / (double)retained_count);
	}
#else
	printk("\tStandard Error(SE): %.3f\n", std_error);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTLIERS */
	printk("\tMin: %.3f (run #%llu)\n", noise_correction((double)stats->min.value, ctrl),
		stats->min.sample);
	printk("\tMax: %.3f (run #%llu)\n", noise_correction((double)stats->max.value, ctrl),
		stats->max.sample);

#ifdef CONFIG_ZTEST_BENCHMARK_PERCENTILES
	if (percentiles_available()) {
		printk("\tp50: %.3f\n", noise_correction((double)percentile(5000), ctrl));
		printk("\tp90: %.3f\n", noise_correction((double)percentile(9000), ctrl));
		printk("\tp99: %.3f\n", noise_correction((double)percentile(9900), ctrl));
		printk("\tp99.9: %.3f\n", noise_correction((double)percentile(9990), ctrl));
		printk("\tp99.99: %.3f\n", noise_correction((double)percentile(9999), ctrl));
	}
#endif /* CONFIG_ZTEST_BENCHMARK_PERCENTILES */

	if (cold != 0) {
		printk("\tCold: %.3f, after %zu warmup iterations\n",
			noise_correction((double)cold, ctrl), warmup);
	}
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */
}

static void update_metrics(struct ztest_benchmark_stats *stats, uint64_t cycles)
{
	double delta, delta2;

	percentiles_record(cycles);

	/* Welfords method */
	stats->samples += 1;
	stats->total += cycles;

	if (cycles < stats->min.value) {
		stats->min.value = cycles;
		stats->min.sample = stats->samples;
	}

	if (cycles > stats->max.value) {
		stats->max.value = cycles;
		stats->max.sample = stats->samples;
	}

	delta = (double)cycles - stats->mean;
	stats->mean += delta / (double)stats->samples;
	delta2 = (double)cycles - stats->mean;
	stats->m2 += delta * delta2;
}

static void ztest_benchmark_run(struct ztest_benchmark *benchmark)
{
	timing_t start, end;

	memset(&benchmark->stats, 0, sizeof(benchmark->stats));
	benchmark->stats.min.value = UINT64_MAX;
	benchmark->cold = 0;
	percentiles_reset();

	/*
	 * The warmup iterations run the whole loop, setup and teardown
	 * included, and are simply not recorded. Only the very first is
	 * kept, as the cold cost.
	 */
	for (size_t i = 0; i < benchmark->warmup + benchmark->iterations; i++) {
		uint64_t cycles;

		if (benchmark->setup) {
			benchmark->setup();
		}

		barrier_dsync_fence_full();
		barrier_isync_fence_full();
		start = timing_counter_get();
		benchmark->run();
		end = timing_counter_get();

		cycles = timing_cycles_get(&start, &end);

		if (i == 0) {
			benchmark->cold = cycles;
		}

		if (i >= benchmark->warmup) {
			update_metrics(&benchmark->stats, cycles);
		}

		if (benchmark->teardown) {
			benchmark->teardown();
		}
	}
}

static struct ztest_benchmark_manual *manual_active;
static size_t manual_recorded;

void ztest_benchmark_record_sample(uint64_t cycles)
{
	__ASSERT(!k_is_in_isr(), "%s must be called from thread context", __func__);

	if (manual_active == NULL) {
		return;
	}

	if (manual_recorded == 0) {
		manual_active->cold = cycles;
	}

	manual_recorded++;

	/*
	 * Drop the warmup samples here rather than asking the body to
	 * tell the two phases apart. Recording every iteration and
	 * discarding the early ones keeps each measured iteration
	 * preceded by exactly the same work as the one before it.
	 */
	if (manual_recorded > manual_active->warmup) {
		update_metrics(&manual_active->stats, cycles);
	}
}

size_t ztest_benchmark_iterations(void)
{
	if (manual_active == NULL) {
		return 0;
	}

	return manual_active->warmup + manual_active->iterations;
}

static void ztest_benchmark_manual_run(struct ztest_benchmark_manual *benchmark)
{
	memset(&benchmark->stats, 0, sizeof(benchmark->stats));
	benchmark->stats.min.value = UINT64_MAX;
	benchmark->cold = 0;
	percentiles_reset();

	if (benchmark->setup) {
		benchmark->setup();
	}

	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	manual_active = benchmark;
	manual_recorded = 0;
	benchmark->run();
	manual_active = NULL;

	if (benchmark->teardown) {
		benchmark->teardown();
	}
}

static void ztest_benchmark_print_timed_results(struct ztest_benchmark_timed *benchmark,
		struct ztest_benchmark_timed_stats *ctrl)
{
	uint64_t total_cycles, net_cycles, net_ns;
	double total_overhead, ops_per_sec, cycles_per_op;
	struct ztest_benchmark_timed_stats *stats = &benchmark->stats;

	if (stats->iterations == 0) {
		goto INCONCLUSIVE;
	}

	total_cycles = stats->duration_cycles;
	total_overhead = ((double)ctrl->duration_cycles / ctrl->iterations) * stats->iterations;

	if (total_cycles < total_overhead) {
		goto INCONCLUSIVE;
	}

	net_cycles = (uint64_t) (total_cycles - total_overhead);
	net_ns = timing_cycles_to_ns(net_cycles);


	ops_per_sec = 0.0;
	cycles_per_op = (double)net_cycles / stats->iterations;
	if (net_ns) {
		ops_per_sec = (stats->iterations * 1e9) / (double)net_ns;
	}

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
	printk("T,%s,%s,%llu,%llu,%llu,%.3f,%.3f\n",
		benchmark->suite->name, benchmark->name,
		stats->iterations, total_cycles, net_cycles, ops_per_sec, cycles_per_op);
	return;

INCONCLUSIVE:
	printk("T,%s,%s\tINCONCLUSIVE\n", benchmark->suite->name, benchmark->name);
#else
	printk_line(benchmark->name, '=');
	printk("\tIterations: %llu\n", stats->iterations);
	printk("\tTotal Time: %llu ns (Gross)\n", timing_cycles_to_ns(total_cycles));
	printk("\tWork Time: %llu ns (Net)\n", net_ns);
	printk("\tOps/Sec: %.3f\n", ops_per_sec);
	printk("\tCycles/Ops: %.3f\n", cycles_per_op);
	return;

INCONCLUSIVE:
	printk_line(benchmark->name, '=');
	printk("\tTest inconclusive\n");
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
}

static void ztest_benchmark_timed_run(struct ztest_benchmark_timed *benchmark)
{
	k_timepoint_t stop;
	timing_t start, end;
	size_t iterations = 0;

	if (benchmark->setup) {
		benchmark->setup();
	}

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
	stop = sys_timepoint_calc(K_MSEC(benchmark->duration_ms));
	start = timing_counter_get();
	while (!sys_timepoint_expired(stop)) {
		/* Batch runs to reduce loop overhead */
		RUN_100(benchmark->run());
		iterations += 100;
	}
	end = timing_counter_get();

	benchmark->stats.duration_cycles = timing_cycles_get(&start, &end);
	benchmark->stats.iterations = iterations;
	if (benchmark->teardown) {
		benchmark->teardown();
	}
}

static __noinline void empty_function(void)
{
	/* Intentionally empty */
}

static struct ztest_benchmark ctrl = {
	.name = "ctrl",
	.run = empty_function,
	.iterations = 1000,
};

static struct ztest_benchmark_timed ctrl_timed = {
	.name = "ctrl_timed",
	.run = empty_function,
	.duration_ms = 100,
};

void benchmark_main(void)
{
	timing_init();
	timing_start();

	k_sched_lock();
	ztest_benchmark_run(&ctrl);
	ztest_benchmark_timed_run(&ctrl_timed);
	k_sched_unlock();

	STRUCT_SECTION_FOREACH(ztest_benchmark_suite, suite) {
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
		printk_line(suite->name, '#');
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */
		if (suite->setup) {
			suite->setup();
		}

		STRUCT_SECTION_FOREACH(ztest_benchmark, benchmark) {
			if (benchmark->suite != suite) {
				continue;
			}
			ztest_benchmark_run(benchmark);
			ztest_benchmark_report(suite->name, benchmark->name, 'S',
					       &benchmark->stats, benchmark->warmup,
					       benchmark->cold, &ctrl.stats);
		}

		STRUCT_SECTION_FOREACH(ztest_benchmark_manual, benchmark) {
			if (benchmark->suite != suite) {
				continue;
			}
			ztest_benchmark_manual_run(benchmark);
			ztest_benchmark_report(suite->name, benchmark->name, 'M',
					       &benchmark->stats, benchmark->warmup,
					       benchmark->cold, &ctrl.stats);
		}

		STRUCT_SECTION_FOREACH(ztest_benchmark_timed, benchmark) {
			if (benchmark->suite != suite) {
				continue;
			}
			ztest_benchmark_timed_run(benchmark);
			ztest_benchmark_print_timed_results(benchmark, &ctrl_timed.stats);
		}

		if (suite->teardown) {
			suite->teardown();
		}
	}
	timing_stop();
}
