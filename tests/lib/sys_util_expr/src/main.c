/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_expr.h>
#include "test_bits.h"

/**
 * @brief Test the hex encoding.
 */
ZTEST(sys_util_expr, test_hex_encode)
{
	zassert_equal(0, EXPR_TO_NUM(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

	zassert_equal(1, EXPR_TO_NUM(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1));

	zassert_equal(0xAAAA5555, EXPR_TO_NUM(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1,
					      0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1));

	zassert_equal(0xFFFFFFFE, EXPR_TO_NUM(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
					      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0));

	zassert_equal(0xFFFFFFFF, EXPR_TO_NUM(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
					      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1));
}

/**
 * @brief Checks the expansion from a predefined hexadecimal number.
 */
ZTEST(sys_util_expr, test_hex)
{
	zassert_equal(0, EXPR_TO_NUM(TEST_BITS_0));
	zassert_equal(1, EXPR_TO_NUM(TEST_BITS_1));
	zassert_equal(0xffe, EXPR_TO_NUM(TEST_BITS_FFE));
	zassert_equal(0xfff, EXPR_TO_NUM(TEST_BITS_FFF));
}

/**
 * @brief Checks the expansion from a predefined decimal number.
 */
ZTEST(sys_util_expr, test_dec)
{
	zassert_equal(0, EXPR_TO_NUM(TEST_BITS_0));
	zassert_equal(1, EXPR_TO_NUM(TEST_BITS_1));
	zassert_equal(4094, EXPR_TO_NUM(TEST_BITS_FFE));
	zassert_equal(4095, EXPR_TO_NUM(TEST_BITS_FFF));
}

/**
 * @brief Check if DeviceTree values ​​are accessible
 */
ZTEST(sys_util_expr, test_dt_value)
{
	if (!DT_HAS_CHOSEN(zephyr_flash)) {
		ztest_test_skip();
	}

	zassert_equal(DT_REG_ADDR(DT_CHOSEN(zephyr_flash)),
		      EXPR_TO_NUM(EXPR_BITS(DT_REG_ADDR(DT_CHOSEN(zephyr_flash)))));
}

ZTEST_SUITE(sys_util_expr, NULL, NULL, NULL, NULL, NULL);
