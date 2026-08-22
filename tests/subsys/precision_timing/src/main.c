/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/precision_timing/precision_timing.h>
#include <zephyr/ztest.h>

static const struct precision_time_domain source_domain = {
	.type = PRECISION_TIME_DOMAIN_PTP,
	.id = 1,
};

static const struct precision_time_domain local_domain = {
	.type = PRECISION_TIME_DOMAIN_PHC,
	.id = 2,
};

static int legacy_ptp_clock_set(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tm);

	return 0;
}

static int legacy_ptp_clock_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	tm->second = 1;
	tm->nanosecond = 2;

	return 0;
}

static int legacy_ptp_clock_adjust(const struct device *dev, int increment)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(increment);

	return 0;
}

static int legacy_ptp_clock_rate_adjust(const struct device *dev, double ratio)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ratio);

	return 0;
}

static DEVICE_API(ptp_clock, legacy_ptp_clock_api) = {
	.set = legacy_ptp_clock_set,
	.get = legacy_ptp_clock_get,
	.adjust = legacy_ptp_clock_adjust,
	.rate_adjust = legacy_ptp_clock_rate_adjust,
};

DEVICE_DEFINE(legacy_ptp_clock, "legacy_ptp_clock", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &legacy_ptp_clock_api);

static DEVICE_API(ptp_clock, readonly_ptp_clock_api) = {
	.get = legacy_ptp_clock_get,
};

DEVICE_DEFINE(readonly_ptp_clock, "readonly_ptp_clock", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &readonly_ptp_clock_api);

static int queried_ptp_clock_caps_error;

static int queried_ptp_clock_get_caps(const struct device *dev, struct ptp_clock_caps *caps)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(caps);

	return queried_ptp_clock_caps_error;
}

static DEVICE_API(ptp_clock, queried_ptp_clock_api) = {
	.set = legacy_ptp_clock_set,
	.get = legacy_ptp_clock_get,
	.adjust = legacy_ptp_clock_adjust,
	.rate_adjust = legacy_ptp_clock_rate_adjust,
	.get_caps = queried_ptp_clock_get_caps,
};

DEVICE_DEFINE(queried_ptp_clock, "queried_ptp_clock", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &queried_ptp_clock_api);

static struct precision_time_observation observation(precision_time_t source,
						     precision_time_t local)
{
	return (struct precision_time_observation){
		.source = {.time = source, .domain = source_domain},
		.local = {.time = local, .domain = local_domain},
		.flags = PRECISION_OBSERVATION_SOURCE_VALID | PRECISION_OBSERVATION_LOCAL_VALID,
	};
}

static struct precision_time_mapping build_offset_mapping(void)
{
	struct precision_time_mapping mapping;
	struct precision_time_observation obs1 = observation(0, 100);
	struct precision_time_observation obs2 = observation(1000, 1100);

	precision_time_mapping_init(&mapping, source_domain, local_domain);
	(void)precision_time_mapping_update(&mapping, &obs1);
	(void)precision_time_mapping_update(&mapping, &obs2);

	return mapping;
}

ZTEST(precision_timing, test_scalar_conversions_check_bounds)
{
	precision_time_t ns;
	uint64_t sec;
	uint32_t nsec;

	zassert_ok(precision_time_from_u64_sec_nsec(12, 34, &ns));
	zassert_equal(ns, 12LL * NSEC_PER_SEC + 34);
	zassert_ok(precision_time_to_u64_sec_nsec(ns, &sec, &nsec));
	zassert_equal(sec, 12);
	zassert_equal(nsec, 34);
	zassert_equal(precision_time_from_u64_sec_nsec(UINT64_MAX, 0, &ns), -ERANGE);
	zassert_equal(precision_time_add(PRECISION_TIME_MAX, 1, &ns), -ERANGE);
	zassert_equal(precision_time_sub(PRECISION_TIME_MIN, 1, &ns), -ERANGE);
	zassert_ok(precision_time_sub(PRECISION_TIME_MIN, PRECISION_TIME_MIN, &ns));
	zassert_equal(ns, 0);
	zassert_ok(precision_time_sub(-1, PRECISION_TIME_MIN, &ns));
	zassert_equal(ns, PRECISION_TIME_MAX);
	zassert_equal(precision_time_sub(0, PRECISION_TIME_MIN, &ns), -ERANGE);
	zassert_equal(precision_time_sub(0, 0, NULL), -EINVAL);
	zassert_equal(precision_time_from_u64_sec_nsec(0, 0, NULL), -EINVAL);
	zassert_equal(precision_time_to_u64_sec_nsec(0, NULL, &nsec), -EINVAL);
}

