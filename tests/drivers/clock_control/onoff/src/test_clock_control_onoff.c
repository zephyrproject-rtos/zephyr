/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#include <zephyr/drivers/clock_control/nrf_clock_control.h>

	static struct device *dev = DEVICE_DT_GET_ONE(COND_CODE_1((NRF_CLOCK_HAS_HFCLK),
								  (nordic_nrf_clock_hfclk),
								  (nordic_nrf_clock_xo)));

static bool clock_is_off(void)
{
	const struct device *const clk = DEVICE_DT_GET_ONE(nordic_nrf_clock);

	zassert_true(device_is_ready(clk), "Device is not ready");

	return clock_control_get_status(clk, CLOCK_CONTROL_NRF_SUBSYS_HF) ==
			CLOCK_CONTROL_STATUS_OFF;
}

static void clock_off(void)
{
	do {
		(void)nrf_clock_control_release(dev, NULL);

	} while (!clock_is_off());
}

ZTEST(clock_control_onoff, test_clock_blocking_on)
{
	struct onoff_client cli;
	int err;

	clock_off();

	sys_notify_init_spinwait(&cli.notify);
	err = nrf_clock_control_request(dev, NULL, &cli);
	zassert_true(err >= 0, "");

	while (sys_notify_fetch_result(&cli.notify, &err) < 0) {
		/* empty */
	}
	zassert_true(err >= 0, "");

	/* clock on, now turn it off */

	err = nrf_clock_control_release(dev, NULL);
	zassert_true(err >= 0, "");
}

ZTEST(clock_control_onoff, test_clock_spinwait_release_before_start)
{
	struct onoff_client cli;
	int err;

	clock_off();
	k_busy_wait(10000);

	sys_notify_init_spinwait(&cli.notify);
	err = nrf_clock_control_request(dev, NULL, &cli);
	zassert_true(err >= 0, "err: %d", err);

	/* Attempt to release while ongoing start. Cannot do that */
	err = nrf_clock_control_cancel_or_release(dev, NULL, &cli);
	zassert_true(err >= 0, "err: %d", err);

	k_busy_wait(100000);

	zassert_true(clock_is_off(), "");
}

static void request_cb(struct onoff_manager *mgr, struct onoff_client *cli,
			uint32_t state, int res)
{
	int err;

	err = onoff_cancel_or_release(mgr, cli);
	zassert_true(err >= 0, "err: %d", err);
}

/* Test checks if premature clock release works ok. If clock is released before
 * it is started it is the best to do that release from the callback to avoid
 * waiting until clock is started in the release context.
 */
ZTEST(clock_control_onoff, test_clock_release_from_callback)
{
	struct onoff_client cli;
	int err;

	clock_off();
	k_busy_wait(100);

	sys_notify_init_callback(&cli.notify, request_cb);
	err = nrf_clock_control_request(dev, NULL, &cli);
	zassert_true(err >= 0, "err: %d", err);

	k_busy_wait(100000);

	/* clock should be turned off in the started callback */
	zassert_true(clock_is_off(), "clock should be off");
}
ZTEST_SUITE(clock_control_onoff, NULL, NULL, NULL, NULL, NULL);
