/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include <zephyr/kernel.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

static uint32_t periodic_idx;
static uint32_t periodic_rollovers;
static uint64_t periodic_data[CONFIG_TIMER_TEST_SAMPLES + 1];
static uint64_t periodic_start, periodic_end;
static struct k_timer periodic_timer;
K_SEM_DEFINE(periodic_sem, 0, 1);

void periodic_fn(struct k_timer *t)
{
	if (periodic_idx < CONFIG_TIMER_TEST_SAMPLES) {

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
		periodic_end = k_cycle_get_64();
#else
		periodic_end = k_cycle_get_32();
#endif
		periodic_data[periodic_idx + 1] = periodic_end;
		periodic_idx++;
	} else {
		k_timer_stop(t);
		k_sem_give(&periodic_sem);
	}
}

/* Get a difference in cycles between one timer count and an earlier one
 * accounting for potentially wrapped values.
 *
 * @retval 0 an unhandled wrap of the timer occurred and the value should be ignored
 */
uint64_t periodic_diff(uint64_t later, uint64_t earlier)
{
	/* Timer wrap around, will be ignored in statistics */
	if (later < earlier) {
		TC_ERROR("Caught a timer wrap around which isn't handled!\n");
		return 0;
	}

	return later - earlier;
}

double cycles_to_us(double cycles)
{
	return 1000000.0 * (cycles / (double)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
}

ZTEST(timer_behavior, test_periodic_behavior)
{
	TC_PRINT("periodic timer behavior test\n");

	k_timer_init(&periodic_timer, periodic_fn, NULL);

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	periodic_start = k_cycle_get_64();
#else
	periodic_start = k_cycle_get_32();
#endif

	periodic_data[0] = periodic_start;
	k_timer_start(&periodic_timer, K_USEC(CONFIG_TIMER_TEST_PERIOD),
		      K_USEC(CONFIG_TIMER_TEST_PERIOD));
	k_sem_take(&periodic_sem, K_FOREVER);

	TC_PRINT("periodic timer samples gathered, calculating statistics\n");

	/* calculate variance, and precision */
	uint64_t total_cycles = 0;

	uint64_t max_cyc = 0;
	uint64_t min_cyc = UINT64_MAX;

	for (int i = 0; i < CONFIG_TIMER_TEST_SAMPLES; i++) {
		uint64_t diff = periodic_diff(periodic_data[i + 1], periodic_data[i]);

		if (diff == 0) {
			periodic_rollovers++;
		} else {
			total_cycles += diff;
			min_cyc = MIN(diff, min_cyc);
			max_cyc = MAX(diff, max_cyc);
		}
	}

	double min_us = cycles_to_us((double)min_cyc);
	double max_us = cycles_to_us((double)max_cyc);

	double mean_cyc =
		(double)total_cycles / (double)(CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers);
	double mean_us = cycles_to_us((double)total_cycles) /
			 (double)(CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers);
	double variance_us = 0;
	double variance_cyc = 0;

	for (int i = 0; i < CONFIG_TIMER_TEST_SAMPLES; i++) {
		uint64_t diff = periodic_diff(periodic_data[i + 1], periodic_data[i]);

		if (diff != 0) {
			double mean_cyc_diff = (double)diff - mean_cyc;
			double mean_us_diff = cycles_to_us((double)diff) - mean_us;
			double mean_cyc_diff_sq = mean_cyc_diff * mean_cyc_diff;
			double mean_us_diff_sq = mean_us_diff * mean_us_diff;

			variance_us += mean_us_diff_sq;
			variance_cyc += mean_cyc_diff_sq;
		}
	}

	/* A measure of how wide the distribution is, ideal is 0 */
	variance_us = variance_us / (double)(CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers);
	variance_cyc = variance_cyc / (double)(CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers);

	/* A measure of timer precision, ideal is 0 */
	double stddev_us = sqrtf(variance_us);
	double stddev_cyc = sqrtf(variance_cyc);

	/* Use double precision math here as integer overflows are possible in doing all the
	 * conversions otherwise
	 */
	double expected_time_us =
		(double)CONFIG_TIMER_TEST_PERIOD * (double)CONFIG_TIMER_TEST_SAMPLES;
	double actual_time_us = cycles_to_us((double)periodic_end - (double)periodic_start);

	/* While this could be non-integer, the mean should be very close to it over time */
	double expected_period =
		((double)CONFIG_TIMER_TEST_PERIOD * (double)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) /
		1000000.0;
	/*
	 * Expected period drift(us) due to round up/down errors during the
	 * conversion between ticks, cycles and delay.
	 */
	k_timeout_t actual_timeout = K_USEC(CONFIG_TIMER_TEST_PERIOD);
	uint64_t cyc_per_tick = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
		/ CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	double expected_period_drift = ((double)actual_timeout.ticks * cyc_per_tick
		- expected_period) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * 1000000;
	double expected_time_drift_us = expected_period_drift
		* CONFIG_TIMER_TEST_SAMPLES;
	double time_diff_us = actual_time_us - expected_time_us
		- expected_time_drift_us;

	TC_PRINT("timer clock rate %d, kernel tick rate %d, expected period %d us, samples %d, "
		"period %f cycles, expected period drift %f us\n",
		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, CONFIG_SYS_CLOCK_TICKS_PER_SEC,
		CONFIG_TIMER_TEST_PERIOD, CONFIG_TIMER_TEST_SAMPLES, expected_period,
		expected_period_drift);
	TC_PRINT("statistics samples %d, rollovers %u, mean %f us, variance %f us, stddev %f us, "
		 "min %f us, max %f us\n",
		 CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers, periodic_rollovers, mean_us,
		 variance_us, stddev_us, min_us, max_us);
	TC_PRINT("statistics samples %d, rollovers %u, mean %f cycles, variance %f cycles, stddev "
		 "%f cycles, min %llu cycles, max %llu cycles\n",
		 CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers, periodic_rollovers, mean_cyc,
		 variance_cyc, stddev_cyc, min_cyc, max_cyc);
	TC_PRINT("timer start cycle %llu, end cycle %llu, total time %f, "
		"expected time %f, expected time drift %f, difference %f\n",
		periodic_start, periodic_end, actual_time_us, expected_time_us,
		expected_time_drift_us, time_diff_us);

	/* Validate the maximum/minimum timer period is off by no more than 10% */
	double test_period = (double)CONFIG_TIMER_TEST_PERIOD;
	double period_max_drift_percentage =
		(double)CONFIG_TIMER_TEST_PERIOD_MAX_DRIFT_PERCENT / 100;
	double min_us_bound = test_period - period_max_drift_percentage * test_period
		+ expected_period_drift;
	double max_us_bound = test_period + period_max_drift_percentage * test_period
		+ expected_period_drift;

	zassert_true(min_us >= min_us_bound,
		"Shortest timer period too short (off by more than expected %d%)",
		CONFIG_TIMER_TEST_PERIOD_MAX_DRIFT_PERCENT);
	zassert_true(max_us <= max_us_bound,
		"Longest timer period too long (off by more than expected %d%)",
		CONFIG_TIMER_TEST_PERIOD_MAX_DRIFT_PERCENT);

	/* Validate the timer deviation (precision/jitter of the timer) is within a configurable
	 * bound
	 */
	zassert_true(stddev_us < (double)CONFIG_TIMER_TEST_MAX_STDDEV,
		     "Standard deviation (in microseconds) outside expected bound");

	/* Validate the timer drift (accuracy over time) is within a configurable bound */
	zassert_true(abs(time_diff_us) < CONFIG_TIMER_TEST_MAX_DRIFT,
		     "Drift (in microseconds) outside expected bound");
}

ZTEST_SUITE(timer_behavior, NULL, NULL, NULL, NULL, NULL);
