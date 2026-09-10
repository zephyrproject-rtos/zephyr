/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>
#include <inttypes.h>

#include "assert.h"

DEFINE_FFF_GLOBALS;

/* Align the test memory to the heap chunk size */
uint8_t heapmem[8192] __aligned(8);

ZTEST(lib_heap_min, test_heap_min_size_assert)
{
	struct sys_heap heap;

	expect_assert();
	sys_heap_init(&heap, (void *)heapmem, Z_HEAP_MIN_SIZE - 1);
	zassert_unreachable();
}

ZTEST(lib_heap_min, test_heap_min_size)
{
	struct sys_heap heap;
	void *mem;

	sys_heap_init(&heap, (void *)heapmem, Z_HEAP_MIN_SIZE);
	mem = sys_heap_alloc(&heap, 1);
	zassert_not_null(mem, "Could not allocate 1 byte from a Z_HEAP_MIN_SIZE heap");
	sys_heap_free(&heap, mem);
}

ZTEST_SUITE(lib_heap_min, NULL, NULL, NULL, NULL, NULL);
