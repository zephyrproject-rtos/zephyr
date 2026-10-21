/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/sys/bitmap.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_BITS 21

static SYS_BITMAP_DEFINE(file_scope_bitmap, TEST_BITS);

/* Guard bytes around the bitmap catch writes outside its bytes. */
struct guarded {
	uint8_t before;
	SYS_BITMAP_DEFINE(bitmap, TEST_BITS);
	uint8_t after;
};

BUILD_ASSERT(SYS_BITMAP_SIZE(0) == 0);
BUILD_ASSERT(SYS_BITMAP_SIZE(1) == 1);
BUILD_ASSERT(SYS_BITMAP_SIZE(8) == 1);
BUILD_ASSERT(SYS_BITMAP_SIZE(9) == 2);
BUILD_ASSERT(SYS_BITMAP_SIZE(64) == 8);
BUILD_ASSERT(sizeof(file_scope_bitmap) == SYS_BITMAP_SIZE(TEST_BITS));

ZTEST(bitmap, test_file_scope_cleared)
{
	for (size_t bit = 0; bit < TEST_BITS; bit++) {
		zassert_false(sys_bitmap_test_bit(file_scope_bitmap, bit), "bit %zu set", bit);
	}
}

ZTEST(bitmap, test_layout)
{
	const uint8_t mask[] = { 0x01, 0x80, 0x24 };
	SYS_BITMAP_DEFINE(bitmap, 24) = { 0 };

	for (size_t bit = 0; bit < 24; bit++) {
		bool expected = (bit == 0U) || (bit == 15U) || (bit == 18U) || (bit == 21U);

		zassert_equal(sys_bitmap_test_bit(mask, bit), expected, "bit %zu", bit);

		if (expected) {
			sys_bitmap_set_bit(bitmap, bit);
		}
	}

	zassert_mem_equal(bitmap, mask, sizeof(mask));
}

ZTEST(bitmap, test_set_clear)
{
	struct guarded g;

	memset(&g, 0xa5, sizeof(g));
	memset(g.bitmap, 0, sizeof(g.bitmap));

	for (size_t bit = 0; bit < TEST_BITS; bit++) {
		sys_bitmap_set_bit(g.bitmap, bit);

		for (size_t other = 0; other < TEST_BITS; other++) {
			zassert_equal(sys_bitmap_test_bit(g.bitmap, other), other <= bit,
				      "bit %zu after setting %zu", other, bit);
		}
	}

	for (size_t bit = 0; bit < TEST_BITS; bit++) {
		sys_bitmap_clear_bit(g.bitmap, bit);

		for (size_t other = 0; other < TEST_BITS; other++) {
			zassert_equal(sys_bitmap_test_bit(g.bitmap, other), other > bit,
				      "bit %zu after clearing %zu", other, bit);
		}
	}

	zassert_equal(g.before, 0xa5);
	zassert_equal(g.after, 0xa5);
}

ZTEST(bitmap, test_set_bit_to)
{
	SYS_BITMAP_DEFINE(bitmap, TEST_BITS) = { 0 };

	sys_bitmap_set_bit_to(bitmap, 9, true);
	zassert_true(sys_bitmap_test_bit(bitmap, 9));
	sys_bitmap_set_bit_to(bitmap, 9, true);
	zassert_true(sys_bitmap_test_bit(bitmap, 9));
	sys_bitmap_set_bit_to(bitmap, 9, false);
	zassert_false(sys_bitmap_test_bit(bitmap, 9));
	sys_bitmap_set_bit_to(bitmap, 9, false);
	zassert_false(sys_bitmap_test_bit(bitmap, 9));
}

ZTEST(bitmap, test_test_and_set_clear)
{
	SYS_BITMAP_DEFINE(bitmap, TEST_BITS) = { 0 };

	zassert_false(sys_bitmap_test_and_set_bit(bitmap, 20));
	zassert_true(sys_bitmap_test_and_set_bit(bitmap, 20));
	zassert_true(sys_bitmap_test_bit(bitmap, 20));

	zassert_true(sys_bitmap_test_and_clear_bit(bitmap, 20));
	zassert_false(sys_bitmap_test_and_clear_bit(bitmap, 20));
	zassert_false(sys_bitmap_test_bit(bitmap, 20));
}

