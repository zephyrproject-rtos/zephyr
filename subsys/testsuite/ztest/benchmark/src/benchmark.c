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

/*
 * Render a corrected extreme as a whole number of cycles.
 *
 * The extremes are single measurements and a cycle is a discrete thing,
 * so printing them with a fraction reads oddly. The correction is still
 * a real number, so rounding has to be directional: the minimum rounds
 * down and the maximum rounds up, which keeps each of them outside the
 * statistics computed from the same samples instead of letting a
 * rounded extreme cross the mean.
 */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
static int64_t noise_corrected_floor(uint64_t value, double ctrl)
{
	return (int64_t)floor(noise_correction((double)value, ctrl));
}

static int64_t noise_corrected_ceil(uint64_t value, double ctrl)
{
	return (int64_t)ceil(noise_correction((double)value, ctrl));
}
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */

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

static void percentiles_report(const char *suite_name, const char *bench_name,
			       struct ztest_benchmark_stats *ctrl_stats)
{
	double ctrl = (double)ctrl_stats->mean;

	if (retained_count == 0) {
		return;
	}

	/*
	 * The retained samples are the first ones taken, not a sample of
	 * the whole run, so percentiles over them are not percentiles of
	 * the distribution: whatever happened after the buffer filled is
	 * missing entirely, and the tail is exactly where it tends to
	 * live. Report nothing rather than something misleading.
	 */
	if (dropped_count != 0) {
		printk("%s %s: no percentiles, %zu of %zu samples did not fit; "
		       "raise CONFIG_ZTEST_BENCHMARK_MAX_SAMPLES\n",
		       suite_name, bench_name, dropped_count,
		       retained_count + dropped_count);
		return;
	}

	percentiles_sort();

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
	printk("P,%s,%s,%zu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
		suite_name, bench_name, retained_count,
		noise_correction((double)retained[0], ctrl),
		noise_correction((double)percentile(5000), ctrl),
		noise_correction((double)percentile(9000), ctrl),
		noise_correction((double)percentile(9900), ctrl),
		noise_correction((double)percentile(9990), ctrl),
		noise_correction((double)percentile(9999), ctrl),
		noise_correction((double)retained[retained_count - 1], ctrl));
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
	printk("\tp50: %.3f\n", noise_correction((double)percentile(5000), ctrl));
	printk("\tp90: %.3f\n", noise_correction((double)percentile(9000), ctrl));
	printk("\tp99: %.3f\n", noise_correction((double)percentile(9900), ctrl));
	printk("\tp99.9: %.3f\n", noise_correction((double)percentile(9990), ctrl));
	printk("\tp99.99: %.3f\n", noise_correction((double)percentile(9999), ctrl));
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */
}
#else
static inline void percentiles_reset(void)
{
}

static inline void percentiles_record(uint64_t cycles)
{
	ARG_UNUSED(cycles);
}

static inline void percentiles_report(const char *suite_name, const char *bench_name,
				      struct ztest_benchmark_stats *ctrl_stats)
{
	ARG_UNUSED(suite_name);
	ARG_UNUSED(bench_name);
	ARG_UNUSED(ctrl_stats);
}
#endif /* CONFIG_ZTEST_BENCHMARK_PERCENTILES */

/*
 * The very first execution of a benchmark, before anything is warm.
 * Reported separately rather than folded into the distribution, where a
 * single cold sample would move the maximum and tell the reader nothing
 * about the steady state.
 */
static void ztest_benchmark_print_cold(const char *suite_name, const char *bench_name,
				       struct ztest_benchmark_stats *stats,
				       struct ztest_benchmark_stats *ctrl_stats)
{
	double value = noise_correction((double)stats->cold, (double)ctrl_stats->mean);

	if (stats->cold == 0) {
		return;
	}

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
	printk("C,%s,%s,%d,%.3f\n", suite_name, bench_name, CONFIG_ZTEST_BENCHMARK_WARMUP, value);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
	printk("\tCold (first run): %.3f\n", value);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */
}

