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
 * @brief Verify that the behavior of EXPR_EQ is equivalent to == in C.
 */
ZTEST(sys_util_expr_compare, test_eq)
{
	zassert_equal(0x0 == 0x0, EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0x0), EXPR_HEX(0x0))));
	zassert_equal(0x0 == 0x1, EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0x0), EXPR_HEX(0x1))));
	zassert_equal(0x1 == 0x0, EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0x1), EXPR_HEX(0x0))));
	zassert_equal(0x1 == 0x1, EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0x1), EXPR_HEX(0x1))));
	zassert_equal(0x2 == 0x1, EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0x2), EXPR_HEX(0x1))));
	zassert_equal(0x2 == 0x2, EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0x2), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFE == 0x1,
		      EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFE == 0x2,
		      EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF == 0x0,
		      EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x0))));
	zassert_equal(0xFFFFFFFF == 0x1,
		      EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFF == 0x2,
		      EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0x0 == 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0x0), EXPR_HEX(0xFFFFFFFF))));
	zassert_equal(0xFFFFFFFF == 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0xFFFFFFFF))));
}

/**
 * @brief Verify that the behavior of EXPR_GT is equivalent to > in C.
 */
ZTEST(sys_util_expr_compare, test_gt)
{
	zassert_equal(0x0 > 0x0, EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0x0), EXPR_HEX(0x0))));
	zassert_equal(0x0 > 0x1, EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0x0), EXPR_HEX(0x1))));
	zassert_equal(0x1 > 0x0, EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0x1), EXPR_HEX(0x0))));
	zassert_equal(0x1 > 0x1, EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0x1), EXPR_HEX(0x1))));
	zassert_equal(0x2 > 0x1, EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0x2), EXPR_HEX(0x1))));
	zassert_equal(0x2 > 0x2, EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0x2), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFE > 0x1,
		      EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFE > 0x2,
		      EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF > 0x0,
		      EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x0))));
	zassert_equal(0xFFFFFFFF > 0x1,
		      EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFF > 0x2,
		      EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0x0 > 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0x0), EXPR_HEX(0xFFFFFFFF))));
	zassert_equal(0xFFFFFFFF > 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0xFFFFFFFF))));
}

/**
 * @brief Verify that the behavior of EXPR_LT is equivalent to < in C.
 */
ZTEST(sys_util_expr_compare, test_lt)
{
	zassert_equal(0x0 < 0x0, EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0x0), EXPR_HEX(0x0))));
	zassert_equal(0x0 < 0x1, EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0x0), EXPR_HEX(0x1))));
	zassert_equal(0x1 < 0x0, EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0x1), EXPR_HEX(0x0))));
	zassert_equal(0x1 < 0x1, EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0x1), EXPR_HEX(0x1))));
	zassert_equal(0x2 < 0x1, EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0x2), EXPR_HEX(0x1))));
	zassert_equal(0x2 < 0x2, EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0x2), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFE < 0x1,
		      EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFE < 0x2,
		      EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF < 0x0,
		      EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x0))));
	zassert_equal(0xFFFFFFFF < 0x1,
		      EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFF < 0x2,
		      EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0x0 < 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0x0), EXPR_HEX(0xFFFFFFFF))));
	zassert_equal(0xFFFFFFFF < 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0xFFFFFFFF))));
}

/**
 * @brief Verify that the behavior of EXPR_GTE is equivalent to >= in C.
 */
ZTEST(sys_util_expr_compare, test_gte)
{
	zassert_equal(0x0 >= 0x0, EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0x0), EXPR_HEX(0x0))));
	zassert_equal(0x0 >= 0x1, EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0x0), EXPR_HEX(0x1))));
	zassert_equal(0x1 >= 0x0, EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0x1), EXPR_HEX(0x0))));
	zassert_equal(0x1 >= 0x1, EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0x1), EXPR_HEX(0x1))));
	zassert_equal(0x2 >= 0x1, EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0x2), EXPR_HEX(0x1))));
	zassert_equal(0x2 >= 0x2, EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0x2), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFE >= 0x1,
		      EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFE >= 0x2,
		      EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF >= 0x0,
		      EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x0))));
	zassert_equal(0xFFFFFFFF >= 0x1,
		      EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFF >= 0x2,
		      EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0x0 >= 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0x0), EXPR_HEX(0xFFFFFFFF))));
	zassert_equal(0xFFFFFFFF >= 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0xFFFFFFFF))));
}

/**
 * @brief Verify that the behavior of EXPR_LTE is equivalent to <= in C.
 */
ZTEST(sys_util_expr_compare, test_lte)
{
	zassert_equal(0x0 <= 0x0, EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0x0), EXPR_HEX(0x0))));
	zassert_equal(0x0 <= 0x1, EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0x0), EXPR_HEX(0x1))));
	zassert_equal(0x1 <= 0x0, EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0x1), EXPR_HEX(0x0))));
	zassert_equal(0x1 <= 0x1, EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0x1), EXPR_HEX(0x1))));
	zassert_equal(0x2 <= 0x1, EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0x2), EXPR_HEX(0x1))));
	zassert_equal(0x2 <= 0x2, EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0x2), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFE <= 0x1,
		      EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFE <= 0x2,
		      EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0xFFFFFFFE), EXPR_HEX(0x2))));
	zassert_equal(0xFFFFFFFF <= 0x0,
		      EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x0))));
	zassert_equal(0xFFFFFFFF <= 0x1,
		      EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x1))));
	zassert_equal(0xFFFFFFFF <= 0x2,
		      EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0x2))));
	zassert_equal(0x0 <= 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0x0), EXPR_HEX(0xFFFFFFFF))));
	zassert_equal(0xFFFFFFFF <= 0xFFFFFFFF,
		      EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(0xFFFFFFFF), EXPR_HEX(0xFFFFFFFF))));
}

ZTEST_SUITE(sys_util_expr_compare, NULL, NULL, NULL, NULL, NULL);
