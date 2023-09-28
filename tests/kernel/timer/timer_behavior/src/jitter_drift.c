/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_TIMER_EXTERNAL_TEST
#define TIMER_OUT_NODE DT_INST(0, test_kernel_timer_behavior_external)
static const struct gpio_dt_spec timer_out = GPIO_DT_SPEC_GET(TIMER_OUT_NODE,
		timerout_gpios);
#endif

static uint32_t periodic_idx;
static uint64_t periodic_data[CONFIG_TIMER_TEST_SAMPLES + 1];
static uint64_t periodic_start, periodic_end;
static struct k_timer periodic_timer;
static struct k_sem periodic_sem;

/*
 * The following code collects periodic time samples using the timer's
 * auto-restart feature based on its period argument.
 */

static void timer_period_fn(struct k_timer *t)
{
	uint64_t curr_cycle;

#ifdef CONFIG_TIMER_EXTERNAL_TEST
	gpio_pin_toggle_dt(&timer_out);
#endif

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	curr_cycle = k_cycle_get_64();
#else
	curr_cycle = k_cycle_get_32();
#endif
	periodic_data[periodic_idx] = curr_cycle;

	if (periodic_idx == 0) {
		periodic_start = curr_cycle;
	}
	++periodic_idx;
	if (periodic_idx >= ARRAY_SIZE(periodic_data)) {
		periodic_end = curr_cycle;
		k_timer_stop(t);
		k_sem_give(&periodic_sem);
	}
}

static void collect_timer_period_time_samples(void)
{
	k_timer_init(&periodic_timer, timer_period_fn, NULL);
	k_timer_start(&periodic_timer, K_NO_WAIT, K_USEC(CONFIG_TIMER_TEST_PERIOD));
}

/*
 * The following code collects periodic time samples by explicitly restarting
 * the timer and relying solely on the timer's start delay argument to
 * create periodicity.
 */

static void timer_startdelay_fn(struct k_timer *t)
{
	uint64_t curr_cycle;

#ifdef CONFIG_TIMER_EXTERNAL_TEST
	gpio_pin_toggle_dt(&timer_out);
#endif

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	curr_cycle = k_cycle_get_64();
#else
	curr_cycle = k_cycle_get_32();
#endif
	periodic_data[periodic_idx] = curr_cycle;

	if (periodic_idx == 0) {
		periodic_start = curr_cycle;
	}
	++periodic_idx;
	if (periodic_idx < ARRAY_SIZE(periodic_data)) {
		k_timer_start(t, K_USEC(CONFIG_TIMER_TEST_PERIOD), K_FOREVER);
	} else {
		periodic_end = curr_cycle;
		k_sem_give(&periodic_sem);
	}
}

static void collect_timer_startdelay_time_samples(void)
{
	k_timer_init(&periodic_timer, timer_startdelay_fn, NULL);
	k_timer_start(&periodic_timer, K_NO_WAIT, K_FOREVER);
}

/* Get a difference in cycles between one timer count and an earlier one
 * accounting for potentially wrapped values.
 *
 * @retval 0 an unhandled wrap of the timer occurred and the value should be ignored
 */
static uint64_t periodic_diff(uint64_t later, uint64_t earlier)
{
	/* Timer wrap around, will be ignored in statistics */
	if (later < earlier) {
		TC_PRINT("WARNING: Caught a timer wrap-around !\n");
		return 0;
	}

	return later - earlier;
}

static double cycles_to_us(uint64_t cycles)
{
	return 1000000.0 * cycles / sys_clock_hw_cycles_per_sec();
}

/**
 * @brief Test a timers jitter and drift over time
 */
