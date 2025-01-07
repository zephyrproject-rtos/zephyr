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
 * @brief Verify that the behavior of EXPR_LSH is equivalent to << in C.
 */
ZTEST(sys_util_expr_shift, test_lshift)
{
	zassert_equal(0x0 << 0x0, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 << 0x1, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 << 0x0, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 << 0x1, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x1 << 30, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_1, TEST_BITS_1E)));
	zassert_equal(0x1 << 31, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_1, TEST_BITS_1F)));
	zassert_equal(0x1 << 32, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_1, TEST_BITS_20)));
	zassert_equal(0x2 << 0x1, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE << 0x1, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE << 0x2, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF << 0x0, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF << 0x1, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF << 0x2, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF << 30, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFF, TEST_BITS_1E)));
	zassert_equal(0xFFFFFFFF << 31, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFF, TEST_BITS_1F)));
	zassert_equal(0xFFFFFFFF << 32, EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFF, TEST_BITS_20)));
	zassert_equal(0xFFFFFFFF << 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_LSH(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_LSH is equivalent to >> in C.
 */
ZTEST(sys_util_expr_shift, test_rshift)
{
	zassert_equal(0x0 >> 0x0, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 >> 0x1, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 >> 0x0, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 >> 0x1, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x1 >> 30, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_1, TEST_BITS_1E)));
	zassert_equal(0x1 >> 31, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_1, TEST_BITS_1F)));
	zassert_equal(0x1 >> 32, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_1, TEST_BITS_20)));
	zassert_equal(0x2 >> 0x1, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE >> 0x1, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE >> 0x2, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF >> 0x0, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF >> 0x1, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF >> 0x2, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF >> 30, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFF, TEST_BITS_1E)));
	zassert_equal(0xFFFFFFFF >> 31, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFF, TEST_BITS_1F)));
	zassert_equal(0xFFFFFFFF >> 32, EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFF, TEST_BITS_20)));
	zassert_equal(0xFFFFFFFF >> 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_RSH(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

ZTEST_SUITE(sys_util_expr_shift, NULL, NULL, NULL, NULL, NULL);
