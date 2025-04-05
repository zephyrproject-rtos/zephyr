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
 * @brief Verify that the behavior of EXPR_MUL is equivalent to uint32_t multiply in C.
 */
ZTEST(sys_util_expr_multiply, test_mul)
{
	zassert_equal(0x0 * 0x0, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 * 0x1, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 * 0x0, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 * 0x1, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 * 0x1, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0x2 * 0x2, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_2, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFE * 0x1, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE * 0x2, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0x7FFFFFFF * 0x0, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_7FFFFFFF, TEST_BITS_0)));
	zassert_equal(0x7FFFFFFF * 0x1, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_7FFFFFFF, TEST_BITS_1)));
	zassert_equal(0x7FFFFFFF * 0x2, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_7FFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF * 0x0, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF * 0x1, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF * 0x2, EXPR_TO_NUM(EXPR_MUL(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF * 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_MUL(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

ZTEST_SUITE(sys_util_expr_multiply, NULL, NULL, NULL, NULL, NULL);
