/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_management.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test);

/* Define clock management outputs for both states */
CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), slow);
CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), fast);

/* Get references to each clock management output and state */
static const struct clock_output *dev1_slow =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), slow);
static const struct clock_output *dev1_fast =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), fast);
static clock_management_state_t dev1_slow_default =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), slow, default);
static clock_management_state_t dev1_fast_default =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), fast, default);
static clock_management_state_t dev1_slow_sleep =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), slow, sleep);
static clock_management_state_t dev1_fast_sleep =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), fast, sleep);

/* Runs before every test, resets clocks to default state */
void reset_clock_states(void *unused)
{
	ARG_UNUSED(unused);
	int ret;

	/* Reset clock tree to default state */
	ret = clock_management_apply_state(dev1_slow, dev1_slow_default);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), slow_default_freq),
		      "Failed to apply default clock management state for slow clock");
	ret = clock_management_apply_state(dev1_fast, dev1_fast_default);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), fast_default_freq),
		      "Failed to apply default clock management state for fast clock");
}

ZTEST(clock_management_minimal, test_default_states)
{
	int ret;
	int slow_default = DT_PROP(DT_NODELABEL(emul_dev1), slow_default_freq);
	int fast_default = DT_PROP(DT_NODELABEL(emul_dev1), fast_default_freq);

	/* Apply default clock states for both clock outputs, make sure
	 * that rates match what is expected
	 */
	TC_PRINT("Applying default clock states\n");

	ret = clock_management_apply_state(dev1_slow, dev1_slow_default);
	zassert_equal(ret, slow_default,
		      "Failed to apply default clock management state for slow clock");
	ret = clock_management_get_rate(dev1_slow);
	TC_PRINT("Slow clock default clock rate: %d\n", ret);
	zassert_equal(ret, slow_default,
		      "Slow clock has invalid clock default clock rate");

	ret = clock_management_apply_state(dev1_fast, dev1_fast_default);
	zassert_equal(ret, fast_default,
		      "Failed to apply default clock management state for fast clock");
	ret = clock_management_get_rate(dev1_fast);
	TC_PRINT("Fast clock default clock rate: %d\n", ret);
	zassert_equal(ret, fast_default,
		      "Fast clock has invalid clock default clock rate");
}

ZTEST(clock_management_minimal, test_sleep_states)
{
	int ret;
	int slow_sleep = DT_PROP(DT_NODELABEL(emul_dev1), slow_sleep_freq);
	int fast_sleep = DT_PROP(DT_NODELABEL(emul_dev1), fast_sleep_freq);

	/* Apply sleep clock states for both clock outputs, make sure
	 * that rates match what is expected
	 */
	TC_PRINT("Applying sleep clock states\n");

	ret = clock_management_apply_state(dev1_slow, dev1_slow_sleep);
	zassert_equal(ret, slow_sleep,
		      "Failed to apply sleep clock management state for slow clock");
	ret = clock_management_get_rate(dev1_slow);
	TC_PRINT("Slow clock sleep clock rate: %d\n", ret);
	zassert_equal(ret, slow_sleep,
		      "Slow clock has invalid clock sleep clock rate");

	ret = clock_management_apply_state(dev1_fast, dev1_fast_sleep);
	zassert_equal(ret, fast_sleep,
		      "Failed to apply sleep clock management state for fast clock");
	ret = clock_management_get_rate(dev1_fast);
	TC_PRINT("Fast clock sleep clock rate: %d\n", ret);
	zassert_equal(ret, fast_sleep,
		      "Fast clock has invalid clock sleep clock rate");
}

ZTEST(clock_management_minimal, test_rate_req)
{
	const struct clock_management_rate_req dev1_slow_req = {
		.min_freq = DT_PROP(DT_NODELABEL(emul_dev1), slow_request_freq),
		.max_freq = DT_PROP(DT_NODELABEL(emul_dev1), slow_request_freq),
	};
	const struct clock_management_rate_req dev1_fast_req = {
		.min_freq = DT_PROP(DT_NODELABEL(emul_dev1), fast_request_freq),
		.max_freq = DT_PROP(DT_NODELABEL(emul_dev1), fast_request_freq),
	};

	int dev1_slow_freq = DT_PROP(DT_NODELABEL(emul_dev1), slow_request_freq);
	int dev1_fast_freq = DT_PROP(DT_NODELABEL(emul_dev1), fast_request_freq);
	int ret;

	/* Apply constraints for slow clock */
	ret = clock_management_req_rate(dev1_slow, &dev1_slow_req);
	zassert_equal(ret, dev1_slow_freq,
		      "Slow clock got incorrect frequency for request");
	TC_PRINT("Slow clock configured to rate %d\n", dev1_slow_freq);
	ret = clock_management_req_rate(dev1_fast, &dev1_fast_req);
	zassert_equal(ret, dev1_fast_freq,
		      "Fast clock got incorrect frequency for request");
	TC_PRINT("Fast clock configured to rate %d\n", dev1_fast_freq);
}

ZTEST_SUITE(clock_management_minimal, NULL, NULL, reset_clock_states, NULL, NULL);