static void ztest_benchmark_print_stats(const char *suite_name, const char *bench_name,
					char record_type, struct ztest_benchmark_stats *stats,
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

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
	printk("%c,%s,%s,%lld,%.3f,%.3f,%.3f,%.3f,%.3f,%lld,%.3f,%lld\n",
		record_type, suite_name, bench_name,
		stats->samples,
		noise_correction((double)stats->total, ctrl * (double)stats->samples),
		noise_correction(stats->mean, ctrl), stddev, std_error,
		noise_correction((double)stats->min.value, ctrl), stats->min.sample,
		noise_correction((double)stats->max.value, ctrl), stats->max.sample);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
	printk_line(bench_name, '=');
	printk("\tSample size:%lld, total cycles: %.3f\n", stats->samples,
			noise_correction((double)stats->total, ctrl * (double)stats->samples));

	printk("\tMean(u): %.3f\n", noise_correction(stats->mean, ctrl));
	printk("\tStandard deviation(s): %.3f\n", stddev);
	printk("\tStandard Error(SE): %.3f\n", std_error);
	printk("\tMin: %lld (run #%llu)\n", noise_corrected_floor(stats->min.value, ctrl),
		stats->min.sample);
	printk("\tMax: %lld (run #%llu)\n", noise_corrected_ceil(stats->max.value, ctrl),
		stats->max.sample);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE */

	percentiles_report(suite_name, bench_name, ctrl_stats);
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
	percentiles_reset();

	/*
	 * The warmup iterations run the whole loop, setup and teardown
	 * included, and are simply not recorded. Only the very first is
	 * kept, as the cold cost.
	 */
	for (size_t i = 0; i < CONFIG_ZTEST_BENCHMARK_WARMUP + benchmark->iterations; i++) {
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
			benchmark->stats.cold = cycles;
		}

		if (i >= CONFIG_ZTEST_BENCHMARK_WARMUP) {
			update_metrics(&benchmark->stats, cycles);
		}

		if (benchmark->teardown) {
			benchmark->teardown();
		}
	}
}

/*
 * The benchmark whose body is running, the span it has open, and which
 * iteration it is on.
 *
 * Only one benchmark runs at a time, so a single set of these serves all
 * of them. manual_span_open separates "no span open" from a span that
 * legitimately began at timestamp zero.
 */
static struct ztest_benchmark_stats *manual_active_stats;
static timing_t manual_span_start;
static bool manual_span_open;
static size_t manual_iteration;

void ztest_benchmark_start_at(timing_t start)
{
	if (manual_active_stats == NULL) {
		return;
	}

	manual_span_start = start;
	manual_span_open = true;
}

void ztest_benchmark_start(void)
{
	if (manual_active_stats == NULL) {
		return;
	}

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
	ztest_benchmark_start_at(timing_counter_get());
}

void ztest_benchmark_end_at(timing_t end)
{
	uint64_t cycles;

	__ASSERT(!k_is_in_isr(), "%s must be called from thread context", __func__);

	if ((manual_active_stats == NULL) || !manual_span_open) {
		return;
	}

	manual_span_open = false;
	cycles = timing_cycles_get(&manual_span_start, &end);

	if (manual_iteration == 0) {
		manual_active_stats->cold = cycles;
	}

	/*
	 * The warmup iterations run the body in full and are simply not
	 * recorded, exactly as for a sampled benchmark.
	 */
	if (manual_iteration >= CONFIG_ZTEST_BENCHMARK_WARMUP) {
		update_metrics(manual_active_stats, cycles);
	}
}

void ztest_benchmark_end(void)
{
	if ((manual_active_stats == NULL) || !manual_span_open) {
		return;
	}

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
	ztest_benchmark_end_at(timing_counter_get());
}

static void ztest_benchmark_manual_run(struct ztest_benchmark_manual *benchmark)
{
	memset(&benchmark->stats, 0, sizeof(benchmark->stats));
	benchmark->stats.min.value = UINT64_MAX;
	percentiles_reset();

	/*
	 * The loop, the setup and the teardown are the framework's, exactly
	 * as for a sampled benchmark. All the body decides is which part of
	 * itself the timestamps go around.
	 */
	for (size_t i = 0; i < CONFIG_ZTEST_BENCHMARK_WARMUP + benchmark->iterations; i++) {
		if (benchmark->setup) {
			benchmark->setup();
		}

		manual_active_stats = &benchmark->stats;
		manual_span_open = false;
		manual_iteration = i;
		benchmark->run();
		manual_active_stats = NULL;

		if (benchmark->teardown) {
			benchmark->teardown();
		}
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
	.iterations = 1000,
	.run = empty_function,
};

static struct ztest_benchmark_timed ctrl_timed = {
	.duration_ms = 100,
	.name = "ctrl_timed",
	.run = empty_function,
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
			ztest_benchmark_print_stats(suite->name, benchmark->name, 'S',
						    &benchmark->stats, &ctrl.stats);
			ztest_benchmark_print_cold(suite->name, benchmark->name,
						   &benchmark->stats, &ctrl.stats);
		}

		STRUCT_SECTION_FOREACH(ztest_benchmark_manual, benchmark) {
			if (benchmark->suite != suite) {
				continue;
			}
			ztest_benchmark_manual_run(benchmark);
			ztest_benchmark_print_stats(suite->name, benchmark->name, 'M',
						    &benchmark->stats, &ctrl.stats);
			ztest_benchmark_print_cold(suite->name, benchmark->name,
						   &benchmark->stats, &ctrl.stats);
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
