/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_wdt_basic
 * @{
 * @defgroup t_wdt_timeout test_wdt_timeout_capabilities
 * @brief TestPurpose: verify Watchdog Timer enable/disable/set/get can work,
 *			and Warm Reset can be triggered when timeout
 * @details
 * - Test Steps
 *   -# Enable Watchdog Timer
 *   -# Configure timeout, mode and interrupt_fn for Watchdog Timer
 *   -# Fetch Watchdog Timer configurations and compare the settings
 *   -# Disable Watchdog Timer to disable the Warm Reset
 *   -# Enable Watchdog Timer again and wait fortimeout and Warm Reset
 *   -# Use General Purpose Sticky Scratchpad 2 (GPS2) as a flag to
 *      indicate whether a Warm Reset happened, because Warm Reset won't
 *      clear it
 * - Expected Results
 *   -# If response mode is configured as Interrupt Reset, the interrupt
 *      callback function will be called first when the first timeout comes.
 *      The second timeout will trigger a Warm Reset and GPS2 will be two.
 *   -# If response mode is configured as Reset, a Warm Reset will be
 *      triggered when timeout and GPS2 will be one.
 * @}
 */

#include <qm_soc_regs.h>
#include <watchdog.h>
#include <zephyr.h>
#include <ztest.h>

#define WDT_DEV_NAME CONFIG_WDT_0_NAME

static void wdt_int_cb(struct device *wdt_dev)
{
	static uint32_t wdt_int_cnt;

	TC_PRINT("%s: Invoked (%u)\n", __func__, ++wdt_int_cnt);
	QM_SCSS_GP->gps2++;

#ifdef INT_RESET
	/* Don't come out from the loopback to avoid have the interrupt
	 * cleared and the system will reset in interrupt reset mode
	 */
	while (1)
	;
#endif
}

static int test_wdt(uint32_t timeout, enum wdt_mode r_mode)
{
	struct wdt_config config;
	struct device *wdt = device_get_binding(WDT_DEV_NAME);

	if (!wdt) {
		TC_PRINT("Cannot get WDT device\n");
		return TC_FAIL;
	}

	config.timeout = timeout;
	config.mode = r_mode;
	if (r_mode == WDT_MODE_INTERRUPT_RESET) {
		config.interrupt_fn = wdt_int_cb;
	} else {
		config.interrupt_fn = NULL;
	}

	/* 1. Verify wdt_enable() */
	wdt_enable(wdt);

	/* 2. Verify wdt_set_config() */
	if (wdt_set_config(wdt, &config)) {
		TC_PRINT("Fail to configure WDT device\n");
		wdt_disable(wdt);
		return TC_FAIL;
	}

	/* 3. Verify wdt_get_config() */
	wdt_get_config(wdt, &config);

	if (timeout != config.timeout || r_mode != config.mode) {
		TC_PRINT("Fetch config doesn't match with set config\n");
		TC_PRINT("timeout = %d, config.timeout = %d", timeout,
			 config.timeout);
		TC_PRINT("r_mode = %d, config.mode = %d", r_mode, config.mode);
		wdt_disable(wdt);
		return TC_FAIL;
	}

	/* 4. Verify wdt_disable(), and Warm Reset won't happen */
	wdt_disable(wdt);

	if (QM_SCSS_GP->gps3) {
		TC_PRINT("A Warm Reset shouldn't happen when Timer disabled\n");
		return TC_FAIL;
	}

	QM_SCSS_GP->gps3 = 1;

	/*
	 * Wait to check whether a Warm Reset will happen.
	 * The sleep time should be longer than Watchdog Timer
	 * timeout value. If a Warm Reset happens, gps3 won't be
	 * cleared and then when the program comes here again,
	 * it will notice this and return a failure.
	 */
	k_sleep(5000);

	QM_SCSS_GP->gps3 = 0;

	/* 5. Verify watchdog trigger a Warm Reset */
	wdt_enable(wdt);

	if (!QM_SCSS_GP->gps2) {
		TC_PRINT("Waiting for WDT reset\n");
		QM_SCSS_GP->gps2 = 1;
		while (1)
			;
	} else {
		if (r_mode == WDT_MODE_INTERRUPT_RESET) {
			if (QM_SCSS_GP->gps2 == 2) {
				wdt_disable(wdt);
				return TC_PASS;
			}
		} else {
			if (QM_SCSS_GP->gps2) {
				wdt_disable(wdt);
				return TC_PASS;
			}
		}
		TC_PRINT("QM_SCSS_GP->gps2 = %d\n", QM_SCSS_GP->gps2);
		wdt_disable(wdt);
		return TC_FAIL;
	}
}

/*
 * Choose WDT_2_26_CYCLES as watchdog timeout cycle, which will
 * timeout in 2.097s. Other timeout cycles are the same.
 */
void test_wdt_int_reset_26(void)
{
	assert_true(test_wdt(WDT_2_26_CYCLES,
			     WDT_MODE_INTERRUPT_RESET) == TC_PASS, NULL);
	QM_SCSS_GP->gps2 = 0;
}

void test_wdt_reset_26(void)
{
	assert_true(test_wdt(WDT_2_26_CYCLES,
			     WDT_MODE_RESET) == TC_PASS, NULL);
	QM_SCSS_GP->gps2 = 0;
}
