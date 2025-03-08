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

/**
 * @brief Verify that the behavior of EXPR_MUL is equivalent to uint32_t multiply in C.
 */
ZTEST(sys_util_expr_multiply, test_mul)
{
	zassert_equal(0x0 * 0x0, EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(0), EXPR_BITS_0X(0))));
	zassert_equal(0x0 * 0x1, EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(0), EXPR_BITS_0X(1))));
	zassert_equal(0x1 * 0x0, EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(1), EXPR_BITS_0X(0))));
	zassert_equal(0x1 * 0x1, EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(1), EXPR_BITS_0X(1))));
	zassert_equal(0x2 * 0x1, EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(2), EXPR_BITS_0X(1))));
	zassert_equal(0x2 * 0x2, EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(2), EXPR_BITS_0X(2))));
	zassert_equal(0xFFFFFFFE * 0x1,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(F, F, F, F, F, F, F, E), EXPR_BITS_0X(1))));
	zassert_equal(0xFFFFFFFE * 0x2,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(F, F, F, F, F, F, F, E), EXPR_BITS_0X(2))));
	zassert_equal(0x7FFFFFFF * 0x0,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(7, F, F, F, F, F, F, F), EXPR_BITS_0X(0))));
	zassert_equal(0x7FFFFFFF * 0x1,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(7, F, F, F, F, F, F, F), EXPR_BITS_0X(1))));
	zassert_equal(0x7FFFFFFF * 0x2,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(7, F, F, F, F, F, F, F), EXPR_BITS_0X(2))));
	zassert_equal(0xFFFFFFFF * 0x0,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(F, F, F, F, F, F, F, F), EXPR_BITS_0X(0))));
	zassert_equal(0xFFFFFFFF * 0x1,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(F, F, F, F, F, F, F, F), EXPR_BITS_0X(1))));
	zassert_equal(0xFFFFFFFF * 0x2,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(F, F, F, F, F, F, F, F), EXPR_BITS_0X(2))));
	zassert_equal(0xFFFFFFFF * 0xFFFFFFFF,
		      EXPR_TO_NUM(EXPR_MUL(EXPR_BITS_0X(F, F, F, F, F, F, F, F), EXPR_BITS_0X(F, F, F, F, F, F, F, F))));
}

ZTEST_SUITE(sys_util_expr_multiply, NULL, NULL, NULL, NULL, NULL);