ZTEST(precision_timing, test_ptp_adapter_derives_legacy_capabilities)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_caps caps;
	struct precision_time_point tp = {
		.time = 3,
		.domain = local_domain,
	};
	const struct precision_clock *precision_clk;

	zassert_ok(precision_clock_ptp_init(&adapter, DEVICE_GET(legacy_ptp_clock), local_domain));
	precision_clk = precision_clock_ptp_get(&adapter);

	zassert_ok(precision_clock_get_caps(precision_clk, &caps));
	zassert_equal(caps.flags, PRECISION_CLOCK_CAP_READ | PRECISION_CLOCK_CAP_SET |
					  PRECISION_CLOCK_CAP_ADJUST_PHASE |
					  PRECISION_CLOCK_CAP_ADJUST_RATE);
	zassert_equal(caps.max_phase_adjust_ns, INT_MAX);
	zassert_equal(caps.min_rate_ppb, -999999999);
	zassert_equal(caps.max_rate_ppb, INT32_MAX);

	zassert_ok(precision_clock_read(precision_clk, &tp));
	zassert_equal(tp.time, NSEC_PER_SEC + 2);
	zassert_ok(precision_clock_set(precision_clk, &tp));
	zassert_ok(precision_clock_adjust_phase(precision_clk, INT_MAX));
	zassert_equal(precision_clock_adjust_phase(precision_clk, (precision_time_t)INT_MAX + 1),
		      -ERANGE);
	zassert_ok(precision_clock_adjust_rate(precision_clk, 0));
	zassert_equal(precision_clock_adjust_rate(precision_clk, -1000000000), -ERANGE);
}

ZTEST(precision_timing, test_ptp_adapter_propagates_capability_query_failures)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_caps caps;
	const struct precision_clock *precision_clk;

	queried_ptp_clock_caps_error = -ENOTSUP;
	zassert_ok(precision_clock_ptp_init(&adapter, DEVICE_GET(queried_ptp_clock), local_domain));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_ok(precision_clock_get_caps(precision_clk, &caps));
	zassert_equal(caps.flags, PRECISION_CLOCK_CAP_READ | PRECISION_CLOCK_CAP_SET |
					  PRECISION_CLOCK_CAP_ADJUST_PHASE |
					  PRECISION_CLOCK_CAP_ADJUST_RATE);
	zassert_equal(caps.max_phase_adjust_ns, INT_MAX);

	queried_ptp_clock_caps_error = -EIO;
	zassert_equal(
		precision_clock_ptp_init(&adapter, DEVICE_GET(queried_ptp_clock), local_domain),
		-EIO);

	queried_ptp_clock_caps_error = -ERANGE;
	zassert_equal(
		precision_clock_ptp_init(&adapter, DEVICE_GET(queried_ptp_clock), local_domain),
		-ERANGE);
}

ZTEST(precision_timing, test_ptp_adapter_rejects_invalid_arguments)
{
	struct precision_clock_ptp_adapter adapter;

	zassert_equal(precision_clock_ptp_init(NULL, DEVICE_GET(legacy_ptp_clock), local_domain),
		      -EINVAL);
	zassert_equal(precision_clock_ptp_init(&adapter, NULL, local_domain), -EINVAL);
}

ZTEST(precision_timing, test_ptp_adapter_reports_missing_legacy_operations)
{
	struct precision_clock_ptp_adapter adapter;
	struct precision_clock_caps caps;
	struct precision_time_point tp = {
		.time = 3,
		.domain = local_domain,
	};
	const struct precision_clock *precision_clk;

	zassert_ok(
		precision_clock_ptp_init(&adapter, DEVICE_GET(readonly_ptp_clock), local_domain));
	precision_clk = precision_clock_ptp_get(&adapter);

	zassert_ok(precision_clock_get_caps(precision_clk, &caps));
	zassert_equal(caps.flags, PRECISION_CLOCK_CAP_READ);
	zassert_equal(caps.min_rate_ppb, 0);
	zassert_equal(caps.max_rate_ppb, 0);

	zassert_ok(precision_clock_read(precision_clk, &tp));
	zassert_equal(precision_clock_set(precision_clk, &tp), -ENOTSUP);
	zassert_equal(precision_clock_adjust_phase(precision_clk, 0), -ENOTSUP);
	zassert_equal(precision_clock_adjust_rate(precision_clk, 0), -ENOTSUP);
}

