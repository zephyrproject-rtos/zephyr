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

#include <drivers/watchdog.h>
#include <zephyr.h>
#include <ztest.h>
#include "test_wdt.h"

#ifdef CONFIG_WDT_0_NAME
#define WDT_DEV_NAME CONFIG_WDT_0_NAME
#else
#ifdef CONFIG_WWDG_STM32
#define WDT_DEV_NAME DT_WWDT_0_NAME
#else
#define WDT_DEV_NAME DT_WDT_0_NAME
#endif
#endif

#define WDT_TEST_STATE_IDLE        0
#define WDT_TEST_STATE_CHECK_RESET 1

#define WDT_TEST_CB0_TEST_VALUE    0x0CB0
#define WDT_TEST_CB1_TEST_VALUE    0x0CB1

#ifdef CONFIG_WDT_NRFX
#define TIMEOUTS                   2
#elif defined(CONFIG_IWDG_STM32)
#define TIMEOUTS                   0
#else
#define TIMEOUTS                   1
#endif

#define TEST_WDT_CALLBACK_1        (TIMEOUTS > 0)
#define TEST_WDT_CALLBACK_2        (TIMEOUTS > 1)


static struct wdt_timeout_cfg m_cfg_wdt0;
#if TEST_WDT_CALLBACK_2
static struct wdt_timeout_cfg m_cfg_wdt1;
#endif

/* m_state indicates state of particular test. Used to check whether testcase
 * should go to reset state or check other values after reset.
 */
volatile uint32_t m_state __attribute__((section(".noinit.test_wdt")));

/* m_testcase_index is incremented after each test to make test possible
 * switch to next testcase.
 */
volatile uint32_t m_testcase_index __attribute__((section(".noinit.test_wdt")));

/* m_testvalue contains value set in interrupt callback to point whether
 * first or second interrupt was fired.
 */
volatile uint32_t m_testvalue __attribute__((section(".noinit.test_wdt")));

#if TEST_WDT_CALLBACK_1
static void wdt_int_cb0(struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_testvalue += WDT_TEST_CB0_TEST_VALUE;
}
#endif

#if TEST_WDT_CALLBACK_2
static void wdt_int_cb1(struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	m_testvalue += WDT_TEST_CB1_TEST_VALUE;
}
#endif

static int test_wdt_no_callback(void)
{
	int err;
	struct device *wdt = device_get_binding(WDT_DEV_NAME);

	if (!wdt) {
		TC_PRINT("Cannot get WDT device\n");
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
	m_cfg_wdt0.window.max = 2000U;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);
	if (err < 0) {
		TC_PRINT("Watchdog install error\n");
	}

	err = wdt_setup(wdt, 0);
	if (err < 0) {
		TC_PRINT("Watchdog setup error\n");
	}

	TC_PRINT("Waiting to restart MCU\n");
	m_testvalue = 0U;
	m_state = WDT_TEST_STATE_CHECK_RESET;
	while (1) {
		k_yield();
	};
}

#if TEST_WDT_CALLBACK_1
static int test_wdt_callback_1(void)
{
	int err;
	struct device *wdt = device_get_binding(WDT_DEV_NAME);

	if (!wdt) {
		TC_PRINT("Cannot get WDT device\n");
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
	m_cfg_wdt0.window.max = 2000U;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);
	if (err < 0) {
		TC_PRINT("Watchdog install error\n");
		return TC_FAIL;
	}

	err = wdt_setup(wdt, 0);
	if (err < 0) {
		TC_PRINT("Watchdog setup error\n");
		return TC_FAIL;
	}

	TC_PRINT("Waiting to restart MCU\n");
	m_testvalue = 0U;
	m_state = WDT_TEST_STATE_CHECK_RESET;
	while (1) {
		k_yield();
	};
}
#endif

#if TEST_WDT_CALLBACK_2
static int test_wdt_callback_2(void)
{
	int err;
	struct device *wdt = device_get_binding(WDT_DEV_NAME);

	if (!wdt) {
		TC_PRINT("Cannot get WDT device\n");
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
	m_cfg_wdt0.window.max = 2000U;
	err = wdt_install_timeout(wdt, &m_cfg_wdt0);

	if (err < 0) {
		TC_PRINT("Watchdog install error\n");
		return TC_FAIL;
	}

	m_cfg_wdt1.callback = wdt_int_cb1;
	m_cfg_wdt1.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt1.window.max = 2000U;
	err = wdt_install_timeout(wdt, &m_cfg_wdt1);
	if (err < 0) {
		TC_PRINT("Watchdog install error\n");
		return TC_FAIL;
	}

	err = wdt_setup(wdt, 0);
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
	};
}
#endif

static int test_wdt_bad_window_max(void)
{
	int err;
	struct device *wdt = device_get_binding(WDT_DEV_NAME);

	if (!wdt) {
		TC_PRINT("Cannot get WDT device\n");
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

void test_wdt(void)
{
	if ((m_testcase_index != 1U) && (m_testcase_index != 2U)) {
		zassert_true(test_wdt_no_callback() == TC_PASS, NULL);
	}
	if (m_testcase_index == 1U) {
#if TEST_WDT_CALLBACK_1
		zassert_true(test_wdt_callback_1() == TC_PASS, NULL);
#else
		m_testcase_index++;
#endif
	}
	if (m_testcase_index == 2U) {
#if TEST_WDT_CALLBACK_2
		zassert_true(test_wdt_callback_2() == TC_PASS, NULL);
#else
		m_testcase_index++;
#endif
	}
	if (m_testcase_index == 3U) {
		zassert_true(test_wdt_bad_window_max() == TC_PASS, NULL);
		m_testcase_index++;
	}
	if (m_testcase_index > 3) {
		m_state = WDT_TEST_STATE_IDLE;
	}
}
