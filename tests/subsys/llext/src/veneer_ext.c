/*
 * Copyright (C) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This extension exercises the veneer stub feature (CONFIG_LLEXT_VENEERS):
 * it calls standard library functions that may be out of direct branch range
 * when the extension is loaded into SRAM, triggering veneer stub generation.
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

#define BUF_SIZE 32

static uint8_t src_buf[BUF_SIZE];
static uint8_t dst_buf[BUF_SIZE];
static const char test_str[] = "veneer_test_string";

void test_entry(void)
{
	size_t len;
	int cmp;

	printk("veneer test: start\n");

	/* Initialize source buffer */
	memset(src_buf, 0xAA, BUF_SIZE);

	/* Test memcpy - copies src to dst */
	memcpy(dst_buf, src_buf, BUF_SIZE);
	zassert_equal(dst_buf[0], 0xAA, "memcpy failed");
	zassert_equal(dst_buf[BUF_SIZE - 1], 0xAA, "memcpy failed");

	/* Test memset - clear dst buffer */
	memset(dst_buf, 0x55, BUF_SIZE);
	zassert_equal(dst_buf[0], 0x55, "memset failed");

	/* Test memmove - overlapping copy within src_buf.
	 * Use 4-byte-aligned src/dst offsets to avoid unaligned memory access
	 */
	memset(src_buf, 0, BUF_SIZE);
	src_buf[0] = 1;
	src_buf[1] = 2;
	src_buf[2] = 3;
	src_buf[3] = 4;
	memmove(&src_buf[4], &src_buf[0], 4);
	zassert_equal(src_buf[4], 1, "memmove failed");
	zassert_equal(src_buf[5], 2, "memmove failed");

	/* Test strlen */
	len = strlen(test_str);
	zassert_equal(len, sizeof(test_str) - 1, "strlen failed: got %zu", len);

	/* Test memcmp */
	memset(src_buf, 0xBB, BUF_SIZE);
	memset(dst_buf, 0xBB, BUF_SIZE);
	cmp = memcmp(src_buf, dst_buf, BUF_SIZE);
	zassert_equal(cmp, 0, "memcmp failed: buffers should be equal");

	dst_buf[0] = 0xCC;
	cmp = memcmp(src_buf, dst_buf, BUF_SIZE);
	zassert_not_equal(cmp, 0, "memcmp failed: buffers should differ");

	printk("veneer test: passed\n");
}
EXPORT_SYMBOL(test_entry);
