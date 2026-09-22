/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

K_MEM_SLAB_DEFINE(slab, 128, 10, 4);

ZTEST_BENCHMARK_SUITE(k_mem_slab, NULL, NULL);

static void *ptr;
static void _dealloc(void)
{
	__ASSERT(ptr != NULL, "Pointer should not be NULL");
	k_mem_slab_free(&slab, ptr);
	ptr = NULL;
}

ZTEST_BENCHMARK(k_mem_slab, alloc, 1000, NULL, _dealloc)
{
	k_mem_slab_alloc(&slab, &ptr, K_NO_WAIT);
}

static void _alloc(void)
{
	int rc;

	rc = k_mem_slab_alloc(&slab, &ptr, K_NO_WAIT);
	__ASSERT(rc == 0, "Allocation failed with error code %d", rc);
	ARG_UNUSED(rc);
}

ZTEST_BENCHMARK(k_mem_slab, free, 1000, _alloc, NULL)
{
	k_mem_slab_free(&slab, ptr);
}
