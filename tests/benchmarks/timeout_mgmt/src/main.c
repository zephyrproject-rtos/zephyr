/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * This file contains the main testing module that invokes all the tests.
 */

#include <zephyr/kernel.h>
#include <zephyr/timestamp.h>
#include "utils.h"
#include <zephyr/tc_util.h>
#include <ksched.h>

#define IS_ODD(x) (((x) & 0x1) != 0)

uint32_t tm_off;

static struct _timeout timeout[CONFIG_BENCHMARK_NUM_TIMEOUTS];

static uint64_t add_cycles[CONFIG_BENCHMARK_NUM_TIMEOUTS];
static uint64_t abort_cycles[CONFIG_BENCHMARK_NUM_TIMEOUTS];
static uint32_t ticks[CONFIG_BENCHMARK_NUM_TIMEOUTS];

static void cycles_reset(unsigned int num_timeouts)
{
	unsigned int i;

	for (i = 0; i < num_timeouts; i++) {
		add_cycles[i] = 0ULL;
		abort_cycles[i] = 0ULL;
	}
}

/* Empty handler */
static void handler(struct _timeout *t)
{
	ARG_UNUSED(t);
}

static void test_decreasing_timeouts(unsigned int num_timeouts)
{
	unsigned int i;
	timing_t start;
	timing_t finish;

	for (i = 0; i < num_timeouts; i++) {
		ticks[i] = num_timeouts - i + CONFIG_BENCHMARK_TIMEOUT_BIAS;

		start = timing_counter_get();
		z_add_timeout(&timeout[i], handler, K_TICKS(ticks[i]));
		finish = timing_counter_get();
		add_cycles[i] += timing_cycles_get(&start, &finish);
	}

	/*
	 * Abort timeouts in the reverse order of when they would expire. It is
	 * possible that some timeouts may have expired by the time we stop
	 * them, but that is deemed acceptable for the characterization curve.
	 */

	for (i = 0; i < num_timeouts; i++) {
		start = timing_counter_get();
		z_abort_timeout(&timeout[i]);
		finish = timing_counter_get();
		abort_cycles[i] += timing_cycles_get(&start, &finish);
	}
}

static void test_increasing_timeouts(unsigned int num_timeouts)
{
	unsigned int i;
	timing_t start;
	timing_t finish;

	/* Add timeouts with increasing waits. */

	for (i = 0; i < num_timeouts; i++) {
		ticks[i] = i + 1 + CONFIG_BENCHMARK_TIMEOUT_BIAS;

		start = timing_counter_get();
		z_add_timeout(&timeout[i], handler, K_TICKS(ticks[i]));
		finish = timing_counter_get();
		add_cycles[i] += timing_cycles_get(&start, &finish);
	}

	/*
	 * Abort timeouts in the same order as they would expire. It is
	 * possible that some timeouts may have expired by the time we stop
	 * them, but that is deemed acceptable for the characterization curve.
	 */

	for (i = 0; i < num_timeouts; i++) {
		start = timing_counter_get();
		z_abort_timeout(&timeout[i]);
		finish = timing_counter_get();
		abort_cycles[i] += timing_cycles_get(&start, &finish);
	}
}

static void test_interleaved_timeouts(unsigned int num_timeouts)
{
	unsigned int i;
	timing_t start;
	timing_t finish;

	/* Add timeouts with interleaving converging to the middle */

	for (i = 0; i < num_timeouts; i++) {
		if (IS_ODD(i)) {
			ticks[i] = (num_timeouts - (i / 2)) + CONFIG_BENCHMARK_TIMEOUT_BIAS;
		} else {
			ticks[i] = (i / 2) + 1 + CONFIG_BENCHMARK_TIMEOUT_BIAS;
		}

		start = timing_counter_get();
		z_add_timeout(&timeout[i], handler, K_TICKS(ticks[i]));
		finish = timing_counter_get();
		add_cycles[i] += timing_cycles_get(&start, &finish);
	}

	/* Remove timeouts with interleaving diverging from the middle */

	for (i = 0; i < num_timeouts; i++) {
		start = timing_counter_get();
		z_abort_timeout(&timeout[num_timeouts - i - 1]);
		finish = timing_counter_get();
		abort_cycles[i] += timing_cycles_get(&start, &finish);
	}
}

static uint64_t sqrt_u64(uint64_t square)
{
	if (square > 1) {
		uint64_t lo = sqrt_u64(square >> 2) << 1;
		uint64_t hi = lo + 1;

		return ((hi * hi) > square) ? lo : hi;
	}

	return square;
}

