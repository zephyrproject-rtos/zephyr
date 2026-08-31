/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/precision_timing/precision_pi.h>
#include <zephyr/precision_timing/precision_time.h>
#include <zephyr/sys/clock.h>
#include <zephyr/ztest.h>

#define zassert_double_close(actual, expected)                                                     \
	zassert_true(fabs((actual) - (expected)) <= 0.000000001, "unexpected floating result")

struct fake_clock_data {
	precision_time_t time_ns;
	precision_time_t phase;
	int64_t scaled_ppm;
	int error;
	int reads;
	int sets;
};

static int fake_clock_read(const struct precision_clock *precision_clk, precision_time_t *time_ns)
{
	struct fake_clock_data *data = precision_clk->data;

	data->reads++;
	if (data->error < 0) {
		return data->error;
	}

	*time_ns = data->time_ns;
	return 0;
}

static int fake_clock_set(const struct precision_clock *precision_clk, precision_time_t time_ns)
{
	struct fake_clock_data *data = precision_clk->data;

	data->sets++;
	if (data->error < 0) {
		return data->error;
	}

	data->time_ns = time_ns;
	return 0;
}

static int fake_clock_adjust_phase(const struct precision_clock *precision_clk,
				   precision_time_t phase_ns)
{
	struct fake_clock_data *data = precision_clk->data;

	if (data->error < 0) {
		return data->error;
	}

	data->phase = phase_ns;
	return 0;
}

static int fake_clock_adjust_rate(const struct precision_clock *precision_clk, int64_t scaled_ppm)
{
	struct fake_clock_data *data = precision_clk->data;

	if (data->error < 0) {
		return data->error;
	}

	data->scaled_ppm = scaled_ppm;
	return 0;
}

static const struct precision_clock_api fake_clock_api = {
	.read = fake_clock_read,
	.set = fake_clock_set,
	.adjust_phase = fake_clock_adjust_phase,
	.adjust_rate = fake_clock_adjust_rate,
};

static struct net_ptp_time ptp_time;
static int ptp_error;
static int ptp_phase;
static double ptp_rate_ratio;

static int fake_ptp_set(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	if (ptp_error < 0) {
		return ptp_error;
	}

	ptp_time = *tm;
	return 0;
}

static int fake_ptp_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	if (ptp_error < 0) {
		return ptp_error;
	}

	*tm = ptp_time;
	return 0;
}

static int fake_ptp_adjust(const struct device *dev, int increment)
{
	ARG_UNUSED(dev);

	if (ptp_error < 0) {
		return ptp_error;
	}

	ptp_phase = increment;
	return 0;
}

static int fake_ptp_rate_adjust(const struct device *dev, double ratio)
{
	ARG_UNUSED(dev);

	if (ptp_error < 0) {
		return ptp_error;
	}

	ptp_rate_ratio = ratio;
	return 0;
}

static DEVICE_API(ptp_clock, fake_ptp_api) = {
	.set = fake_ptp_set,
	.get = fake_ptp_get,
	.adjust = fake_ptp_adjust,
	.rate_adjust = fake_ptp_rate_adjust,
};

DEVICE_DEFINE(fake_ptp_clock, "fake_ptp_clock", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_ptp_api);

ZTEST(precision_timing, test_time_addition)
{
	precision_time_t result;

	zassert_ok(precision_time_add(10, 20, &result));
	zassert_equal(result, 30);
	zassert_ok(precision_time_add(-10, -20, &result));
	zassert_equal(result, -30);
	zassert_ok(precision_time_add(-10, 20, &result));
	zassert_equal(result, 10);
	zassert_equal(precision_time_add(PRECISION_TIME_MAX, 1, &result), -ERANGE);
	zassert_equal(precision_time_add(PRECISION_TIME_MIN, -1, &result), -ERANGE);
	zassert_equal(precision_time_add(0, 0, NULL), -EINVAL);
}

