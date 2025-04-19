/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_expr.h>
#include <zephyr/sys/util_macro_expr.h>
#include "test_bits.h"

#define ZERO  0
#define SEVEN 7

#define C_ADD(x, y) (uint32_t)(x + y)
#define C_SUB(x, y) (uint32_t)(x - y)
#define C_MUL(x, y) (uint32_t)(x * y)
#define C_DIV(x, y) (uint32_t)(x / y)
#define C_MOD(x, y) (uint32_t)(x % y)
#define C_AND(x, y) (uint32_t)(x & y)
#define C_OR(x, y)  (uint32_t)(x | y)
#define C_XOR(x, y) (uint32_t)(x ^ y)
#define C_LSH(x, y) (uint32_t)(x << y)
#define C_RSH(x, y) (uint32_t)(x >> y)

ZTEST(sys_util_expr_decimal, test_EXPR_ADD_DEC)
{
	zassert_equal(EXPR_ADD_DEC(0, 0), C_ADD(0, 0));
	zassert_equal(EXPR_ADD_DEC(1, 0), C_ADD(1, 0));
	zassert_equal(EXPR_ADD_DEC(0, 1), C_ADD(0, 1));
	zassert_equal(EXPR_ADD_DEC(1, 1), C_ADD(1, 1));
	zassert_equal(EXPR_ADD_DEC(2, 1), C_ADD(2, 1));
	zassert_equal(EXPR_ADD_DEC(1, 2), C_ADD(1, 2));
	zexpect_equal(EXPR_ADD_DEC(2048, 2047), C_ADD(2048, 2047));
	zexpect_equal(EXPR_ADD_DEC(4000, 500), C_ADD(4000, 500));
	zexpect_equal(EXPR_ADD_DEC(SEVEN, SEVEN), C_ADD(SEVEN, SEVEN));
	zexpect_equal(EXPR_ADD_DEC(4000, SEVEN), C_ADD(4000, SEVEN));
	zexpect_equal(EXPR_ADD_DEC(SEVEN, 4090), C_ADD(SEVEN, 4090));
	zassert_equal(EXPR_ADD_DEC(4294967288, SEVEN), C_ADD(4294967288, SEVEN));
	zassert_equal(EXPR_ADD_DEC(4294967289, SEVEN), C_ADD(4294967289, SEVEN));
	zassert_equal(EXPR_ADD_DEC(1, 4294967294), C_ADD(1, 4294967294));
	zassert_equal(EXPR_ADD_DEC(1, 4294967295), C_ADD(1, 4294967295));
	zassert_equal(EXPR_ADD_DEC(2, 4294967295), C_ADD(2, 4294967295));
}

ZTEST(sys_util_expr_decimal, test_EXPR_SUB_DEC)
{
	zassert_equal(EXPR_SUB_DEC(0, 0), C_SUB(0, 0));
	zassert_equal(EXPR_SUB_DEC(1, 0), C_SUB(1, 0));
	zassert_equal(EXPR_SUB_DEC(0, 1), C_SUB(0, 1));
	zassert_equal(EXPR_SUB_DEC(1, 1), C_SUB(1, 1));
	zassert_equal(EXPR_SUB_DEC(2, 1), C_SUB(2, 1));
	zassert_equal(EXPR_SUB_DEC(1, 2), C_SUB(1, 2));
	zexpect_equal(EXPR_SUB_DEC(2048, 2047), C_SUB(2048, 2047));
	zexpect_equal(EXPR_SUB_DEC(4000, 500), C_SUB(4000, 500));
	zexpect_equal(EXPR_SUB_DEC(SEVEN, SEVEN), C_SUB(SEVEN, SEVEN));
	zexpect_equal(EXPR_SUB_DEC(4000, SEVEN), C_SUB(4000, SEVEN));
	zexpect_equal(EXPR_SUB_DEC(SEVEN, 4090), C_SUB(SEVEN, 4090));
	zassert_equal(EXPR_SUB_DEC(4294967295, 1), C_SUB(4294967295, 1));
	zassert_equal(EXPR_SUB_DEC(4294967295, 2), C_SUB(4294967295, 2));
}

