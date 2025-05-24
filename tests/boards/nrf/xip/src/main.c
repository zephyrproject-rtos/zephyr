/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#if defined(CONFIG_NORDIC_QSPI_NOR)
#include <zephyr/drivers/flash/nrf_qspi_nor.h>
#endif

#include "extflash.h"

#define EXTFLASH_NODE DT_NODELABEL(dut)

static const struct device *dev_flash = DEVICE_DT_GET(EXTFLASH_NODE);

static void check_extflash_string(void)
{
	TC_PRINT("Accessing extflash_string at %p: %s\n",
		extflash_string, extflash_string);
	zassert_equal(0, strcmp(extflash_string, EXPECTED_EXTFLASH_STRING));
}

static void xip_enable(bool enable)
{
#if defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(dev_flash, enable);
#endif
}

ZTEST(xip, test_xip_enable_disable)
{
	xip_enable(true);
	extflash_function1();
	check_extflash_string();
	xip_enable(false);

	/* This is to ensure that the next XIP access will result in a new
	 * transfer from the flash chip, as the required data will not be
	 * available in cache.
	 */
	k_sleep(K_MSEC(10));

	xip_enable(true);
	extflash_function2();
	check_extflash_string();
	xip_enable(false);
}

ZTEST(xip, test_xip_enabled_at_boot)
{
	if (!IS_ENABLED(CONFIG_NORDIC_QSPI_NOR_XIP)) {
		ztest_test_skip();
	}

	extflash_function1();
	check_extflash_string();

	xip_enable(true);
	extflash_function2();
	xip_enable(false);

	k_sleep(K_MSEC(10));

	/* XIP enabled at boot should stay active after it is temporarily
	 * enabled at runtime.
	 */
	extflash_function1();
	check_extflash_string();
}

static void third_xip_user(void)
{
	xip_enable(true);
	check_extflash_string();
	xip_enable(false);
}

static void second_xip_user(void)
{
	xip_enable(true);

	extflash_function2();

	third_xip_user();

	xip_enable(false);
}

ZTEST(xip, test_xip_multiple_users)
{
	xip_enable(true);

	extflash_function1();

	second_xip_user();

	extflash_function1();
	check_extflash_string();

	xip_enable(false);
}

ZTEST_SUITE(xip, NULL, NULL, NULL, NULL, NULL);