ZTEST(precision_timing, test_time_subtraction)
{
	precision_time_t result;

	zassert_ok(precision_time_sub(20, 10, &result));
	zassert_equal(result, 10);
	zassert_ok(precision_time_sub(-20, -10, &result));
	zassert_equal(result, -10);
	zassert_equal(precision_time_sub(PRECISION_TIME_MIN, 1, &result), -ERANGE);
	zassert_equal(precision_time_sub(PRECISION_TIME_MAX, -1, &result), -ERANGE);
	zassert_ok(precision_time_sub(PRECISION_TIME_MIN, PRECISION_TIME_MIN, &result));
	zassert_equal(result, 0);
	zassert_ok(precision_time_sub(-1, PRECISION_TIME_MIN, &result));
	zassert_equal(result, PRECISION_TIME_MAX);
	zassert_equal(precision_time_sub(0, PRECISION_TIME_MIN, &result), -ERANGE);
}

ZTEST(precision_timing, test_pi_update_and_reset)
{
	struct precision_pi pi;
	double output;

	precision_pi_init(&pi, 0.7, 0.3);
	zassert_double_close(pi.kp, 0.7);
	zassert_double_close(pi.ki, 0.3);
	zassert_double_close(pi.integral, 0.0);

	output = precision_pi_update(&pi, 10.0);
	zassert_double_close(output, 10.0);
	output = precision_pi_update(&pi, 10.0);
	zassert_double_close(output, 13.0);
	output = precision_pi_update(&pi, -10.0);
	zassert_double_close(output, -4.0);

	precision_pi_reset(&pi);
	zassert_double_close(pi.integral, 0.0);
	zassert_double_close(pi.kp, 0.7);
}

ZTEST(precision_timing, test_pi_instances_and_gains_are_independent)
{
	struct precision_pi first;
	struct precision_pi second;

	precision_pi_init(&first, 1.0, 0.5);
	precision_pi_init(&second, 2.0, 0.0);

	zassert_double_close(precision_pi_update(&first, 4.0), 6.0);
	zassert_double_close(precision_pi_update(&second, 4.0), 8.0);
	zassert_double_close(precision_pi_update(&first, 4.0), 8.0);
	zassert_double_close(precision_pi_update(&second, -4.0), -8.0);
	zassert_double_close(second.integral, 0.0);
}

ZTEST(precision_timing, test_clock_dispatch_and_error_propagation)
{
	struct fake_clock_data data = {.time_ns = 123};
	struct precision_clock precision_clk = {.api = &fake_clock_api, .data = &data};
	precision_time_t time_ns;

	zassert_ok(precision_clock_read(&precision_clk, &time_ns));
	zassert_equal(time_ns, 123);
	zassert_equal(data.reads, 1);
	zassert_ok(precision_clock_set(&precision_clk, -456));
	zassert_equal(data.time_ns, -456);
	zassert_ok(precision_clock_adjust_phase(&precision_clk, 789));
	zassert_equal(data.phase, 789);
	zassert_ok(precision_clock_adjust_rate(&precision_clk, PRECISION_CLOCK_SCALED_PPM_ONE));
	zassert_equal(data.scaled_ppm, PRECISION_CLOCK_SCALED_PPM_ONE);

	data.error = -EIO;
	zassert_equal(precision_clock_read(&precision_clk, &time_ns), -EIO);
	zassert_equal(precision_clock_set(&precision_clk, 0), -EIO);
	zassert_equal(precision_clock_adjust_phase(&precision_clk, 0), -EIO);
	zassert_equal(precision_clock_adjust_rate(&precision_clk, 0), -EIO);
}

ZTEST(precision_timing, test_clock_ppb_to_scaled_ppm)
{
	int64_t scaled_ppm;

	zassert_ok(precision_clock_ppb_to_scaled_ppm(1000.0, &scaled_ppm));
	zassert_equal(scaled_ppm, PRECISION_CLOCK_SCALED_PPM_ONE);
	zassert_ok(precision_clock_ppb_to_scaled_ppm(-1000.0, &scaled_ppm));
	zassert_equal(scaled_ppm, -PRECISION_CLOCK_SCALED_PPM_ONE);
	zassert_ok(precision_clock_ppb_to_scaled_ppm(0.0, &scaled_ppm));
	zassert_equal(scaled_ppm, 0);
	zassert_equal(precision_clock_ppb_to_scaled_ppm(DBL_MAX, &scaled_ppm), -ERANGE);
	zassert_equal(precision_clock_ppb_to_scaled_ppm(-DBL_MAX, &scaled_ppm), -ERANGE);
	zassert_equal(precision_clock_ppb_to_scaled_ppm((double)NAN, &scaled_ppm), -ERANGE);
	zassert_equal(precision_clock_ppb_to_scaled_ppm((double)INFINITY, &scaled_ppm), -ERANGE);
	zassert_equal(precision_clock_ppb_to_scaled_ppm(-(double)INFINITY, &scaled_ppm), -ERANGE);
	zassert_equal(precision_clock_ppb_to_scaled_ppm(0.0, NULL), -EINVAL);
}

