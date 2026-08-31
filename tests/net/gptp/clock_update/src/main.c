/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_pi.h>
#include <zephyr/ztest.h>

#include "gptp_mi.h"

struct fake_clock_data {
	precision_time_t time;
	precision_time_t set_value;
	int64_t scaled_ppm;
	int read_error;
	int set_error;
	int rate_error;
	unsigned int read_calls;
	unsigned int set_calls;
	unsigned int rate_calls;
};

static struct fake_clock_data fake_data;
static struct precision_pi pi;

static int fake_clock_read(const struct precision_clock *precision_clk, precision_time_t *time_ns)
{
	struct fake_clock_data *data = precision_clk->data;

	data->read_calls++;
	if (data->read_error != 0) {
		return data->read_error;
	}

	*time_ns = data->time;
	return 0;
}

static int fake_clock_set(const struct precision_clock *precision_clk, precision_time_t time_ns)
{
	struct fake_clock_data *data = precision_clk->data;

	data->set_calls++;
	data->set_value = time_ns;
	if (data->set_error != 0) {
		return data->set_error;
	}

	data->time = time_ns;
	return 0;
}

static int fake_clock_adjust_phase(const struct precision_clock *precision_clk,
				   precision_time_t phase_ns)
{
	ARG_UNUSED(precision_clk);
	ARG_UNUSED(phase_ns);

	return 0;
}

static int fake_clock_adjust_rate(const struct precision_clock *precision_clk,
				  int64_t scaled_ppm)
{
	struct fake_clock_data *data = precision_clk->data;

	data->rate_calls++;
	data->scaled_ppm = scaled_ppm;
	return data->rate_error;
}

static const struct precision_clock_api fake_clock_api = {
	.read = fake_clock_read,
	.set = fake_clock_set,
	.adjust_phase = fake_clock_adjust_phase,
	.adjust_rate = fake_clock_adjust_rate,
};

static const struct precision_clock fake_clock = {
	.api = &fake_clock_api,
	.data = &fake_data,
};

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&fake_data, 0, sizeof(fake_data));
	precision_pi_init(&pi, 0.7, 0.3);
}

ZTEST(gptp_clock_update, test_small_offset_adjusts_rate)
{
	int ret;

	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 1000);

	zassert_ok(ret);
	zassert_equal(fake_data.rate_calls, 1);
	zassert_equal(fake_data.scaled_ppm, PRECISION_CLOCK_SCALED_PPM_ONE);
	zassert_equal(fake_data.read_calls, 0);
	zassert_equal(fake_data.set_calls, 0);
}

ZTEST(gptp_clock_update, test_small_negative_offset_adjusts_rate)
{
	int ret;

	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, -1000);

	zassert_ok(ret);
	zassert_equal(fake_data.rate_calls, 1);
	zassert_equal(fake_data.scaled_ppm, -PRECISION_CLOCK_SCALED_PPM_ONE);
	zassert_equal(fake_data.read_calls, 0);
	zassert_equal(fake_data.set_calls, 0);
}

ZTEST(gptp_clock_update, test_zero_offset_applies_nominal_rate)
{
	int ret;

	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 0);

	zassert_ok(ret);
	zassert_equal(fake_data.rate_calls, 1);
	zassert_equal(fake_data.scaled_ppm, 0);
	zassert_equal(fake_data.read_calls, 0);
	zassert_equal(fake_data.set_calls, 0);
}

ZTEST(gptp_clock_update, test_large_positive_pi_output_is_rejected)
{
	int ret;

	precision_pi_init(&pi, DBL_MAX, 0.0);
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 1);

	zassert_equal(ret, -ERANGE);
	zassert_equal(fake_data.rate_calls, 0);
}

ZTEST(gptp_clock_update, test_large_negative_pi_output_is_rejected)
{
	int ret;

	precision_pi_init(&pi, DBL_MAX, 0.0);
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, -1);

	zassert_equal(ret, -ERANGE);
	zassert_equal(fake_data.rate_calls, 0);
}

ZTEST(gptp_clock_update, test_nan_pi_output_is_rejected)
{
	int ret;

	precision_pi_init(&pi, (double)NAN, 0.0);
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 1);

	zassert_equal(ret, -ERANGE);
	zassert_equal(fake_data.rate_calls, 0);
}

ZTEST(gptp_clock_update, test_infinite_pi_outputs_are_rejected)
{
	int ret;

	precision_pi_init(&pi, (double)INFINITY, 0.0);
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 1);
	zassert_equal(ret, -ERANGE);
	zassert_equal(fake_data.rate_calls, 0);

	precision_pi_init(&pi, (double)INFINITY, 0.0);
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, -1);
	zassert_equal(ret, -ERANGE);
	zassert_equal(fake_data.rate_calls, 0);
}

ZTEST(gptp_clock_update, test_large_positive_offset_steps_forward)
{
	int ret;

	fake_data.time = 2 * NSEC_PER_SEC;
	ret = gptp_apply_clock_update(&pi, &fake_clock, 1, -900000000);

	zassert_ok(ret);
	zassert_equal(fake_data.set_calls, 1);
	zassert_equal(fake_data.time, 2100000000);
	zassert_equal(fake_data.rate_calls, 0);
}

ZTEST(gptp_clock_update, test_large_negative_offset_steps_backward)
{
	int ret;

	fake_data.time = 2 * NSEC_PER_SEC;
	ret = gptp_apply_clock_update(&pi, &fake_clock, -1, 900000000);

	zassert_ok(ret);
	zassert_equal(fake_data.set_calls, 1);
	zassert_equal(fake_data.time, 1900000000);
	zassert_equal(fake_data.rate_calls, 0);
}

ZTEST(gptp_clock_update, test_negative_target_is_rejected)
{
	int ret;

	fake_data.time = 50000000;
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, -60000000);

	zassert_equal(ret, -ERANGE);
	zassert_equal(fake_data.read_calls, 1);
	zassert_equal(fake_data.set_calls, 0);
	zassert_equal(fake_data.time, 50000000);
}

ZTEST(gptp_clock_update, test_read_failure_leaves_clock_unchanged)
{
	int ret;

	fake_data.time = 2 * NSEC_PER_SEC;
	fake_data.read_error = -EIO;
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 60000000);

	zassert_equal(ret, -EIO);
	zassert_equal(fake_data.read_calls, 1);
	zassert_equal(fake_data.set_calls, 0);
	zassert_equal(fake_data.time, 2 * NSEC_PER_SEC);
}

ZTEST(gptp_clock_update, test_set_failure_leaves_clock_unchanged)
{
	int ret;

	fake_data.time = 2 * NSEC_PER_SEC;
	fake_data.set_error = -EIO;
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 60000000);

	zassert_equal(ret, -EIO);
	zassert_equal(fake_data.set_calls, 1);
	zassert_equal(fake_data.set_value, 2060000000);
	zassert_equal(fake_data.time, 2 * NSEC_PER_SEC);
}

ZTEST(gptp_clock_update, test_rate_failure_leaves_clock_unchanged)
{
	int ret;

	fake_data.time = 2 * NSEC_PER_SEC;
	fake_data.rate_error = -EIO;
	ret = gptp_apply_clock_update(&pi, &fake_clock, 0, 1000);

	zassert_equal(ret, -EIO);
	zassert_equal(fake_data.rate_calls, 1);
	zassert_equal(fake_data.read_calls, 0);
	zassert_equal(fake_data.set_calls, 0);
	zassert_equal(fake_data.time, 2 * NSEC_PER_SEC);
}

ZTEST_SUITE(gptp_clock_update, NULL, NULL, before, NULL, NULL);
