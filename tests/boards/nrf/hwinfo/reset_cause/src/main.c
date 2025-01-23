/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/cache.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(resetreason, LOG_LEVEL_INF);

static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static int my_wdt_channel;

#define NOINIT_SECTION ".noinit.test_wdt"
volatile uint32_t machine_state __attribute__((section(NOINIT_SECTION)));
volatile uint32_t supported __attribute__((section(NOINIT_SECTION)));
volatile uint32_t wdt_status __attribute__((section(NOINIT_SECTION)));
volatile uint32_t reboot_status __attribute__((section(NOINIT_SECTION)));

/* Value used to indicate that the watchdog has fired. */
#define WDT_HAS_FIRED (0x12345678U)
#define REBOOT_WAS_DONE (0x87654321U)

/* Highest value in the switch statement in the main() */
#define LAST_STATE (2)

static void wdt_int_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	wdt_status = WDT_HAS_FIRED;

	/* Flush cache as reboot may invalidate all lines. */
	sys_cache_data_flush_range((void *) &wdt_status, sizeof(wdt_status));
}

/* Print LOG delimiter. */
static void print_bar(void)
{
	LOG_INF("===================================================================");
}

static void print_supported_reset_cause(void)
{
	int32_t ret;

	/* Store supported reset causes in global variable placed at NOINIT_SECTION. */
	ret = hwinfo_get_supported_reset_cause((uint32_t *) &supported);

	/* Print which reset causes are supported. */
	if (ret == 0) {
		LOG_INF("Supported reset causes are:");
		if (supported & RESET_PIN) {
			LOG_INF(" 0: RESET_PIN is supported");
		} else {
			LOG_INF(" 0: no support for RESET_PIN");
		}
		if (supported & RESET_SOFTWARE) {
			LOG_INF(" 1: RESET_SOFTWARE is supported");
		} else {
			LOG_INF(" 1: no support for RESET_SOFTWARE");
		}
		if (supported & RESET_BROWNOUT) {
			LOG_INF(" 2: RESET_BROWNOUT is supported");
		} else {
			LOG_INF(" 2: no support for RESET_BROWNOUT");
		}
		if (supported & RESET_POR) {
			LOG_INF(" 3: RESET_POR is supported");
		} else {
			LOG_INF(" 3: no support for RESET_POR");
		}
		if (supported & RESET_WATCHDOG) {
			LOG_INF(" 4: RESET_WATCHDOG is supported");
		} else {
			LOG_INF(" 4: no support for RESET_WATCHDOG");
		}
		if (supported & RESET_DEBUG) {
			LOG_INF(" 5: RESET_DEBUG is supported");
		} else {
			LOG_INF(" 5: no support for RESET_DEBUG");
		}
		if (supported & RESET_SECURITY) {
			LOG_INF(" 6: RESET_SECURITY is supported");
		} else {
			LOG_INF(" 6: no support for RESET_SECURITY");
		}
		if (supported & RESET_LOW_POWER_WAKE) {
			LOG_INF(" 7: RESET_LOW_POWER_WAKE is supported");
		} else {
			LOG_INF(" 7: no support for RESET_LOW_POWER_WAKE");
		}
		if (supported & RESET_CPU_LOCKUP) {
			LOG_INF(" 8: RESET_CPU_LOCKUP is supported");
		} else {
			LOG_INF(" 8: no support for RESET_CPU_LOCKUP");
		}
		if (supported & RESET_PARITY) {
			LOG_INF(" 9: RESET_PARITY is supported");
		} else {
			LOG_INF(" 9: no support for RESET_PARITY");
		}
		if (supported & RESET_PLL) {
			LOG_INF("10: RESET_PLL is supported");
		} else {
			LOG_INF("10: no support for RESET_PLL");
		}
		if (supported & RESET_CLOCK) {
			LOG_INF("11: RESET_CLOCK is supported");
		} else {
			LOG_INF("11: no support for RESET_CLOCK");
		}
		if (supported & RESET_HARDWARE) {
			LOG_INF("12: RESET_HARDWARE is supported");
		} else {
			LOG_INF("12: no support for RESET_HARDWARE");
		}
		if (supported & RESET_USER) {
			LOG_INF("13: RESET_USER is supported");
		} else {
			LOG_INF("13: no support for RESET_USER");
		}
		if (supported & RESET_TEMPERATURE) {
			LOG_INF("14: RESET_TEMPERATURE is supported");
		} else {
			LOG_INF("14: no support for RESET_TEMPERATURE");
		}
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_supported_reset_cause() is NOT supported");
		supported = 0;
	} else {
		LOG_ERR("hwinfo_get_supported_reset_cause() failed (ret = %d)", ret);
	}
	sys_cache_data_flush_range((void *) &supported, sizeof(supported));
	print_bar();
}

