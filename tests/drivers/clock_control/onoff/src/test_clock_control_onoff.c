/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#include <zephyr/drivers/clock_control/nrf_clock_control.h>

static struct onoff_manager *get_mgr(void)
{
	return z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
}

static bool clock_is_off(void)
{
	const struct device *clk = DEVICE_DT_GET_ONE(nordic_nrf_clock);

	zassert_true(device_is_ready(clk), "Device is not ready");

	return clock_control_get_status(clk, CLOCK_CONTROL_NRF_SUBSYS_HF) ==
			CLOCK_CONTROL_STATUS_OFF;
}

static void clock_off(void)
{
	struct onoff_manager *mgr = get_mgr();

	do {
		(void)onoff_release(mgr);

	} while (!clock_is_off());
}

void test_clock_blocking_on(void)
{
	struct onoff_client cli;
	struct onoff_manager *mgr = get_mgr();
	int err;

	clock_off();

	sys_notify_init_spinwait(&cli.notify);
	err = onoff_request(mgr, &cli);
	zassert_true(err >= 0, "");

	while (sys_notify_fetch_result(&cli.notify, &err) < 0) {
		/* empty */
	}
	zassert_true(err >= 0, "");

	/* clock on, now turn it off */

	err = onoff_release(mgr);
	zassert_true(err >= 0, "");
}

void test_clock_spinwait_release_before_start(void)
{
	struct onoff_client cli;
	struct onoff_manager *mgr = get_mgr();
	int err;

	clock_off();
	k_busy_wait(10000);

	sys_notify_init_spinwait(&cli.notify);
	err = onoff_request(mgr, &cli);
	zassert_true(err >= 0, "err: %d", err);

	/* Attempt to release while ongoing start. Cannot do that */
	err = onoff_cancel_or_release(mgr, &cli);
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
void test_clock_release_from_callback(void)
{
	struct onoff_client cli;
	struct onoff_manager *mgr = get_mgr();
	int err;

	clock_off();
	k_busy_wait(100);

	sys_notify_init_callback(&cli.notify, request_cb);
	err = onoff_request(mgr, &cli);
	zassert_true(err >= 0, "err: %d", err);

	k_busy_wait(100000);

	/* clock should be turned off in the started callback */
	zassert_true(clock_is_off(), "clock should be off");
}


void test_main(void)
{
	ztest_test_suite(test_clock_control_onoff,
		ztest_unit_test(test_clock_blocking_on),
		ztest_unit_test(test_clock_spinwait_release_before_start),
		ztest_unit_test(test_clock_release_from_callback)
			 );
	ztest_run_test_suite(test_clock_control_onoff);
}
