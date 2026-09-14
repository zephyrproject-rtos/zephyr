/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/common/init.h>

#define BUF_SIZE 64

static ZTEST_BMEM __aligned(8) uint8_t buf[BUF_SIZE];

/*
 * Fill buf with zeros, call arch_early_memset() on buf[off .. off + n) and
 * verify that exactly those bytes hold the fill value.
 */
static void check_early_memset(size_t off, uint8_t val, size_t n)
{
	zassert_true(off + n <= BUF_SIZE, "test case overruns buf");

	memset(buf, 0, sizeof(buf));

	arch_early_memset(buf + off, val, n);

	for (size_t i = 0; i < BUF_SIZE; i++) {
		uint8_t expect = (i >= off && i < off + n) ? val : 0x00;

		zassert_equal(buf[i], expect,
			      "byte %zu: got 0x%02x, want 0x%02x",
			      i, buf[i], expect);
	}
}

ZTEST(early_mem_funcs, test_unaligned_dst)
{
	/* unaligned destination, length < 8 */
	check_early_memset(1, 0xAA, 4);
	/* unaligned destination, length > 8 */
	check_early_memset(3, 0x5A, 17);
}

ZTEST(early_mem_funcs, test_aligned_dst)
{
	/* aligned destination, length < 8 */
	check_early_memset(0, 0xC3, 3);
	/* aligned destination, 8-byte loop plus a tail */
	check_early_memset(0, 0x11, 21);
	/* aligned destination, exact multiple of 8 */
	check_early_memset(8, 0x80, 32);
	/* aligned destination, exact fast-path boundary (n == 8) */
	check_early_memset(0, 0x33, 8);
	/* 0xFF through the 8-byte loop (all-ones multiplication boundary) */
	check_early_memset(0, 0xFF, 9);
	/* fill value 0x00 through both the loop and the byte path */
	check_early_memset(0, 0x00, 9);
}

ZTEST(early_mem_funcs, test_zero_length)
{
	/* nothing must be written, aligned or not */
	check_early_memset(0, 0xFF, 0);
	check_early_memset(2, 0xFF, 0);
}

ZTEST_SUITE(early_mem_funcs, NULL, NULL, NULL, NULL, NULL);
