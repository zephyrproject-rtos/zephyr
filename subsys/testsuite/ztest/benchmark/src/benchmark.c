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

static void print_double(const char *str, double value)
{
	static const uint64_t fractional_precision = 3;
	int64_t digit = (int64_t)value;
	int64_t frac = (int64_t)((value - digit) * (int64_t)pow(10, fractional_precision));

	printk("\t%s: %lld.%03lld\n", str, digit, frac);
}

static double noise_correction(double value, double ctrl)
{
	return value - ctrl;
}

static int64_t discrete_noise_correction(uint64_t value, double ctrl)
{
	return (int64_t)value - (int64_t)trunc(ctrl);
}

static void ztest_benchmark_print_results(struct ztest_benchmark *benchmark, double ctrl)
{
	double stddev = 0.0;
	double sample_variance;
	double std_error = 0.0;
	struct ztest_benchmark_stats *stats = benchmark->stats;

	if (stats->samples > 1) {
		sample_variance = stats->m2 / (double)(stats->samples - 1);
		stddev = sqrt(sample_variance);
		std_error = stddev / sqrt(stats->samples);
	}

	printk_line(benchmark->name, '=');
	printk("\tSample size:%lld, total cycles: %lld\n", stats->samples,
			discrete_noise_correction(stats->total, ctrl * stats->samples));
	print_double("Mean(u)", noise_correction(stats->mean, ctrl));
	print_double("Standard deviation(s)", stddev);
	print_double("Standard Error(SE)", std_error);
	printk("\tMin: %lld (run #%llu)\n", discrete_noise_correction(stats->min.value, ctrl),
		stats->min.sample);
	printk("\tMax: %lld (run #%llu)\n", discrete_noise_correction(stats->max.value, ctrl),
		stats->max.sample);

	if (benchmark->counter && benchmark->counter->print) {
		benchmark->counter->print(benchmark->counter);
	}
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

	memset(benchmark->stats, 0, sizeof(*benchmark->stats));
	benchmark->stats->min.value = UINT64_MAX;

	if (benchmark->setup) {
		benchmark->setup();
	}

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
	for (size_t i = 0; i < benchmark->iterations; i++) {
		start = timing_counter_get();
		benchmark->run();
		end = timing_counter_get();
		update_metrics(benchmark->stats, timing_cycles_get(&start, &end));
		if (benchmark->counter && benchmark->counter->count) {
			benchmark->counter->count(benchmark->counter);
		}
	}
	if (benchmark->teardown) {
		benchmark->teardown();
	}

}

static void ztest_benchmark_print_timed_results(struct ztest_benchmark_timed *benchmark,
		struct ztest_benchmark_timed *ctrl)
{
	uint64_t total_cycles = benchmark->duration_cycles;
	uint64_t overhead_per_iter = ctrl->iterations > 0 ?
		ctrl->duration_cycles / ctrl->iterations : 0;
	uint64_t total_overhead = overhead_per_iter * benchmark->iterations;
	uint64_t net_cycles = total_cycles > total_overhead ?
		total_cycles - total_overhead : 0;
	uint64_t net_ns = timing_cycles_to_ns(net_cycles);
	double avg_ns_net = net_ns / (double)benchmark->iterations;
	double ops_per_sec_net = net_ns > 0 ?
		(benchmark->iterations * pow(10, 9)) / (double)net_ns : 0.0;

	printk_line(benchmark->name, '=');

	printk("\tIterations: %zu\n", benchmark->iterations);
	printk("\tTotal Time: %llu ns (Gross)\n", timing_cycles_to_ns(total_cycles));
	printk("\tWork Time: %llu ns (Net)\n", net_ns);

	print_double("Avg time/Iter (ns)", avg_ns_net);
	print_double("Ops/Sec", ops_per_sec_net);
	print_double("Cycles/Iter", (double)net_cycles / (double)benchmark->iterations);
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

	benchmark->duration_cycles = timing_cycles_get(&start, &end);
	benchmark->iterations = iterations;
	if (benchmark->teardown) {
		benchmark->teardown();
	}
}

static void empty_function(void)
{
	/* Intentionally empty */
}

static struct ztest_benchmark_stats ctrl_stats;
static struct ztest_benchmark ctrl = {
	.name = "ctrl",
	.run = empty_function,
	.iterations = 1000,
	.stats = &ctrl_stats,
};

static struct ztest_benchmark_timed ctrl_timed = {
	.name = "ctrl_timed",
	.run = empty_function,
	.duration_ms = 500,
};

void benchmark_main(void)
{
	timing_init();
	timing_start();

	k_msleep(100); /* system stabilization */

	ztest_benchmark_run(&ctrl);
	ztest_benchmark_timed_run(&ctrl_timed);
	STRUCT_SECTION_FOREACH(ztest_benchmark_suite, suite) {
		printk_line(suite->name, '#');
		if (suite->setup) {
			suite->setup();
		}

		STRUCT_SECTION_FOREACH(ztest_benchmark, benchmark) {
			if (benchmark->suite != suite) {
				continue;
			}
			ztest_benchmark_run(benchmark);
			ztest_benchmark_print_results(benchmark, ctrl.stats->mean);
		}

		STRUCT_SECTION_FOREACH(ztest_benchmark_timed, benchmark) {
			if (benchmark->suite != suite) {
				continue;
			}
			ztest_benchmark_timed_run(benchmark);
			ztest_benchmark_print_timed_results(benchmark, &ctrl_timed);
		}

		if (suite->teardown) {
			suite->teardown();
		}
	}
	timing_stop();
}
