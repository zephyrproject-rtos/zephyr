/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/kernel.h>
#include <string.h>
#include "heap.h"

/* Test with small heap (< 256KB, uses 16-bit chunk sizes) */
#define SMALL_HEAP_SIZE 4096
/* Test with big heap (> 256KB, uses 32-bit chunk sizes) */
#define BIG_HEAP_SIZE (512 * 1024)
#define ALLOC_SIZE 128

static uint8_t __aligned(8) small_heap_mem[SMALL_HEAP_SIZE];
static uint8_t __aligned(8) big_heap_mem[BIG_HEAP_SIZE];
static struct sys_heap small_heap;
static struct sys_heap big_heap;

/* Helper function to get chunk ID */
static chunkid_t mem_to_chunkid(struct z_heap *h, void *p)
{
	uint8_t *mem = p, *base = (uint8_t *)chunk_buf(h);
	return (mem - chunk_header_bytes(h) - base) / CHUNK_UNIT;
}

/* Test fixture setup */
static void *heap_canary_setup(void)
{
	/* Initialize small heap */
	memset(small_heap_mem, 0, sizeof(small_heap_mem));
	sys_heap_init(&small_heap, small_heap_mem, SMALL_HEAP_SIZE);

	/* Initialize big heap */
	memset(big_heap_mem, 0, sizeof(big_heap_mem));
	sys_heap_init(&big_heap, big_heap_mem, BIG_HEAP_SIZE);

	return NULL;
}

static void heap_canary_before(void *fixture)
{
	ARG_UNUSED(fixture);
	/* Re-initialize heaps before each test */
	memset(small_heap_mem, 0, sizeof(small_heap_mem));
	sys_heap_init(&small_heap, small_heap_mem, SMALL_HEAP_SIZE);

	memset(big_heap_mem, 0, sizeof(big_heap_mem));
	sys_heap_init(&big_heap, big_heap_mem, BIG_HEAP_SIZE);
}

static void heap_canary_after(void *fixture)
{
	ARG_UNUSED(fixture);
}

static void heap_canary_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
}

/**
 * @brief Test canary corruption detection on free - Small Heap
 *
 * This test:
 * 1. Allocates memory from small heap (16-bit chunks)
 * 2. Corrupts the canary in the chunk header
 * 3. Attempts to free the memory
 * 4. Should trigger an assertion failure due to corrupted canary
 */
ZTEST(heap_canary, test_small_heap_canary_corruption_on_free)
{
	void *ptr;
	struct z_heap *h = small_heap.heap;
	chunkid_t c;
	chunk_unit_t *buf;
	struct z_heap_custom_header *custom;

	printk("\n=== Small Heap: Canary Corruption on Free ===\n");
	printk("Heap type: %s\n", big_heap(h) ? "BIG" : "SMALL");

	/* Step 1: Allocate memory */
	ptr = sys_heap_alloc(&small_heap, ALLOC_SIZE);
	zassert_not_null(ptr, "Allocation failed");
	printk("Allocated %d bytes at %p\n", ALLOC_SIZE, ptr);

	/* Step 2: Get chunk ID and corrupt the canary */
	c = mem_to_chunkid(h, ptr);
	buf = chunk_buf(h);
	custom = (struct z_heap_custom_header *)&buf[c];

	printk("Chunk ID: %u\n", c);
	printk("Original canary: 0x%016llx\n", custom->canary);

	/* Corrupt the canary */
	custom->canary = 0xDEADBEEFDEADBEEFU;
	printk("Corrupted canary to: 0x%016llx\n", custom->canary);

	printk("\nExpecting assertion on free...\n");

	/* Step 3: Free the memory - this should trigger assertion */
	sys_heap_free(&small_heap, ptr);

	/* Should never reach here if assertions are enabled */
	zassert_unreachable("Should have asserted on corrupted canary");
}

/**
 * @brief Test canary corruption detection on free - Big Heap
 *
 * This test:
 * 1. Allocates memory from big heap (32-bit chunks)
 * 2. Corrupts the canary in the chunk header
 * 3. Attempts to free the memory
 * 4. Should trigger an assertion failure due to corrupted canary
 */
