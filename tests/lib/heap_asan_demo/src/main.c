/*
 * Copyright (c) 2025 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>
#include <string.h>

#define HEAP_SIZE 4096

static uint8_t heap_mem[HEAP_SIZE];
static struct sys_heap test_heap;

static void *test_setup(void)
{
	sys_heap_init(&test_heap, heap_mem, HEAP_SIZE);
	return NULL;
}

ZTEST(heap_asan_demo, test_heap_basic_operations)
{
	void *ptr1;
	void *ptr2;
	void *ptr3;
	void *ptr4;

	TC_PRINT("\n=== Heap ASAN Demo Test ===\n");

#if CONFIG_HEAP_ASAN_DEMO_CRASH_UNALLOCATED
	TC_PRINT("\n[CRASH TEST] Accessing unallocated memory\n");
	void *ptr = (void *)&heap_mem[HEAP_SIZE/2];

	TC_PRINT("Attempting to write to unallocated address %p\n", ptr);
	TC_PRINT("Expected: ASAN crash\n");
	memset(ptr, 0xAA, 16);
	zassert_unreachable("Accessed unallocated heap memory without crashing");
#endif /* CONFIG_HEAP_ASAN_DEMO_CRASH_UNALLOCATED */

	TC_PRINT("\nStep 1: Allocate 100 bytes\n");
	ptr1 = sys_heap_alloc(&test_heap, 100);
	zassert_not_null(ptr1, "Failed to allocate memory from heap");
	TC_PRINT("  Allocated at %p\n", ptr1);

	memset(ptr1, 0xA1, 100);
	TC_PRINT("  Wrote pattern 0xA1\n");

#if CONFIG_HEAP_ASAN_DEMO_CRASH_BUFFER_OVERFLOW
	TC_PRINT("\n[CRASH TEST] Buffer overflow\n");
	size_t usable_size = sys_heap_usable_size(&test_heap, ptr1);

	TC_PRINT("Usable size: %zu bytes\n", usable_size);
	TC_PRINT("Attempting to write %zu bytes (overflow by 16)\n", usable_size + 16);
	TC_PRINT("Expected: ASAN crash\n");
	memset(ptr1, 0x41, usable_size + 16);
	zassert_unreachable("Buffer overflow did not crash as expected");
#endif /* CONFIG_HEAP_ASAN_DEMO_CRASH_BUFFER_OVERFLOW */

	TC_PRINT("\nStep 2: Allocate 200 bytes\n");
	ptr2 = sys_heap_alloc(&test_heap, 200);
	zassert_not_null(ptr2, "Failed to allocate memory from heap");
	TC_PRINT("  Allocated at %p\n", ptr2);

	memset(ptr2, 0xA2, 200);
	TC_PRINT("  Wrote pattern 0xA2\n");

	TC_PRINT("\nStep 3: Allocate aligned (32-byte) 100 bytes\n");
	ptr3 = sys_heap_aligned_alloc(&test_heap, 32, 100);
	zassert_not_null(ptr3, "Failed to allocate aligned memory");
	TC_PRINT("  Allocated at %p\n", ptr3);

	zassert_equal((uintptr_t)ptr3 & 0x1F, 0, "Memory not properly aligned");
	TC_PRINT("  Alignment verified\n");

	memset(ptr3, 0xA3, 100);
	TC_PRINT("  Wrote pattern 0xA3\n");

	TC_PRINT("\nStep 4: Realloc ptr1 to 200 bytes\n");
	TC_PRINT("  Original ptr1: %p\n", ptr1);
	ptr4 = sys_heap_realloc(&test_heap, ptr1, 200);
	zassert_not_null(ptr4, "Failed to reallocate memory");
	TC_PRINT("  Reallocated at %p\n", ptr4);

#if CONFIG_HEAP_ASAN_DEMO_CRASH_USE_AFTER_REALLOC
	if (ptr4 != ptr1) {
		TC_PRINT("\n[CRASH TEST] Use after realloc\n");
		TC_PRINT("Memory was relocated: old=%p, new=%p\n", ptr1, ptr4);
		TC_PRINT("Attempting to access old pointer\n");
		TC_PRINT("Expected: ASAN crash\n");
		memset(ptr1, 0x55, 16);
		zassert_unreachable("Use after realloc did not crash as expected");
	} else {
		TC_PRINT("\n[SKIP] Realloc did not relocate, skipping crash test\n");
	}
#endif /* CONFIG_HEAP_ASAN_DEMO_CRASH_USE_AFTER_REALLOC */

	TC_PRINT("\nStep 5: Verify data preserved during realloc\n");
	for (int i = 0; i < 100; i++) {
		zassert_equal(((uint8_t *)ptr4)[i], 0xA1, "Data not preserved during realloc");
	}
	TC_PRINT("  First 100 bytes preserved\n");

	memset((uint8_t *)ptr4 + 100, 0xB1, 100);
	TC_PRINT("  Extended area writable\n");

	TC_PRINT("\nStep 6: Free allocated memory\n");
	sys_heap_free(&test_heap, ptr2);
	TC_PRINT("  Freed ptr2\n");

	sys_heap_free(&test_heap, ptr3);
	TC_PRINT("  Freed ptr3\n");

	sys_heap_free(&test_heap, ptr4);
	TC_PRINT("  Freed ptr4\n");

#if CONFIG_HEAP_ASAN_DEMO_CRASH_USE_AFTER_FREE
	TC_PRINT("\n[CRASH TEST] Use after free\n");
	TC_PRINT("Attempting to access freed ptr2 at %p\n", ptr2);
	TC_PRINT("Expected: ASAN crash\n");
	memset(ptr2, 0x55, 16);
	zassert_unreachable("Use after free did not crash as expected");
#endif /* CONFIG_HEAP_ASAN_DEMO_CRASH_USE_AFTER_FREE */

	TC_PRINT("\n=== Test Complete ===\n");
}

ZTEST_SUITE(heap_asan_demo, NULL, test_setup, NULL, NULL, NULL);
