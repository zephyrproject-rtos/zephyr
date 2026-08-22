/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/precision_timing/precision_timing.h>
#include <zephyr/ztest.h>

#include "gptp_clock.h"

struct fake_clock_data {
	int rate_adjust_calls;
	double last_ratio;
	int rate_adjust_ret;
	uint32_t caps_flags;
};

static int fake_clock_rate_adjust(const struct device *dev, double ratio)
{
	struct fake_clock_data *data = dev->data;

	data->rate_adjust_calls++;
	data->last_ratio = ratio;

	return data->rate_adjust_ret;
}

static int fake_clock_get_caps(const struct device *dev, struct ptp_clock_caps *caps)
{
	struct fake_clock_data *data = dev->data;

	*caps = (struct ptp_clock_caps){
		.flags = data->caps_flags,
		.resolution_ns = 1,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
	};

	return 0;
}

static DEVICE_API(ptp_clock, fake_clock_api) = {
	.rate_adjust = fake_clock_rate_adjust,
	.get_caps = fake_clock_get_caps,
};

static struct fake_clock_data fake_clock_a_data;
static struct fake_clock_data fake_clock_b_data;

DEVICE_DEFINE(fake_clock_a, "fake_clock_a", NULL, NULL, &fake_clock_a_data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_clock_api);
DEVICE_DEFINE(fake_clock_b, "fake_clock_b", NULL, NULL, &fake_clock_b_data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_clock_api);

struct gptp_clock_data gptp_clock;

static const uint8_t gm_a[GPTP_CLOCK_ID_LEN] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
static const uint8_t gm_b[GPTP_CLOCK_ID_LEN] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};

static void reset_test_state(void)
{
	struct precision_pi_config config = {
		.source_domain = {.type = PRECISION_TIME_DOMAIN_GPTP, .id = 0},
		.local_domain = {.type = PRECISION_TIME_DOMAIN_PHC, .id = 0},
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};

	memset(&gptp_clock, 0, sizeof(gptp_clock));
	memset(&fake_clock_a_data, 0, sizeof(fake_clock_a_data));
	memset(&fake_clock_b_data, 0, sizeof(fake_clock_b_data));
	fake_clock_a_data.caps_flags = PTP_CLOCK_CAP_RATE_ADJUST;
	fake_clock_b_data.caps_flags = PTP_CLOCK_CAP_RATE_ADJUST;
	zassert_ok(precision_pi_init(&gptp_clock.discipline, &config));
	precision_time_mapping_init(&gptp_clock.mapping, config.source_domain, config.local_domain);
}

static void clock_discipline_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_test_state();
}

ZTEST(gptp_clock_discipline, test_source_activation_tracks_grandmaster_and_phc)
{
	const struct device *clock_a = DEVICE_GET(fake_clock_a);
	const struct device *clock_b = DEVICE_GET(fake_clock_b);

	zassert_ok(gptp_clock_source_activate(clock_a, 1, gm_a));
	zassert_true(gptp_clock_source_matches(clock_a, 1, gm_a));
	zassert_equal(gptp_clock.discipline.config.local_domain.id, 1);
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 0);

	gptp_clock.discipline.state = PRECISION_SYNC_LOCKED;
	gptp_clock.discipline.drift_ppb = 123;
	gptp_clock.mapping.valid = true;
	zassert_ok(gptp_clock_source_activate(clock_a, 1, gm_a));
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_LOCKED);
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 0);

	zassert_ok(gptp_clock_source_activate(clock_a, 1, gm_b));
	zassert_true(gptp_clock_source_matches(clock_a, 1, gm_b));
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_UNSYNCED);
	zassert_false(gptp_clock.mapping.valid);
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 1);
	zassert_equal(fake_clock_a_data.last_ratio, 1.0);

	zassert_ok(gptp_clock_source_activate(clock_b, 2, gm_b));
	zassert_true(gptp_clock_source_matches(clock_b, 2, gm_b));
	zassert_equal(gptp_clock.discipline.config.local_domain.id, 2);
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 2);
	zassert_equal(fake_clock_b_data.rate_adjust_calls, 0);

	zassert_ok(gptp_clock_source_reset());
	zassert_false(gptp_clock.active_source_valid);
	zassert_is_null(gptp_clock.active_clock);
	zassert_equal(gptp_clock.discipline.config.local_domain.id, 0);
	zassert_equal(fake_clock_b_data.rate_adjust_calls, 1);
}

