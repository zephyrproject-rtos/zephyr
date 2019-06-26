/*
 * Copyright (c) 2019 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/math_extras.h>
#include <inttypes.h>

static void VNAME(u32_add)(void)
{
	u32_t result = 42;

	zassert_false(u32_add_overflow(2, 3, &result), NULL);
	zassert_equal(result, 5, NULL);

	zassert_false(u32_add_overflow(2, 0, &result), NULL);
	zassert_equal(result, 2, NULL);

	zassert_false(u32_add_overflow(0, 3, &result), NULL);
	zassert_equal(result, 3, NULL);

	zassert_false(u32_add_overflow(0, UINT32_MAX, &result), NULL);
	zassert_equal(result, UINT32_MAX, NULL);
	zassert_true(u32_add_overflow(1, UINT32_MAX, &result), NULL);
	zassert_equal(result, 0, NULL);

	zassert_false(u32_add_overflow(UINT32_MAX, 0, &result), NULL);
	zassert_equal(result, UINT32_MAX, NULL);
	zassert_true(u32_add_overflow(UINT32_MAX, 2, &result), NULL);
	zassert_equal(result, 1, NULL);
}

static void VNAME(u32_mul)(void)
{
	u32_t result = 42;

	zassert_false(u32_mul_overflow(2, 3, &result), NULL);
	zassert_equal(result, 6, NULL);

	zassert_false(u32_mul_overflow(UINT32_MAX, 1, &result), NULL);
	zassert_equal(result, UINT32_MAX, NULL);
	zassert_true(u32_mul_overflow(UINT32_MAX, 2, &result), NULL);
	zassert_equal(result, UINT32_MAX * 2, NULL);

	zassert_false(u32_mul_overflow(1, UINT32_MAX, &result), NULL);
	zassert_equal(result, UINT32_MAX, NULL);
	zassert_true(u32_mul_overflow(2, UINT32_MAX, &result), NULL);
	zassert_equal(result, UINT32_MAX * 2, NULL);
}

static void VNAME(u64_add)(void)
{
	u64_t result = 42;

	zassert_false(u64_add_overflow(2, 3, &result), NULL);
	zassert_equal(result, 5, NULL);

	zassert_false(u64_add_overflow(2, 0, &result), NULL);
	zassert_equal(result, 2, NULL);

	zassert_false(u64_add_overflow(0, 3, &result), NULL);
	zassert_equal(result, 3, NULL);

	zassert_false(u64_add_overflow(0, UINT64_MAX, &result), NULL);
	zassert_equal(result, UINT64_MAX, NULL);
	zassert_true(u64_add_overflow(1, UINT64_MAX, &result), NULL);
	zassert_equal(result, 0, NULL);

	zassert_false(u64_add_overflow(UINT64_MAX, 0, &result), NULL);
	zassert_equal(result, UINT64_MAX, NULL);
	zassert_true(u64_add_overflow(UINT64_MAX, 2, &result), NULL);
	zassert_equal(result, 1, NULL);
}

static void VNAME(u64_mul)(void)
{
	u64_t result = 42;

	zassert_false(u64_mul_overflow(2, 3, &result), NULL);
	zassert_equal(result, 6, NULL);

	zassert_false(u64_mul_overflow(UINT64_MAX, 1, &result), NULL);
	zassert_equal(result, UINT64_MAX, NULL);
	zassert_true(u64_mul_overflow(UINT64_MAX, 2, &result), NULL);
	zassert_equal(result, UINT64_MAX * 2, NULL);

	zassert_false(u64_mul_overflow(1, UINT64_MAX, &result), NULL);
	zassert_equal(result, UINT64_MAX, NULL);
	zassert_true(u64_mul_overflow(2, UINT64_MAX, &result), NULL);
	zassert_equal(result, UINT64_MAX * 2, NULL);
}

static void VNAME(size_add)(void)
{
	size_t result = 42;

	zassert_false(size_add_overflow(2, 3, &result), NULL);
	zassert_equal(result, 5, NULL);

	zassert_false(size_add_overflow(2, 0, &result), NULL);
	zassert_equal(result, 2, NULL);

	zassert_false(size_add_overflow(0, 3, &result), NULL);
	zassert_equal(result, 3, NULL);

	zassert_false(size_add_overflow(0, SIZE_MAX, &result), NULL);
	zassert_equal(result, SIZE_MAX, NULL);
	zassert_true(size_add_overflow(1, SIZE_MAX, &result), NULL);
	zassert_equal(result, 0, NULL);

	zassert_false(size_add_overflow(SIZE_MAX, 0, &result), NULL);
	zassert_equal(result, SIZE_MAX, NULL);
	zassert_true(size_add_overflow(SIZE_MAX, 2, &result), NULL);
	zassert_equal(result, 1, NULL);
}

static void VNAME(size_mul)(void)
{
	size_t result = 42;

	zassert_false(size_mul_overflow(2, 3, &result), NULL);
	zassert_equal(result, 6, NULL);

	zassert_false(size_mul_overflow(SIZE_MAX, 1, &result), NULL);
	zassert_equal(result, SIZE_MAX, NULL);
	zassert_true(size_mul_overflow(SIZE_MAX, 2, &result), NULL);
	zassert_equal(result, SIZE_MAX * 2, NULL);

	zassert_false(size_mul_overflow(1, SIZE_MAX, &result), NULL);
	zassert_equal(result, SIZE_MAX, NULL);
	zassert_true(size_mul_overflow(2, SIZE_MAX, &result), NULL);
	zassert_equal(result, SIZE_MAX * 2, NULL);
}

static void VNAME(u32_clz)(void)
{
	zassert_equal(u32_count_leading_zeros(0), 32, NULL);
	zassert_equal(u32_count_leading_zeros(1), 31, NULL);
	zassert_equal(u32_count_leading_zeros(0xf00f), 16, NULL);
	zassert_equal(u32_count_leading_zeros(0xf00ff00f), 0, NULL);
	zassert_equal(u32_count_leading_zeros(0xffffffff), 0, NULL);
}

static void VNAME(u64_clz)(void)
{
	zassert_equal(u64_count_leading_zeros(0), 64, NULL);
	zassert_equal(u64_count_leading_zeros(1), 63, NULL);
	zassert_equal(u64_count_leading_zeros(0xf00f), 48, NULL);
	zassert_equal(u64_count_leading_zeros(0xf00ff00f), 32, NULL);
	zassert_equal(u64_count_leading_zeros(0xffffffff), 32, NULL);
	zassert_equal(u64_count_leading_zeros(0xf00f00000000ull), 16, NULL);
	zassert_equal(u64_count_leading_zeros(0xf00ff00f00000000ull), 0, NULL);
	zassert_equal(u64_count_leading_zeros(0xffffffff00000000ull), 0, NULL);
}

static void VNAME(u32_ctz)(void)
{
	zassert_equal(u32_count_trailing_zeros(0), 32, NULL);
	zassert_equal(u32_count_trailing_zeros(1), 0, NULL);
	zassert_equal(u32_count_trailing_zeros(6), 1, NULL);
	zassert_equal(u32_count_trailing_zeros(0x00f00f00), 8, NULL);
	zassert_equal(u32_count_trailing_zeros(0xf00ffc00), 10, NULL);
	zassert_equal(u32_count_trailing_zeros(0xffffffff), 0, NULL);
	zassert_equal(u32_count_trailing_zeros(0x80000000), 31, NULL);
}

static void VNAME(u64_ctz)(void)
{
	zassert_equal(u64_count_trailing_zeros(0), 64, NULL);
	zassert_equal(u64_count_trailing_zeros(1), 0, NULL);
	zassert_equal(u64_count_trailing_zeros(6), 1, NULL);
	zassert_equal(u64_count_trailing_zeros(0x00f00f00), 8, NULL);
	zassert_equal(u64_count_trailing_zeros(0xf00ffc00), 10, NULL);
	zassert_equal(u64_count_trailing_zeros(0xffffffffffffffffull), 0, NULL);
	zassert_equal(u64_count_trailing_zeros(0x8000000080000000ull), 31,
		      NULL);
	zassert_equal(u64_count_trailing_zeros(0xc000000000000000ull), 62,
		      NULL);
}
