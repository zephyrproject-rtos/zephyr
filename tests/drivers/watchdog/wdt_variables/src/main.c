/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_vars, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/watchdog.h>

#define WDT_WINDOW_MAX	(500)

/* Watchdog related variables */
static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;

/* No init section will contain WDT_HAS_FIRED if watchdog has fired */
#define WDT_HAS_FIRED	(12345678U)
#define TEST_VALUE		(2U)

#define NOINIT_SECTION ".noinit.test_wdt"
static volatile uint32_t wdt_status __attribute__((section(NOINIT_SECTION)));

/* Global variables to verify */
static int global_tmp_0;
static int global_tmp_1 = TEST_VALUE;


static void wdt_int_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	wdt_status = WDT_HAS_FIRED;

	/* Flush cache as reboot may invalidate all lines. */
	sys_cache_data_flush_range((void *) &wdt_status, sizeof(wdt_status));
}

int main(void)
{
	int ret;
	static int tmp_0;
	static int tmp_1 = TEST_VALUE;

	LOG_INF("wdt_variables test on %s", CONFIG_BOARD_TARGET);

	global_tmp_0++;
	global_tmp_1++;
	tmp_0++;
	tmp_1++;

	LOG_DBG("global_tmp_0 = %d", global_tmp_0);
	LOG_DBG("global_tmp_1 = %d", global_tmp_1);
	LOG_DBG("tmp_0 = %d", tmp_0);
	LOG_DBG("tmp_1 = %d", tmp_1);

	/* When watchdog fires, variable wdt_status is set to the value of WDT_HAS_FIRED
	 * in WDT callback wdt_int_cb(). Then, target is reset.
	 * Check value of wdt_status to prevent reset loop.
	 */
	if (wdt_status != WDT_HAS_FIRED) {

		LOG_INF("Reset wasn't due to watchdog.");

		if (!device_is_ready(my_wdt_device)) {
			LOG_ERR("WDT device %s is not ready", my_wdt_device->name);
			return 1;
		}

		/* Configure Watchdog */
		m_cfg_wdt0.callback = wdt_int_cb;
		m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
		m_cfg_wdt0.window.min = 0U;
		m_cfg_wdt0.window.max = WDT_WINDOW_MAX;
		ret = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
		if (ret < 0) {
			LOG_ERR("wdt_install_timeout() returned %d", ret);
			return 1;
		}

		/* Start Watchdog */
		ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
		if (ret < 0) {
			LOG_ERR("wdt_setup() returned %d", ret);
			return 1;
		}

		LOG_INF("Watchdog shall fire in ~%u milliseconds", WDT_WINDOW_MAX);
		k_sleep(K_FOREVER);
	} else {
		bool test_passing = true;

		LOG_INF("Watchod has fired");

		if (global_tmp_0 != 1) {
			LOG_ERR("global_tmp_0 is %d instead of 1", global_tmp_0);
			test_passing = false;
		}

		if (global_tmp_1 != (TEST_VALUE + 1)) {
			LOG_ERR("global_tmp_1 is %d instead of %d", global_tmp_1, TEST_VALUE + 1);
			test_passing = false;
		}

		if (tmp_0 != 1) {
			LOG_ERR("tmp_0 is %d instead of 1", tmp_0);
			test_passing = false;
		}

		if (tmp_1 != (TEST_VALUE + 1)) {
			LOG_ERR("tmp_1 is %d instead of %d", tmp_1, TEST_VALUE + 1);
			test_passing = false;
		}

		/* Cleanup */
		wdt_status = 0;
		sys_cache_data_flush_range((void *) &wdt_status, sizeof(wdt_status));

		if (test_passing) {
			LOG_INF("Test completed successfully");
		} else {
			LOG_INF("Test failed");
		}
	}

	return 0;
}
