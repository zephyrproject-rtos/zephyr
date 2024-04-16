/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/**
 * @brief Test .
 *
 */
ZTEST(kconfigoptions, test_kconfigoptions_array_hex)
{
	zassert_equal(0x1, CONFIG_FLASH_CONTROLLER_ADDRESS,
		      "Unexpected flash controller address (%d)", CONFIG_FLASH_CONTROLLER_ADDRESS);
}

ZTEST(kconfigoptions, test_kconfigoptions_array_int)
{
	zassert_equal(4194304, CONFIG_FLASH_CONTROLLER_SIZE,
		      "Unexpected flash controller size (%d)", CONFIG_FLASH_CONTROLLER_SIZE);
}

ZTEST(kconfigoptions, test_kconfigoptions_addr_nodelabel)
{
	zassert_equal(1, CONFIG_FLASH_CONTROLLER_ADDR_INT_NODELABEL, "Unexpected value");
	zassert_equal(1, CONFIG_FLASH_CONTROLLER_ADDR_HEX_NODELABEL, "Unexpected value");
}

ZTEST(kconfigoptions, test_kconfigoptions_size_nodelabel)
{
	zassert_equal(4194304, CONFIG_FLASH_CONTROLLER_SIZE_INT_NODELABEL, "Unexpected value");
	zassert_equal(4194304, CONFIG_FLASH_CONTROLLER_SIZE_HEX_NODELABEL, "Unexpected value");
}

ZTEST_SUITE(kconfigoptions, NULL, NULL, NULL, NULL, NULL);
