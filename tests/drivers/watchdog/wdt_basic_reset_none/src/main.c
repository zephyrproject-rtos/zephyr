/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/watchdog.h>


/*
 * To use this test, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 */
#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(watchdog0))
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_s32_swt)
#define WDT_NODE DT_INST(0, nxp_s32_swt)
#endif

#define WDT_FEED_TRIES		2
#define WDT_MAX_WINDOW		1000
#define WDT_TIMEOUT		K_MSEC(1100)
#define SLEEP_TIME		K_MSEC(500)
#define WDT_TEST_CB_TEST_VALUE	0xCB

static struct wdt_timeout_cfg m_cfg_wdt0;
static volatile int wdt_interrupted_flag;
static volatile int wdt_feed_flag;


static void wdt_callback(const struct device *dev, int channel_id)
{
	wdt_interrupted_flag += WDT_TEST_CB_TEST_VALUE;
	zassert_equal(WDT_FEED_TRIES, wdt_feed_flag,
			"%d: Invalid number of feeding (expected: %d)",
			wdt_feed_flag, WDT_FEED_TRIES);
}

static int test_wdt_callback_reset_none(void)
{
	int err;
	const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);

	if (!device_is_ready(wdt)) {
		TC_PRINT("WDT device is not ready\n");
		return TC_FAIL;
	}

	m_cfg_wdt0.window.min = 0U;
	m_cfg_wdt0.window.max = WDT_MAX_WINDOW;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_NONE;
	m_cfg_wdt0.callback = wdt_callback;

	err = wdt_install_timeout(wdt, &m_cfg_wdt0);
	if (err == -ENOTSUP) {
		TC_PRINT("Some of the options are not supported, skip\n");
		return TC_SKIP;
	} else if (err != 0) {
		TC_PRINT("Watchdog install error\n");
		return TC_FAIL;
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err != 0) {
		TC_PRINT("Watchdog setup error\n");
		return TC_FAIL;
	}

	TC_PRINT("Feeding watchdog %d times\n", WDT_FEED_TRIES);
	wdt_feed_flag = 0;
	wdt_interrupted_flag = 0;
	for (int i = 0; i < WDT_FEED_TRIES; ++i) {
		TC_PRINT("Feeding %d\n", i+1);
		wdt_feed(wdt, 0);
		wdt_feed_flag++;
		k_sleep(SLEEP_TIME);
	}

	k_timeout_t timeout = WDT_TIMEOUT;
	uint64_t start_time = k_uptime_ticks();

	while (wdt_interrupted_flag == 0) {
		if (k_uptime_ticks() - start_time >= timeout.ticks) {
			break;
		}
	}

	zassert_equal(wdt_interrupted_flag, WDT_TEST_CB_TEST_VALUE,
			"wdt callback failed");

	err = wdt_disable(wdt);
	if (err == -EPERM) {
		TC_PRINT("Some of the options are not permitted, skip\n");
		return TC_SKIP;
	} else if (err != 0) {
		TC_PRINT("Disable watchdog error\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_wdt_bad_window_max(void)
{
	int err;
	const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);

	if (!device_is_ready(wdt)) {
		TC_PRINT("WDT device is not ready\n");
		return TC_FAIL;
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_NONE;
	m_cfg_wdt0.window.max = 0U;
	m_cfg_wdt0.window.min = 0U;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);
	if (err == -EINVAL) {
		return TC_PASS;
	}

	return TC_FAIL;
}


ZTEST(wdt_basic_reset_none, test_wdt_callback_reset_none)
{
	switch (test_wdt_callback_reset_none()) {
	case TC_SKIP:
		ztest_test_skip();
		break;
	case TC_PASS:
		ztest_test_pass();
		break;
	default:
		ztest_test_fail();
	}
}

ZTEST(wdt_basic_reset_none, test_wdt_bad_window_max)
{
	zassert_true(test_wdt_bad_window_max() == TC_PASS);
}

ZTEST_SUITE(wdt_basic_reset_none, NULL, NULL, NULL, NULL, NULL);
