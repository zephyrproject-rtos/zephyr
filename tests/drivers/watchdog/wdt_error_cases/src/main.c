/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/ztest_error_hook.h>

/*
 * To use this test, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 */
#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(watchdog0))
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
#define WDT_NODE DT_INST(0, nordic_nrf_wdt)
#elif DT_HAS_COMPAT_STATUS_OKAY(zephyr_counter_watchdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_counter_watchdog)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
#define NOINIT_SECTION ".dtcm_noinit.test_wdt"
#else
#define NOINIT_SECTION ".noinit.test_wdt"
#endif

/* Bit fields used to select tests to be run on the target */
#define WDT_DISABLE_SUPPORTED                     BIT(0)
#define WDT_FLAG_RESET_NONE_SUPPORTED             BIT(1)
#define WDT_FLAG_RESET_CPU_CORE_SUPPORTED         BIT(2)
#define WDT_FLAG_RESET_SOC_SUPPORTED              BIT(3)
#define WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED BIT(4)
#define WDT_OPT_PAUSE_IN_SLEEP_SUPPORTED          BIT(5)
#define WDT_OPT_PAUSE_HALTED_BY_DBG_SUPPORTED     BIT(6)
#define WDT_FEED_CAN_STALL                        BIT(7)

/* Common for all targets: */
#define DEFAULT_WINDOW_MAX (500U)
#define DEFAULT_WINDOW_MIN (0U)

/* Align tests to the specific target: */
#if defined(CONFIG_SOC_SERIES_NRF53X) || defined(CONFIG_SOC_SERIES_NRF54LX) || \
	defined(CONFIG_SOC_NRF54H20) || defined(CONFIG_SOC_NRF9280)
#define WDT_TEST_FLAGS                                                                             \
	(WDT_DISABLE_SUPPORTED | WDT_FLAG_RESET_SOC_SUPPORTED |                                    \
	 WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED | WDT_OPT_PAUSE_IN_SLEEP_SUPPORTED |            \
	 WDT_OPT_PAUSE_HALTED_BY_DBG_SUPPORTED)
#define DEFAULT_FLAGS            (WDT_FLAG_RESET_SOC)
#define MAX_INSTALLABLE_TIMEOUTS (8)
#define WDT_WINDOW_MAX_ALLOWED   (0x07CFFFFFU)
#define DEFAULT_OPTIONS          (WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG)
#else
/* By default run most of the error checks.
 * See Readme.txt on how to align test scope for the specific target.
 */
#define WDT_TEST_FLAGS                                                                             \
	(WDT_DISABLE_SUPPORTED | WDT_FLAG_RESET_SOC_SUPPORTED |                                    \
	 WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED)
#define DEFAULT_FLAGS            (WDT_FLAG_RESET_SOC)
#define MAX_INSTALLABLE_TIMEOUTS (8)
#define WDT_WINDOW_MAX_ALLOWED   (0xFFFFFFFFU)
#define DEFAULT_OPTIONS          (WDT_OPT_PAUSE_IN_SLEEP)
#endif

static const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);
static struct wdt_timeout_cfg m_cfg_wdt0;

/* Following variables are incremented in WDT callbacks
 * to indicate whether interrupt was fired or not.
 */
volatile uint32_t m_test_06b_value __attribute__((section(NOINIT_SECTION)));
#define TEST_06B_TAG (0x12345678U)
volatile uint32_t m_test_08b_value __attribute__((section(NOINIT_SECTION)));
#define TEST_08B_TAG (0x23456789U)
volatile uint32_t m_test_08d_A_value __attribute__((section(NOINIT_SECTION)));
#define TEST_08D_A_TAG (0x3456789AU)
volatile uint32_t m_test_08d_B_value __attribute__((section(NOINIT_SECTION)));
#define TEST_08D_B_TAG (0x456789ABU)

static void wdt_test_06b_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_test_06b_value = TEST_06B_TAG;
}

static void wdt_test_08b_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_test_08b_value = TEST_08B_TAG;
}

static void wdt_test_08d_A_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_test_08d_A_value = TEST_08D_A_TAG;
}

static void wdt_test_08d_B_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_test_08d_B_value = TEST_08D_B_TAG;
}