ZTEST(precision_timing, test_domain_mapping_converts_and_invalidates)
{
	struct precision_time_mapping mapping;
	struct precision_time_observation obs = observation(-100, 50);
	struct precision_time_point source = {
		.time = -90,
		.domain = source_domain,
	};
	struct precision_time_point local;

	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_equal(precision_time_mapping_source_to_local(&mapping, &source, &local), -EAGAIN);

	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_ok(precision_time_mapping_source_to_local(&mapping, &source, &local));
	zassert_equal(local.time, 60);
	zassert_true(precision_time_domain_equal(&local.domain, &local_domain));

	precision_time_mapping_invalidate(&mapping);
	zassert_equal(precision_time_mapping_source_to_local(&mapping, &source, &local), -EAGAIN);
}

ZTEST(precision_timing, test_domain_mapping_inverts_local_to_source)
{
	struct precision_time_mapping mapping;
	struct precision_time_observation obs = observation(-100, 50);
	struct precision_time_point local = {
		.time = 60,
		.domain = local_domain,
	};
	struct precision_time_point source;

	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_equal(precision_time_mapping_local_to_source(&mapping, &local, &source), -EAGAIN);

	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_ok(precision_time_mapping_local_to_source(&mapping, &local, &source));
	zassert_equal(source.time, -90);
	zassert_true(precision_time_domain_equal(&source.domain, &source_domain));

	local.domain = source_domain;
	zassert_equal(precision_time_mapping_local_to_source(&mapping, &local, &source), -EINVAL);
}

ZTEST(precision_timing, test_domain_mapping_handles_long_nanosecond_deltas)
{
	struct precision_time_mapping mapping;
	struct precision_time_observation obs = observation(0, 100);
	struct precision_time_point source = {
		.time = 10LL * NSEC_PER_SEC,
		.domain = source_domain,
	};
	struct precision_time_point local;
	struct precision_time_point round_trip;

	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_ok(precision_time_mapping_source_to_local(&mapping, &source, &local));
	zassert_equal(local.time, 10LL * NSEC_PER_SEC + 100);

	zassert_ok(precision_time_mapping_local_to_source(&mapping, &local, &round_trip));
	zassert_equal(round_trip.time, source.time);
}

ZTEST(precision_timing, test_domain_mapping_checks_extrapolation_range)
{
	struct precision_time_mapping mapping;
	struct precision_time_observation obs = observation(1, PRECISION_TIME_MAX);
	struct precision_time_point source = {
		.time = 1,
		.domain = source_domain,
	};
	struct precision_time_point local;

	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_ok(precision_time_mapping_source_to_local(&mapping, &source, &local));
	zassert_equal(local.time, PRECISION_TIME_MAX);

	source.time = 2;
	zassert_equal(precision_time_mapping_source_to_local(&mapping, &source, &local), -ERANGE);
	zassert_equal(local.time, PRECISION_TIME_MAX);

	obs = observation(4, PRECISION_TIME_MIN + 3);
	source.time = 1;
	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_ok(precision_time_mapping_source_to_local(&mapping, &source, &local));
	zassert_equal(local.time, PRECISION_TIME_MIN);

	source.time = 0;
	zassert_equal(precision_time_mapping_source_to_local(&mapping, &source, &local), -ERANGE);
	zassert_equal(local.time, PRECISION_TIME_MIN);
}

ZTEST(precision_timing, test_domain_mapping_checks_inverse_extrapolation_range)
{
	struct precision_time_mapping mapping;
	struct precision_time_observation obs = observation(PRECISION_TIME_MAX - 10, 1);
	struct precision_time_point local = {
		.time = PRECISION_TIME_MAX,
		.domain = local_domain,
	};
	struct precision_time_point source = {
		.time = 123,
	};

	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_equal(precision_time_mapping_local_to_source(&mapping, &local, &source), -ERANGE);
	zassert_equal(source.time, 123);

#ifdef CONFIG_TIMEUTIL_APPLY_SKEW
	obs = observation(1, 1);
	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_ok(timeutil_sync_state_set_skew(&mapping.state, 0.5f, NULL));
	zassert_ok(precision_time_mapping_local_to_source(&mapping, &local, &source));
	zassert_equal(source.time, (PRECISION_TIME_MAX / 2) + 2);
#endif /* CONFIG_TIMEUTIL_APPLY_SKEW */
}

ZTEST(precision_timing, test_domain_mapping_rejects_unrepresentable_bias)
{
	struct precision_time_mapping mapping;
	struct precision_time_observation obs = observation(PRECISION_TIME_MIN + 1, 0);
	struct precision_time_point source = {
		.time = PRECISION_TIME_MIN + 3,
		.domain = source_domain,
	};
	struct precision_time_point local;

	precision_time_mapping_init(&mapping, source_domain, local_domain);
	zassert_equal(precision_time_mapping_update(&mapping, &obs), -ERANGE);
	zassert_false(mapping.valid);

	obs.source.time = PRECISION_TIME_MIN + 2;
	zassert_ok(precision_time_mapping_update(&mapping, &obs));
	zassert_ok(precision_time_mapping_source_to_local(&mapping, &source, &local));
	zassert_equal(local.time, 1);
}

