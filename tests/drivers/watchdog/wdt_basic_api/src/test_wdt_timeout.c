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

#ifdef CONFIG_BOARD_ESP32
#include <soc/rtc_cntl_reg.h>
#else
#include <qm_soc_regs.h>
#endif
#include <watchdog.h>
#include <zephyr.h>
#include <ztest.h>

#ifdef CONFIG_BOARD_ESP32
#define WDT_DEV_NAME CONFIG_WDT_ESP32_DEVICE_NAME
#define MWDT1_GLOBAL_RST	0x08
#define RWDT_SYSTEM_RST		0x10
#else
#define WDT_DEV_NAME CONFIG_WDT_0_NAME
#endif

#ifdef CONFIG_BOARD_ESP32
static volatile u32_t *gp_retention_reg0 = (u32_t *)(RTC_CNTL_STORE0_REG);
static volatile u32_t *gp_retention_reg1 = (u32_t *)(RTC_CNTL_STORE1_REG);

static int wdt_reset_reason(void)
{
	/*Reading reset cause of PRO CPU*/
	volatile u32_t *wdt_rtc_rst_procpu =
		(u32_t *)(RTC_CNTL_RESET_STATE_REG);
	/* RTC_CNTL_RESET_CAUSE_PROCPU :bitpos:[5:0] */
	return *wdt_rtc_rst_procpu & 0x1F;
}
#endif

static void wdt_int_cb(struct device *wdt_dev)
{
	static u32_t wdt_int_cnt;

	TC_PRINT("%s: Invoked (%u)\n", __func__, ++wdt_int_cnt);
#ifdef CONFIG_BOARD_ESP32
	(*gp_retention_reg1)++;
#else
	QM_SCSS_GP->gps2++;
#endif
#ifdef INT_RESET
	/* Don't come out from the loopback to avoid have the interrupt
	 * cleared and the system will reset in interrupt reset mode
	 */
	while (1)
	;
#endif
}

static int test_wdt(u32_t timeout, enum wdt_mode r_mode)
{
	struct wdt_config config;
	struct device *wdt = device_get_binding(WDT_DEV_NAME);

#ifdef CONFIG_BOARD_ESP32
	/*Initialize retention registers to 0 on first boot*/
	if (wdt_reset_reason() == RWDT_SYSTEM_RST) {
		*gp_retention_reg0 = 0;
		*gp_retention_reg1 = 0;
	}
#endif
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

#ifdef CONFIG_BOARD_ESP32
	if (*gp_retention_reg0) {
		TC_PRINT("A Warm Reset shouldn't happen when Timer disabled\n");
		return TC_FAIL;
	}
	*gp_retention_reg0 = 1;
#else
	if (QM_SCSS_GP->gps3) {
		TC_PRINT("A Warm Reset shouldn't happen when Timer disabled\n");
		return TC_FAIL;
	}

	QM_SCSS_GP->gps3 = 1;
#endif
	/*
	 * Wait to check whether a Warm Reset will happen.
	 * The sleep time should be longer than Watchdog Timer
	 * timeout value. If a Warm Reset happens, gps3 won't be
	 * cleared and then when the program comes here again,
	 * it will notice this and return a failure.
	 */
	k_sleep(5000);

#ifdef CONFIG_BOARD_ESP32
	*gp_retention_reg0 = 0;
#else
	QM_SCSS_GP->gps3 = 0;
#endif

	/* 5. Verify watchdog trigger a Warm Reset */
	wdt_enable(wdt);

#ifdef CONFIG_BOARD_ESP32
	if (!*gp_retention_reg1 && (wdt_reset_reason() == RWDT_SYSTEM_RST)) {
		TC_PRINT("Waiting for WDT reset\n");
		*gp_retention_reg1 = 1;
		while (1)
			;
	} else {
		if (r_mode == WDT_MODE_INTERRUPT_RESET) {
			if ((*gp_retention_reg1 == 2) &&
				(wdt_reset_reason() == MWDT1_GLOBAL_RST)) {
				wdt_disable(wdt);
				return TC_PASS;
			}
		} else {
			if (*gp_retention_reg1 &&
				(wdt_reset_reason() == MWDT1_GLOBAL_RST)) {
				wdt_disable(wdt);
				return TC_PASS;
			}
		}
		TC_PRINT("gp_retention_reg1 = %d\n", *gp_retention_reg1);
		wdt_disable(wdt);
		return TC_FAIL;
	}

#else
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
#endif
}

/*
 * Choose WDT_2_26_CYCLES as watchdog timeout cycle, which will
 * timeout in 2.097s. Other timeout cycles are the same.
 */
void test_wdt_int_reset_26(void)
{
	zassert_true(test_wdt(WDT_2_26_CYCLES,
			     WDT_MODE_INTERRUPT_RESET) == TC_PASS, NULL);
#ifdef CONFIG_BOARD_ESP32
	*gp_retention_reg1 = 0;
	*gp_retention_reg0 = 0;
#else
	QM_SCSS_GP->gps2 = 0;
#endif
}

void test_wdt_reset_26(void)
{
	zassert_true(test_wdt(WDT_2_26_CYCLES,
			     WDT_MODE_RESET) == TC_PASS, NULL);
#ifdef CONFIG_BOARD_ESP32
	*gp_retention_reg1 = 0;
	*gp_retention_reg0 = 0;
#else
	QM_SCSS_GP->gps2 = 0;
#endif
}
