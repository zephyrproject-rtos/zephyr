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
 * @brief Verify that the behavior of EXPR_OR is equivalent to | in C.
 */
ZTEST(sys_util_expr_logical, test_or)
{
	zassert_equal(0x0 | 0x0, EXPR_TO_NUM(EXPR_OR(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 | 0x1, EXPR_TO_NUM(EXPR_OR(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 | 0x0, EXPR_TO_NUM(EXPR_OR(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 | 0x1, EXPR_TO_NUM(EXPR_OR(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 | 0x1, EXPR_TO_NUM(EXPR_OR(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE | 0x1, EXPR_TO_NUM(EXPR_OR(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE | 0x2, EXPR_TO_NUM(EXPR_OR(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF | 0x0, EXPR_TO_NUM(EXPR_OR(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF | 0x1, EXPR_TO_NUM(EXPR_OR(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF | 0x2, EXPR_TO_NUM(EXPR_OR(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF | 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_OR(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_AND is equivalent to & in C.
 */
ZTEST(sys_util_expr_logical, test_and)
{
	zassert_equal(0x0 & 0x0, EXPR_TO_NUM(EXPR_AND(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 & 0x1, EXPR_TO_NUM(EXPR_AND(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 & 0x0, EXPR_TO_NUM(EXPR_AND(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 & 0x1, EXPR_TO_NUM(EXPR_AND(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 & 0x1, EXPR_TO_NUM(EXPR_AND(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE & 0x1, EXPR_TO_NUM(EXPR_AND(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE & 0x2, EXPR_TO_NUM(EXPR_AND(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF & 0x0, EXPR_TO_NUM(EXPR_AND(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF & 0x1, EXPR_TO_NUM(EXPR_AND(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF & 0x2, EXPR_TO_NUM(EXPR_AND(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF & 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_AND(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_XOR is equivalent to ^ in C.
 */
ZTEST(sys_util_expr_logical, test_xor)
{
	zassert_equal(0x0 ^ 0x0, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 ^ 0x1, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 ^ 0x0, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 ^ 0x1, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 ^ 0x1, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE ^ 0x1, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE ^ 0x2, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF ^ 0x0, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF ^ 0x1, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF ^ 0x2, EXPR_TO_NUM(EXPR_XOR(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF ^ 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_XOR(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_NOT is equivalent to ~ in C.
 */
ZTEST(sys_util_expr_logical, test_not)
{
	zassert_equal(~0x0, EXPR_TO_NUM(EXPR_NOT(TEST_BITS_0)));
	zassert_equal(~0x1, EXPR_TO_NUM(EXPR_NOT(TEST_BITS_1)));
	zassert_equal(~0x2, EXPR_TO_NUM(EXPR_NOT(TEST_BITS_2)));
	zassert_equal(~0xFFFFFFFE, EXPR_TO_NUM(EXPR_NOT(TEST_BITS_FFFFFFFE)));
	zassert_equal(~0xFFFFFFFF, EXPR_TO_NUM(EXPR_NOT(TEST_BITS_FFFFFFFF)));
}

ZTEST_SUITE(sys_util_expr_logical, NULL, NULL, NULL, NULL, NULL);