ZTEST(precision_timing, test_invalid_domains_never_match)
{
	struct precision_time_mapping mapping = {0};
	struct precision_pi_discipline discipline = {0};
	struct precision_pi_config config = {
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.gain_den = 1,
	};
	struct precision_time_observation obs = {
		.flags = PRECISION_OBSERVATION_SOURCE_VALID | PRECISION_OBSERVATION_LOCAL_VALID,
	};
	struct precision_time_point point = {0};

	zassert_equal(precision_time_mapping_update(&mapping, &obs), -EINVAL);
	zassert_false(mapping.valid);
	zassert_equal(precision_time_mapping_source_to_local(&mapping, &point, &point), -EINVAL);
	zassert_equal(precision_pi_process(&discipline, &obs, NULL), -EINVAL);
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
}

ZTEST(precision_timing, test_pi_adjusts_rate_and_locks)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.lock_sample_count = 2,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs;

	zassert_ok(precision_pi_init(&discipline, &config));

	obs = observation(900, 1000);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.action, PRECISION_DISCIPLINE_ADJUST_RATE);
	zassert_equal(result.offset_ns, -100);
	zassert_equal(result.rate_ppb, -100);
	zassert_equal(result.state, PRECISION_SYNC_ACQUIRING);

	obs = observation(2000, 2000);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.state, PRECISION_SYNC_ACQUIRING);

	obs = observation(3000, 3000);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.state, PRECISION_SYNC_LOCKED);
}

ZTEST(precision_timing, test_pi_does_not_wind_up_at_rate_limits)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_sample_count = 0,
		.min_rate_ppb = -25000000,
		.max_rate_ppb = 25000000,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs;

	zassert_ok(precision_pi_init(&discipline, &config));

	obs = observation(1500LL * NSEC_PER_MSEC, NSEC_PER_SEC);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.rate_ppb, config.max_rate_ppb);
	zassert_equal(discipline.drift_ppb, 0);

	obs = observation(2500LL * NSEC_PER_MSEC, 2LL * NSEC_PER_SEC);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.rate_ppb, config.max_rate_ppb);
	zassert_equal(discipline.drift_ppb, 0);

	obs = observation(2990LL * NSEC_PER_MSEC, 3LL * NSEC_PER_SEC);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.rate_ppb, -10000000);
	zassert_equal(discipline.drift_ppb, -3000000);

	precision_pi_reset(&discipline);

	obs = observation(500LL * NSEC_PER_MSEC, NSEC_PER_SEC);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.rate_ppb, config.min_rate_ppb);
	zassert_equal(discipline.drift_ppb, 0);

	obs = observation(1500LL * NSEC_PER_MSEC, 2LL * NSEC_PER_SEC);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.rate_ppb, config.min_rate_ppb);
	zassert_equal(discipline.drift_ppb, 0);

	obs = observation(3010LL * NSEC_PER_MSEC, 3LL * NSEC_PER_SEC);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.rate_ppb, 10000000);
	zassert_equal(discipline.drift_ppb, 3000000);
}

ZTEST(precision_timing, test_pi_steps_and_rejects_locked_outliers)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.outlier_threshold_ns = 100,
		.lock_sample_count = 1,
		.outlier_sample_count = 2,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs;

	zassert_ok(precision_pi_init(&discipline, &config));

	obs = observation(2 * NSEC_PER_SEC, 0);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.action, PRECISION_DISCIPLINE_STEP);
	zassert_equal(result.phase_correction_ns, 2 * NSEC_PER_SEC);

	obs = observation(10, 10);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.state, PRECISION_SYNC_LOCKED);

	obs = observation(20, 200);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.action, PRECISION_DISCIPLINE_IGNORE);
	zassert_equal(discipline.outlier_samples, 1);

	obs = observation(30, 220);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.action, PRECISION_DISCIPLINE_RESET);
	zassert_equal(discipline.state, PRECISION_SYNC_UNSYNCED);
}

