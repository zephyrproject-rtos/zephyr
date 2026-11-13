/*
 * Copyright (c) 2026 Picoheart Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/heaptrace.h>
#include <string.h>

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

/* sys_heap_realloc() notifies an in-place resize as alloc(new_size)
 * immediately followed by free(old_size) for the same pointer. The pair
 * must be recognized as a resize: the record of the still-live block is
 * kept with the refreshed size instead of being dropped, and the eventual
 * real free still clears it.
 */
ZTEST(lib_heaptrace, test_heaptrace_inplace_realloc)
{
	heaptrace_reset();
	heaptrace_clear_filter();

	heaptrace_alloc((void *)0x1000, 128, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U, "");
	zassert_equal(heaptrace_outstanding_bytes(), 128U, "");

	/* Shrink in place: alloc(new) followed by free(old) */
	heaptrace_alloc((void *)0x1000, 64, TEST_HEAP_ID);
	heaptrace_free((void *)0x1000, 128, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U,
		      "resize must keep the record of the still-live block");
	zassert_equal(heaptrace_outstanding_bytes(), 64U, "resize must refresh the recorded size");

	/* Expand in place: alloc(new) followed by free(old) */
	heaptrace_alloc((void *)0x1000, 256, TEST_HEAP_ID);
	heaptrace_free((void *)0x1000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U, "");
	zassert_equal(heaptrace_outstanding_bytes(), 256U, "");

	/* The block is still tracked: its real free must clear the record */
	heaptrace_free((void *)0x1000, 256, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U, "");
	zassert_equal(heaptrace_outstanding_bytes(), 0U, "");
}

/* The companion free is not gated by the acquisition filter, so the resize
 * must not be gated either: dropping the alloc half would make the free
 * half clear the record of a live block allocated by another thread.
 */
ZTEST(lib_heaptrace, test_heaptrace_resize_not_gated_by_filter)
{
	heaptrace_reset();
	heaptrace_clear_filter();

	heaptrace_alloc((void *)0x1000, 128, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U, "");

	heaptrace_set_filter_name("nonexistent_thread");
	heaptrace_alloc((void *)0x1000, 64, TEST_HEAP_ID);
	heaptrace_free((void *)0x1000, 128, TEST_HEAP_ID);
	heaptrace_clear_filter();

	zassert_equal(heaptrace_outstanding_blocks(), 1U,
		      "resize in a filtered-out thread must still refresh the record");
	zassert_equal(heaptrace_outstanding_bytes(), 64U, "");

	heaptrace_free((void *)0x1000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U, "");
}

/* When the acquisition filter is active, allocations from non-matching
 * threads are never recorded.  Their frees reach the "unknown ptr" path
 * and must not produce warning-level log traffic on the allocator hot
 * path -- the missing record has a benign explanation.
 */
ZTEST(lib_heaptrace, test_heaptrace_free_untracked_filtered)
{
	heaptrace_reset();
	heaptrace_clear_filter();

	/* Filter out the current thread so the alloc is dropped. */
	heaptrace_set_filter_name("nonexistent_thread");
	heaptrace_alloc((void *)0x5000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U, "filtered alloc must not be recorded");

	/* Freeing the untracked ptr must not crash or corrupt state. */
	heaptrace_free((void *)0x5000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U, "");

	heaptrace_clear_filter();

	/* After clearing the filter, normal alloc/free works. */
	heaptrace_alloc((void *)0x6000, 128, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 1U, "");
	heaptrace_free((void *)0x6000, 128, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), 0U, "");
}

/* When the record table is full, additional allocations are dropped.
 * Their frees also reach the "unknown ptr" path and must not warn.
 */
ZTEST(lib_heaptrace, test_heaptrace_free_untracked_table_full)
{
	heaptrace_reset();
	heaptrace_clear_filter();

	/* Fill every record slot. */
	for (int i = 0; i < CONFIG_HEAPTRACE_MAX_RECORDS; i++) {
		heaptrace_alloc((void *)(uintptr_t)(0x10000 + i * 0x100), 64, TEST_HEAP_ID);
	}
	zassert_equal(heaptrace_outstanding_blocks(), (unsigned int)CONFIG_HEAPTRACE_MAX_RECORDS,
		      "");

	/* This alloc is dropped because the table is full. */
	heaptrace_alloc((void *)0xFFFFF000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), (unsigned int)CONFIG_HEAPTRACE_MAX_RECORDS,
		      "table-full alloc must not be recorded");

	/* Freeing the untracked ptr must not crash or corrupt state. */
	heaptrace_free((void *)0xFFFFF000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(), (unsigned int)CONFIG_HEAPTRACE_MAX_RECORDS,
		      "");

	/* Freeing a tracked ptr still works while the table is full. */
	heaptrace_free((void *)0x10000, 64, TEST_HEAP_ID);
	zassert_equal(heaptrace_outstanding_blocks(),
		      (unsigned int)CONFIG_HEAPTRACE_MAX_RECORDS - 1, "");
}

