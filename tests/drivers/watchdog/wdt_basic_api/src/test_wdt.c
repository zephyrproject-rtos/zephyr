/*
 * Copyright (c) 2018 Nordic Semiconductor.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_wdt_basic
 * @{
 * @defgroup t_wdt_timeout test_wdt_timeout_capabilities
 * @brief TestPurpose: verify Watchdog Timer install/setup/feed can work,
 *        and reset can be triggered when timeout
 * @details
 * There are three tests. Each test provide watchdog installation, setup and
 * wait for reset. Three variables are placed in noinit section to prevent
 * clearing them during board reset.These variables save number of the current
 * test case, current test state and value to check if test passed or not.
 *
 * - Test Steps - test_wdt_no_callback
 *   -# Get device.
 *   -# Check if the state was changed and test should be finished.
 *   -# Set callback to NULL value.
 *   -# Install watchdog with current configuration.
 *   -# Setup watchdog with no additions options.
 *   -# Wait for reset.
 * - Expected Results
 *   -# If reset comes, the same testcase should be executed but state should
 *      be set to finish value and test should return with success.
 *
 * - Test Steps - test_wdt_callback_1
 *   -# Get device.
 *   -# Check if the state was changed. If so check testvalue if interrupt
 *      occurred.
 *   -# Set callback as pointer to wdt_int_cb0.
 *   -# Install watchdog with current configuration.
 *   -# Setup watchdog with no additions options.
 *   -# Wait for reset.
 * - Expected Results
 *   -# If reset comes, the same testcase should be executed but state should be
 *      set to finish value and test checks if m_testvalue was set in interrupt
 *      right before reset.
 *
 * - Test Steps - test_wdt_callback_2
 *   -# Get device.
 *   -# Check if the state was changed. If so check testvalue if interrupt
 *      occurred.
 *   -# Install two watchdogs: set pointer to wdt_int_cb0 as callback for first
 *      watchdog and wdt_int_cb1 for second one.
 *   -# Install watchdog with current configuration.
 *   -# Setup watchdog with no additions options.
 *   -# Wait for reset and feed first watchdog.
 * - Expected Results
 *   -# If reset comes, the same testcase should be executed but state should be
 *      set to finish value and test checks if m_testvalue was set in callback
 *      of second watchdog right before reset.
 *
 * @}
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * To use this test, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 */
#if DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_window_watchdog)
#define WDT_NODE DT_INST(0, st_stm32_window_watchdog)
#define TIMEOUTS 0
#define WDT_TEST_MAX_WINDOW 200
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define WDT_NODE DT_INST(0, st_stm32_watchdog)
#define TIMEOUTS 0
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
#define WDT_NODE DT_INST(0, nordic_nrf_wdt)
#define TIMEOUTS 2
#elif DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_watchdog)
#define WDT_NODE DT_INST(0, espressif_esp32_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(silabs_gecko_wdog)
#define WDT_NODE DT_INST(0, silabs_gecko_wdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_wdog32)
#define WDT_NODE DT_INST(0, nxp_kinetis_wdog32)
#elif DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_watchdog)
#define WDT_NODE DT_INST(0, microchip_xec_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_watchdog)
#define WDT_NODE DT_INST(0, nuvoton_npcx_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(ti_cc32xx_watchdog)
#define WDT_NODE DT_INST(0, ti_cc32xx_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_imx_wdog)
#define WDT_NODE DT_INST(0, nxp_imx_wdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_wwdgt)
#define WDT_NODE DT_INST(0, gd_gd32_wwdgt)
#elif DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_fwdgt)
#define WDT_NODE DT_INST(0, gd_gd32_fwdgt)
#elif DT_HAS_COMPAT_STATUS_OKAY(zephyr_counter_watchdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_counter_watchdog)
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_watchdog)
#define WDT_TEST_MAX_WINDOW 20000U
#define TIMEOUTS 0
#endif

#define WDT_TEST_STATE_IDLE        0
#define WDT_TEST_STATE_CHECK_RESET 1

#define WDT_TEST_CB0_TEST_VALUE    0x0CB0
#define WDT_TEST_CB1_TEST_VALUE    0x0CB1

#ifndef WDT_TEST_MAX_WINDOW
#define WDT_TEST_MAX_WINDOW                2000U
#endif

#ifndef TIMEOUTS
#define TIMEOUTS                   1
#endif

#define TEST_WDT_CALLBACK_1        (TIMEOUTS > 0)
#define TEST_WDT_CALLBACK_2        (TIMEOUTS > 1)


static struct wdt_timeout_cfg m_cfg_wdt0;
#if TEST_WDT_CALLBACK_2
static struct wdt_timeout_cfg m_cfg_wdt1;
#endif

#if defined(CONFIG_SOC_SERIES_STM32F7X) || defined(CONFIG_SOC_SERIES_STM32H7X)
/* STM32H7 and STM32F7 guarantee last write RAM retention over reset,
 * only for 64bits
 * See details in Application Note AN5342
 */
#define DATATYPE uint64_t
#else
#define DATATYPE uint32_t
#endif

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
#define NOINIT_SECTION ".dtcm_noinit.test_wdt"
#else
#define NOINIT_SECTION ".noinit.test_wdt"
#endif

/* m_state indicates state of particular test. Used to check whether testcase
 * should go to reset state or check other values after reset.
 */
volatile DATATYPE m_state __attribute__((section(NOINIT_SECTION)));

/* m_testcase_index is incremented after each test to make test possible
 * switch to next testcase.
 */
volatile DATATYPE m_testcase_index __attribute__((section(NOINIT_SECTION)));