ZTEST(precision_timing, test_pi_source_timeout_enters_holdover_then_unsyncs)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.source_timeout_ns = 1000,
		.holdover_ns = 5000,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs;

	zassert_ok(precision_pi_init(&discipline, &config));

	/* No observation yet: nothing to time out. */
	zassert_equal(precision_pi_check_source_timeout(&discipline, 0, &result), -EAGAIN);

	obs = observation(1000, 1000);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.state, PRECISION_SYNC_LOCKED);

	/* Within the source timeout the lock is preserved. */
	zassert_ok(precision_pi_check_source_timeout(&discipline, 1500, &result));
	zassert_equal(result.state, PRECISION_SYNC_LOCKED);

	/* Past the source timeout but inside holdover. */
	zassert_equal(precision_pi_check_source_timeout(&discipline, 2500, &result), -ESTALE);
	zassert_equal(discipline.state, PRECISION_SYNC_HOLDOVER);
	zassert_equal(result.action, PRECISION_DISCIPLINE_IGNORE);

	/* Past the holdover window resets the discipline to unsynced. */
	zassert_equal(precision_pi_check_source_timeout(&discipline, 7500, &result), -ESTALE);
	zassert_equal(discipline.state, PRECISION_SYNC_UNSYNCED);
	zassert_equal(result.action, PRECISION_DISCIPLINE_RESET);
	zassert_false(discipline.has_last_update);
}

ZTEST(precision_timing, test_pi_source_timeout_populates_result_on_time_overflow)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.source_timeout_ns = 1000,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs = observation(PRECISION_TIME_MIN, PRECISION_TIME_MIN);

	zassert_ok(precision_pi_init(&discipline, &config));
	zassert_ok(precision_pi_process(&discipline, &obs, &result));

	result.action = PRECISION_DISCIPLINE_STEP;
	result.state = PRECISION_SYNC_FAULT;
	result.offset_ns = 1;
	result.rate_ppb = 1;
	result.rejected_observations = 1U;

	zassert_equal(precision_pi_check_source_timeout(&discipline, PRECISION_TIME_MAX, &result),
		      -ERANGE);
	zassert_equal(result.action, PRECISION_DISCIPLINE_IGNORE);
	zassert_equal(result.state, PRECISION_SYNC_LOCKED);
	zassert_equal(result.offset_ns, 0);
	zassert_equal(result.rate_ppb, 0);
	zassert_equal(result.rejected_observations, 0U);
}

ZTEST(precision_timing, test_pi_zero_holdover_does_not_expire)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.source_timeout_ns = 1000,
		.holdover_ns = 0,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs = observation(1000, 1000);

	zassert_ok(precision_pi_init(&discipline, &config));
	zassert_ok(precision_pi_process(&discipline, &obs, &result));

	zassert_equal(precision_pi_check_source_timeout(&discipline, 2500, &result), -ESTALE);
	zassert_equal(discipline.state, PRECISION_SYNC_HOLDOVER);
	zassert_true(discipline.has_last_update);

	zassert_equal(precision_pi_check_source_timeout(&discipline, 60 * NSEC_PER_SEC, &result),
		      -ESTALE);
	zassert_equal(discipline.state, PRECISION_SYNC_HOLDOVER);
	zassert_true(discipline.has_last_update);
}

ZTEST(precision_timing, test_pi_fault_blocks_control_until_reset)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.source_timeout_ns = 1000,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs = observation(1100, 1000);

	zassert_ok(precision_pi_init(&discipline, &config));
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.action, PRECISION_DISCIPLINE_ADJUST_RATE);

	precision_pi_fault(&discipline);
	zassert_equal(discipline.state, PRECISION_SYNC_FAULT);
	zassert_equal(discipline.frequency_correction_ppb, 0);
	zassert_equal(precision_pi_process(&discipline, &obs, &result), -EIO);
	zassert_equal(result.action, PRECISION_DISCIPLINE_IGNORE);
	zassert_equal(result.state, PRECISION_SYNC_FAULT);
	zassert_equal(precision_pi_check_source_timeout(&discipline, 3000, &result), -EIO);
	zassert_equal(result.state, PRECISION_SYNC_FAULT);

	precision_pi_reset(&discipline);
	zassert_equal(discipline.state, PRECISION_SYNC_UNSYNCED);
	obs.local.time = 2000;
	obs.source.time = 2100;
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_equal(result.action, PRECISION_DISCIPLINE_ADJUST_RATE);
}

struct fake_clock_data {
	precision_time_t time;
	int32_t rate_ppb;
};

static int fake_clock_read(const struct precision_clock *precision_clk,
			   struct precision_time_point *tp)
{
	struct fake_clock_data *data = (struct fake_clock_data *)precision_clk->adapter;

	tp->time = data->time;
	tp->domain = precision_clk->domain;

	return 0;
}

static int fake_clock_set(const struct precision_clock *precision_clk,
			  const struct precision_time_point *tp)
{
	struct fake_clock_data *data = (struct fake_clock_data *)precision_clk->adapter;

	data->time = tp->time;

	return 0;
}

