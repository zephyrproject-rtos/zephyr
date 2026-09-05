/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Kernel heap allocation. Allocating and freeing are separate
 * benchmarks so that the two costs, which differ, do not hide inside
 * one number.
 */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(heap, NULL, NULL);

#define BLOCK_SIZE 64

static void *block;

static void heap_alloc(void)
{
	block = k_malloc(BLOCK_SIZE);
	__ASSERT(block != NULL, "k_malloc failed; raise CONFIG_HEAP_MEM_POOL_SIZE");
}

static void heap_free(void)
{
	k_free(block);
	block = NULL;
}

ZTEST_BENCHMARK(heap, malloc, BENCH_SAMPLES, NULL, heap_free)
{
	block = k_malloc(BLOCK_SIZE);
}

ZTEST_BENCHMARK(heap, free, BENCH_SAMPLES, heap_alloc, NULL)
{
	k_free(block);
}
