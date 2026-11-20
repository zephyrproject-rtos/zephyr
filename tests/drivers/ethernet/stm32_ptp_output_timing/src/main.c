/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "eth_stm32_hal_ptp_output.h"

#define TEST_PERIOD_NS       NSEC_PER_SEC
#define TEST_WIDTH_NS        (200ULL * NSEC_PER_MSEC)
#define TEST_MAX_WIDTH_NS    (500ULL * NSEC_PER_MSEC)
#define TEST_GUARD_NS        NSEC_PER_MSEC
#define TEST_TICK_NS         (NSEC_PER_SEC / 10000U)
#define TEST_MAX_DELAY_NS    NSEC_PER_SEC
#define TEST_MIN_LEAD_NS     NSEC_PER_MSEC
#define TEST_BUSY_MARGIN_NS  NSEC_PER_MSEC
#define TEST_MIN_RATE_PCT    90U
#define TEST_NOMINAL_RATE    100U
#define TEST_MAX_RATE_PCT    110U
#define TEST_FIRST_TARGET_NS (10ULL * NSEC_PER_SEC)

static uint64_t test_phc_time(uint64_t monotonic_ns, uint32_t rate_pct)
{
	return monotonic_ns * rate_pct / 100U;
}

static void wait_until_rearm_ready(struct eth_stm32_ptp_output_rearm_timing *timing,
				   uint32_t min_rate_pct, uint32_t max_rate_pct,
				   uint32_t actual_rate_pct, uint64_t *monotonic_ns,
				   uint64_t *phc_ns)
{
	for (uint32_t attempt = 0U; attempt < 100U; attempt++) {
		uint64_t delay_ns;

		zassert_ok(eth_stm32_ptp_output_rearm_observe(timing, *phc_ns, *monotonic_ns,
							      min_rate_pct, TEST_TICK_NS));
		if (eth_stm32_ptp_output_rearm_ready(timing, *phc_ns, *monotonic_ns)) {
			return;
		}

		zassert_ok(eth_stm32_ptp_output_rearm_delay(timing, *phc_ns, *monotonic_ns,
							    max_rate_pct, TEST_TICK_NS,
							    TEST_MAX_DELAY_NS, &delay_ns));
		zassert_true(delay_ns > 0U);
		*monotonic_ns += delay_ns;
		*phc_ns = test_phc_time(*monotonic_ns, actual_rate_pct);
	}

	zassert_unreachable("rearm timing did not converge");
}

static void verify_three_pulses(uint32_t min_rate_pct, uint32_t max_rate_pct,
				uint32_t actual_rate_pct, uint64_t width_ns)
{
	struct eth_stm32_ptp_output_rearm_timing timing;
	uint64_t monotonic_ns = 0U;
	uint64_t phc_ns = 0U;

	zassert_ok(eth_stm32_ptp_output_rearm_timing_init(&timing, TEST_FIRST_TARGET_NS,
							  width_ns + TEST_GUARD_NS));
	zassert_ok(eth_stm32_ptp_output_rearm_observe(&timing, 0U, 0U, min_rate_pct, TEST_TICK_NS));
	zassert_false(timing.monotonic_guard_active);

	for (uint32_t pulse = 1U; pulse < 3U; pulse++) {
		uint64_t next_target_ns;

		wait_until_rearm_ready(&timing, min_rate_pct, max_rate_pct, actual_rate_pct,
				       &monotonic_ns, &phc_ns);
		next_target_ns = timing.target_ns + TEST_PERIOD_NS;
		zassert_true(next_target_ns >= phc_ns + TEST_MIN_LEAD_NS);
		zassert_ok(eth_stm32_ptp_output_rearm_timing_init(&timing, next_target_ns,
								  width_ns + TEST_GUARD_NS));
	}
}

ZTEST(stm32_ptp_output_timing, test_far_future_waveform_rearms_second_and_third_pulses)
{
	verify_three_pulses(TEST_MIN_RATE_PCT, TEST_MAX_RATE_PCT, TEST_MIN_RATE_PCT, TEST_WIDTH_NS);
	verify_three_pulses(TEST_MIN_RATE_PCT, TEST_MAX_RATE_PCT, TEST_NOMINAL_RATE, TEST_WIDTH_NS);
	verify_three_pulses(TEST_MIN_RATE_PCT, TEST_MAX_RATE_PCT, TEST_MAX_RATE_PCT, TEST_WIDTH_NS);
	verify_three_pulses(TEST_MIN_RATE_PCT, TEST_MAX_RATE_PCT, TEST_MIN_RATE_PCT,
			    TEST_MAX_WIDTH_NS);
	verify_three_pulses(TEST_MIN_RATE_PCT, TEST_MAX_RATE_PCT, TEST_NOMINAL_RATE,
			    TEST_MAX_WIDTH_NS);
	verify_three_pulses(TEST_MIN_RATE_PCT, TEST_MAX_RATE_PCT, TEST_MAX_RATE_PCT,
			    TEST_MAX_WIDTH_NS);
}