/* m_testvalue contains value set in interrupt callback to point whether
 * first or second interrupt was fired.
 */
volatile DATATYPE m_testvalue __attribute__((section(NOINIT_SECTION)));

#if TEST_WDT_CALLBACK_1
static void wdt_int_cb0(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_testvalue += WDT_TEST_CB0_TEST_VALUE;
}
#endif

#if TEST_WDT_CALLBACK_2
static void wdt_int_cb1(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_testvalue += WDT_TEST_CB1_TEST_VALUE;
}
#endif

static int test_wdt_no_callback(void)
{
	int err;
	const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);

	if (!device_is_ready(wdt)) {
		TC_PRINT("WDT device is not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("Testcase: %s\n", __func__);

	if (m_state == WDT_TEST_STATE_CHECK_RESET) {
		m_state = WDT_TEST_STATE_IDLE;
		m_testcase_index = 1U;
		TC_PRINT("Testcase passed\n");
		return TC_PASS;
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = WDT_TEST_MAX_WINDOW;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);
	if (err < 0) {
		TC_PRINT("Watchdog install error\n");
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err < 0) {
		TC_PRINT("Watchdog setup error\n");
	}

	TC_PRINT("Waiting to restart MCU\n");
	m_testvalue = 0U;
	m_state = WDT_TEST_STATE_CHECK_RESET;
	while (1) {
		k_yield();
	}
}

#if TEST_WDT_CALLBACK_1
static int test_wdt_callback_1(void)
{
	int err;
	const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);

	if (!device_is_ready(wdt)) {
		TC_PRINT("WDT device is not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("Testcase: %s\n", __func__);

	if (m_state == WDT_TEST_STATE_CHECK_RESET) {
		m_state = WDT_TEST_STATE_IDLE;
		m_testcase_index++;
		if (m_testvalue == WDT_TEST_CB0_TEST_VALUE) {
			TC_PRINT("Testcase passed\n");
			return TC_PASS;
		} else {
			return TC_FAIL;
		}
	}

	m_testvalue = 0U;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.callback = wdt_int_cb0;
	m_cfg_wdt0.window.max = WDT_TEST_MAX_WINDOW;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);
	if (err < 0) {
		if (err == -ENOTSUP) {
			TC_PRINT("CB1 not supported on platform\n");
			m_testcase_index++;
			return TC_PASS;

		}
		TC_PRINT("Watchdog install error\n");
		return TC_FAIL;
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err < 0) {
		TC_PRINT("Watchdog setup error\n");
		return TC_FAIL;
	}

	TC_PRINT("Waiting to restart MCU\n");
	m_testvalue = 0U;
	m_state = WDT_TEST_STATE_CHECK_RESET;
	while (1) {
		k_yield();
	}
}
#endif

#if TEST_WDT_CALLBACK_2
static int test_wdt_callback_2(void)
{
	int err;
	const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);

	if (!device_is_ready(wdt)) {
		TC_PRINT("WDT device is not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("Testcase: %s\n", __func__);

	if (m_state == WDT_TEST_STATE_CHECK_RESET) {
		m_state = WDT_TEST_STATE_IDLE;
		m_testcase_index++;
		if (m_testvalue == WDT_TEST_CB1_TEST_VALUE) {
			TC_PRINT("Testcase passed\n");
			return TC_PASS;
		} else {
			return TC_FAIL;
		}
	}


	m_testvalue = 0U;
	m_cfg_wdt0.callback = wdt_int_cb0;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = WDT_TEST_MAX_WINDOW;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);

	if (err < 0) {
		TC_PRINT("Watchdog install error\n");
		return TC_FAIL;
	}

	m_cfg_wdt1.callback = wdt_int_cb1;
	m_cfg_wdt1.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt1.window.max = WDT_TEST_MAX_WINDOW;
	err = wdt_install_timeout(wdt, &m_cfg_wdt1);
	if (err < 0) {
		TC_PRINT("Watchdog install error\n");
		return TC_FAIL;
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err < 0) {
		TC_PRINT("Watchdog setup error\n");
		return TC_FAIL;
	}

	TC_PRINT("Waiting to restart MCU\n");
	m_testvalue = 0U;
	m_state = WDT_TEST_STATE_CHECK_RESET;

	while (1) {
		wdt_feed(wdt, 0);
		k_sleep(K_MSEC(100));
	}
}
#endif

static int test_wdt_bad_window_max(void)
{
	int err;
	const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);

	if (!device_is_ready(wdt)) {
		TC_PRINT("WDT device is not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("Testcase: %s\n", __func__);

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = 0U;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);
	if (err == -EINVAL) {
		return TC_PASS;
	}

	return TC_FAIL;
}

ZTEST(wdt_basic_test_suite, test_wdt)
{
	if ((m_testcase_index != 1U) && (m_testcase_index != 2U)) {
		zassert_true(test_wdt_no_callback() == TC_PASS);
	}
	if (m_testcase_index == 1U) {
#if TEST_WDT_CALLBACK_1
		zassert_true(test_wdt_callback_1() == TC_PASS);
#else
		m_testcase_index++;
#endif
	}
	if (m_testcase_index == 2U) {
#if TEST_WDT_CALLBACK_2
		zassert_true(test_wdt_callback_2() == TC_PASS);
#else
		m_testcase_index++;
#endif
	}
	if (m_testcase_index == 3U) {
		zassert_true(test_wdt_bad_window_max() == TC_PASS);
		m_testcase_index++;
	}
	if (m_testcase_index > 3) {
		m_state = WDT_TEST_STATE_IDLE;
	}
}
