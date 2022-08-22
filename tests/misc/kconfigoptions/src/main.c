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
void test_kconfigoptions_array_hex(void)
{
	zassert_equal(0x1, CONFIG_FLASH_CONTROLLER_ADDRESS,
		      "Unexpected flash controller address (%d)", CONFIG_FLASH_CONTROLLER_ADDRESS);
}

void test_kconfigoptions_array_int(void)
{
	zassert_equal(4194304, CONFIG_FLASH_CONTROLLER_SIZE,
		      "Unexpected flash controller size (%d)", CONFIG_FLASH_CONTROLLER_SIZE);
}

void test_main(void)
{
	ztest_test_suite(kconfigoptions,
			 ztest_unit_test(test_kconfigoptions_array_hex),
			 ztest_unit_test(test_kconfigoptions_array_int)
			);
	ztest_run_test_suite(kconfigoptions);
}
