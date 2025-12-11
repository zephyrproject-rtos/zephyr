/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/ztest.h>

ZTEST(test_kconfig_functions, test_arithmetic)
{
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_ADD_10, 10);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_ADD_10_3, 10 + 3);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_ADD_10_3_2, 10 + 3 + 2);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_SUB_10, 10);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_SUB_10_3, 10 - 3);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_SUB_10_3_2, 10 - 3 - 2);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_MUL_10, 10);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_MUL_10_3, 10 * 3);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_MUL_10_3_2, 10 * 3 * 2);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_DIV_10, 10);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_DIV_10_3, 10 / 3);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_DIV_10_3_2, 10 / 3 / 2);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_MOD_10, 10);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_MOD_10_3, 10 % 3);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_MOD_10_3_2, 10 % 3 % 2);
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_INC_1, 1 + 1);
	zassert_str_equal(CONFIG_KCONFIG_ARITHMETIC_INC_1_1, "0x2,0x2");
	zassert_str_equal(CONFIG_KCONFIG_ARITHMETIC_INC_INC_1_1, "3,3");
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_DEC_1, 1 - 1);
	zassert_str_equal(CONFIG_KCONFIG_ARITHMETIC_DEC_1_1, "0x0,0x0");
	zassert_str_equal(CONFIG_KCONFIG_ARITHMETIC_DEC_DEC_1_1, "-1,-1");
	zassert_equal(CONFIG_KCONFIG_ARITHMETIC_ADD_INC_1_1, (1 + 1) + (1 + 1));
}

ZTEST(test_kconfig_functions, test_min_max)
{
	zassert_equal(CONFIG_KCONFIG_MIN_10, 10);
	zassert_equal(CONFIG_KCONFIG_MIN_10_3, MIN(10, 3));
	zassert_equal(CONFIG_KCONFIG_MIN_10_3_2, MIN(MIN(10, 3), 2));
	zassert_equal(CONFIG_KCONFIG_MAX_10, 10);
	zassert_equal(CONFIG_KCONFIG_MAX_10_3, MAX(10, 3));
	zassert_equal(CONFIG_KCONFIG_MAX_10_3_2, MAX(MAX(10, 3), 2));
}

ZTEST(test_kconfig_functions, test_dt_compat_enabled)
{
	zassert_true(IS_ENABLED(CONFIG_KCONFIG_VND_GPIO_ENABLED_Y));
	zassert_true(IS_ENABLED(CONFIG_KCONFIG_VND_CAN_CONTROLLER_ENABLED_Y));
	zassert_false(IS_ENABLED(CONFIG_KCONFIG_VND_PWM_ENABLED_N));
}

ZTEST(test_kconfig_functions, test_dt_num_compat_enabled)
{
	zassert_equal(CONFIG_KCONFIG_VND_GPIO_ENABLED_NUM_4, 4);
	zassert_equal(CONFIG_KCONFIG_VND_CAN_CONTROLLER_ENABLED_NUM_2, 2);
	zassert_equal(CONFIG_KCONFIG_VND_PWM_ENABLED_NUM_0, 0);
}

ZTEST_SUITE(test_kconfig_functions, NULL, NULL, NULL, NULL, NULL);
