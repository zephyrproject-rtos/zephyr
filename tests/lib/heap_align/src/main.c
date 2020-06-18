/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>
#include <sys/sys_heap.h>

/* Must be a power of two */
#define HEAP_SZ 0x1000

BUILD_ASSERT((HEAP_SZ & (HEAP_SZ - 1)) == 0);

uint64_t heapmem[1 + 2 * HEAP_SZ];

/* Heap metadata sizes */
uint64_t *heap_start, *heap_end;

/* Note that this test is making whitebox assumptions about the
 * behavior of the heap in order to exercise coverage of the
 * underlying code: that chunk headers are 8 bytes, that heap chunks
 * are returned low-adddress to high, and that freed blocks are merged
 * immediately with adjacent free blocks.
 */
static void check_heap_align(struct sys_heap *h,
			     uintptr_t prefix, uintptr_t align)
{
	void *p, *q, *r;

	p = sys_heap_alloc(h, prefix);
	zassert_true(prefix == 0 || p != NULL, "prefix allocation failed");

	q = sys_heap_aligned_alloc(h, align, 8);
	zassert_true((((uintptr_t)q) & (align - 1)) == 0, "block not aligned");

	/* Make sure ALL the split memory goes back into the heap and
	 * we can allocate the full remaining suffix
	 */
	r = sys_heap_alloc(h, ((char *)heap_end - (char *)q) - 16);
	zassert_true(r != NULL, "suffix allocation failed");

	sys_heap_free(h, p);
	sys_heap_free(h, q);
	sys_heap_free(h, r);

	/* Make sure it's still valid, and empty */
	zassert_true(sys_heap_validate(h), "heap invalid");
	p = sys_heap_alloc(h, heap_end - heap_start - 8);
	zassert_true(p != NULL, "heap not empty");
	sys_heap_free(h, p);
}

static void test_aligned_alloc(void)
{
	struct sys_heap heap = {};
	uintptr_t addr = (uintptr_t) &heapmem[0];
	void *p;

	/* Choose a "maximally misaligned" buffer for the heap, that
	 * starts 8 bytes (one chunk header) after the biggest aligned
	 * block within heapmem
	 */
	addr = 8 + ((addr + (HEAP_SZ - 1)) & ~(HEAP_SZ - 1));
	sys_heap_init(&heap, (void *)addr, HEAP_SZ);

	p = sys_heap_alloc(&heap, 1);
	zassert_true(p != NULL, "initial alloc failed");
	sys_heap_free(&heap, p);

	/* Heap starts where that first chunk was, and ends one 8-byte
	 * chunk header before the end of its memory
	 */
	heap_start = p;
	heap_end = (uint64_t *)(addr + HEAP_SZ - 8);

	for (uintptr_t prefix = 0; prefix <= 8; prefix += 8) {
		for (uintptr_t align = 8; align <= HEAP_SZ / 4; align *= 2) {
			check_heap_align(&heap, prefix, align);
		}
	}
}

void test_main(void)
{
	ztest_test_suite(lib_heap_align_test,
			 ztest_unit_test(test_aligned_alloc));

	ztest_run_test_suite(lib_heap_align_test);
}