ZTEST(sys_util_expr_decimal, test_EXPR_MUL_DEC)
{
	zassert_equal(EXPR_MUL_DEC(0, 0), C_MUL(0, 0));
	zassert_equal(EXPR_MUL_DEC(1, 0), C_MUL(1, 0));
	zassert_equal(EXPR_MUL_DEC(0, 1), C_MUL(0, 1));
	zassert_equal(EXPR_MUL_DEC(1, 1), C_MUL(1, 1));
	zassert_equal(EXPR_MUL_DEC(2, 1), C_MUL(2, 1));
	zassert_equal(EXPR_MUL_DEC(1, 2), C_MUL(1, 2));
	zexpect_equal(EXPR_MUL_DEC(SEVEN, SEVEN), C_MUL(SEVEN, SEVEN));
	zexpect_equal(EXPR_MUL_DEC(2048, 2047), C_MUL(2048, 2047));
	zexpect_equal(EXPR_MUL_DEC(4000, 500), C_MUL(4000, 500));
	zexpect_equal(EXPR_MUL_DEC(4000, SEVEN), C_MUL(4000, SEVEN));
	zexpect_equal(EXPR_MUL_DEC(SEVEN, 4090), C_MUL(SEVEN, 4090));
	zexpect_equal(EXPR_MUL_DEC(2147483647, 2), C_MUL(2147483647, 2));
	zexpect_equal(EXPR_MUL_DEC(4294967295, 2), C_MUL(4294967295, 2));
}

ZTEST(sys_util_expr_decimal, test_EXPR_DIV_DEC)
{
	zassert_equal(EXPR_DIV_DEC(0, 1), C_DIV(0, 1));
	zassert_equal(EXPR_DIV_DEC(1, 1), C_DIV(1, 1));
	zassert_equal(EXPR_DIV_DEC(2, 1), C_DIV(2, 1));
	zassert_equal(EXPR_DIV_DEC(1, 2), C_DIV(1, 2));
	zexpect_equal(EXPR_DIV_DEC(2048, 2047), C_DIV(2048, 2047));
	zexpect_equal(EXPR_DIV_DEC(4000, 500), C_DIV(4000, 500));
	zexpect_equal(EXPR_DIV_DEC(SEVEN, SEVEN), C_DIV(SEVEN, SEVEN));
	zexpect_equal(EXPR_DIV_DEC(4000, SEVEN), C_DIV(4000, SEVEN));
	zexpect_equal(EXPR_DIV_DEC(SEVEN, 4090), C_DIV(SEVEN, 4090));
	zexpect_equal(EXPR_DIV_DEC(4294967295, 2), C_DIV(4294967295, 2));
	zexpect_equal(EXPR_DIV_DEC(4294967295, 2147483647), C_DIV(4294967295, 2147483647));
	zexpect_equal(EXPR_DIV_DEC(4294967295, 4294967294), C_DIV(4294967295, 4294967294));
	zexpect_equal(EXPR_DIV_DEC(4294967295, 4294967295), C_DIV(4294967295, 4294967295));
}

ZTEST(sys_util_expr_decimal, test_EXPR_MOD_DEC)
{
	zassert_equal(EXPR_MOD_DEC(0, 1), C_MOD(0, 1));
	zassert_equal(EXPR_MOD_DEC(1, 1), C_MOD(1, 1));
	zassert_equal(EXPR_MOD_DEC(2, 1), C_MOD(2, 1));
	zassert_equal(EXPR_MOD_DEC(1, 2), C_MOD(1, 2));
	zexpect_equal(EXPR_MOD_DEC(2048, 2047), C_MOD(2048, 2047));
	zexpect_equal(EXPR_MOD_DEC(4000, 500), C_MOD(4000, 500));
	zexpect_equal(EXPR_MOD_DEC(SEVEN, SEVEN), C_MOD(SEVEN, SEVEN));
	zexpect_equal(EXPR_MOD_DEC(4000, SEVEN), C_MOD(4000, SEVEN));
	zexpect_equal(EXPR_MOD_DEC(SEVEN, 4090), C_MOD(SEVEN, 4090));
	zexpect_equal(EXPR_MOD_DEC(4294967295, 2), C_MOD(4294967295, 2));
	zexpect_equal(EXPR_MOD_DEC(4294967295, 2147483647), C_MOD(4294967295, 2147483647));
	zexpect_equal(EXPR_MOD_DEC(4294967295, 4294967294), C_MOD(4294967295, 4294967294));
	zexpect_equal(EXPR_MOD_DEC(4294967295, 4294967295), C_MOD(4294967295, 4294967295));
}

