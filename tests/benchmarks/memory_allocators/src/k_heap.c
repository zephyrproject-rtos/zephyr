/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>


static struct k_heap heap;
static uint8_t heap_buffer[1024];

static void suite_setup(void)
{
	k_heap_init(&heap, heap_buffer, sizeof(heap_buffer));
}

ZTEST_BENCHMARK_SUITE(k_heap, suite_setup, NULL);

static void *ptr;
static void dealloc_global(void)
{
	__ASSERT(ptr != NULL, "Pointer should not be NULL");
	k_heap_free(&heap, ptr);
	ptr = NULL;
}

ZTEST_BENCHMARK(k_heap, alloc, 1000, NULL, dealloc_global)
{
	ptr = k_heap_alloc(&heap, 128, K_NO_WAIT);
}

static void alloc_global(void)
{
	ptr = k_heap_alloc(&heap, 128, K_NO_WAIT);
}

ZTEST_BENCHMARK(k_heap, free, 1000, alloc_global, NULL)
{
	k_heap_free(&heap, ptr);
}

ZTEST_BENCHMARK(k_heap, realloc, 1000, alloc_global, dealloc_global)
{
	ptr = k_heap_realloc(&heap, ptr, 256, K_NO_WAIT);
}
