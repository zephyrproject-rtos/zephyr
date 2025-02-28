/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_expr.h>

#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#pragma GCC diagnostic ignored "-Woverflow"

#define Z_EXPR_HEX_0xFFFFFFFE                                                                      \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  \
		1, 0
#define Z_EXPR_HEX_0x7FFFFFFF                                                                      \
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  \
		1, 1
#define Z_EXPR_HEX_0xFFFFFFFF                                                                      \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  \
		1, 1

/**
 * @brief Test the hex encoding.
 */
ZTEST(sys_util_expr, test_hex_encode)
{
	zassert_equal(0, EXPR_HEX_ENCODE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

	zassert_equal(1, EXPR_HEX_ENCODE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1));

	zassert_equal(0xAAAA5555, EXPR_HEX_ENCODE(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0,
						  1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1));

	zassert_equal(0xFFFFFFFE, EXPR_HEX_ENCODE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
						  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0));

	zassert_equal(0xFFFFFFFF, EXPR_HEX_ENCODE(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
						  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1));
}

/**
 * @brief Checks the expansion from a predefined hexadecimal number.
 */
ZTEST(sys_util_expr, test_hex)
{
	zassert_equal(0, EXPR_HEX_ENCODE(EXPR_HEX(0x0)));
	zassert_equal(1, EXPR_HEX_ENCODE(EXPR_HEX(0x1)));
	zassert_equal(0xffe, EXPR_HEX_ENCODE(EXPR_HEX(0xffe)));
	zassert_equal(0xfff, EXPR_HEX_ENCODE(EXPR_HEX(0xfff)));
}

/**
 * @brief Checks the expansion from a predefined decimal number.
 */
ZTEST(sys_util_expr, test_dec)
{
	zassert_equal(0, EXPR_HEX_ENCODE(EXPR_DEC(0)));
	zassert_equal(1, EXPR_HEX_ENCODE(EXPR_DEC(1)));
	zassert_equal(4094, EXPR_HEX_ENCODE(EXPR_DEC(4094)));
	zassert_equal(4095, EXPR_HEX_ENCODE(EXPR_DEC(4095)));
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
		      EXPR_HEX_ENCODE(EXPR_DEC(DT_REG_ADDR(DT_CHOSEN(zephyr_flash)))));
}

ZTEST_SUITE(sys_util_expr, NULL, NULL, NULL, NULL, NULL);