static void do_test_using(void (*sample_collection_fn)(void))
{
	k_timeout_t actual_timeout = K_USEC(CONFIG_TIMER_TEST_PERIOD);
	uint64_t expected_duration = (uint64_t)actual_timeout.ticks * CONFIG_TIMER_TEST_SAMPLES;

	TC_PRINT("collecting time samples for approx %llu seconds\n",
		 k_ticks_to_ms_ceil64(expected_duration) / MSEC_PER_SEC);

	periodic_idx = 0;
	k_sem_init(&periodic_sem, 0, 1);

	/* Align to tick boundary. Otherwise the first handler execution
	 * might turn out to be significantly late and cause the test to
	 * fail. This can happen if k_timer_start() is called right before
	 * the upcoming tick boundary and in consequence the tick passes
	 * between the moment when the kernel decides what tick to use for
	 * the next timeout and the moment when the system timer actually
	 * sets up that timeout.
	 */
	k_sleep(K_TICKS(1));

	sample_collection_fn();
	k_sem_take(&periodic_sem, K_FOREVER);

	TC_PRINT("periodic timer samples gathered, calculating statistics\n");

	/* calculate variance, and precision */
	uint64_t total_cycles = 0;
	uint32_t periodic_rollovers = 0;

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

#ifndef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	/*
	 * Account for rollovers if any, and only when k_cycle_get_32()
	 * is used. This should not happen with k_cycle_get_64() and will
	 * be trapped later otherwise.
	 */
	periodic_end += (1ULL << 32) * periodic_rollovers;
#endif

	double min_us = cycles_to_us(min_cyc);
	double max_us = cycles_to_us(max_cyc);

	double mean_cyc =
		(double)total_cycles / (double)(CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers);
	double mean_us = cycles_to_us(total_cycles) /
			 (double)(CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers);
	double variance_us = 0;
	double variance_cyc = 0;

	for (int i = 0; i < CONFIG_TIMER_TEST_SAMPLES; i++) {
		uint64_t diff = periodic_diff(periodic_data[i + 1], periodic_data[i]);

		if (diff != 0) {
			double mean_cyc_diff = (double)diff - mean_cyc;
			double mean_us_diff = cycles_to_us(diff) - mean_us;
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
	double actual_time_us = cycles_to_us(periodic_end - periodic_start);

	/* While this could be non-integer, the mean should be very close to it over time */
	double expected_period =
		(double)CONFIG_TIMER_TEST_PERIOD * (double)sys_clock_hw_cycles_per_sec()
		/ 1000000.0;
	/*
	 * Expected period drift(us) due to round up/down errors during the
	 * conversion between ticks, cycles and delay.
	 */
	uint32_t cyc_per_tick = sys_clock_hw_cycles_per_sec()
				/ CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	double expected_period_drift = ((double)actual_timeout.ticks * cyc_per_tick
		- expected_period) / sys_clock_hw_cycles_per_sec() * 1000000;
	double expected_time_drift_us = expected_period_drift
		* CONFIG_TIMER_TEST_SAMPLES;
	double time_diff_us = actual_time_us - expected_time_us
		- expected_time_drift_us;

	TC_PRINT("timer clock rate %d, kernel tick rate %d\n",
		 sys_clock_hw_cycles_per_sec(), CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	if ((USEC_PER_SEC / CONFIG_TIMER_TEST_PERIOD) > CONFIG_SYS_CLOCK_TICKS_PER_SEC) {
		TC_PRINT("test timer period (%u us) is smaller than "
			 "system tick period (%u us)\n",
			 CONFIG_TIMER_TEST_PERIOD, k_ticks_to_us_near32(1));
		zassert_true(expected_period_drift != 0.0);
	}
	if (expected_period_drift != 0.0) {
		TC_PRINT("expected period drift: %.8g us\n", expected_period_drift);
	}
	TC_PRINT("period duration statistics for %d samples (%u rollovers):\n",
		 CONFIG_TIMER_TEST_SAMPLES - periodic_rollovers, periodic_rollovers);
	TC_PRINT("  expected: %d us,       \t%f cycles\n",
		 CONFIG_TIMER_TEST_PERIOD, expected_period);
	TC_PRINT("  min:      %f us, \t%llu cycles\n", min_us, min_cyc);
	TC_PRINT("  max:      %f us, \t%llu cycles\n", max_us, max_cyc);
	TC_PRINT("  mean:     %f us, \t%f cycles\n", mean_us, mean_cyc);
	TC_PRINT("  variance: %f us, \t%f cycles\n", variance_us, variance_cyc);
	TC_PRINT("  stddev:   %f us, \t%f cycles\n", stddev_us, stddev_cyc);
	TC_PRINT("timer start cycle %llu, end cycle %llu,\n"
		 "total time %f us, expected time %f us,\n"
		 "expected time drift %f us, difference %f us\n",
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

ZTEST(timer_jitter_drift, test_jitter_drift_timer_period)
{
	TC_PRINT("periodic timer behavior test using built-in restart mechanism\n");
#ifdef CONFIG_TIMER_EXTERNAL_TEST
	TC_PRINT("===== External Tool Sync Point =====\n");
	TC_PRINT("===== builtin =====\n");
	TC_PRINT("===== Waiting %d seconds =====\n",
		 CONFIG_TIMER_EXTERNAL_TEST_SYNC_DELAY);
	k_sleep(K_SECONDS(CONFIG_TIMER_EXTERNAL_TEST_SYNC_DELAY));
	gpio_pin_configure_dt(&timer_out, GPIO_OUTPUT_LOW);
#endif
	do_test_using(collect_timer_period_time_samples);
}

ZTEST(timer_jitter_drift, test_jitter_drift_timer_startdelay)
{
	TC_PRINT("periodic timer behavior test using explicit start with delay\n");
#ifdef CONFIG_TIMER_EXTERNAL_TEST
	TC_PRINT("===== External Tool Sync Point =====\n");
	TC_PRINT("===== startdelay =====\n");
	TC_PRINT("===== Waiting %d seconds =====\n",
		 CONFIG_TIMER_EXTERNAL_TEST_SYNC_DELAY);
	k_sleep(K_SECONDS(CONFIG_TIMER_EXTERNAL_TEST_SYNC_DELAY));
	gpio_pin_configure_dt(&timer_out, GPIO_OUTPUT_LOW);
#endif
	do_test_using(collect_timer_startdelay_time_samples);
}

ZTEST_SUITE(timer_jitter_drift, NULL, NULL, NULL, NULL, NULL);
