/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>

static struct sys_heap heap;
static uint8_t heap_buffer[1024];

static void suite_setup(void)
{
	sys_heap_init(&heap, heap_buffer, sizeof(heap_buffer));
}

ZTEST_BENCHMARK_SUITE(sys_heap, suite_setup, NULL);

static void *ptr;
static void dealloc_global(void)
{
	__ASSERT(ptr != NULL, "Pointer should not be NULL");
	sys_heap_free(&heap, ptr);
	ptr = NULL;
}

ZTEST_BENCHMARK(sys_heap, alloc, 1000, NULL, dealloc_global)
{
	ptr = sys_heap_alloc(&heap, 128);
}

static void alloc_global(void)
{
	ptr = sys_heap_alloc(&heap, 128);
}

ZTEST_BENCHMARK(sys_heap, free, 1000, alloc_global, NULL)
{
	sys_heap_free(&heap, ptr);
}

ZTEST_BENCHMARK(sys_heap, realloc, 1000, alloc_global, dealloc_global)
{
	ptr = sys_heap_realloc(&heap, ptr, 256);
}