ZTEST(sys_util_expr_decimal, test_EXPR_NOT_DEC)
{
	zexpect_equal(EXPR_NOT_DEC(2047), 4294965248);
	zexpect_equal(EXPR_NOT_DEC(ZERO), 4294967295);
	zexpect_equal(EXPR_NOT_DEC(SEVEN), 4294967288);
}

ZTEST(sys_util_expr_decimal, test_EXPR_AND_DEC)
{
	zexpect_equal(EXPR_AND_DEC(0, 0), C_AND(0, 0));
	zexpect_equal(EXPR_AND_DEC(1, 0), C_AND(1, 0));
	zexpect_equal(EXPR_AND_DEC(0, 1), C_AND(0, 1));
	zexpect_equal(EXPR_AND_DEC(1, 1), C_AND(1, 1));
	zexpect_equal(EXPR_AND_DEC(3, 1), C_AND(3, 1));
	zexpect_equal(EXPR_AND_DEC(3, 2), C_AND(3, 2));
	zexpect_equal(EXPR_AND_DEC(2048, 2047), C_AND(2048, 2047));
	zexpect_equal(EXPR_AND_DEC(4000, 500), C_AND(4000, 500));
	zexpect_equal(EXPR_AND_DEC(SEVEN, SEVEN), C_AND(SEVEN, SEVEN));
	zexpect_equal(EXPR_AND_DEC(4000, SEVEN), C_AND(4000, SEVEN));
	zexpect_equal(EXPR_AND_DEC(SEVEN, 4090), C_AND(SEVEN, 4090));
	zexpect_equal(EXPR_AND_DEC(4294967295, 0), C_AND(4294967295, 0));
	zexpect_equal(EXPR_AND_DEC(2147483648, 1), C_AND(2147483648, 1));
	zexpect_equal(EXPR_AND_DEC(4294967295, 1), C_AND(4294967295, 1));
	zexpect_equal(EXPR_AND_DEC(4294967295, 2147483648), C_AND(4294967295, 2147483648));
}

ZTEST(sys_util_expr_decimal, test_EXPR_OR_DEC)
{
	zexpect_equal(EXPR_OR_DEC(0, 0), C_OR(0, 0));
	zexpect_equal(EXPR_OR_DEC(1, 0), C_OR(1, 0));
	zexpect_equal(EXPR_OR_DEC(0, 1), C_OR(0, 1));
	zexpect_equal(EXPR_OR_DEC(1, 1), C_OR(1, 1));
	zexpect_equal(EXPR_OR_DEC(3, 1), C_OR(3, 1));
	zexpect_equal(EXPR_OR_DEC(3, 2), C_OR(3, 2));
	zexpect_equal(EXPR_OR_DEC(2048, 2047), C_OR(2048, 2047));
	zexpect_equal(EXPR_OR_DEC(4000, 500), C_OR(4000, 500));
	zexpect_equal(EXPR_OR_DEC(SEVEN, SEVEN), C_OR(SEVEN, SEVEN));
	zexpect_equal(EXPR_OR_DEC(4000, SEVEN), C_OR(4000, SEVEN));
	zexpect_equal(EXPR_OR_DEC(SEVEN, 4090), C_OR(SEVEN, 4090));
	zexpect_equal(EXPR_OR_DEC(4294967295, 0), C_OR(4294967295, 0));
	zexpect_equal(EXPR_OR_DEC(2147483648, 1), C_OR(2147483648, 1));
	zexpect_equal(EXPR_OR_DEC(4294967295, 1), C_OR(4294967295, 1));
	zexpect_equal(EXPR_OR_DEC(4294967295, 2147483648), C_OR(4294967295, 2147483648));
}