/**
 * @brief wdt_disable() negative test
 *
 * Confirm that wdt_disable() returns
 * -EFAULT when watchdog instance is not enabled.
 *
 */
ZTEST(wdt_coverage, test_01_wdt_disable_before_wdt_setup)
{
	int ret;

	if (!(WDT_TEST_FLAGS & WDT_DISABLE_SUPPORTED)) {
		/* Skip this test because wdt_disable() is NOT supported. */
		ztest_test_skip();
	}

	/* Call wdt_disable before enabling wdt */
	ret = wdt_disable(wdt);
	zassert_true(ret == -EFAULT,
		     "Calling wdt_disable before watchdog was started should return -EFAULT (-14), "
		     "got unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_setup() negative test
 *
 * Confirm that wdt_setup() returns error value or ASSERTION FAIL
 * when it's called before wdt_install_timeouts().
 *
 */
ZTEST(wdt_coverage, test_02_wdt_setup_before_setting_timeouts)
{
	int ret;

	/* Call wdt_setup before wdt_install_timeouts() */
	ztest_set_assert_valid(true);
	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret < 0,
		     "Calling wdt_setup before installing timeouts should fail, got unexpected "
		     "value of %d",
		     ret);
}

/**
 * @brief wdt_feed() negative test
 *
 * Confirm that wdt_feed() returns error value
 * when it's called before wdt_setup().
 * Test scenario where none of timeout channels is configured.
 *
 */
ZTEST(wdt_coverage, test_03_wdt_feed_before_wdt_setup_channel_not_configured)
{
	int ret;

	/* Call wdt_feed() before wdt_setup() (channel wasn't configured) */
	ret = wdt_feed(wdt, 0);
	zassert_true(ret == -EINVAL,
		     "wdt_feed() shall return error value when called before wdt_setup(), got "
		     "unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_install_timeout() negative test
 *
 * Confirm that wdt_install_timeout() returns
 * -ENOTSUP when flag WDT_FLAG_RESET_NONE is not supported
 *
 */
ZTEST(wdt_coverage, test_04a_wdt_install_timeout_WDT_FLAG_RESET_NONE_not_supported)
{
	int ret;

	if (WDT_TEST_FLAGS & WDT_FLAG_RESET_NONE_SUPPORTED) {
		/* Skip this test because WDT_FLAG_RESET_NONE is supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_NONE;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -ENOTSUP,
		     "WDT_FLAG_RESET_NONE is not supported on this target and should fail, got "
		     "unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_install_timeout() negative test
 *
 * Confirm that wdt_install_timeout() returns
 * -ENOTSUP when flag WDT_FLAG_RESET_CPU_CORE is not supported
 *
 */
ZTEST(wdt_coverage, test_04b_wdt_install_timeout_WDT_FLAG_RESET_CPU_CORE_not_supported)
{
	int ret;

	if (WDT_TEST_FLAGS & WDT_FLAG_RESET_CPU_CORE_SUPPORTED) {
		/* Skip this test because WDT_FLAG_RESET_CPU_CORE is supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_CPU_CORE;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -ENOTSUP,
		     "WDT_FLAG_RESET_CPU_CORE is not supported on this target and should fail, got "
		     "unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_install_timeout() negative test
 *
 * Confirm that wdt_install_timeout() returns
 * -ENOTSUP when flag WDT_FLAG_RESET_SOC is not supported
 *
 */
ZTEST(wdt_coverage, test_04c_wdt_install_timeout_WDT_FLAG_RESET_SOC_not_supported)
{
	int ret;

	if (WDT_TEST_FLAGS & WDT_FLAG_RESET_SOC_SUPPORTED) {
		/* Skip this test because WDT_FLAG_RESET_SOC is supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -ENOTSUP,
		     "WDT_FLAG_RESET_SOC is not supported on this target and should fail, got "
		     "unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_install_timeout() negative test
 *
 * Confirm that wdt_install_timeout() returns
 * -EINVAL when window timeout is out of possible range
 *
 */
ZTEST(wdt_coverage, test_04w_wdt_install_timeout_with_invalid_window)
{
	int ret;

	/* set defaults */
	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	/* ----------------- window.min
	 * Check that window.min can't be different than 0
	 */
	m_cfg_wdt0.window.min = 1U;
	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -EINVAL,
		     "Calling wdt_install_timeout with window.min = 1 should return -EINVAL (-22), "
		     "got unexpected value of %d",
		     ret);

	/* Set default window.min */
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	/* ----------------- window.max
	 * Check that window.max can't be equal to 0
	 */
	m_cfg_wdt0.window.max = 0U;
	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -EINVAL,
		     "Calling wdt_install_timeout with window.max = 0 should return -EINVAL (-22), "
		     "got unexpected value of %d",
		     ret);

	/* Check that window.max can't exceed maximum allowed value */
	m_cfg_wdt0.window.max = WDT_WINDOW_MAX_ALLOWED + 1;
	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -EINVAL,
		     "Calling wdt_install_timeout with window.max = %d should return -EINVAL "
		     "(-22), got unexpected value of %d",
		     WDT_WINDOW_MAX_ALLOWED + 1, ret);
}

/**
 * @brief wdt_install_timeout() negative test
 *
 * Confirm that wdt_install_timeout() returns
 * -EINVAL when watchdog supports only one timeout value
 * for all timeouts and the supplied timeout window differs
 * from windows for alarms installed so far.
 *
 */
ZTEST(wdt_coverage, test_04wm_wdt_install_timeout_with_multiple_timeout_values)
{
	int ret;

	if (!(WDT_TEST_FLAGS & WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED)) {
		/* Skip this test because timeouts with different values are supported */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	/* Call wdt_install_timeout again with different window */
	m_cfg_wdt0.window.max = WDT_WINDOW_MAX_ALLOWED >> 1;
	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -EINVAL,
		     "wdt_install_timeout should return -EINVAL (-22), got unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_install_timeout() negative test
 *
 * Confirm that wdt_install_timeout() returns ASSERTION FAIL or
 * -EBUSY when called after the watchdog instance has been already setup.
 *
 */
ZTEST(wdt_coverage, test_05_wdt_install_timeout_after_wdt_setup)
{
	int ret;

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);

	/* Call wdt_install_timeout again to test invalid use */
	ztest_set_assert_valid(true);
	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -EBUSY,
		     "Calling wdt_install_timeout after wdt_setup should return -EBUSY (-16), got "
		     "unexpected value of %d",
		     ret);

	/* Assumption: wdt_disable() is called after this test */
}

/**
 * @brief wdt_setup() negative test
 *
 * Confirm that wdt_setup() returns
 * -ENOTSUP when option WDT_OPT_PAUSE_IN_SLEEP is not supported
 *
 */
ZTEST(wdt_coverage, test_06a_wdt_setup_WDT_OPT_PAUSE_IN_SLEEP_not_supported)
{
	int ret;

	if (WDT_TEST_FLAGS & WDT_OPT_PAUSE_IN_SLEEP_SUPPORTED) {
		/* Skip this test because WDT_OPT_PAUSE_IN_SLEEP is supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, WDT_OPT_PAUSE_IN_SLEEP);
	zassert_true(ret == -ENOTSUP,
		     "WDT_OPT_PAUSE_IN_SLEEP is not supported on this target and should fail, got "
		     "unexpected value of %d",
		     ret);

	ret = wdt_setup(wdt, WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG);
	zassert_true(ret == -ENOTSUP,
		     "WDT_OPT_PAUSE_IN_SLEEP is not supported on this target and should fail, got "
		     "unexpected value of %d",
		     ret);
}

/**
 * @brief Test that wdt_setup(device, WDT_OPT_PAUSE_IN_SLEEP) works as expected
 *
 * Confirm that when WDT_OPT_PAUSE_IN_SLEEP is set,
 * watchdog will not fire when thread is sleeping.
 *
 */
ZTEST(wdt_coverage, test_06b_wdt_setup_WDT_OPT_PAUSE_IN_SLEEP_functional)
{
	int ret;

	if (!(WDT_TEST_FLAGS & WDT_OPT_PAUSE_IN_SLEEP_SUPPORTED)) {
		/* Skip this test because WDT_OPT_PAUSE_IN_SLEEP can NOT be used. */
		ztest_test_skip();
	}

	/* When test fails, watchdog sets m_test_06b_value to TEST_06B_TAG in WDT callback
	 * wdt_test_06b_cb. Then, target is reset. Check value of m_test_06b_value to prevent reset
	 * loop on this test.
	 */
	if (m_test_06b_value == TEST_06B_TAG) {
		m_test_06b_value = 0U;
		zassert_true(false, "Watchod has fired while it shouldn't");
	}

	/* Clear flag that is set when the watchdog fires */
	m_test_06b_value = 0U;

	m_cfg_wdt0.callback = wdt_test_06b_cb;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	/* Set timeout window to ~500 ms */
	m_cfg_wdt0.window.max = 500U;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, WDT_OPT_PAUSE_IN_SLEEP);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);
	TC_PRINT("Test has failed if there is reset after this line\n");

	/* Sleep for longer time than watchdog timeout */
	k_sleep(K_SECONDS(1));

	/* m_test_06b_value is set to TEST_06B_TAG in WDT callback */
	zassert_equal(m_test_06b_value, 0, "Watchod has fired while it shouldn't");

	/* Assumption: wdt_disable() is called after each test */
}

/**
 * @brief wdt_setup() negative test
 *
 * Confirm that wdt_setup() returns
 * -ENOTSUP when option WDT_OPT_PAUSE_HALTED_BY_DBG is not supported
 *
 */
ZTEST(wdt_coverage, test_06c_wdt_setup_WDT_OPT_PAUSE_HALTED_BY_DBG_not_supported)
{
	int ret;

	if (WDT_TEST_FLAGS & WDT_OPT_PAUSE_HALTED_BY_DBG_SUPPORTED) {
		/* Skip this test because WDT_OPT_PAUSE_HALTED_BY_DBG is supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	zassert_true(ret == -ENOTSUP,
		     "WDT_OPT_PAUSE_HALTED_BY_DBG is not supported on this target and should fail, "
		     "got unexpected value of %d",
		     ret);

	ret = wdt_setup(wdt, WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG);
	zassert_true(ret == -ENOTSUP,
		     "WDT_OPT_PAUSE_HALTED_BY_DBG is not supported on this target and should fail, "
		     "got unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_setup() corner case
 *
 * Confirm that wdt_setup() returns
 * 0 - success, when no option is provided
 *
 */
ZTEST(wdt_coverage, test_06d_wdt_setup_without_any_OPT)
{
	int ret;

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, 0x0);
	zassert_true(ret == 0, "Got unexpected value of %d, while expected is 0", ret);
}

/**
 * @brief wdt_setup() negative test
 *
 * Confirm that wdt_setup() returns
 * -EBUSY when watchdog instance has been already setup.
 *
 */
ZTEST(wdt_coverage, test_07_wdt_setup_already_done)
{
	int ret;

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);

	/* Call wdt_setup again to test invalid use */
	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == -EBUSY,
		     "Calling wdt_setup for the second time should return -EBUSY (-16), got "
		     "unexpected value of %d",
		     ret);

	/* Assumption: wdt_disable() is called after this test */
}

/**
 * @brief wdt_setup() negative test
 *
 * Confirm that wdt_disable() returns
 * -EPERM when watchdog can not be disabled directly by application code.
 *
 */
ZTEST(wdt_coverage, test_08a_wdt_disable_not_supported)
{
	int ret;

	if (WDT_TEST_FLAGS & WDT_DISABLE_SUPPORTED) {
		/* Skip this test because wdt_disable() is supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	/* Assumption - test suite execution finishes before WDT timeout will fire */
	m_cfg_wdt0.window.max = WDT_WINDOW_MAX_ALLOWED;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);

	/* Call wdt_disable to test not allowed use */
	ret = wdt_disable(wdt);
	zassert_true(ret == -EPERM,
		     "Disabling WDT is not supported on this target and should return -EPERM (-1), "
		     "got unexpected value of %d",
		     ret);
}

/**
 * @brief Test that wdt_disable() stops watchdog
 *
 * Confirm that wdt_disable() prevents previously configured
 * watchdog from resetting the core.
 *
 */
ZTEST(wdt_coverage, test_08b_wdt_disable_check_not_firing)
{
	int ret;

	if (!(WDT_TEST_FLAGS & WDT_DISABLE_SUPPORTED)) {
		/* Skip this test because wdt_disable() is NOT supported. */
		ztest_test_skip();
	}

	/* When test fails, watchdog sets m_test_08b_value to TEST_08B_TAG in WDT callback
	 * wdt_test_08b_cb. Then, target is reset. Check value of m_test_08b_value to prevent reset
	 * loop on this test.
	 */
	if (m_test_08b_value == TEST_08B_TAG) {
		m_test_08b_value = 0U;
		zassert_true(false, "Watchod has fired while it shouldn't");
	}

	/* Clear flag that is set when the watchdog fires */
	m_test_08b_value = 0U;

	m_cfg_wdt0.callback = wdt_test_08b_cb;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	/* Set timeout window to ~500 ms */
	m_cfg_wdt0.window.max = 500U;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);
	TC_PRINT("Test has failed if there is reset after this line\n");

	/* Wait for 450 ms, then disable the watchdog
	 * Don't change to k_sleep() because use of WDT_OPT_PAUSE_IN_SLEEP
	 * will break test scenario.
	 */
	k_busy_wait(450000);
	ret = wdt_disable(wdt);
	zassert_true(ret == 0, "Watchdog disable error, got unexpected value of %d", ret);

	/* Wait a bit more to see if watchdog fires
	 * Don't change to k_sleep() because use of WDT_OPT_PAUSE_IN_SLEEP
	 * will break test scenario.
	 */
	k_busy_wait(300000);

	/* m_test_08b_value is set to TEST_08B_TAG in WDT callback */
	zassert_equal(m_test_08b_value, 0, "Watchod has fired while it shouldn't");
}

/**
 * @brief Test that after wdt_disable() timeouts can be reconfigured
 *
 * Confirm that after wdt_disable() it is possible to configure
 * timeout channel that was configured previously.
 *
 */
ZTEST(wdt_coverage, test_08c_wdt_disable_check_timeouts_reusable)
{
	int ret, id1, id2;

	if (!(WDT_TEST_FLAGS & WDT_DISABLE_SUPPORTED)) {
		/* Skip this test because wdt_disable() is NOT supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	id1 = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(id1 >= 0, "Watchdog install error, got unexpected value of %d", id1);
	TC_PRINT("Configured WDT channel %d\n", id1);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);

	ret = wdt_disable(wdt);
	zassert_true(ret == 0, "Watchdog disable error, got unexpected value of %d", ret);

	id2 = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(id2 >= 0, "Watchdog install error, got unexpected value of %d", id2);
	TC_PRINT("Configured WDT channel %d\n", id2);

	/* test that timeout channel id2 is NOT greater than previously returned channel id1 */
	zassert_true(id2 <= id1,
		     "First usable timeout channel after wdt_disable() is %d, expected number no "
		     "greater than %d",
		     id2, id1);
}

/**
 * @brief Test that after wdt_disable() uninstalled timeouts don't have to be feed
 *
 * Confirm that wdt_disable() uninstalls all timeouts.
 * When new timeout is configured, only this one has to be feed.
 *
 */
ZTEST(wdt_coverage, test_08d_wdt_disable_check_timeouts_uninstalled)
{
	int ret, id_A, id_B, i;

	if (!(WDT_TEST_FLAGS & WDT_DISABLE_SUPPORTED)) {
		/* Skip this test because wdt_disable() is NOT supported. */
		ztest_test_skip();
	}

	/* m_test_08d_A_value is set to TEST_08D_A_TAG in callback wdt_test_08d_A_cb */
	if (m_test_08d_A_value == TEST_08D_A_TAG) {
		m_test_08d_A_value = 0U;
		zassert_true(false, "Timeout A has fired while it shouldn't");
	}

	/* m_test_08d_B_value is set to TEST_08D_B_TAG in callback wdt_test_08d_B_cb */
	if (m_test_08d_B_value == TEST_08D_B_TAG) {
		m_test_08d_B_value = 0U;
		zassert_true(false, "Timeout B has fired while it shouldn't");
	}

	/* Clear flags that are set when the watchdog fires */
	m_test_08d_A_value = 0U;
	m_test_08d_B_value = 0U;

	/* Configure Timeout A */
	m_cfg_wdt0.callback = wdt_test_08d_A_cb;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	/* Set timeout window to ~500 ms */
	m_cfg_wdt0.window.max = 500U;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	id_A = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(id_A >= 0, "Watchdog install error, got unexpected value of %d", id_A);
	TC_PRINT("Configured WDT channel %d\n", id_A);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);

	ret = wdt_disable(wdt);
	zassert_true(ret == 0, "Watchdog disable error, got unexpected value of %d", ret);

	/* Configure Timeout B */
	m_cfg_wdt0.callback = wdt_test_08d_B_cb;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	/* Set timeout window to ~500 ms */
	m_cfg_wdt0.window.max = 500U;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	id_B = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(id_B >= 0, "Watchdog install error, got unexpected value of %d", id_B);
	TC_PRINT("Configured WDT channel %d\n", id_B);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);
	TC_PRINT("Test has failed if there is reset after this line\n");

	/* Confirm that only Timeout B has to be feed */
	for (i = 0; i < 4; i++) {
		k_busy_wait(450000);
		wdt_feed(wdt, id_B);
	}

	ret = wdt_disable(wdt);
	zassert_true(ret == 0, "Watchdog disable error, got unexpected value of %d", ret);

	/* m_test_08d_A_value is set to TEST_08D_A_TAG in callback wdt_test_08d_A_cb */
	zassert_equal(m_test_08d_A_value, 0, "Timeout A has fired while it shouldn't");

	/* m_test_08d_B_value is set to TEST_08D_B_TAG in callback wdt_test_08d_B_cb */
	zassert_equal(m_test_08d_B_value, 0, "Timeout B has fired while it shouldn't");
}

/**
 * @brief Test error code when wdt_setup() is called after wdt_disable()
 *
 * Confirm that wdt_setup() returns error value or ASSERTION FAIL
 * when it's called before any timeout was configured with wdt_install_timeouts().
 * All timeouts were uninstalled by calling wdt_disable().
 *
 */
ZTEST(wdt_coverage, test_08e_wdt_setup_immediately_after_wdt_disable)
{
	int ret;

	if (!(WDT_TEST_FLAGS & WDT_DISABLE_SUPPORTED)) {
		/* Skip this test because wdt_disable() is NOT supported. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret >= 0, "Watchdog install error, got unexpected value of %d", ret);
	TC_PRINT("Configured WDT channel %d\n", ret);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);

	ret = wdt_disable(wdt);
	zassert_true(ret == 0, "Watchdog disable error, got unexpected value of %d", ret);

	/* Call wdt_setup when no timeouts are configured. */
	/* Timeouts were removed by calling wdt_disable(). */
	ztest_set_assert_valid(true);
	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret < 0,
		     "Calling wdt_setup before installing timeouts should fail, got unexpected "
		     "value of %d",
		     ret);
}

/**
 * @brief wdt_feed() negative test
 *
 * Confirm that wdt_feed() returns error or ASSERTION FAIL
 * when it's called before wdt_setup().
 * Test scenario where timeout channel is configured.
 *
 */
ZTEST(wdt_coverage, test_09a_wdt_feed_before_wdt_setup_channel_configured)
{
	int ret, ch_id;

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ch_id = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ch_id >= 0, "Watchdog install error, got unexpected value of %d", ch_id);
	TC_PRINT("Configured WDT channel %d\n", ch_id);

	/* Call wdt_feed() before wdt_setup() (channel was configured) */
	ztest_set_assert_valid(true);
	ret = wdt_feed(wdt, ch_id);
	zassert_true(ret < 0,
		     "wdt_feed() shall return error value when called before wdt_setup(), got "
		     "unexpected value of %d",
		     ret);
}

/**
 * @brief wdt_feed() negative test
 *
 * Confirm that wdt_feed() returns
 * -EINVAL when there is no installed timeout for supplied channel.
 *
 */
ZTEST(wdt_coverage, test_09b_wdt_feed_invalid_channel)
{
	int ret, ch_id, ch_invalid;

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ch_id = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ch_id >= 0, "Watchdog install error, got unexpected value of %d", ch_id);
	TC_PRINT("Configured WDT channel %d\n", ch_id);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);
	TC_PRINT("Test has failed if there is reset after this line\n");

	/* Call wdt_feed() on not configured channel */
	ch_invalid = ch_id + 2;
	ret = wdt_feed(wdt, ch_invalid);
	zassert_true(ret == -EINVAL,
		     "wdt_feed(%d) shall return -EINVAL (-22), got unexpected value of %d",
		     ch_invalid, ret);

	/* Call wdt_feed() on not configured channel */
	ch_invalid = ch_id + 1;
	ret = wdt_feed(wdt, ch_invalid);
	zassert_true(ret == -EINVAL,
		     "wdt_feed(%d) shall return -EINVAL (-22), got unexpected value of %d",
		     ch_invalid, ret);

	/* Call wdt_feed() on invalid channel (no such channel) */
	ret = wdt_feed(wdt, -1);
	zassert_true(ret == -EINVAL,
		     "wdt_feed(-1) shall return -EINVAL (-22), got unexpected value of %d", ret);

	/* Call wdt_feed() on invalid channel (no such channel) */
	ret = wdt_feed(wdt, MAX_INSTALLABLE_TIMEOUTS);
	zassert_true(ret == -EINVAL,
		     "wdt_feed(%d) shall return -EINVAL (-22), got unexpected value of %d",
		     MAX_INSTALLABLE_TIMEOUTS, ret);

	/* Assumption: wdt_disable() is called after each test */
}

