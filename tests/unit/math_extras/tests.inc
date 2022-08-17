/*
 * Copyright (c) 2019 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/math_extras.h>
#include <inttypes.h>

static void VNAME(u32_add)(void)
{
	uint32_t result = 42;

	zassert_false(u32_add_overflow(2, 3, &result));
	zassert_equal(result, 5);

	zassert_false(u32_add_overflow(2, 0, &result));
	zassert_equal(result, 2);

	zassert_false(u32_add_overflow(0, 3, &result));
	zassert_equal(result, 3);

	zassert_false(u32_add_overflow(0, UINT32_MAX, &result));
	zassert_equal(result, UINT32_MAX);
	zassert_true(u32_add_overflow(1, UINT32_MAX, &result));
	zassert_equal(result, 0);

	zassert_false(u32_add_overflow(UINT32_MAX, 0, &result));
	zassert_equal(result, UINT32_MAX);
	zassert_true(u32_add_overflow(UINT32_MAX, 2, &result));
	zassert_equal(result, 1);
}

static void VNAME(u32_mul)(void)
{
	uint32_t result = 42;

	zassert_false(u32_mul_overflow(2, 3, &result));
	zassert_equal(result, 6);

	zassert_false(u32_mul_overflow(UINT32_MAX, 1, &result));
	zassert_equal(result, UINT32_MAX);
	zassert_true(u32_mul_overflow(UINT32_MAX, 2, &result));
	zassert_equal(result, UINT32_MAX * 2);

	zassert_false(u32_mul_overflow(1, UINT32_MAX, &result));
	zassert_equal(result, UINT32_MAX);
	zassert_true(u32_mul_overflow(2, UINT32_MAX, &result));
	zassert_equal(result, UINT32_MAX * 2);
}

static void VNAME(u64_add)(void)
{
	uint64_t result = 42;

	zassert_false(u64_add_overflow(2, 3, &result));
	zassert_equal(result, 5);

	zassert_false(u64_add_overflow(2, 0, &result));
	zassert_equal(result, 2);

	zassert_false(u64_add_overflow(0, 3, &result));
	zassert_equal(result, 3);

	zassert_false(u64_add_overflow(0, UINT64_MAX, &result));
	zassert_equal(result, UINT64_MAX);
	zassert_true(u64_add_overflow(1, UINT64_MAX, &result));
	zassert_equal(result, 0);

	zassert_false(u64_add_overflow(UINT64_MAX, 0, &result));
	zassert_equal(result, UINT64_MAX);
	zassert_true(u64_add_overflow(UINT64_MAX, 2, &result));
	zassert_equal(result, 1);
}

static void VNAME(u64_mul)(void)
{
	uint64_t result = 42;

	zassert_false(u64_mul_overflow(2, 3, &result));
	zassert_equal(result, 6);

	zassert_false(u64_mul_overflow(UINT64_MAX, 1, &result));
	zassert_equal(result, UINT64_MAX);
	zassert_true(u64_mul_overflow(UINT64_MAX, 2, &result));
	zassert_equal(result, UINT64_MAX * 2);

	zassert_false(u64_mul_overflow(1, UINT64_MAX, &result));
	zassert_equal(result, UINT64_MAX);
	zassert_true(u64_mul_overflow(2, UINT64_MAX, &result));
	zassert_equal(result, UINT64_MAX * 2);
}

static void VNAME(size_add)(void)
{
	size_t result = 42;

	zassert_false(size_add_overflow(2, 3, &result));
	zassert_equal(result, 5);

	zassert_false(size_add_overflow(2, 0, &result));
	zassert_equal(result, 2);

	zassert_false(size_add_overflow(0, 3, &result));
	zassert_equal(result, 3);

	zassert_false(size_add_overflow(0, SIZE_MAX, &result));
	zassert_equal(result, SIZE_MAX);
	zassert_true(size_add_overflow(1, SIZE_MAX, &result));
	zassert_equal(result, 0);

	zassert_false(size_add_overflow(SIZE_MAX, 0, &result));
	zassert_equal(result, SIZE_MAX);
	zassert_true(size_add_overflow(SIZE_MAX, 2, &result));
	zassert_equal(result, 1);
}

static void VNAME(size_mul)(void)
{
	size_t result = 42;

	zassert_false(size_mul_overflow(2, 3, &result));
	zassert_equal(result, 6);

	zassert_false(size_mul_overflow(SIZE_MAX, 1, &result));
	zassert_equal(result, SIZE_MAX);
	zassert_true(size_mul_overflow(SIZE_MAX, 2, &result));
	zassert_equal(result, SIZE_MAX * 2);

	zassert_false(size_mul_overflow(1, SIZE_MAX, &result));
	zassert_equal(result, SIZE_MAX);
	zassert_true(size_mul_overflow(2, SIZE_MAX, &result));
	zassert_equal(result, SIZE_MAX * 2);
}

static void VNAME(u32_clz)(void)
{
	zassert_equal(u32_count_leading_zeros(0), 32);
	zassert_equal(u32_count_leading_zeros(1), 31);
	zassert_equal(u32_count_leading_zeros(0xf00f), 16);
	zassert_equal(u32_count_leading_zeros(0xf00ff00f), 0);
	zassert_equal(u32_count_leading_zeros(0xffffffff), 0);
}

static void VNAME(u64_clz)(void)
{
	zassert_equal(u64_count_leading_zeros(0), 64);
	zassert_equal(u64_count_leading_zeros(1), 63);
	zassert_equal(u64_count_leading_zeros(0xf00f), 48);
	zassert_equal(u64_count_leading_zeros(0xf00ff00f), 32);
	zassert_equal(u64_count_leading_zeros(0xffffffff), 32);
	zassert_equal(u64_count_leading_zeros(0xf00f00000000ull), 16);
	zassert_equal(u64_count_leading_zeros(0xf00ff00f00000000ull), 0);
	zassert_equal(u64_count_leading_zeros(0xffffffff00000000ull), 0);
}

static void VNAME(u32_ctz)(void)
{
	zassert_equal(u32_count_trailing_zeros(0), 32);
	zassert_equal(u32_count_trailing_zeros(1), 0);
	zassert_equal(u32_count_trailing_zeros(6), 1);
	zassert_equal(u32_count_trailing_zeros(0x00f00f00), 8);
	zassert_equal(u32_count_trailing_zeros(0xf00ffc00), 10);
	zassert_equal(u32_count_trailing_zeros(0xffffffff), 0);
	zassert_equal(u32_count_trailing_zeros(0x80000000), 31);
}

static void VNAME(u64_ctz)(void)
{
	zassert_equal(u64_count_trailing_zeros(0), 64);
	zassert_equal(u64_count_trailing_zeros(1), 0);
	zassert_equal(u64_count_trailing_zeros(6), 1);
	zassert_equal(u64_count_trailing_zeros(0x00f00f00), 8);
	zassert_equal(u64_count_trailing_zeros(0xf00ffc00), 10);
	zassert_equal(u64_count_trailing_zeros(0xffffffffffffffffull), 0);
	zassert_equal(u64_count_trailing_zeros(0x8000000080000000ull), 31,
		      NULL);
	zassert_equal(u64_count_trailing_zeros(0xc000000000000000ull), 62,
		      NULL);
}