/* Print current value of reset cause. */
static void print_current_reset_cause(uint32_t *cause)
{
	int32_t ret;

	ret = hwinfo_get_reset_cause(cause);
	if (ret == 0) {
		LOG_INF("Current reset cause is:");
		if (*cause & RESET_PIN) {
			LOG_INF(" 0: reset due to RESET_PIN");
		}
		if (*cause & RESET_SOFTWARE) {
			LOG_INF(" 1: reset due to RESET_SOFTWARE");
		}
		if (*cause & RESET_BROWNOUT) {
			LOG_INF(" 2: reset due to RESET_BROWNOUT");
		}
		if (*cause & RESET_POR) {
			LOG_INF(" 3: reset due to RESET_POR");
		}
		if (*cause & RESET_WATCHDOG) {
			LOG_INF(" 4: reset due to RESET_WATCHDOG");
		}
		if (*cause & RESET_DEBUG) {
			LOG_INF(" 5: reset due to RESET_DEBUG");
		}
		if (*cause & RESET_SECURITY) {
			LOG_INF(" 6: reset due to RESET_SECURITY");
		}
		if (*cause & RESET_LOW_POWER_WAKE) {
			LOG_INF(" 7: reset due to RESET_LOW_POWER_WAKE");
		}
		if (*cause & RESET_CPU_LOCKUP) {
			LOG_INF(" 8: reset due to RESET_CPU_LOCKUP");
		}
		if (*cause & RESET_PARITY) {
			LOG_INF(" 9: reset due to RESET_PARITY");
		}
		if (*cause & RESET_PLL) {
			LOG_INF("10: reset due to RESET_PLL");
		}
		if (*cause & RESET_CLOCK) {
			LOG_INF("11: reset due to RESET_CLOCK");
		}
		if (*cause & RESET_HARDWARE) {
			LOG_INF("12: reset due to RESET_HARDWARE");
		}
		if (*cause & RESET_USER) {
			LOG_INF("13: reset due to RESET_USER");
		}
		if (*cause & RESET_TEMPERATURE) {
			LOG_INF("14: reset due to RESET_TEMPERATURE");
		}
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_reset_cause() is NOT supported");
		*cause = 0;
	} else {
		LOG_ERR("hwinfo_get_reset_cause() failed (ret = %d)", ret);
	}
	print_bar();
}

/* Clear reset cause. */
static void test_clear_reset_cause(void)
{
	int32_t ret, temp;

	ret = hwinfo_clear_reset_cause();
	if (ret == 0) {
		LOG_INF("hwinfo_clear_reset_cause() was executed");
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_reset_cause() is NOT supported");
	} else {
		LOG_ERR("hwinfo_get_reset_cause() failed (ret = %d)", ret);
	}
	print_bar();

	/* Print current value of reset causes -> expected 0 */
	print_current_reset_cause(&temp);
	LOG_INF("TEST that all reset causes were cleared");
	if (temp == 0) {
		LOG_INF("PASS: reset causes were cleared");
	} else {
		LOG_ERR("FAIL: reset case = %u while expected is 0", temp);
	}
	print_bar();
}

