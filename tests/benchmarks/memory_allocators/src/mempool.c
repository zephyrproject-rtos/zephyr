/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_BENCHMARK_SUITE(mempool, NULL, NULL);

static void *ptr;
static void _dealloc(void)
{
	__ASSERT(ptr != NULL, "Pointer should not be NULL");
	k_free(ptr);
	ptr = NULL;
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(mempool, alloc, 1000, NULL, _dealloc)
{
	ptr = k_malloc(128);
}

static void _alloc(void)
{
	ptr = k_malloc(128);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(mempool, free, 1000, _alloc, NULL)
{
	k_free(ptr);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(mempool, realloc, 1000, _alloc, _dealloc)
{
	ptr = k_realloc(ptr, 256);
}