ZTEST(sys_util_expr_decimal, test_EXPR_XOR_DEC)
{
	zexpect_equal(EXPR_XOR_DEC(0, 0), C_XOR(0, 0));
	zexpect_equal(EXPR_XOR_DEC(1, 0), C_XOR(1, 0));
	zexpect_equal(EXPR_XOR_DEC(0, 1), C_XOR(0, 1));
	zexpect_equal(EXPR_XOR_DEC(1, 1), C_XOR(1, 1));
	zexpect_equal(EXPR_XOR_DEC(3, 1), C_XOR(3, 1));
	zexpect_equal(EXPR_XOR_DEC(3, 2), C_XOR(3, 2));
	zexpect_equal(EXPR_XOR_DEC(2048, 2047), C_XOR(2048, 2047));
	zexpect_equal(EXPR_XOR_DEC(4000, 500), C_XOR(4000, 500));
	zexpect_equal(EXPR_XOR_DEC(SEVEN, SEVEN), C_XOR(SEVEN, SEVEN));
	zexpect_equal(EXPR_XOR_DEC(4000, SEVEN), C_XOR(4000, SEVEN));
	zexpect_equal(EXPR_XOR_DEC(SEVEN, 4090), C_XOR(SEVEN, 4090));
	zexpect_equal(EXPR_XOR_DEC(4294967295, 0), C_XOR(4294967295, 0));
	zexpect_equal(EXPR_XOR_DEC(2147483648, 1), C_XOR(2147483648, 1));
	zexpect_equal(EXPR_XOR_DEC(4294967295, 1), C_XOR(4294967295, 1));
	zexpect_equal(EXPR_XOR_DEC(4294967295, 2147483648), C_XOR(4294967295, 2147483648));
}

ZTEST(sys_util_expr_decimal, test_EXPR_LSH_DEC)
{
	zexpect_equal(EXPR_LSH_DEC(0, 0), C_LSH(0, 0));
	zexpect_equal(EXPR_LSH_DEC(0, 1), C_LSH(0, 1));
	zexpect_equal(EXPR_LSH_DEC(1, 0), C_LSH(1, 0));
	zexpect_equal(EXPR_LSH_DEC(1, 1), C_LSH(1, 1));
	zexpect_equal(EXPR_LSH_DEC(1, 11), C_LSH(1, 11));
	zexpect_equal(EXPR_LSH_DEC(1, 12), C_LSH(1, 12));
	zexpect_equal(EXPR_LSH_DEC(1, 31), C_LSH(1, 31));
	zexpect_equal(EXPR_LSH_DEC(1, 32), C_LSH(1, 32));
	zexpect_equal(EXPR_LSH_DEC(SEVEN, SEVEN), C_LSH(SEVEN, SEVEN));
	zexpect_equal(EXPR_LSH_DEC(31, SEVEN), C_LSH(31, SEVEN));
	zexpect_equal(EXPR_LSH_DEC(SEVEN, 4090), C_LSH(SEVEN, 4090));
}

ZTEST(sys_util_expr_decimal, test_EXPR_RSH_DEC)
{
	zexpect_equal(EXPR_RSH_DEC(0, 0), C_RSH(0, 0));
	zexpect_equal(EXPR_RSH_DEC(0, 1), C_RSH(0, 1));
	zexpect_equal(EXPR_RSH_DEC(1, 0), C_RSH(1, 0));
	zexpect_equal(EXPR_RSH_DEC(1, 1), C_RSH(1, 1));
	zexpect_equal(EXPR_RSH_DEC(2048, 11), C_RSH(2048, 11));
	zexpect_equal(EXPR_RSH_DEC(4096, 12), C_RSH(4096, 12));
	zexpect_equal(EXPR_RSH_DEC(2147483648, 31), C_RSH(2147483648, 31));
	zexpect_equal(EXPR_RSH_DEC(2147483648, 32), C_RSH(2147483648, 32));
	zexpect_equal(EXPR_RSH_DEC(SEVEN, SEVEN), C_RSH(SEVEN, SEVEN));
	zexpect_equal(EXPR_RSH_DEC(1234, SEVEN), C_RSH(1234, SEVEN));
	zexpect_equal(EXPR_RSH_DEC(SEVEN, 4090), C_RSH(SEVEN, 4090));
}

ZTEST_SUITE(sys_util_expr_decimal, NULL, NULL, NULL, NULL, NULL);
