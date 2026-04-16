/*
 * Copyright (c) 2026 Contributors to the Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/pid.h>

static struct pid_state state;

static void *pid_test_setup(void)
{
	return NULL;
}

static void pid_test_before(void *fixture)
{
	ARG_UNUSED(fixture);
	pid_init(&state);
}

ZTEST_SUITE(pid_tests, NULL, pid_test_setup, pid_test_before, NULL, NULL);

ZTEST(pid_tests, test_init_zeros_state)
{
	pid_init(&state);
	zassert_equal(state.integral, 0, "Integral should be zero after init");
	zassert_equal(state.prev_error, 0, "Previous error should be zero after init");
}

ZTEST(pid_tests, test_reset_clears_state)
{
	state.integral = 12345;
	state.prev_error = 67;
	pid_reset(&state);
	zassert_equal(state.integral, 0, "Integral should be zero after reset");
	zassert_equal(state.prev_error, 0, "Previous error should be zero after reset");
}

ZTEST(pid_tests, test_proportional_only)
{
	const struct pid_cfg cfg = {
		.kp = PID_Q16(2),
		.ki = 0,
		.kd = 0,
		.output_min = -1000,
		.output_max = 1000,
		.integral_min = -1000,
		.integral_max = 1000,
	};

	/* Error = 100 - 0 = 100, output = 2 * 100 = 200 */
	int32_t out = pid_update(&cfg, &state, 100, 0);
	zassert_equal(out, 200, "P-only: expected 200, got %d", out);

	/* Error = 100 - 100 = 0, output = 0 */
	pid_reset(&state);
	out = pid_update(&cfg, &state, 100, 100);
	zassert_equal(out, 0, "P-only at setpoint: expected 0, got %d", out);

	/* Negative error */
	pid_reset(&state);
	out = pid_update(&cfg, &state, 0, 50);
	zassert_equal(out, -100, "P-only negative: expected -100, got %d", out);
}

ZTEST(pid_tests, test_integral_accumulation)
{
	const struct pid_cfg cfg = {
		.kp = 0,
		.ki = PID_Q16(1),
		.kd = 0,
		.output_min = -10000,
		.output_max = 10000,
		.integral_min = -10000,
		.integral_max = 10000,
	};

	/* First iteration: integral = 1 * 10 = 10 */
	int32_t out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 10, "I step 1: expected 10, got %d", out);

	/* Second iteration: integral = 10 + 10 = 20 */
	out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 20, "I step 2: expected 20, got %d", out);

	/* Third iteration: integral = 20 + 10 = 30 */
	out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 30, "I step 3: expected 30, got %d", out);
}

ZTEST(pid_tests, test_derivative_response)
{
	const struct pid_cfg cfg = {
		.kp = 0,
		.ki = 0,
		.kd = PID_Q16(1),
		.output_min = -1000,
		.output_max = 1000,
		.integral_min = -1000,
		.integral_max = 1000,
	};

	/* First call: error = 10, prev_error = 0, derivative = 10 */
	int32_t out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 10, "D step 1: expected 10, got %d", out);

	/* Second call: same error, derivative = 0 */
	out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 0, "D step 2: expected 0, got %d", out);

	/* Third call: error changes to 20, derivative = 10 */
	out = pid_update(&cfg, &state, 20, 0);
	zassert_equal(out, 10, "D step 3: expected 10, got %d", out);
}

ZTEST(pid_tests, test_output_clamping)
{
	const struct pid_cfg cfg = {
		.kp = PID_Q16(10),
		.ki = 0,
		.kd = 0,
		.output_min = -50,
		.output_max = 50,
		.integral_min = -1000,
		.integral_max = 1000,
	};

	/* Error = 100, output = 10 * 100 = 1000, clamped to 50 */
	int32_t out = pid_update(&cfg, &state, 100, 0);
	zassert_equal(out, 50, "Clamp max: expected 50, got %d", out);

	/* Negative: clamped to -50 */
	pid_reset(&state);
	out = pid_update(&cfg, &state, 0, 100);
	zassert_equal(out, -50, "Clamp min: expected -50, got %d", out);
}

ZTEST(pid_tests, test_anti_windup)
{
	const struct pid_cfg cfg = {
		.kp = 0,
		.ki = PID_Q16(1),
		.kd = 0,
		.output_min = -10000,
		.output_max = 10000,
		.integral_min = -25,
		.integral_max = 25,
	};

	/* Accumulate integral: 10 per step, should clamp at 25 */
	pid_update(&cfg, &state, 10, 0);  /* integral = 10 */
	pid_update(&cfg, &state, 10, 0);  /* integral = 20 */
	int32_t out = pid_update(&cfg, &state, 10, 0);  /* integral would be 30, clamped to 25 */
	zassert_equal(out, 25, "Anti-windup: expected 25, got %d", out);

	/* Further accumulation should not exceed clamp */
	out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 25, "Anti-windup sustained: expected 25, got %d", out);
}

ZTEST(pid_tests, test_pid_combined)
{
	const struct pid_cfg cfg = {
		.kp = PID_Q16(1),
		.ki = PID_Q16(1),
		.kd = PID_Q16(1),
		.output_min = -10000,
		.output_max = 10000,
		.integral_min = -10000,
		.integral_max = 10000,
	};

	/* First call: error=10, P=10, I=10, D=10, total=30 */
	int32_t out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 30, "PID combined step 1: expected 30, got %d", out);

	/* Second call: error=10, P=10, I=20, D=0, total=30 */
	out = pid_update(&cfg, &state, 10, 0);
	zassert_equal(out, 30, "PID combined step 2: expected 30, got %d", out);

	/* Third call: error=5, P=5, I=25, D=-5, total=25 */
	out = pid_update(&cfg, &state, 5, 0);
	zassert_equal(out, 25, "PID combined step 3: expected 25, got %d", out);
}

ZTEST(pid_tests, test_zero_error_produces_zero)
{
	const struct pid_cfg cfg = {
		.kp = PID_Q16(5),
		.ki = 0,
		.kd = PID_Q16(3),
		.output_min = -1000,
		.output_max = 1000,
		.integral_min = -1000,
		.integral_max = 1000,
	};

	int32_t out = pid_update(&cfg, &state, 50, 50);
	zassert_equal(out, 0, "Zero error: expected 0, got %d", out);
}