static int fake_clock_adjust_rate(const struct precision_clock *precision_clk, int32_t rate_ppb)
{
	struct fake_clock_data *data = (struct fake_clock_data *)precision_clk->adapter;

	data->rate_ppb = rate_ppb;

	return 0;
}

static const struct precision_clock_api fake_clock_api = {
	.read = fake_clock_read,
	.set = fake_clock_set,
	.adjust_rate = fake_clock_adjust_rate,
};

ZTEST(precision_timing, test_precision_clock_checks_domains_and_unsupported_ops)
{
	struct fake_clock_data data = {
		.time = 100,
	};
	struct precision_clock precision_clk = {
		.api = &fake_clock_api,
		.adapter = &data,
		.domain = local_domain,
	};
	struct precision_time_point tp = {
		.time = 200,
		.domain = source_domain,
	};

	zassert_ok(precision_clock_read(&precision_clk, &tp));
	zassert_equal(tp.time, 100);
	zassert_true(precision_time_domain_equal(&tp.domain, &local_domain));

	zassert_equal(precision_clock_set(&precision_clk, &tp), 0);
	zassert_equal(data.time, 100);

	tp.domain = source_domain;
	zassert_equal(precision_clock_set(&precision_clk, &tp), -EINVAL);
	zassert_equal(precision_clock_adjust_phase(&precision_clk, 1), -ENOTSUP);
	zassert_ok(precision_clock_adjust_rate(&precision_clk, 1234));
	zassert_equal(data.rate_ppb, 1234);
}

ZTEST(precision_timing, test_pi_init_rejects_outlier_without_sample_count)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.outlier_threshold_ns = 100,
		.outlier_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;

	/* Outlier rejection enabled but no sample count would reset the servo
	 * on the first outlier, so the configuration is rejected.
	 */
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);

	/* A non-zero sample count makes the configuration valid. */
	config.outlier_sample_count = 2;
	zassert_ok(precision_pi_init(&discipline, &config));

	/* Disabled outlier rejection does not require a sample count. */
	config.outlier_threshold_ns = 0;
	config.outlier_sample_count = 0;
	zassert_ok(precision_pi_init(&discipline, &config));
}

ZTEST(precision_timing, test_pi_init_rejects_invalid_ranges)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.min_rate_ppb = -100,
		.max_rate_ppb = 100,
		.gain_den = 1,
	};
	struct precision_pi_discipline discipline;

	config.step_threshold_ns = -1;
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
	config.step_threshold_ns = 0;
	config.lock_threshold_ns = -1;
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
	config.lock_threshold_ns = 0;
	config.outlier_threshold_ns = -1;
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
	config.outlier_threshold_ns = 0;
	config.max_uncertainty_ns = -1;
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
	config.max_uncertainty_ns = 0;
	config.source_timeout_ns = -1;
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
	config.source_timeout_ns = 0;
	config.holdover_ns = -1;
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
	config.holdover_ns = 0;
	config.min_rate_ppb = 101;
	zassert_equal(precision_pi_init(&discipline, &config), -EINVAL);
}

ZTEST(precision_timing, test_domain_mapping_is_trivially_copyable)
{
	struct precision_time_point src = {
		.time = 500,
		.domain = source_domain,
	};
	struct precision_time_mapping first = build_offset_mapping();
	struct precision_time_mapping second = build_offset_mapping();
	struct precision_time_point from_first;
	struct precision_time_point from_second;

	/* A mapping returned by value must stay usable: its timeutil sync
	 * state must not reference storage from the builder's stack frame.
	 */
	zassert_ok(precision_time_mapping_source_to_local(&first, &src, &from_first));
	zassert_ok(precision_time_mapping_source_to_local(&second, &src, &from_second));
	zassert_equal(from_first.time, 600);
	zassert_equal(from_first.time, from_second.time);

	/* Copies evolve independently. */
	precision_time_mapping_invalidate(&first);
	zassert_equal(precision_time_mapping_source_to_local(&first, &src, &from_first), -EAGAIN);
	zassert_ok(precision_time_mapping_source_to_local(&second, &src, &from_second));
	zassert_equal(from_second.time, 600);
}

