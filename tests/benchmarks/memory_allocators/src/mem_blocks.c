/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/sys/mem_blocks.h>

SYS_MEM_BLOCKS_DEFINE(blocks, 128, 10, 4);

ZTEST_BENCHMARK_SUITE(mem_blocks, NULL, NULL);

static void *ptr;
static void _dealloc(void)
{
	__ASSERT(ptr != NULL, "Pointer should not be NULL");
	sys_mem_blocks_free(&blocks, 1, &ptr);
	ptr = NULL;
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(mem_blocks, alloc, 1000, NULL, _dealloc)
{
	sys_mem_blocks_alloc(&blocks, 1, &ptr);
}

static void _alloc(void)
{
	int rc;

	rc = sys_mem_blocks_alloc(&blocks, 1, &ptr);
	__ASSERT(rc == 0, "Allocation failed with error code %d", rc);
	ARG_UNUSED(rc);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(mem_blocks, free, 1000, _alloc, NULL)
{
	sys_mem_blocks_free(&blocks, 1, &ptr);
}