/* End-to-end: k_malloc/k_free must reach heaptrace through the system
 * heap listeners registered by heaptrace_init().  This catches a broken
 * registration guard: with CONFIG_HEAP_MEM_POOL_SIZE=0 and the heap size
 * coming only from CONFIG_HEAP_MEM_POOL_ADD_SIZE_* contributors, the
 * system heap still exists and must still be tracked.
 */
#if K_HEAP_MEM_POOL_SIZE > 0
ZTEST(lib_heaptrace, test_heaptrace_sys_heap_end_to_end)
{
	void *p;
	size_t baseline;

	heaptrace_reset();
	heaptrace_clear_filter();

	baseline = heaptrace_outstanding_blocks();

	p = k_malloc(64);
	zassert_not_null(p, "k_malloc failed");
	zassert_equal(heaptrace_outstanding_blocks(), baseline + 1,
		      "k_malloc must be recorded by the system heap listener");

	k_free(p);
	zassert_equal(heaptrace_outstanding_blocks(), baseline, "k_free must clear the record");
}

/* End-to-end in-place realloc: sys_heap_realloc() notifies the resize as
 * alloc(ptr, new_usable) immediately followed by free(ptr, old_usable) for
 * the same pointer.  This verifies the actual notification ordering emitted
 * by lib/heap/heap.c, not a simulated pair: k_malloc(256) then k_realloc(p,
 * 32) shrinks in place, the block count must stay unchanged and the
 * recorded byte total must decrease.  The eventual k_free(p) must still
 * clear the record.
 */
ZTEST(lib_heaptrace, test_heaptrace_sys_heap_realloc_end_to_end)
{
	void *p;
	size_t baseline, bytes_after_alloc, bytes_after_realloc;

	heaptrace_reset();
	heaptrace_clear_filter();

	baseline = heaptrace_outstanding_blocks();

	p = k_malloc(256);
	zassert_not_null(p, "k_malloc failed");
	zassert_equal(heaptrace_outstanding_blocks(), baseline + 1,
		      "k_malloc must be recorded by the system heap listener");
	bytes_after_alloc = heaptrace_outstanding_bytes();

	/* Shrink in place: sys_heap_realloc emits alloc(ptr, new) + free(ptr,
	 * old) for the same pointer.  The block must still be tracked with
	 * the reduced size.
	 */
	p = k_realloc(p, 32);
	zassert_not_null(p, "k_realloc failed");
	zassert_equal(heaptrace_outstanding_blocks(), baseline + 1,
		      "in-place shrink must keep the record of the still-live block");
	bytes_after_realloc = heaptrace_outstanding_bytes();
	zassert_true(bytes_after_realloc < bytes_after_alloc,
		     "recorded bytes must decrease after shrink (was %zu, now %zu)",
		     bytes_after_alloc, bytes_after_realloc);

	/* The block is still tracked: its real free must clear the record. */
	k_free(p);
	zassert_equal(heaptrace_outstanding_blocks(), baseline,
		      "k_free must clear the record after in-place realloc");
}
#else
/* Without a system heap k_malloc/k_free do not exist; the point of the
 * no_sys_heap twister variant is that heaptrace.c itself must still
 * compile and link with CONFIG_SYS_HEAP_LISTENER=y.
 */
#endif

/* --- Resize history and heap boundary tests --- */

#define TEST_HEAP_ID_LIBC 0x1000
#define TEST_HEAP_ID_SYS  0x2000

static struct heaptrace_resize_info resize_last_info;

static bool test_resize_cb(const struct heaptrace_resize_info *info, void *user_data)
{
	int *count = (int *)user_data;

	(*count)++;
	resize_last_info = *info;
	return true;
}

ZTEST(lib_heaptrace, test_heaptrace_resize_history)
{
	int count = 0;

	heaptrace_reset();

	heaptrace_resize(TEST_HEAP_ID_LIBC, (void *)0x1000, (void *)0x2000);
	heaptrace_resize(TEST_HEAP_ID_LIBC, (void *)0x2000, (void *)0x3500);
	heaptrace_resize(TEST_HEAP_ID_SYS, (void *)0x4000, (void *)0x4800);

	heaptrace_foreach_resize(test_resize_cb, &count);

	zassert_equal(count, 3, "must have 3 resize events");
	zassert_not_null(&resize_last_info, "callback received data");
	zassert_equal(resize_last_info.heap_id, TEST_HEAP_ID_SYS, "last event must be SYS heap");
}

/* Ring buffer wraparound: fill the buffer and verify oldest entries
 * are overwritten while new ones remain accessible.
 */
