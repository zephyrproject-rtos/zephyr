/*
 * Copyright (c) 2022 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control.h>

#define TEST_FIXED_RATE_CLK0 DT_NODELABEL(test_fixed_rate_clk0)
#define TEST_FIXED_RATE_CLK0_RATE DT_PROP(TEST_FIXED_RATE_CLK0, clock_frequency)

/*
 * Basic test for checking correctness of clock_api implementation
 */
ZTEST(fixed_clk, test_fixed_rate_clk_on_off_status_rate)
{
	const struct device *dev = DEVICE_DT_GET(TEST_FIXED_RATE_CLK0);
	enum clock_control_status status;
	uint32_t rate;
	int err;

	zassert_equal(device_is_ready(dev), true, "%s: Device wasn't ready",
		      dev->name);

	status = clock_control_get_status(dev, 0);
	zassert_equal(status, CLOCK_CONTROL_STATUS_ON,
		      "%s: Unexpected status (%d)", dev->name, status);

	err = clock_control_on(dev, 0);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev->name, err);

	status = clock_control_get_status(dev, 0);
	zassert_equal(status, CLOCK_CONTROL_STATUS_ON,
		      "%s: Unexpected status (%d)", dev->name, status);

	err = clock_control_off(dev, 0);
	zassert_equal(0, err, "%s: Expected 0, got (%d)",
		      dev->name, err);

	status = clock_control_get_status(dev, 0);
	zassert_equal(status, CLOCK_CONTROL_STATUS_ON,
		      "%s: Unexpected status (%d)", dev->name, status);

	err = clock_control_get_rate(dev, 0, &rate);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev->name, err);
	zassert_equal(TEST_FIXED_RATE_CLK0_RATE, rate,
		      "%s: Got wrong rate, expected %u, got %u\n",
		      dev->name, TEST_FIXED_RATE_CLK0_RATE, rate);
}

ZTEST_SUITE(fixed_clk, NULL, NULL, NULL, NULL, NULL);
