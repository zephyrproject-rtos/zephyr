/*
 * Copyright (c) 2026 Picoheart Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/heaptrace.h>

#define TEST_HEAP_ID 0xDEAD

ZTEST(lib_heaptrace, test_heaptrace_alloc_free)
{
	heaptrace_reset();
	heaptrace_clear_filter();

	heaptrace_alloc((void *)0x1000, 64, TEST_HEAP_ID);
	heaptrace_alloc((void *)0x2000, 128, TEST_HEAP_ID);

	zassert_equal(heaptrace_outstanding_blocks(), 2U, "");
	zassert_equal(heaptrace_outstanding_bytes(), 192U, "");

	heaptrace_free((void *)0x1000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U, "");
	zassert_equal(heaptrace_outstanding_bytes(), 128U, "");

	heaptrace_free((void *)0x2000, 128, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U, "");
	zassert_equal(heaptrace_outstanding_bytes(), 0U, "");
}

ZTEST(lib_heaptrace, test_heaptrace_filter)
{
	heaptrace_reset();
	heaptrace_clear_filter();

	zassert_equal(heaptrace_get_filter_type(), HEAPTRACE_FILTER_NONE, "");

	heaptrace_set_filter_name("nonexistent");
	heaptrace_alloc((void *)0x1000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U, "filtered alloc should be dropped");

	heaptrace_clear_filter();
	heaptrace_alloc((void *)0x2000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U, "unfiltered alloc should be recorded");
}

/* Free must not be gated by the acquisition filter: a block allocated by a
 * filtered thread may be freed by another thread, and dropping such frees
 * would leave phantom leaks in the record table.
 */
ZTEST(lib_heaptrace, test_heaptrace_free_not_gated_by_filter)
{
	heaptrace_reset();
	heaptrace_clear_filter();

	/* Allocate while no filter is set, then set a filter that does not
	 * match the current thread. The subsequent free must still remove
	 * the record.
	 */
	heaptrace_alloc((void *)0x3000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U, "");

	heaptrace_set_filter_name("nonexistent_thread");
	heaptrace_free((void *)0x3000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U,
		      "free must remove the record even when filter does not match");
	heaptrace_clear_filter();
}

ZTEST_SUITE(lib_heaptrace, NULL, NULL, NULL, NULL, NULL);