/**
 * @brief wdt_feed() negative test
 *
 * Confirm that wdt_feed() returns
 * -EAGAIN when completing the feed operation would stall the caller, for example
 * due to an in-progress watchdog operation such as a previous wdt_feed() call.
 *
 */
ZTEST(wdt_coverage, test_09c_wdt_feed_stall)
{
	int ret, ch_id, i;

	if (!(WDT_TEST_FLAGS & WDT_FEED_CAN_STALL)) {
		/* Skip this test because wdt_feed() can NOT stall. */
		ztest_test_skip();
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	ch_id = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ch_id >= 0, "Watchdog install error, got unexpected value of %d", ch_id);
	TC_PRINT("Configured WDT channel %d\n", ch_id);

	ret = wdt_setup(wdt, DEFAULT_OPTIONS);
	zassert_true(ret == 0, "Watchdog setup error, got unexpected value of %d", ret);
	TC_PRINT("Test has failed if there is reset after this line\n");

	for (i = 0; i < 5; i++) {
		ret = wdt_feed(wdt, ch_id);
		if (i == 0) {
			zassert_true(ret == 0, "wdt_feed error, got unexpected value of %d", ret);
		} else {
			zassert_true(
				ret == -EAGAIN,
				"wdt_feed shall return -EAGAIN (-11), got unexpected value of %d",
				ret);
		}
	}
}