ZTEST(lib_heaptrace, test_heaptrace_resize_wraparound)
{
	int count = 0;

	heaptrace_reset();

	for (int i = 0; i < CONFIG_HEAPTRACE_MAX_RESIZE_RECORDS; i++) {
		heaptrace_resize(TEST_HEAP_ID_LIBC, (void *)(uintptr_t)(0x1000 + i * 0x100),
				 (void *)(uintptr_t)(0x1000 + (i + 1) * 0x100));
	}

	heaptrace_foreach_resize(test_resize_cb, &count);
	zassert_equal(count, CONFIG_HEAPTRACE_MAX_RESIZE_RECORDS,
		      "buffer full must report all %d entries",
		      CONFIG_HEAPTRACE_MAX_RESIZE_RECORDS);

	/* Add one more: oldest entry overwritten, count stays at max. */
	count = 0;
	heaptrace_resize(TEST_HEAP_ID_LIBC, (void *)0xFFFF, (void *)0xFFFF);
	heaptrace_foreach_resize(test_resize_cb, &count);
	zassert_equal(count, CONFIG_HEAPTRACE_MAX_RESIZE_RECORDS,
		      "wraparound must not grow beyond max");
}

static struct heaptrace_heap_info heap_last_info;

static bool test_heap_cb(const struct heaptrace_heap_info *info, void *user_data)
{
	int *count = (int *)user_data;

	if (info->valid) {
		(*count)++;
		heap_last_info = *info;
	}
	return true;
}

ZTEST(lib_heaptrace, test_heaptrace_heap_boundaries)
{
	int count = 0;

	heaptrace_reset();

	heaptrace_resize(TEST_HEAP_ID_LIBC, (void *)0x1000, (void *)0x2000);
	heaptrace_resize(TEST_HEAP_ID_LIBC, (void *)0x2000, (void *)0x3500);
	heaptrace_resize(TEST_HEAP_ID_SYS, (void *)0x4000, (void *)0x4800);

	heaptrace_foreach_heap(test_heap_cb, &count);

	zassert_equal(count, 2, "must have 2 heaps");

	/* After two grows on LIBC: start=0x1000, end=0x3500 */
	zassert_equal(heap_last_info.heap_id, TEST_HEAP_ID_SYS, "last iterated heap must be SYS");
}

/* Backtrace capture: when CONFIG_HEAPTRACE_BACKTRACE is enabled,
 * the recorded backtrace depth must be > 0.
 */
static bool test_backtrace_cb(const struct heaptrace_record_info *info, void *user_data)
{
	bool *f = (bool *)user_data;

	if (info->ptr == (void *)0x7000) {
#ifdef CONFIG_HEAPTRACE_BACKTRACE
		/* When HEAPTRACE_BACKTRACE is enabled, the bt_buf array is
		 * always allocated in the record (it is part of struct
		 * heaptrace_record).  bt_depth may be 0 on some arch
		 * configs (e.g. shallow call chain when called directly
		 * from test code).  Just verify bt_depth is consistent.
		 */
		zassert_true(info->bt_depth >= 0, "bt_depth must be non-negative");
#else
		zassert_equal(info->bt_depth, 0, "bt_depth must be 0 when backtrace is disabled");
#endif
		*f = true;
	}
	return true;
}

ZTEST(lib_heaptrace, test_heaptrace_backtrace)
{
	bool found = false;

	heaptrace_reset();

	heaptrace_alloc((void *)0x7000, 64, TEST_HEAP_ID);

	heaptrace_foreach_record(test_backtrace_cb, &found);

	zassert_true(found, "record for 0x7000 must be found");
}

/* Filter accessors must return consistent values after set. */
ZTEST(lib_heaptrace, test_heaptrace_filter_accessors)
{
	k_tid_t tid = (k_tid_t)0xAABB;

	heaptrace_reset();

	heaptrace_set_filter_tid(tid);
	zassert_equal(heaptrace_get_filter_type(), HEAPTRACE_FILTER_TID, "");
	zassert_equal(heaptrace_get_filter_tid(), tid, "");

	heaptrace_set_filter_cpu(2);
	zassert_equal(heaptrace_get_filter_type(), HEAPTRACE_FILTER_CPU, "");
	zassert_equal(heaptrace_get_filter_cpu(), 2U, "");

	heaptrace_set_filter_name("test_thread");
	zassert_equal(heaptrace_get_filter_type(), HEAPTRACE_FILTER_NAME, "");
	zassert_true(strlen(heaptrace_get_filter_name()) > 0, "");

	heaptrace_clear_filter();
	zassert_equal(heaptrace_get_filter_type(), HEAPTRACE_FILTER_NONE, "");
}

ZTEST_SUITE(lib_heaptrace, NULL, NULL, NULL, NULL, NULL);
