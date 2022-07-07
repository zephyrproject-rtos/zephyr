/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/heap_listener.h>
#include <zephyr/zephyr.h>
#include <ztest.h>

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/*
 * Function used by malloc() to obtain or free memory to the system.
 * Returns the heap end before applying the change.
 */
extern void *sbrk(intptr_t count);

static uintptr_t current_heap_end(void)
{
	return (uintptr_t)sbrk(0);
}

static ptrdiff_t heap_difference;

static void heap_resized(uintptr_t heap_id, void *old_heap_end, void *new_heap_end)
{
	ARG_UNUSED(heap_id);

	heap_difference += ((char *)new_heap_end - (char *)old_heap_end);
}

static HEAP_LISTENER_RESIZE_DEFINE(listener, HEAP_ID_LIBC, heap_resized);

void *ptr;

/**
 * @brief Test that heap listener is notified when libc heap size changes.
 *
 * This test calls the malloc() and free() followed by malloc_trim() functions
 * and verifies that the heap listener is notified of allocating or returning
 * memory from the system.
 */
void test_alloc_and_trim(void)
{
	uintptr_t saved_heap_end;

	TC_PRINT("Allocating memory...\n");

	heap_listener_register(&listener);
	saved_heap_end = current_heap_end();
	ptr = malloc(4096);

	TC_PRINT("Total heap size change: %zi\n", heap_difference);

	zassert_true(heap_difference > 0, "Heap increase not detected");
	zassert_equal(current_heap_end() - saved_heap_end, heap_difference,
		      "Heap increase not detected");

	TC_PRINT("Freeing memory...\n");

	heap_difference = 0;
	saved_heap_end = current_heap_end();
	free(ptr);
	malloc_trim(0);

	/*
	 * malloc_trim() may not free any memory to the system if there is not enough to free.
	 * Therefore, do not require that heap_difference < 0.
	 */
	zassert_equal(current_heap_end() - saved_heap_end, heap_difference,
		      "Heap decrease not detected");

	heap_listener_unregister(&listener);
}

void test_main(void)
{
	ztest_test_suite(newlib_libc_heap_listener,
			 ztest_unit_test(test_alloc_and_trim));

	ztest_run_test_suite(newlib_libc_heap_listener);
}
