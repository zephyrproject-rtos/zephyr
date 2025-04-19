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
 * @brief Verify that the behavior of EXPR_ADD is equivalent to uint32 addition in C.
 */
ZTEST(sys_util_expr_add_sub, test_add)
{
	zassert_equal(0x0 + 0x0, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 + 0x1, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 + 0x0, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 + 0x1, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 + 0x1, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE + 0x1, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE + 0x2, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF + 0x0, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF + 0x1, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF + 0x2, EXPR_TO_NUM(EXPR_ADD(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF + 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_ADD(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

/**
 * @brief Verify that the behavior of EXPR_SUB is equivalent to uint32_t subtraction in C.
 */
ZTEST(sys_util_expr_add_sub, test_sub)
{
	zassert_equal(0x0 - 0x0, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_0, TEST_BITS_0)));
	zassert_equal(0x0 - 0x1, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_0, TEST_BITS_1)));
	zassert_equal(0x1 - 0x0, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_1, TEST_BITS_0)));
	zassert_equal(0x1 - 0x1, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_1, TEST_BITS_1)));
	zassert_equal(0x2 - 0x1, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_2, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE - 0x1, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_FFFFFFFE, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFE - 0x2, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_FFFFFFFE, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF - 0x0, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_FFFFFFFF, TEST_BITS_0)));
	zassert_equal(0xFFFFFFFF - 0x1, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_FFFFFFFF, TEST_BITS_1)));
	zassert_equal(0xFFFFFFFF - 0x2, EXPR_TO_NUM(EXPR_SUB(TEST_BITS_FFFFFFFF, TEST_BITS_2)));
	zassert_equal(0xFFFFFFFF - 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_SUB(TEST_BITS_FFFFFFFF, TEST_BITS_FFFFFFFF)));
}

ZTEST_SUITE(sys_util_expr_add_sub, NULL, NULL, NULL, NULL, NULL);