ZTEST(precision_timing, test_pi_matches_float_reference_servo)
{
	/*
	 * Offsets (source - local) that keep the servo in the rate-adjust
	 * regime while exercising fixed-point truncation against a floating
	 * point reference of the PI servo the shared engine replaces.
	 */
	static const int64_t offsets[] = {
		47000, -31000, 22345, -15678, 9013, -6007, 4111, -2733, 1777, -1201,
		811,   -509,   337,   -211,   143,  -89,   61,   -37,   23,   -13,
	};
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs;
	const double kp = 0.7;
	const double ki = 0.3;
	double ref_drift = 0.0;
	int64_t local = 1000;

	zassert_ok(precision_pi_init(&discipline, &config));

	for (int i = 0; i < (int)ARRAY_SIZE(offsets); i++) {
		double ref_ppb;
		double err;

		obs = observation(local + offsets[i], local);
		zassert_ok(precision_pi_process(&discipline, &obs, &result));
		zassert_equal(result.action, PRECISION_DISCIPLINE_ADJUST_RATE);

		ref_drift += ki * (double)offsets[i];
		ref_ppb = kp * (double)offsets[i] + ref_drift;

		err = ref_ppb - (double)result.rate_ppb;
		if (err < 0) {
			err = -err;
		}

		/*
		 * Fixed-point truncation stays within one part-per-billion for
		 * the proportional term plus one per accumulated integral
		 * sample, so the deviation from the float servo is bounded by
		 * i + 2 after i + 1 processed samples.
		 */
		zassert_true(err <= (double)(i + 2),
			     "sample %d: fixed=%d float=%d deviates too far", i, result.rate_ppb,
			     (int)ref_ppb);

		local += 1000;
	}

	zassert_equal(result.state, PRECISION_SYNC_LOCKED);
}

ZTEST(precision_timing, test_pi_accessors_report_configuration_and_status)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.source_timeout_ns = 1000,
		.holdover_ns = 5000,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_pi_config readback;
	struct precision_pi_status status;
	struct precision_time_observation obs;

	zassert_equal(precision_pi_get_config(NULL, &readback), -EINVAL);
	zassert_equal(precision_pi_get_status(NULL, &status), -EINVAL);

	zassert_ok(precision_pi_init(&discipline, &config));
	zassert_ok(precision_pi_get_config(&discipline, &readback));
	zassert_equal(readback.source_timeout_ns, 1000);
	zassert_equal(readback.kp_num, 7);

	zassert_ok(precision_pi_get_status(&discipline, &status));
	zassert_equal(status.state, PRECISION_SYNC_UNSYNCED);
	zassert_false(status.has_observation);

	obs = observation(1000, 900);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));

	zassert_ok(precision_pi_get_status(&discipline, &status));
	zassert_equal(status.state, PRECISION_SYNC_LOCKED);
	zassert_true(status.has_observation);
	zassert_equal(status.last_update_ns, 900);
	zassert_equal(status.last_offset_ns, 100);
	zassert_equal(status.frequency_correction_ppb, result.rate_ppb);
}

ZTEST(precision_timing, test_pi_setters_update_limits_and_domain)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.source_timeout_ns = 1000,
		.holdover_ns = 5000,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_time_domain other_domain = {
		.type = PRECISION_TIME_DOMAIN_PHC,
		.id = 9,
	};
	struct precision_time_domain invalid_domain = {
		.type = PRECISION_TIME_DOMAIN_INVALID,
		.id = 0,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_pi_config readback;
	struct precision_pi_status status;
	struct precision_time_observation obs;

	zassert_ok(precision_pi_init(&discipline, &config));

	zassert_equal(precision_pi_set_rate_limits(&discipline, 10, -10), -EINVAL);
	zassert_ok(precision_pi_set_rate_limits(&discipline, -500, 500));
	zassert_ok(precision_pi_get_config(&discipline, &readback));
	zassert_equal(readback.min_rate_ppb, -500);
	zassert_equal(readback.max_rate_ppb, 500);

	zassert_equal(precision_pi_set_source_timeout(&discipline, -1, 0), -EINVAL);
	zassert_ok(precision_pi_set_source_timeout(&discipline, 4000, 8000));
	zassert_ok(precision_pi_get_config(&discipline, &readback));
	zassert_equal(readback.source_timeout_ns, 4000);
	zassert_equal(readback.holdover_ns, 8000);

	obs = observation(1000, 900);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));
	zassert_ok(precision_pi_get_status(&discipline, &status));
	zassert_true(status.has_observation);

	/* Keeping the same domain must not disturb the accepted observation. */
	zassert_ok(precision_pi_set_local_domain(&discipline, local_domain));
	zassert_ok(precision_pi_get_status(&discipline, &status));
	zassert_true(status.has_observation);

	zassert_equal(precision_pi_set_local_domain(&discipline, invalid_domain), -EINVAL);

	/* A real domain change drops observations made against the old domain. */
	zassert_ok(precision_pi_set_local_domain(&discipline, other_domain));
	zassert_ok(precision_pi_get_status(&discipline, &status));
	zassert_false(status.has_observation);
	zassert_ok(precision_pi_get_config(&discipline, &readback));
	zassert_equal(readback.local_domain.id, 9);
}

