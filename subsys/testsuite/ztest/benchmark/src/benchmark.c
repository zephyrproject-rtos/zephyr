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

static void ztest_benchmark_print_results(struct ztest_benchmark *benchmark,
					  struct ztest_benchmark_stats *ctrl_stats)
{
	double ctrl = (double)ctrl_stats->mean;
	double stddev = 0.0;
	double sample_variance;
	double std_error = 0.0;
	struct ztest_benchmark_stats *stats = &benchmark->stats;

	if (stats->samples > 1) {
		sample_variance = stats->m2 / (double)(stats->samples - 1);
		stddev = sqrt(sample_variance);
		std_error = stddev / sqrt(stats->samples);
	}

#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV
	printk("S,%s,%s,%lld,%lld,%.3f,%.3f,%.3f,%lld,%lld,%lld,%lld\n",
		benchmark->suite->name, benchmark->name,
		stats->samples,
		discrete_noise_correction(stats->total, ctrl * stats->samples),
		noise_correction(stats->mean, ctrl), stddev, std_error,
		discrete_noise_correction(stats->min.value, ctrl), stats->min.sample,
		discrete_noise_correction(stats->max.value, ctrl), stats->max.sample);
#endif /* CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV */
#ifdef CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE
	printk_line(benchmark->name, '=');
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
}

static void update_metrics(struct ztest_benchmark_stats *stats, uint64_t cycles)
{
	double delta, delta2;

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
			ztest_benchmark_print_results(benchmark, &ctrl.stats);
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