void test_reset_software(uint32_t cause)
{
	/* Check that reset cause from sys_reboot is detected. */
	if (supported & RESET_SOFTWARE) {
		if (reboot_status != REBOOT_WAS_DONE) {
			/* If software reset hasn't happen yet, do it. */
			reboot_status = REBOOT_WAS_DONE;
			LOG_INF("Test RESET_SOFTWARE - Rebooting");

			/* Flush cache as reboot may invalidate all lines. */
			sys_cache_data_flush_range((void *) &machine_state, sizeof(machine_state));
			sys_cache_data_flush_range((void *) &reboot_status, sizeof(reboot_status));
			sys_reboot(SYS_REBOOT_COLD);
		} else {
			/* Software reset was done */
			LOG_INF("TEST that RESET_SOFTWARE was detected");
			if (cause & RESET_SOFTWARE) {
				LOG_INF("PASS: RESET_SOFTWARE detected");
				print_bar();
				/* Check RESET_SOFTWARE can be cleared */
				test_clear_reset_cause();
			} else {
				LOG_ERR("FAIL: RESET_SOFTWARE not set");
				print_bar();
			}
			/* Cleanup */
			reboot_status = 0;
			sys_cache_data_flush_range((void *) &reboot_status, sizeof(reboot_status));
		}
	}
}

void test_reset_watchdog(uint32_t cause)
{
	int32_t ret;

	/* Check that reset cause from expired watchdog is detected. */
	if (supported & RESET_WATCHDOG) {
		if (wdt_status != WDT_HAS_FIRED) {
			/* If watchdog hasn't fired yet, configure it do so. */
			uint32_t watchdog_window = 2000U;

			if (!device_is_ready(my_wdt_device)) {
				LOG_ERR("WDT device %s is not ready", my_wdt_device->name);
				return;
			}

			m_cfg_wdt0.callback = wdt_int_cb;
			m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
			m_cfg_wdt0.window.max = watchdog_window;
			m_cfg_wdt0.window.min = 0U;
			my_wdt_channel = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
			if (my_wdt_channel < 0) {
				LOG_ERR("wdt_install_timeout() returned %d", my_wdt_channel);
				return;
			}

			ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
			if (ret < 0) {
				LOG_ERR("wdt_setup() returned %d", ret);
				return;
			}

			/* Flush cache as reboot may invalidate all lines. */
			sys_cache_data_flush_range((void *) &machine_state, sizeof(machine_state));
			LOG_INF("Watchdog shall fire in ~%u miliseconds", watchdog_window);
			print_bar();
			k_sleep(K_FOREVER);
		} else {
			/* Watchdog has fired. */
			LOG_INF("TEST that RESET_WATCHDOG was detected");
			if (cause & RESET_WATCHDOG) {
				LOG_INF("PASS: RESET_WATCHDOG detected");
				print_bar();
				/* Check RESET_WATCHDOG can be cleared */
				test_clear_reset_cause();
			} else {
				LOG_ERR("FAIL: RESET_WATCHDOG not set");
				print_bar();
			}
			/* Cleanup */
			wdt_status = 0;
			sys_cache_data_flush_range((void *) &wdt_status, sizeof(wdt_status));
		}
	}
}

int main(void)
{
	uint32_t cause;

	LOG_INF("HW Info reset reason test on %s", CONFIG_BOARD_TARGET);
	if (wdt_status == WDT_HAS_FIRED) {
		LOG_INF("This boot is due to expected watchdog reset");
	}
	if (reboot_status == REBOOT_WAS_DONE) {
		LOG_INF("This boot is due to expected software reset");
	}
	print_bar();

	/* Test relies on REST_PIN to correctly start. */
	print_current_reset_cause(&cause);
	if (cause & RESET_PIN) {
		LOG_INF("TEST that RESET_PIN was detected");
		LOG_INF("PASS: RESET_PIN detected");
		print_bar();
		/* Check RESET_PIN can be cleared */
		test_clear_reset_cause();
		machine_state = 0;
		reboot_status = 0;
		wdt_status = 0;
	}

	while (machine_state <= LAST_STATE) {
		LOG_DBG("machine_state = %u", machine_state);
		LOG_DBG("reboot_status = %u", reboot_status);
		LOG_DBG("wdt_status = %u", wdt_status);

		switch (machine_state) {
		case 0: /* Print (an store) which reset causes are supported. */
			print_supported_reset_cause();
			machine_state++;
			break;
		case 1: /* Test RESET_SOFTWARE. */
			test_reset_software(cause);
			machine_state++;
		case 2: /* Test RESET_WATCHDOG. */
			test_reset_watchdog(cause);
			machine_state++;
		}
	}

	LOG_INF("All tests done");
	return 0;
}