ZTEST(precision_timing, test_pi_time_to_expiry_tracks_timeout_and_holdover)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.source_timeout_ns = 1000,
		.holdover_ns = 5000,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs;
	precision_time_t remaining;

	zassert_ok(precision_pi_init(&discipline, &config));
	zassert_equal(precision_pi_time_to_expiry(NULL, 0, &remaining), -EINVAL);
	zassert_equal(precision_pi_time_to_expiry(&discipline, 0, NULL), -EINVAL);

	/* Without an accepted observation there is nothing to schedule. */
	zassert_equal(precision_pi_time_to_expiry(&discipline, 0, &remaining), -EAGAIN);

	obs = observation(1000, 1000);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));

	/* Before the timeout, aim just past the source timeout boundary. */
	zassert_ok(precision_pi_time_to_expiry(&discipline, 1400, &remaining));
	zassert_equal(remaining, 601);

	/* Inside holdover, aim just past the holdover boundary. */
	zassert_ok(precision_pi_time_to_expiry(&discipline, 2500, &remaining));
	zassert_equal(remaining, 4501);

	/* A backwards local step restarts the full interval. */
	zassert_ok(precision_pi_time_to_expiry(&discipline, 500, &remaining));
	zassert_equal(remaining, 1000);

	/* Once holdover elapsed there is nothing left to poll. */
	zassert_equal(precision_pi_time_to_expiry(&discipline, 8000, &remaining), -EAGAIN);
}

ZTEST(precision_timing, test_pi_time_to_expiry_ignores_disabled_timeouts)
{
	struct precision_pi_config config = {
		.source_domain = source_domain,
		.local_domain = local_domain,
		.step_threshold_ns = NSEC_PER_SEC,
		.lock_threshold_ns = 10,
		.source_timeout_ns = 0,
		.holdover_ns = 0,
		.lock_sample_count = 0,
		.min_rate_ppb = INT32_MIN,
		.max_rate_ppb = INT32_MAX,
		.kp_num = 7,
		.ki_num = 3,
		.gain_den = 10,
	};
	struct precision_pi_discipline discipline;
	struct precision_discipline_result result;
	struct precision_time_observation obs;
	precision_time_t remaining;

	zassert_ok(precision_pi_init(&discipline, &config));
	obs = observation(1000, 1000);
	zassert_ok(precision_pi_process(&discipline, &obs, &result));

	/* A disabled source timeout never expires. */
	zassert_equal(precision_pi_time_to_expiry(&discipline, 9999, &remaining), -EAGAIN);

	/* Indefinite holdover has no second boundary to poll for either. */
	zassert_ok(precision_pi_set_source_timeout(&discipline, 1000, 0));
	zassert_ok(precision_pi_time_to_expiry(&discipline, 1400, &remaining));
	zassert_equal(remaining, 601);
	zassert_equal(precision_pi_time_to_expiry(&discipline, 9999, &remaining), -EAGAIN);
}

ZTEST(precision_timing, test_deadline_schedules_and_expires_once)
{
	struct precision_deadline deadline = {0};

	/* Tolerate a null deadline and never report a cancelled one as due. */
	precision_deadline_cancel(NULL);
	precision_deadline_schedule(NULL, 1);
	zassert_false(precision_deadline_due(NULL));
	zassert_false(precision_deadline_due(&deadline));

	/* A non-positive delay cancels instead of expiring immediately. */
	precision_deadline_schedule(&deadline, 0);
	zassert_false(deadline.scheduled);
	precision_deadline_schedule(&deadline, -1);
	zassert_false(deadline.scheduled);

	/* Sub-millisecond delays round up so the deadline never expires early. */
	precision_deadline_schedule(&deadline, 1);
	zassert_true(deadline.scheduled);
	zassert_true(deadline.expiry_ms > k_uptime_get());

	precision_deadline_schedule(&deadline, 10 * NSEC_PER_MSEC);
	zassert_false(precision_deadline_due(&deadline));

	deadline.expiry_ms = k_uptime_get();
	zassert_true(precision_deadline_due(&deadline));

	/* An expired deadline is reported once and then cancelled. */
	zassert_false(deadline.scheduled);
	zassert_false(precision_deadline_due(&deadline));

	precision_deadline_schedule(&deadline, PRECISION_TIME_MAX);
	zassert_true(deadline.scheduled);
	zassert_true(deadline.expiry_ms >= PRECISION_TIME_MAX / NSEC_PER_MSEC);

	precision_deadline_cancel(&deadline);
	zassert_false(deadline.scheduled);
}

ZTEST_SUITE(precision_timing, NULL, NULL, NULL, NULL, NULL);
