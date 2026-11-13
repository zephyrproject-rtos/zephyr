/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/sys/multi_heap.h>
#include <zephyr/ztest.h>

ZTEST(shared_multi_heap_registration, test_total_capacity)
{
	static uint8_t heap_mem[MAX_MULTI_HEAPS + 1U][256] __aligned(8);
	struct shared_multi_heap_region region = {.size = sizeof(heap_mem[0])};
	void *block;

	zassert_ok(shared_multi_heap_pool_init());

	for (size_t i = 0; i < MAX_MULTI_HEAPS; i++) {
		region.addr = (uintptr_t)heap_mem[i];
		region.attr = i % 2U == 0U ? SMH_REG_ATTR_CACHEABLE : SMH_REG_ATTR_NON_CACHEABLE;
		zassert_ok(shared_multi_heap_add(&region, NULL), "could not add region %zu", i);
	}

	region.addr = (uintptr_t)heap_mem[MAX_MULTI_HEAPS];
	region.attr = SMH_REG_ATTR_EXTERNAL;
	zassert_equal(shared_multi_heap_add(&region, NULL), -ENOMEM);
	zassert_is_null(shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 1));

	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 1);
	zassert_not_null(block);
	shared_multi_heap_free(block);
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 1);
	zassert_not_null(block);
	shared_multi_heap_free(block);
}

ZTEST_SUITE(shared_multi_heap_registration, NULL, NULL, NULL, NULL, NULL);
