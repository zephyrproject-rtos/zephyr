/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <inttypes.h>
#include <string.h>

#include "assert.h"

DEFINE_FFF_GLOBALS;

/* Align the test memory to the heap chunk size */
uint8_t heapmem[8192] __aligned(8);

ZTEST(lib_heap_min, test_heap_min_size_assert)
{
	struct sys_heap heap;

	Z_TEST_SKIP_IFNDEF(CONFIG_ASSERT);

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

ZTEST(lib_heap_min, test_heap_too_small)
{
	static uint8_t buf[Z_HEAP_MIN_SIZE + 16] __aligned(8);
	struct sys_heap heap;

	Z_TEST_SKIP_IFDEF(CONFIG_ASSERT);

	for (size_t sz = 0; sz < Z_HEAP_MIN_SIZE; sz++) {
		memset(buf, 0xa5, sizeof(buf));
		sys_heap_init(&heap, buf, sz);

		zassert_is_null(sys_heap_alloc(&heap, 1), "size %zu", sz);
		zassert_is_null(sys_heap_aligned_alloc(&heap, 16, 1), "size %zu", sz);
		zassert_is_null(sys_heap_realloc(&heap, NULL, 1), "size %zu", sz);

		for (size_t i = sz; i < sizeof(buf); i++) {
			zassert_equal(buf[i], 0xa5, "size %zu wrote offset %zu", sz, i);
		}
	}

	memset(buf, 0xa5, sizeof(buf));
	sys_heap_init(&heap, buf + 1, 10);
	zassert_is_null(sys_heap_alloc(&heap, 1));
	zassert_equal(buf[0], 0xa5);
	for (size_t i = 11; i < sizeof(buf); i++) {
		zassert_equal(buf[i], 0xa5, "misaligned region wrote offset %zu", i);
	}
}

ZTEST(lib_heap_min, test_shared_multi_heap_too_small)
{
	static uint8_t small_buf[Z_HEAP_MIN_SIZE - 1] __aligned(8);
	static uint8_t buf[Z_HEAP_MIN_SIZE * 4] __aligned(8);
	struct shared_multi_heap_region small_region = {
		.addr = (uintptr_t)small_buf,
		.size = sizeof(small_buf),
		.attr = SMH_REG_ATTR_CACHEABLE,
	};
	struct shared_multi_heap_region region = {
		.addr = (uintptr_t)buf,
		.size = sizeof(buf),
		.attr = SMH_REG_ATTR_CACHEABLE,
	};
	void *block;

	Z_TEST_SKIP_IFDEF(CONFIG_ASSERT);

	zassert_equal(shared_multi_heap_pool_init(), 0);
	zassert_equal(shared_multi_heap_add(&small_region, NULL), -EINVAL);
	zassert_equal(shared_multi_heap_add(&region, NULL), 0);

	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 16);
	zassert_not_null(block, "no allocation after a rejected region");
	shared_multi_heap_free(block);
}

ZTEST_SUITE(lib_heap_min, NULL, NULL, NULL, NULL, NULL);
