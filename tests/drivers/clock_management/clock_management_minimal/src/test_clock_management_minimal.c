/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_management.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test);

/* Define clock management data */
CLOCK_MANAGEMENT_DT_DEFINE(DT_NODELABEL(emul_dev1));
static const struct clock_management_data *data =
	CLOCK_MANAGEMENT_DT_GET(DT_NODELABEL(emul_dev1));

/* Get references to each clock management output and state */
clock_output_t dev1_slow =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), slow);
clock_output_t dev1_fast =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), fast);
clock_request_t default_request = CLOCK_MANAGEMENT_STATE_DEFAULT;
clock_request_t sleep_request = CLOCK_MANAGEMENT_STATE_SLEEP;


/* Runs before every test, resets clocks to default state */
void reset_clock_states(void *unused)
{
	ARG_UNUSED(unused);
	int ret;

	/* Reset clock tree to default state */
	ret = clock_management_request_state(data, default_request);
	zassert_equal(ret, 0, "Failed to apply default clock management request");
	ret = clock_management_get_rate(data, dev1_slow);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), slow_default_freq),
		      "Failed to apply default clock management state for slow clock");
	ret = clock_management_get_rate(data, dev1_fast);
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
	TC_PRINT("Requesting default clock state\n");

	ret = clock_management_request_state(data, default_request);
	zassert_equal(ret, 0,
		      "Failed to apply default clock management request");
	ret = clock_management_get_rate(data, dev1_slow);
	TC_PRINT("Slow clock default clock rate: %d\n", ret);
	zassert_equal(ret, slow_default,
		      "Slow clock has invalid clock default clock rate");
	ret = clock_management_get_rate(data, dev1_fast);
	TC_PRINT("Fast clock default clock rate: %d\n", ret);
	zassert_equal(ret, fast_default,
		      "Fast clock has invalid clock default clock rate");
}

#ifdef CONFIG_CLOCK_MANAGEMENT_KEEP_SLEEP_STATE
ZTEST(clock_management_minimal, test_sleep_states)
{
	int ret;
	int slow_sleep = DT_PROP(DT_NODELABEL(emul_dev1), slow_sleep_freq);
	int fast_sleep = DT_PROP(DT_NODELABEL(emul_dev1), fast_sleep_freq);

	/* Apply sleep clock states for both clock outputs, make sure
	 * that rates match what is expected
	 */
	TC_PRINT("Requesting sleep clock state\n");

	ret = clock_management_request_state(data, sleep_request);
	zassert_equal(ret, 0,
		      "Failed to apply sleep clock management request");
	ret = clock_management_get_rate(data, dev1_slow);
	TC_PRINT("Slow clock sleep clock rate: %d\n", ret);
	zassert_equal(ret, slow_sleep,
		      "Slow clock has invalid clock sleep clock rate");
	ret = clock_management_get_rate(data, dev1_fast);
	TC_PRINT("Fast clock sleep clock rate: %d\n", ret);
	zassert_equal(ret, fast_sleep,
		      "Fast clock has invalid clock sleep clock rate");
}
#else
ZTEST(clock_management_minimal, test_sleep_states_skipped)
{
	int ret;
	int slow_default = DT_PROP(DT_NODELABEL(emul_dev1), slow_default_freq);
	int fast_default = DT_PROP(DT_NODELABEL(emul_dev1), fast_default_freq);

	/* The sleep request is compiled out when
	 * CONFIG_CLOCK_MANAGEMENT_KEEP_SLEEP_STATE is disabled. Requesting
	 * it must fail with -ENOENT and leave the clocks untouched.
	 */
	TC_PRINT("Requesting skipped sleep clock state\n");

	ret = clock_management_request_state(data, sleep_request);
	zassert_equal(ret, -ENOENT,
		      "Skipped sleep request should return -ENOENT");
	ret = clock_management_get_rate(data, dev1_slow);
	zassert_equal(ret, slow_default,
		      "Slow clock rate should be unchanged");
	ret = clock_management_get_rate(data, dev1_fast);
	zassert_equal(ret, fast_default,
		      "Fast clock rate should be unchanged");
}
#endif /* CONFIG_CLOCK_MANAGEMENT_KEEP_SLEEP_STATE */

ZTEST_SUITE(clock_management_minimal, NULL, NULL, reset_clock_states, NULL, NULL);