ZTEST(precision_timing, test_ptp_adapter_converts_and_dispatches)
{
	struct precision_clock_ptp_adapter adapter;
	const struct precision_clock *precision_clk;
	precision_time_t time_ns;

	ptp_error = 0;
	ptp_time.second = 12;
	ptp_time.nanosecond = 34;
	zassert_ok(precision_clock_ptp_init(&adapter, DEVICE_GET(fake_ptp_clock)));
	precision_clk = precision_clock_ptp_get(&adapter);

	zassert_ok(precision_clock_read(precision_clk, &time_ns));
	zassert_equal(time_ns, 12LL * NSEC_PER_SEC + 34);
	zassert_ok(precision_clock_set(precision_clk, 56LL * NSEC_PER_SEC + 78));
	zassert_equal(ptp_time.second, 56);
	zassert_equal(ptp_time.nanosecond, 78);
	zassert_ok(precision_clock_adjust_phase(precision_clk, -123));
	zassert_equal(ptp_phase, -123);
	zassert_ok(precision_clock_adjust_rate(precision_clk,
					2 * PRECISION_CLOCK_SCALED_PPM_ONE));
	zassert_double_close(ptp_rate_ratio, 1.000002);
	zassert_ok(precision_clock_adjust_rate(precision_clk, 0));
	zassert_double_close(ptp_rate_ratio, 1.0);
	zassert_ok(precision_clock_adjust_rate(precision_clk,
					-(3 * PRECISION_CLOCK_SCALED_PPM_ONE / 2)));
	zassert_double_close(ptp_rate_ratio, 0.9999985);
	zassert_equal(precision_clock_set(precision_clk, -1), -ERANGE);
	zassert_equal(precision_clock_adjust_phase(precision_clk, (precision_time_t)INT_MAX + 1),
		      -ERANGE);
}

ZTEST(precision_timing, test_ptp_adapter_errors)
{
	struct precision_clock_ptp_adapter adapter;
	const struct precision_clock *precision_clk;
	precision_time_t time_ns;

	zassert_equal(precision_clock_ptp_init(NULL, DEVICE_GET(fake_ptp_clock)), -EINVAL);
	zassert_equal(precision_clock_ptp_init(&adapter, NULL), -EINVAL);

	ptp_error = -EIO;
	zassert_ok(precision_clock_ptp_init(&adapter, DEVICE_GET(fake_ptp_clock)));
	precision_clk = precision_clock_ptp_get(&adapter);
	zassert_equal(precision_clock_read(precision_clk, &time_ns), -EIO);
	zassert_equal(precision_clock_set(precision_clk, 0), -EIO);
	zassert_equal(precision_clock_adjust_phase(precision_clk, 0), -EIO);
	zassert_equal(precision_clock_adjust_rate(precision_clk, 0), -EIO);
}

ZTEST(precision_timing, test_ptp_adapter_rejects_unrepresentable_time)
{
	struct precision_clock_ptp_adapter adapter;
	precision_time_t time_ns;

	ptp_error = 0;
	zassert_ok(precision_clock_ptp_init(&adapter, DEVICE_GET(fake_ptp_clock)));

	ptp_time.second = PRECISION_TIME_MAX / NSEC_PER_SEC;
	ptp_time.nanosecond = PRECISION_TIME_MAX % NSEC_PER_SEC + 1;
	zassert_equal(precision_clock_read(precision_clock_ptp_get(&adapter), &time_ns), -ERANGE);

	ptp_time.second = 0;
	ptp_time.nanosecond = NSEC_PER_SEC;
	zassert_equal(precision_clock_read(precision_clock_ptp_get(&adapter), &time_ns), -ERANGE);
}

ZTEST_SUITE(precision_timing, NULL, NULL, NULL, NULL, NULL);
