/*
 * Copyright (c) 2023 Andriy Gelman <andriy.gelman@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/ztest.h>

#define NODELABEL DT_NODELABEL(samplenode)
static const struct device *clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(NODELABEL));

static void *pwm_clock_setup(void)
{
	int ret;
	uint32_t clock_rate;
	uint32_t clock_rate_dt = DT_PROP_BY_PHANDLE(NODELABEL, clocks, clock_frequency);

	zassert_equal(device_is_ready(clk_dev), true, "%s: PWM clock device is not ready",
		      clk_dev->name);

	ret = clock_control_get_rate(clk_dev, 0, &clock_rate);
	zassert_equal(0, ret, "%s: Unexpected err (%d) from clock_control_get_rate",
		      clk_dev->name, ret);

	zassert_equal(clock_rate_dt, clock_rate,
		      "%s: devicetree clock rate mismatch. Expected %dHz Fetched %dHz",
		      clk_dev->name, clock_rate_dt, clock_rate);

	ret = clock_control_on(clk_dev, 0);
	zassert_equal(0, ret, "%s: Unexpected err (%d) from clock_control_on", clk_dev->name, ret);

	return NULL;
}

ZTEST(pwm_clock, test_clock_control_get_rate)
{
	int ret;
	uint32_t clock_rate;

	ret = clock_control_get_rate(clk_dev, 0, &clock_rate);
	zassert_equal(0, ret, "%s: Unexpected err (%d) from clock_control_get_rate",
		      clk_dev->name, ret);
}

ZTEST(pwm_clock, test_clock_control_set_rate)
{
	int ret;
	uint32_t clock_rate, clock_rate_new;

	ret = clock_control_get_rate(clk_dev, 0, &clock_rate);
	zassert_equal(0, ret, "%s: Unexpected err (%d) from clock_control_get_rate",
		      clk_dev->name, ret);

	clock_rate /= 2;

	ret = clock_control_set_rate(clk_dev, 0, (clock_control_subsys_rate_t)clock_rate);
	zassert_equal(0, ret, "%s: unexpected err (%d) from clock_control_set_rate",
		      clk_dev->name, ret);

	ret = clock_control_get_rate(clk_dev, 0, &clock_rate_new);
	zassert_equal(0, ret, "%s: Unexpected err (%d) from clock_control_get_rate",
		      clk_dev->name, ret);

	zassert_equal(clock_rate, clock_rate_new,
		      "%s: Clock rate mismatch. Expected %dHz Fetched %dHz", clk_dev->name,
		      clock_rate, clock_rate_new);
}

ZTEST_SUITE(pwm_clock, NULL, pwm_clock_setup, NULL, NULL, NULL);
