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
 * @brief Verify that the behavior of EXPR_EQ is equivalent to == in C.
 */
ZTEST(sys_util_expr_compare, test_eq)
{
	zassert_equal(0x0 == 0x0, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 == 0x1, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 == 0x0, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 == 0x1, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 == 0x1, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0x2 == 0x2, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_2, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFE == 0x1, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE == 0x2, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF == 0x0, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF == 0x1, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF == 0x2, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0x0 == 0xFFFFFFFF, EXPR_TO_NUM(EXPR_EQ(TEST_BITS_0, TEST_BITS_FFFFFFFF)));
	zassert_equal(0xFFFFFFFF == 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_EQ(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_GT is equivalent to > in C.
 */
ZTEST(sys_util_expr_compare, test_gt)
{
	zassert_equal(0x0 > 0x0, EXPR_TO_NUM(EXPR_GT(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 > 0x1, EXPR_TO_NUM(EXPR_GT(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 > 0x0, EXPR_TO_NUM(EXPR_GT(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 > 0x1, EXPR_TO_NUM(EXPR_GT(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 > 0x1, EXPR_TO_NUM(EXPR_GT(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0x2 > 0x2, EXPR_TO_NUM(EXPR_GT(TEST_BITS_2, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFE > 0x1, EXPR_TO_NUM(EXPR_GT(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE > 0x2, EXPR_TO_NUM(EXPR_GT(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF > 0x0, EXPR_TO_NUM(EXPR_GT(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF > 0x1, EXPR_TO_NUM(EXPR_GT(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF > 0x2, EXPR_TO_NUM(EXPR_GT(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0x0 > 0xFFFFFFFF, EXPR_TO_NUM(EXPR_GT(TEST_BITS_0, TEST_BITS_FFFFFFFF)));
	zassert_equal(0xFFFFFFFF > 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_GT(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_LT is equivalent to < in C.
 */
ZTEST(sys_util_expr_compare, test_lt)
{
	zassert_equal(0x0 < 0x0, EXPR_TO_NUM(EXPR_LT(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 < 0x1, EXPR_TO_NUM(EXPR_LT(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 < 0x0, EXPR_TO_NUM(EXPR_LT(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 < 0x1, EXPR_TO_NUM(EXPR_LT(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 < 0x1, EXPR_TO_NUM(EXPR_LT(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0x2 < 0x2, EXPR_TO_NUM(EXPR_LT(TEST_BITS_2, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFE < 0x1, EXPR_TO_NUM(EXPR_LT(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE < 0x2, EXPR_TO_NUM(EXPR_LT(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF < 0x0, EXPR_TO_NUM(EXPR_LT(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF < 0x1, EXPR_TO_NUM(EXPR_LT(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF < 0x2, EXPR_TO_NUM(EXPR_LT(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0x0 < 0xFFFFFFFF, EXPR_TO_NUM(EXPR_LT(TEST_BITS_0, TEST_BITS_FFFFFFFF)));
	zassert_equal(0xFFFFFFFF < 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_LT(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_GE is equivalent to >= in C.
 */
ZTEST(sys_util_expr_compare, test_gte)
{
	zassert_equal(0x0 >= 0x0, EXPR_TO_NUM(EXPR_GE(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 >= 0x1, EXPR_TO_NUM(EXPR_GE(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 >= 0x0, EXPR_TO_NUM(EXPR_GE(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 >= 0x1, EXPR_TO_NUM(EXPR_GE(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 >= 0x1, EXPR_TO_NUM(EXPR_GE(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0x2 >= 0x2, EXPR_TO_NUM(EXPR_GE(TEST_BITS_2, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFE >= 0x1, EXPR_TO_NUM(EXPR_GE(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE >= 0x2, EXPR_TO_NUM(EXPR_GE(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF >= 0x0, EXPR_TO_NUM(EXPR_GE(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF >= 0x1, EXPR_TO_NUM(EXPR_GE(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF >= 0x2, EXPR_TO_NUM(EXPR_GE(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0x0 >= 0xFFFFFFFF, EXPR_TO_NUM(EXPR_GE(TEST_BITS_0, TEST_BITS_FFFFFFFF)));
	zassert_equal(0xFFFFFFFF >= 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_GE(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_LE is equivalent to <= in C.
 */
ZTEST(sys_util_expr_compare, test_lte)
{
	zassert_equal(0x0 <= 0x0, EXPR_TO_NUM(EXPR_LE(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 <= 0x1, EXPR_TO_NUM(EXPR_LE(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 <= 0x0, EXPR_TO_NUM(EXPR_LE(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 <= 0x1, EXPR_TO_NUM(EXPR_LE(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 <= 0x1, EXPR_TO_NUM(EXPR_LE(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0x2 <= 0x2, EXPR_TO_NUM(EXPR_LE(TEST_BITS_2, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFE <= 0x1, EXPR_TO_NUM(EXPR_LE(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE <= 0x2, EXPR_TO_NUM(EXPR_LE(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF <= 0x0, EXPR_TO_NUM(EXPR_LE(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF <= 0x1, EXPR_TO_NUM(EXPR_LE(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF <= 0x2, EXPR_TO_NUM(EXPR_LE(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0x0 <= 0xFFFFFFFF, EXPR_TO_NUM(EXPR_LE(TEST_BITS_0, TEST_BITS_FFFFFFFF)));
	zassert_equal(0xFFFFFFFF <= 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_LE(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

ZTEST_SUITE(sys_util_expr_compare, NULL, NULL, NULL, NULL, NULL);
