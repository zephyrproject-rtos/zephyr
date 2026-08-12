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

static double noise_correction(double value, double ctrl)
{
	return value - ctrl;
}

static int64_t discrete_noise_correction(uint64_t value, double ctrl)
{
	return (int64_t)value - (int64_t)trunc(ctrl);
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

static void percentiles_report(const char *suite_name, const char *bench_name,
			       struct ztest_benchmark_stats *ctrl_stats)
{
	double ctrl = (double)ctrl_stats->mean;

	if (retained_count == 0) {
		return;
	}

	percentiles_sort();

	if (dropped_count != 0) {
		printk("%s %s: percentiles cover the first %zu samples, %zu were dropped; "
		       "raise CONFIG_ZTEST_BENCHMARK_MAX_SAMPLES\n",
		       suite_name, bench_name, retained_count, dropped_count);
	}

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
	printk("P,%s,%s,%zu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
		suite_name, bench_name, retained_count,
		discrete_noise_correction(retained[0], ctrl),
		discrete_noise_correction(percentile(5000), ctrl),
		discrete_noise_correction(percentile(9000), ctrl),
		discrete_noise_correction(percentile(9900), ctrl),
		discrete_noise_correction(percentile(9990), ctrl),
		discrete_noise_correction(percentile(9999), ctrl),
		discrete_noise_correction(retained[retained_count - 1], ctrl));
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
	printk("\tp50: %lld\n", discrete_noise_correction(percentile(5000), ctrl));
	printk("\tp90: %lld\n", discrete_noise_correction(percentile(9000), ctrl));
	printk("\tp99: %lld\n", discrete_noise_correction(percentile(9900), ctrl));
	printk("\tp99.9: %lld\n", discrete_noise_correction(percentile(9990), ctrl));
	printk("\tp99.99: %lld\n", discrete_noise_correction(percentile(9999), ctrl));
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
	printk("%c,%s,%s,%lld,%lld,%.3f,%.3f,%.3f,%lld,%lld,%lld,%lld\n",
		record_type, suite_name, bench_name,
		stats->samples,
		discrete_noise_correction(stats->total, ctrl * stats->samples),
		noise_correction(stats->mean, ctrl), stddev, std_error,
		discrete_noise_correction(stats->min.value, ctrl), stats->min.sample,
		discrete_noise_correction(stats->max.value, ctrl), stats->max.sample);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
	printk_line(bench_name, '=');
	printk("\tSample size:%lld, total cycles: %lld\n", stats->samples,
			discrete_noise_correction(stats->total, ctrl * stats->samples));

	printk("\tMean(u): %.3f\n", noise_correction(stats->mean, ctrl));
	printk("\tStandard deviation(s): %.3f\n", stddev);
	printk("\tStandard Error(SE): %.3f\n", std_error);
	printk("\tMin: %lld (run #%llu)\n", discrete_noise_correction(stats->min.value, ctrl),
		stats->min.sample);
	printk("\tMax: %lld (run #%llu)\n", discrete_noise_correction(stats->max.value, ctrl),
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

	for (size_t i = 0; i < benchmark->iterations; i++) {
		if (benchmark->setup) {
			benchmark->setup();
		}

		barrier_dsync_fence_full();
		barrier_isync_fence_full();
		start = timing_counter_get();
		benchmark->run();
		end = timing_counter_get();

		update_metrics(&benchmark->stats, timing_cycles_get(&start, &end));

		if (benchmark->teardown) {
			benchmark->teardown();
		}
	}
}

static struct ztest_benchmark_stats *manual_active_stats;

void ztest_benchmark_record_sample(uint64_t cycles)
{
	__ASSERT(!k_is_in_isr(), "%s must be called from thread context", __func__);

	if (manual_active_stats != NULL) {
		update_metrics(manual_active_stats, cycles);
	}
}

static void ztest_benchmark_manual_run(struct ztest_benchmark_manual *benchmark)
{
	memset(&benchmark->stats, 0, sizeof(benchmark->stats));
	benchmark->stats.min.value = UINT64_MAX;
	percentiles_reset();

	if (benchmark->setup) {
		benchmark->setup();
	}

	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	manual_active_stats = &benchmark->stats;
	benchmark->run();
	manual_active_stats = NULL;

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
			ztest_benchmark_print_stats(suite->name, benchmark->name, 'S',
						    &benchmark->stats, &ctrl.stats);
		}

		STRUCT_SECTION_FOREACH(ztest_benchmark_manual, benchmark) {
			if (benchmark->suite != suite) {
				continue;
			}
			ztest_benchmark_manual_run(benchmark);
			ztest_benchmark_print_stats(suite->name, benchmark->name, 'M',
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
