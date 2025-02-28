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
 * @brief Verify that the behavior of EXPR_DIV is equivalent to uint32_t division in C.
 */
ZTEST(sys_util_expr_division, test_div)
{
	zassert_equal(0x0 / 0x1, EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0x0), EXPR_HEX(0x1))));
	zassert_equal(0x1 / 0x1, EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0x1), EXPR_HEX(0x1))));
	zassert_equal(0x2 / 0x1, EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0x2), EXPR_HEX(0x1))));
	zassert_equal(0x2 / 0x2, EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0x2), EXPR_HEX(0x2))));
	zassert_equal(0x3 / 0x2, EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0x3), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFE / 0x1,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFE / 0x2,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x2))));
	zassert_equal(0x7FFFFFFF / 0x1,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0x7FFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0x7FFFFFFF / 0x2,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0x7FFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF / 0x1,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFF / 0x2,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF / 0x7,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x7))));
	zassert_equal(0xFFFFFFFF / 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0xFFFFFFFF))));
}

/**
 * @brief Verify that the behavior of EXPR_MOD is equivalent to uint32_t modulo in C.
 */
ZTEST(sys_util_expr_division, test_mod)
{
	zassert_equal(0x0 % 0x1, EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0x0), EXPR_HEX(0x1))));
	zassert_equal(0x1 % 0x1, EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0x1), EXPR_HEX(0x1))));
	zassert_equal(0x2 % 0x1, EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0x2), EXPR_HEX(0x1))));
	zassert_equal(0x2 % 0x2, EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0x2), EXPR_HEX(0x2))));
	zassert_equal(0x3 % 0x2, EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0x3), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFE % 0x1,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFE % 0x2,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x2))));
	zassert_equal(0x7FFFFFFF % 0x1,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0x7FFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0x7FFFFFFF % 0x2,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0x7FFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF % 0x1,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFF % 0x2,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF % 0x7,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x7))));
	zassert_equal(0xFFFFFFFF % 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0xFFFFFFFF))));
}

ZTEST_SUITE(sys_util_expr_division, NULL, NULL, NULL, NULL, NULL);