ZTEST(bitmap, test_find_next)
{
	/* Bits 1, 8, 9 and 19 set; bits 21 to 23 are outside the bitmap and set too. */
	const uint8_t bitmap[] = { 0x02, 0x03, 0xe8 };

	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, 0), 1);
	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, 1), 1);
	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, 2), 8);
	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, 9), 9);
	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, 10), 19);
	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, 20), TEST_BITS);
	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, TEST_BITS), TEST_BITS);
	zassert_equal(sys_bitmap_find_next_set(bitmap, TEST_BITS, 100), TEST_BITS);
	zassert_equal(sys_bitmap_find_next_set(bitmap, 24, 20), 21);

	zassert_equal(sys_bitmap_find_next_clear(bitmap, TEST_BITS, 0), 0);
	zassert_equal(sys_bitmap_find_next_clear(bitmap, TEST_BITS, 1), 2);
	zassert_equal(sys_bitmap_find_next_clear(bitmap, TEST_BITS, 8), 10);
	zassert_equal(sys_bitmap_find_next_clear(bitmap, TEST_BITS, 19), 20);
	zassert_equal(sys_bitmap_find_next_clear(bitmap, TEST_BITS, 21), TEST_BITS);
}

ZTEST(bitmap, test_find_next_clear_full)
{
	SYS_BITMAP_DEFINE(bitmap, TEST_BITS);

	memset(bitmap, 0xff, sizeof(bitmap));
	bitmap[sizeof(bitmap) - 1] = (uint8_t)BIT_MASK(TEST_BITS % BITS_PER_BYTE);

	zassert_equal(sys_bitmap_find_next_clear(bitmap, TEST_BITS, 0), TEST_BITS);

	sys_bitmap_clear_bit(bitmap, TEST_BITS - 1);
	zassert_equal(sys_bitmap_find_next_clear(bitmap, TEST_BITS, 0), TEST_BITS - 1);
}

/* Searches that run past the last byte without finding a match. */
ZTEST(bitmap, test_find_next_none)
{
	const uint8_t zeros[2] = { 0x00, 0x00 };
	const uint8_t ones[2] = { 0xff, 0xff };

	zassert_equal(sys_bitmap_find_next_set(zeros, 16, 0), 16);
	zassert_equal(sys_bitmap_find_next_set(zeros, 16, 9), 16);
	zassert_equal(sys_bitmap_find_next_set(zeros, 8, 0), 8);
	zassert_equal(sys_bitmap_find_next_clear(ones, 16, 0), 16);
	zassert_equal(sys_bitmap_find_next_clear(ones, 16, 15), 16);
	zassert_equal(sys_bitmap_find_next_clear(ones, 8, 0), 8);
}

ZTEST(bitmap, test_count_set)
{
	const uint8_t bitmap[] = { 0x02, 0x03, 0xe8 };
	uint8_t ones[40];

	zassert_equal(sys_bitmap_count_set(bitmap, 0), 0);
	zassert_equal(sys_bitmap_count_set(bitmap, 1), 0);
	zassert_equal(sys_bitmap_count_set(bitmap, 2), 1);
	zassert_equal(sys_bitmap_count_set(bitmap, 16), 3);
	zassert_equal(sys_bitmap_count_set(bitmap, TEST_BITS), 4);
	zassert_equal(sys_bitmap_count_set(bitmap, 24), 7);

	memset(ones, 0xff, sizeof(ones));
	zassert_equal(sys_bitmap_count_set(ones, 317), 317);
	zassert_equal(sys_bitmap_count_set(ones, 320), 320);
}

ZTEST(bitmap, test_for_each_set_bit)
{
	const uint8_t bitmap[] = { 0x02, 0x03, 0xe8 };
	const size_t expected[] = { 1, 8, 9, 19 };
	size_t count = 0;
	size_t bit;

	SYS_BITMAP_FOR_EACH_SET_BIT(bitmap, TEST_BITS, bit) {
		zassert_true(count < ARRAY_SIZE(expected), "bit %zu unexpected", bit);
		zassert_equal(bit, expected[count]);
		count++;
	}

	zassert_equal(count, ARRAY_SIZE(expected));

	count = 0;
	SYS_BITMAP_FOR_EACH_SET_BIT(bitmap, 0, bit) {
		count++;
	}

	zassert_equal(count, 0);

	count = 0;
	SYS_BITMAP_FOR_EACH_SET_BIT(bitmap, 8, bit) {
		count++;
	}

	zassert_equal(count, 1);
}

ZTEST_SUITE(bitmap, NULL, NULL, NULL, NULL, NULL);
