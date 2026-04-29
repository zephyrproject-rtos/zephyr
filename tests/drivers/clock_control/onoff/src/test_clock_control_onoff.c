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

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static struct onoff_manager *get_mgr(void)
{
	return z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
}
#else
static const struct device *dev = DEVICE_DT_GET_ONE(COND_CODE_1(NRF_CLOCK_HAS_HFCLK,
							  (nordic_nrf_clock_hfclk),
							  (nordic_nrf_clock_xo)));
#endif

static bool clock_is_off(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	const struct device *const clk = DEVICE_DT_GET_ONE(nordic_nrf_clock);
#else
	const struct device *const clk = DEVICE_DT_GET_ONE(COND_CODE_1(NRF_CLOCK_HAS_HFCLK,
								       (nordic_nrf_clock_hfclk),
								       (nordic_nrf_clock_xo)));
#endif

	zassert_true(device_is_ready(clk), "Device is not ready");

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	return clock_control_get_status(clk, CLOCK_CONTROL_NRF_SUBSYS_HF) ==
			CLOCK_CONTROL_STATUS_OFF;
#else
	return clock_control_get_status(clk, NULL) == CLOCK_CONTROL_STATUS_OFF;
#endif
}

static void clock_off(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr = get_mgr();
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC)
	while (z_nrf_clock_calibration_is_in_progress()) {
		/* empty */
	}
#endif
	do {
#if defined(CONFIG_CLOCK_CONTROL_NRF)
		(void)onoff_release(mgr);
#else
		(void)nrf_clock_control_release(dev, NULL);
#endif

	} while (!clock_is_off());
}

ZTEST(clock_control_onoff, test_clock_blocking_on)
{
	struct onoff_client cli;
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr = get_mgr();
#endif
	int err;

	clock_off();

	sys_notify_init_spinwait(&cli.notify);
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_request(mgr, &cli);
#else
	err = nrf_clock_control_request(dev, NULL, &cli);
#endif
	zassert_true(err >= 0, "");

	while (sys_notify_fetch_result(&cli.notify, &err) < 0) {
		/* empty */
	}
	zassert_true(err >= 0, "");

	/* clock on, now turn it off */

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_release(mgr);
#else
	err = nrf_clock_control_release(dev, NULL);
#endif
	zassert_true(err >= 0, "");
}

ZTEST(clock_control_onoff, test_clock_spinwait_release_before_start)
{
	struct onoff_client cli;
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr = get_mgr();
#endif
	int err;

	clock_off();
	k_busy_wait(10000);

	sys_notify_init_spinwait(&cli.notify);
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_request(mgr, &cli);
#else
	err = nrf_clock_control_request(dev, NULL, &cli);
#endif
	zassert_true(err >= 0, "err: %d", err);

	/* Attempt to release while ongoing start. Cannot do that */
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_cancel_or_release(mgr, &cli);
#else
	err = nrf_clock_control_cancel_or_release(dev, NULL, &cli);
#endif
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
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr = get_mgr();
#endif
	int err;

	clock_off();
	k_busy_wait(100);

	sys_notify_init_callback(&cli.notify, request_cb);
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_request(mgr, &cli);
#else
	err = nrf_clock_control_request(dev, NULL, &cli);
#endif
	zassert_true(err >= 0, "err: %d", err);

	k_busy_wait(100000);

	/* clock should be turned off in the started callback */
	zassert_true(clock_is_off(), "clock should be off");
}
ZTEST_SUITE(clock_control_onoff, NULL, NULL, NULL, NULL, NULL);