static uint64_t max_width_for_rates(uint32_t min_rate_pct, uint32_t max_rate_pct)
{
	const struct eth_stm32_ptp_output_rearm_limits limits = {
		.period_ns = TEST_PERIOD_NS,
		.min_lead_time_ns = TEST_MIN_LEAD_NS,
		.falling_edge_guard_ns = TEST_GUARD_NS,
		.scheduling_margin_ns = TEST_BUSY_MARGIN_NS + 5U * TEST_TICK_NS,
		.hardware_max_width_ns = TEST_MAX_WIDTH_NS,
		.resolution_ns = 20U,
		.min_rate_pct = min_rate_pct,
		.max_rate_pct = max_rate_pct,
	};
	uint64_t max_width_ns = 0U;

	zassert_ok(eth_stm32_ptp_output_max_pulse_width(&limits, &max_width_ns));

	return max_width_ns;
}

ZTEST(stm32_ptp_output_timing, test_rate_envelope_limits_maximum_pulse_width)
{
	uint64_t available_real_ns;
	uint64_t guard_real_ns;
	uint64_t max_width_ns;

	max_width_ns = max_width_for_rates(50U, 110U);
	zassert_equal(max_width_ns, 452340900U);
	zassert_ok(eth_stm32_ptp_output_real_duration_floor(TEST_PERIOD_NS - TEST_MIN_LEAD_NS, 110U,
							    &available_real_ns));
	zassert_ok(eth_stm32_ptp_output_real_duration_ceil(max_width_ns + TEST_GUARD_NS, 50U,
							   &guard_real_ns));
	zassert_true(5U * TEST_TICK_NS + TEST_BUSY_MARGIN_NS + guard_real_ns <= available_real_ns);
	zassert_ok(eth_stm32_ptp_output_real_duration_ceil(max_width_ns + 20U + TEST_GUARD_NS, 50U,
							   &guard_real_ns));
	zassert_true(5U * TEST_TICK_NS + TEST_BUSY_MARGIN_NS + guard_real_ns > available_real_ns);
	verify_three_pulses(50U, 110U, 50U, max_width_ns);
	verify_three_pulses(50U, 110U, TEST_NOMINAL_RATE, max_width_ns);
	verify_three_pulses(50U, 110U, 110U, max_width_ns);

	max_width_ns = max_width_for_rates(90U, 110U);
	zassert_equal(max_width_ns, TEST_MAX_WIDTH_NS);

	max_width_ns = max_width_for_rates(100U, 100U);
	zassert_equal(max_width_ns, TEST_MAX_WIDTH_NS);
}

ZTEST(stm32_ptp_output_timing, test_monotonic_guard_survives_phc_steps)
{
	struct eth_stm32_ptp_output_rearm_timing timing;
	uint64_t deadline_ns;

	zassert_ok(eth_stm32_ptp_output_rearm_timing_init(&timing, TEST_FIRST_TARGET_NS,
							  TEST_WIDTH_NS + TEST_GUARD_NS));
	zassert_ok(eth_stm32_ptp_output_rearm_guard_start(&timing, TEST_FIRST_TARGET_NS,
							  TEST_MIN_RATE_PCT, TEST_TICK_NS));
	deadline_ns = timing.safe_after_monotonic_ns;

	zassert_false(eth_stm32_ptp_output_rearm_ready(
		&timing, timing.safe_after_ns + TEST_PERIOD_NS, deadline_ns - 1U));
	zassert_false(
		eth_stm32_ptp_output_rearm_ready(&timing, timing.safe_after_ns - 1U, deadline_ns));
	zassert_true(eth_stm32_ptp_output_rearm_ready(&timing, timing.safe_after_ns, deadline_ns));
}

ZTEST(stm32_ptp_output_timing, test_rate_conversion_and_boundaries)
{
	struct eth_stm32_ptp_output_rearm_timing timing;
	uint64_t duration_ns;

	zassert_ok(eth_stm32_ptp_output_real_duration_ceil(500ULL * NSEC_PER_MSEC,
							   TEST_MIN_RATE_PCT, &duration_ns));
	zassert_equal(duration_ns, 555555556U);
	zassert_ok(eth_stm32_ptp_output_real_duration_floor(TEST_FIRST_TARGET_NS, TEST_MAX_RATE_PCT,
							    &duration_ns));
	zassert_equal(duration_ns, 9090909090ULL);
	zassert_equal(eth_stm32_ptp_output_real_duration_ceil(1U, 0U, &duration_ns), -EINVAL);
	zassert_equal(eth_stm32_ptp_output_rearm_timing_init(&timing, UINT64_MAX, 1U), -ERANGE);
}

ZTEST_SUITE(stm32_ptp_output_timing, NULL, NULL, NULL, NULL, NULL);