ZTEST(heap_canary, test_big_heap_canary_corruption_on_free)
{
	void *ptr;
	struct z_heap *h = big_heap.heap;
	chunkid_t c;
	chunk_unit_t *buf;
	struct z_heap_custom_header *custom;

	printk("\n=== Big Heap: Canary Corruption on Free ===\n");
	printk("Heap type: %s\n", big_heap(h) ? "BIG" : "SMALL");

	/* Step 1: Allocate memory */
	ptr = sys_heap_alloc(&big_heap, ALLOC_SIZE);
	zassert_not_null(ptr, "Allocation failed");
	printk("Allocated %d bytes at %p\n", ALLOC_SIZE, ptr);

	/* Step 2: Get chunk ID and corrupt the canary */
	c = mem_to_chunkid(h, ptr);
	buf = chunk_buf(h);
	custom = (struct z_heap_custom_header *)&buf[c];

	printk("Chunk ID: %u\n", c);
	printk("Original canary: 0x%016llx\n", custom->canary);

	/* Corrupt the canary */
	custom->canary = 0xDEADBEEFDEADBEEFU;
	printk("Corrupted canary to: 0x%016llx\n", custom->canary);

	printk("\nExpecting assertion on free...\n");

	/* Step 3: Free the memory - this should trigger assertion */
	sys_heap_free(&big_heap, ptr);

	/* Should never reach here if assertions are enabled */
	zassert_unreachable("Should have asserted on corrupted canary");
}

/**
 * @brief Test canary corruption on aligned_alloc - Small Heap
 *
 * This test:
 * 1. Allocates aligned memory from small heap
 * 2. Corrupts the canary
 * 3. Attempts to free
 * 4. Should trigger assertion when validating canary
 */
ZTEST(heap_canary, test_small_heap_canary_corruption_on_aligned_alloc)
{
	void *ptr;
	struct z_heap *h = small_heap.heap;
	chunkid_t c;
	chunk_unit_t *buf;
	struct z_heap_custom_header *custom;

	printk("\n=== Small Heap: Canary Corruption on Aligned Alloc ===\n");
	printk("Heap type: %s\n", big_heap(h) ? "BIG" : "SMALL");

	/* Step 1: Allocate aligned memory */
	ptr = sys_heap_aligned_alloc(&small_heap, 32, ALLOC_SIZE);
	zassert_not_null(ptr, "Aligned allocation failed");
	zassert_true(((uintptr_t)ptr & 31) == 0, "Alignment check failed");
	printk("Allocated %d bytes (32-byte aligned) at %p\n", ALLOC_SIZE, ptr);

	/* Step 2: Corrupt the canary */
	c = mem_to_chunkid(h, ptr);
	buf = chunk_buf(h);
	custom = (struct z_heap_custom_header *)&buf[c];

	printk("Original canary: 0x%016llx\n", custom->canary);
	custom->canary = 0xBADCAFEBADCAFE;
	printk("Corrupted canary to: 0x%016llx\n", custom->canary);

	printk("\nExpecting assertion on free...\n");

	/* Step 3: Free - should trigger assertion */
	sys_heap_free(&small_heap, ptr);

	/* Should never reach here */
	zassert_unreachable("Should have asserted on corrupted canary");
}

/**
 * @brief Test canary corruption on aligned_alloc - Big Heap
 *
 * This test:
 * 1. Allocates aligned memory from big heap
 * 2. Corrupts the canary
 * 3. Attempts to free
 * 4. Should trigger assertion when validating canary
 */
ZTEST(heap_canary, test_big_heap_canary_corruption_on_aligned_alloc)
{
	void *ptr;
	struct z_heap *h = big_heap.heap;
	chunkid_t c;
	chunk_unit_t *buf;
	struct z_heap_custom_header *custom;

	printk("\n=== Big Heap: Canary Corruption on Aligned Alloc ===\n");
	printk("Heap type: %s\n", big_heap(h) ? "BIG" : "SMALL");

	/* Step 1: Allocate aligned memory */
	ptr = sys_heap_aligned_alloc(&big_heap, 64, ALLOC_SIZE);
	zassert_not_null(ptr, "Aligned allocation failed");
	zassert_true(((uintptr_t)ptr & 63) == 0, "Alignment check failed");
	printk("Allocated %d bytes (64-byte aligned) at %p\n", ALLOC_SIZE, ptr);

	/* Step 2: Corrupt the canary */
	c = mem_to_chunkid(h, ptr);
	buf = chunk_buf(h);
	custom = (struct z_heap_custom_header *)&buf[c];

	printk("Original canary: 0x%016llx\n", custom->canary);
	custom->canary = 0xBADCAFEBADCAFE;
	printk("Corrupted canary to: 0x%016llx\n", custom->canary);

	printk("\nExpecting assertion on free...\n");

	/* Step 3: Free - should trigger assertion */
	sys_heap_free(&big_heap, ptr);

	/* Should never reach here */
	zassert_unreachable("Should have asserted on corrupted canary");
}

ZTEST_SUITE(heap_canary, NULL, heap_canary_setup, heap_canary_before,
	    heap_canary_after, heap_canary_teardown);