/**
 * @brief wdt_install_timeout() negative test
 *
 * Confirm that wdt_install_timeout() returns
 * -ENOMEM when no more timeouts can be installed.
 *
 */
ZTEST(wdt_coverage, test_10_wdt_install_timeout_max_number_of_timeouts)
{
	int i, ret;

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = DEFAULT_FLAGS;
	m_cfg_wdt0.window.max = DEFAULT_WINDOW_MAX;
	m_cfg_wdt0.window.min = DEFAULT_WINDOW_MIN;

	for (i = 0; i < MAX_INSTALLABLE_TIMEOUTS; i++) {
		ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
		/* Assumption - timeouts are counted from 0 to (MAX_INSTALLABLE_TIMEOUTS - 1) */
		zassert_true(ret < MAX_INSTALLABLE_TIMEOUTS,
			     "Watchdog install error, got unexpected value of %d", ret);
		TC_PRINT("Configured WDT channel %d\n", ret);
	}

	/* Call wdt_install_timeout again to test if error value is returned */
	ret = wdt_install_timeout(wdt, &m_cfg_wdt0);
	zassert_true(ret == -ENOMEM,
		     "wdt_install_timeout shall return -ENOMEM (-12), got unexpected value of %d",
		     ret);
}

static void *suite_setup(void)
{
	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("===================================================================\n");

	return NULL;
}

static void before_test(void *not_used)
{
	ARG_UNUSED(not_used);
	int ret_val;

	ret_val = device_is_ready(wdt);
	zassert_true(ret_val, "WDT device is not ready, got unexpected value of %d", ret_val);
}

static void cleanup_after_test(void *f)
{
	if (WDT_TEST_FLAGS & WDT_DISABLE_SUPPORTED) {
		/* Disable watchdog so it doesn't break other tests */
		wdt_disable(wdt);
	}
}

ZTEST_SUITE(wdt_coverage, NULL, suite_setup, before_test, cleanup_after_test, NULL);