ZTEST(gptp_clock_discipline, test_failed_source_switch_faults_old_clock)
{
	const struct device *clock_a = DEVICE_GET(fake_clock_a);
	const struct device *clock_b = DEVICE_GET(fake_clock_b);

	zassert_ok(gptp_clock_source_activate(clock_a, 1, gm_a));
	fake_clock_a_data.rate_adjust_ret = -EIO;

	zassert_equal(gptp_clock_source_activate(clock_b, 2, gm_b), -EIO);
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_FAULT);
	zassert_true(gptp_clock_source_matches(clock_a, 1, gm_a));
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 1);
	zassert_false(gptp_clock_source_matches(clock_b, 2, gm_b));

	zassert_ok(gptp_clock_source_activate(clock_a, 1, gm_a));
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_FAULT,
		      "same source must not clear a clock fault");

	fake_clock_a_data.rate_adjust_ret = 0;
	zassert_ok(gptp_clock_source_activate(clock_b, 2, gm_b));
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_UNSYNCED);
	zassert_true(gptp_clock_source_matches(clock_b, 2, gm_b));
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 2);
}

ZTEST(gptp_clock_discipline, test_unsupported_rate_adjust_preserves_discipline)
{
	struct precision_clock_ptp_adapter adapter;
	const struct precision_clock *precision_clk;
	const struct device *clk_dev = DEVICE_GET(fake_clock_a);

	fake_clock_a_data.caps_flags = 0U;
	zassert_ok(precision_clock_ptp_init(&adapter, clk_dev,
					    gptp_clock.discipline.config.local_domain));
	precision_clk = precision_clock_ptp_get(&adapter);
	gptp_clock.discipline.state = PRECISION_SYNC_LOCKED;
	gptp_clock.mapping.valid = true;

	zassert_ok(gptp_clock_adjust_rate(precision_clk, 123));
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_LOCKED);
	zassert_true(gptp_clock.mapping.valid);
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 0);

	zassert_ok(gptp_clock_servo_reset(precision_clk));
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_UNSYNCED);
	zassert_false(gptp_clock.mapping.valid);
	zassert_equal(fake_clock_a_data.rate_adjust_calls, 0);
}

ZTEST(gptp_clock_discipline, test_unsupported_set_preserves_discipline)
{
	struct precision_clock_ptp_adapter adapter;
	const struct precision_clock *precision_clk;
	const struct device *clk_dev = DEVICE_GET(fake_clock_a);
	struct precision_time_point target = {
		.time = NSEC_PER_SEC,
		.domain = gptp_clock.discipline.config.local_domain,
	};

	fake_clock_a_data.caps_flags = PTP_CLOCK_CAP_READ;
	zassert_ok(precision_clock_ptp_init(&adapter, clk_dev, target.domain));
	precision_clk = precision_clock_ptp_get(&adapter);
	gptp_clock.discipline.state = PRECISION_SYNC_LOCKED;
	gptp_clock.mapping.valid = true;

	zassert_ok(gptp_clock_set(precision_clk, &target));
	zassert_equal(gptp_clock.discipline.state, PRECISION_SYNC_LOCKED);
	zassert_true(gptp_clock.mapping.valid);
}

ZTEST(gptp_clock_discipline, test_source_timeout_uses_received_interval)
{
	gptp_clock_source_timeout_update(123456789U);
	zassert_equal(gptp_clock.discipline.config.source_timeout_ns, 123456789);
	zassert_equal(gptp_clock.discipline.config.holdover_ns, 123456789);

	gptp_clock_source_timeout_update(UINT64_MAX);
	zassert_equal(gptp_clock.discipline.config.source_timeout_ns, PRECISION_TIME_MAX);
	zassert_equal(gptp_clock.discipline.config.holdover_ns, PRECISION_TIME_MAX);
}

ZTEST(gptp_clock_discipline, test_source_timeout_check_waits_for_deadline)
{
	gptp_clock.discipline.has_last_update = true;
	gptp_clock.discipline.last_update_ns = 1000;
	gptp_clock_source_timeout_update(100 * NSEC_PER_MSEC);

	zassert_true(gptp_clock.source_timeout.scheduled);
	zassert_false(gptp_clock_source_timeout_due());

	gptp_clock.source_timeout.expiry_ms = k_uptime_get();
	zassert_true(gptp_clock_source_timeout_due());
	zassert_false(gptp_clock.source_timeout.scheduled);
	zassert_false(gptp_clock_source_timeout_due());

	gptp_clock_source_timeout_reschedule(1000 + 100 * NSEC_PER_MSEC);
	zassert_true(gptp_clock.source_timeout.scheduled);
}

ZTEST_SUITE(gptp_clock_discipline, NULL, NULL, clock_discipline_before, NULL, NULL);