static void compute_and_report_stats(unsigned int num_timeouts,
				     unsigned int num_iterations,
				     uint64_t *cycles, const char *str)
{
	uint64_t minimum = cycles[0];
	uint64_t maximum = cycles[0];
	uint64_t total = cycles[0];
	uint64_t average;
	uint64_t std_dev = 0;
	uint64_t tmp;
	uint64_t diff;
	unsigned int i;

	for (i = 1; i < num_timeouts; i++) {
		if (cycles[i] > maximum) {
			maximum = cycles[i];
		}

		if (cycles[i] < minimum) {
			minimum = cycles[i];
		}

		total += cycles[i];
	}

	minimum /= (uint64_t)num_iterations;
	maximum /= (uint64_t)num_iterations;
	average = total / (num_timeouts * num_iterations);

	for (i = 0; i < num_timeouts; i++) {
		tmp = cycles[i] / num_iterations;
		diff = (average > tmp) ? (average - tmp) : (tmp - average);

		std_dev += (diff * diff);
	}
	std_dev /= num_timeouts;
	std_dev = sqrt_u64(std_dev);

	printk("------------------------------------\n");
	printk("%s\n", str);

	printk("    Minimum : %7llu cycles (%7u nsec)\n", minimum,
	       (uint32_t)timing_cycles_to_ns(minimum));
	printk("    Maximum : %7llu cycles (%7u nsec)\n", maximum,
	       (uint32_t)timing_cycles_to_ns(maximum));
	printk("    Average : %7llu cycles (%7u nsec)\n", average,
	       (uint32_t)timing_cycles_to_ns(average));
	printk("    Std Deviation: %7llu cycles (%7u nsec)\n", std_dev,
	       (uint32_t)timing_cycles_to_ns(std_dev));
}

int main(void)
{
	unsigned int i;
	unsigned int freq;
#ifdef CONFIG_BENCHMARK_VERBOSE
	char description[120];
	char tag[50];
#endif

	timing_init();

	bench_test_init();

	freq = timing_freq_get_mhz();

	printk("Time Measurements for timeout system\n");
	printk("Timing results: Clock frequency: %u MHz\n", freq);

	timing_start();

	cycles_reset(CONFIG_BENCHMARK_NUM_TIMEOUTS);

	for (i = 0; i < CONFIG_BENCHMARK_NUM_ITERATIONS; i++) {
		test_increasing_timeouts(CONFIG_BENCHMARK_NUM_TIMEOUTS);
	}

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_TIMEOUTS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 add_cycles,
				 "Add timeouts with increasing waits");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_TIMEOUTS; i++) {
		snprintf(tag, sizeof(tag),
			 "timeout.add.increasing.%04u.ticks", ticks[i]);
		snprintf(description, sizeof(description),
			 "%-40s - Add %u ticks timeout",
			 tag, ticks[i]);
		PRINT_STATS_AVG(description, (uint32_t)add_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_TIMEOUTS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 abort_cycles,
				 "Abort timeouts in order of expiration");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_TIMEOUTS; i++) {
		snprintf(tag, sizeof(tag),
			 "timeout.abort.increasing.%04u.ticks", ticks[i]);
		snprintf(description, sizeof(description),
			 "%-40s - Abort %u ticks timeout",
			 tag, ticks[i]);
		PRINT_STATS_AVG(description, (uint32_t)abort_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	cycles_reset(CONFIG_BENCHMARK_NUM_TIMEOUTS);

	for (i = 0; i < CONFIG_BENCHMARK_NUM_ITERATIONS; i++) {
		test_decreasing_timeouts(CONFIG_BENCHMARK_NUM_TIMEOUTS);
	}

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_TIMEOUTS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 add_cycles,
				 "Add timeouts with decreasing waits");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_TIMEOUTS; i++) {
		snprintf(tag, sizeof(tag),
			 "timeout.add.decreasing.%04u.ticks", ticks[i]);
		snprintf(description, sizeof(description),
			 "%-40s - Add %u ticks timeout", tag, ticks[i]);
		PRINT_STATS_AVG(description, (uint32_t)add_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_TIMEOUTS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 abort_cycles,
				 "Abort timeouts in reverse expiration order");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_TIMEOUTS; i++) {
		snprintf(tag, sizeof(tag),
			"timeout.abort.decreasing.%04u.ticks", ticks[i]);
		snprintf(description, sizeof(description),
			 "%-40s - Abort %u ticks timeout", tag, ticks[i]);
		PRINT_STATS_AVG(description, (uint32_t)abort_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	cycles_reset(CONFIG_BENCHMARK_NUM_TIMEOUTS);

	for (i = 0; i < CONFIG_BENCHMARK_NUM_ITERATIONS; i++) {
		test_interleaved_timeouts(CONFIG_BENCHMARK_NUM_TIMEOUTS);
	}

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_TIMEOUTS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 add_cycles,
				 "Add timeouts converging to middle");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_TIMEOUTS; i++) {
		snprintf(tag, sizeof(tag),
			 "timeout.add.converging.%04u.ticks", ticks[i]);
		snprintf(description, sizeof(description),
			 "%-40s - Add %u ticks timeout", tag, ticks[i]);
		PRINT_STATS_AVG(description, (uint32_t)add_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_TIMEOUTS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 abort_cycles,
				 "Abort timeouts diverging from middle");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_TIMEOUTS; i++) {
		snprintf(tag, sizeof(tag),
			"timeout.abort.diverging.%04u.ticks",
			ticks[CONFIG_BENCHMARK_NUM_TIMEOUTS - i - 1]);
		snprintf(description, sizeof(description),
			 "%-40s - Abort %u ticks timeout",
			 tag, ticks[CONFIG_BENCHMARK_NUM_TIMEOUTS - i - 1]);
		PRINT_STATS_AVG(description, (uint32_t)abort_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	timing_stop();

	TC_END_REPORT(0);

	return 0;
}
