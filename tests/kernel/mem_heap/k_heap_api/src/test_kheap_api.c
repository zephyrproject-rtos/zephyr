/*
 * Copyright (c) 2020 Intel Corporation.
 * Copyright (c) 2025 Abderrahmane JARMOUNI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest_error_hook.h>
#include "test_kheap.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
struct k_thread tdata;

K_HEAP_DEFINE(k_heap_test, HEAP_SIZE);

#define ALLOC_SIZE_1 1024
#define ALLOC_SIZE_2 1536
#define CALLOC_NUM   256
#define CALLOC_SIZE  sizeof(uint32_t)

static void tIsr_kheap_alloc_nowait(void *data)
{
	ARG_UNUSED(data);

	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, K_NO_WAIT);

	zassert_not_null(p, "k_heap_alloc operation failed");
	k_heap_free(&k_heap_test, p);
}

static void thread_alloc_heap(void *p1, void *p2, void *p3)
{
	char *p;

	k_timeout_t timeout = Z_TIMEOUT_MS(200);

	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_2, K_NO_WAIT);

	zassert_is_null(p, "k_heap_alloc should fail but did not");

	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_2, timeout);

	zassert_not_null(p, "k_heap_alloc failed to allocate memory");

	k_heap_free(&k_heap_test, p);
}

static void thread_alloc_heap_null(void *p1, void *p2, void *p3)
{
	char *p;

	k_timeout_t timeout = Z_TIMEOUT_MS(200);

	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_2, K_NO_WAIT);

	zassert_is_null(p, "k_heap_alloc should fail but did not");

	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_2, timeout);

	zassert_is_null(p, "k_heap_alloc should fail but did not");

	k_heap_free(&k_heap_test, p);
}

/*test cases*/

/* These need to be adjacent in BSS */
volatile uint32_t heap_guard0;
K_HEAP_DEFINE(tiny_heap, 1);
volatile uint32_t heap_guard1;

/**
 * @brief Test a minimum-size static k_heap
 *
 * @ingroup k_heap_api_tests
 *
 * @details Create a minimum size (1-byte) static heap, verify that it
 * works to allocate that byte at runtime and that it doesn't overflow
 * its memory bounds.
 *
 * @see K_HEAP_DEFINE()
 * @see k_heap_alloc()
 */
ZTEST(k_heap_api, test_k_heap_min_size)
{
	const uint32_t guard_bits = 0x5a5a5a5a;

	/* Make sure static initialization didn't scribble on them */
	zassert_true(heap_guard0 == 0 && heap_guard1 == 0,
		     "static heap initialization overran buffer");

	heap_guard0 = guard_bits;
	heap_guard1 = guard_bits;

	char *p0 = k_heap_alloc(&tiny_heap, 1, K_NO_WAIT);
	char *p1 = k_heap_alloc(&tiny_heap, 1, K_NO_WAIT);

	zassert_not_null(p0, "allocation failed");
	zassert_is_null(p1, "second allocation unexpectedly succeeded");

	*p0 = 0xff;
	k_heap_free(&tiny_heap, p0);

	zassert_equal(heap_guard0, guard_bits, "heap overran buffer");
	zassert_equal(heap_guard1, guard_bits, "heap overran buffer");
}

/**
 * @brief Test k_heap_free() API functionality.
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test validates k_heap_free() API, by using below steps
 * 1. allocate the memory from the heap,
 * 2. free a NULL pointer (should have no effect)
 * 3. free the allocated memory
 * 4. allocate  memory more than the first allocation.
 * The allocation in the 4th step should succeed if k_heap_free() works as expected
 *
 * @see k_heap_alloc, k_heap_free()
 */
ZTEST(k_heap_api, test_k_heap_free)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, timeout);

	zassert_not_null(p, "k_heap_alloc operation failed");

	/* Free NULL pointer: should not crash or corrupt heap */
	k_heap_free(&k_heap_test, NULL);

	k_heap_free(&k_heap_test, p);
	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_2, timeout);
	zassert_not_null(p, "k_heap_alloc operation failed");
	for (int i = 0; i < ALLOC_SIZE_2; i++) {
		p[i] = '0';
	}
	k_heap_free(&k_heap_test, p);
}

/**
 * @brief Validate allocation and free heap memory in isr context.
 *
 * @details The test validates k_heap_alloc() in isr context, the timeout
 * param should be K_NO_WAIT, because this situation isn't allow to wait.
 *
 * @ingroup k_heap_api_tests
 *
 * @see k_heap_alloc()
 * @see k_heap_free()
 */
ZTEST(k_heap_api, test_kheap_alloc_in_isr_nowait)
{
	irq_offload((irq_offload_routine_t)tIsr_kheap_alloc_nowait, NULL);
}

/**
 * @brief Validate the k_heap support wait between different threads.
 *
 * @details In main thread alloc a buffer from the heap, then run the
 * child thread. If there isn't enough space in the heap, the child thread
 * will wait timeout long until main thread free the buffer to heap.
 *
 * @ingroup k_heap_api_tests
 *
 * @see k_heap_alloc()
 * @see k_heap_free()
 */
ZTEST(k_heap_api, test_k_heap_alloc_pending)
{
	/*
	 * Allocate first to make sure subsequent allocations
	 * either fail (K_NO_WAIT) or pend.
	 */
	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_2, K_NO_WAIT);

	zassert_not_null(p, "k_heap_alloc operation failed");

	/* Create a thread which will pend on allocation */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_alloc_heap, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	/* Sleep long enough for child thread to go into pending */
	k_msleep(5);

	/*
	 * Free memory so the child thread can finish memory allocation
	 * without failing.
	 */
	k_heap_free(&k_heap_test, p);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Validate the k_heap alloc_pending_null support.
 *
 * @details In main thread alloc two buffer from the heap, then run the
 * child thread which alloc a buffer larger than remaining space. The child thread
 * will wait timeout long until main thread free one of the buffer to heap, space in
 * the heap is still not enough and then return null after timeout.
 *
 * @ingroup k_heap_api_tests
 *
 * @see k_heap_alloc()
 * @see k_heap_free()
 */
ZTEST(k_heap_api, test_k_heap_alloc_pending_null)
{
	/*
	 * Allocate first to make sure subsequent allocations
	 * either fail (K_NO_WAIT) or pend.
	 */
	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, K_NO_WAIT);
	char *q = (char *)k_heap_alloc(&k_heap_test, 512, K_NO_WAIT);

	zassert_not_null(p, "k_heap_alloc operation failed");
	zassert_not_null(q, "k_heap_alloc operation failed");

	/* Create a thread which will pend on allocation */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_alloc_heap_null, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	/* Sleep long enough for child thread to go into pending */
	k_msleep(5);

	/*
	 * Free some memory but new thread will still not be able
	 * to finish memory allocation without error.
	 */
	k_heap_free(&k_heap_test, q);

	k_thread_join(tid, K_FOREVER);

	k_heap_free(&k_heap_test, p);
}

/**
 * @brief Test k_heap_calloc() and k_heap_free() API usage
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test allocates 256 unsigned integers of 4 bytes for a
 * total of 1024 bytes from the 2048 byte heap. It checks if allocation
 * and initialization are successful or not.
 * Also tests k_heap_calloc() overflow and zero-size edge cases
 *
 * @see k_heap_calloc(), k_heap_free()
 */
ZTEST(k_heap_api, test_k_heap_calloc)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	uint32_t *p = (uint32_t *)k_heap_calloc(&k_heap_test, CALLOC_NUM, CALLOC_SIZE, timeout);

	zassert_not_null(p, "k_heap_calloc operation failed");
	for (int i = 0; i < CALLOC_NUM; i++) {
		zassert_equal(p[i], 0U);
	}

	k_heap_free(&k_heap_test, p);

	/* Overflow: num * size wraps around */
	p = k_heap_calloc(&k_heap_test, SIZE_MAX, SIZE_MAX, K_NO_WAIT);
	zassert_is_null(p, "k_heap_calloc with overflow should fail");

	/* Zero-size, should not crash */
	p = k_heap_calloc(&k_heap_test, 0, 0, K_NO_WAIT);
	(void)p;
}

/**
 * @brief Test k_heap_array_get()
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test ensures that valid values are returned
 *
 * @see k_heap_array_get()
 */
ZTEST(k_heap_api, test_k_heap_array_get)
{
	struct k_heap *ha = NULL;
	bool test_heap_found = false;
	int n;

	n = k_heap_array_get(&ha);
	zassert_not_equal(0, n, "No heaps returned");
	zassert_not_null(ha, "Heap array pointer not populated");

	/* Ensure that k_heap_test exists in the array */
	for (int i = 0; i < n; i++) {
		if (&k_heap_test == &ha[i]) {
			test_heap_found = true;
		}
	}
	zassert_true(test_heap_found);
}

/**
 * @brief Test k_heap_realloc() API functionality.
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test validates k_heap_realloc() API by:
 * 1. Allocating memory and reallocating to a larger size
 * 2. Reallocating to a smaller size
 * 3. Verifying data integrity after reallocation
 *
 * @see k_heap_realloc()
 */
ZTEST(k_heap_api, test_k_heap_realloc)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p, *p2, *p3;

	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, timeout);
	zassert_not_null(p, "k_heap_alloc operation failed");

	/* Fill the allocated memory with a pattern */
	for (int i = 0; i < ALLOC_SIZE_1; i++) {
		p[i] = 'A' + (i % 26);
	}

	/* Reallocate to a larger size */
	p2 = (char *)k_heap_realloc(&k_heap_test, p, ALLOC_SIZE_2, timeout);
	zassert_not_null(p2, "k_heap_realloc operation failed");

	/* Verify the original data is preserved */
	for (int i = 0; i < ALLOC_SIZE_1; i++) {
		zassert_equal(p2[i], 'A' + (i % 26), "Data integrity check failed");
	}

	/* Reallocate to a smaller size */
	p3 = (char *)k_heap_realloc(&k_heap_test, p2, ALLOC_SIZE_1 / 2, timeout);
	zassert_not_null(p3, "k_heap_realloc operation failed");

	/* Verify the data is preserved up to the new size */
	for (int i = 0; i < ALLOC_SIZE_1 / 2; i++) {
		zassert_equal(p3[i], 'A' + (i % 26), "Data integrity check failed");
	}

	k_heap_free(&k_heap_test, p3);
}

/**
 * @brief Test k_heap_realloc() with NULL pointer.
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test validates that k_heap_realloc() behaves like k_heap_alloc()
 * when called with a NULL pointer.
 *
 * @see k_heap_realloc()
 */
ZTEST(k_heap_api, test_k_heap_realloc_null)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p = (char *)k_heap_realloc(&k_heap_test, NULL, ALLOC_SIZE_1, timeout);

	zassert_not_null(p, "k_heap_realloc with NULL pointer failed");
	k_heap_free(&k_heap_test, p);
}

/**
 * @brief Test k_heap_realloc() with size 0.
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test validates that k_heap_realloc() behaves like k_heap_free()
 * when called with size 0.
 *
 * @see k_heap_realloc()
 */
ZTEST(k_heap_api, test_k_heap_realloc_zero)
{
	char *p, *p2;

	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, K_NO_WAIT);
	zassert_not_null(p, "k_heap_alloc operation failed");

	/* Realloc with size 0 should free the memory */
	p2 = (char *)k_heap_realloc(&k_heap_test, p, 0, K_NO_WAIT);
	zassert_is_null(p2, "k_heap_realloc with size 0 should return NULL");
}

/**
 * @brief Test k_heap_realloc() failure case.
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test validates that k_heap_realloc() returns NULL when
 * trying to reallocate to a size larger than the heap.
 *
 * @see k_heap_realloc()
 */
ZTEST(k_heap_api, test_k_heap_realloc_fail)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p, *p2;

	p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, timeout);
	zassert_not_null(p, "k_heap_alloc operation failed");

	/* Try to reallocate to a size larger than the heap */
	p2 = (char *)k_heap_realloc(&k_heap_test, p, HEAP_SIZE + 1, timeout);
	zassert_is_null(p2, "k_heap_realloc should fail for size larger than heap");

	k_heap_free(&k_heap_test, p);
}

/**
 * @brief Test k_heap_aligned_alloc() API usage and edge cases
 *
 * @ingroup k_heap_api_tests
 *
 * @details Allocates a block with a specific alignment from the heap
 * and checks alignment, then tries oversize and invalid alignment.
 *
 * @see k_heap_aligned_alloc()
 */
ZTEST(k_heap_api, test_k_heap_aligned_alloc)
{
	void *p;

	/* Allocate 128 bytes aligned to 16 bytes */
	p = k_heap_aligned_alloc(&k_heap_test, 16, 128, K_NO_WAIT);
	zassert_not_null(p, "k_heap_aligned_alloc failed");
	zassert_true(((uintptr_t)p % 16) == 0, "Pointer not 16-byte aligned");
	k_heap_free(&k_heap_test, p);

	/* Oversize allocation returns NULL */
	p = k_heap_aligned_alloc(&k_heap_test, 8, HEAP_SIZE * 2, K_NO_WAIT);
	zassert_is_null(p, "k_heap_aligned_alloc with oversize should fail");

	ztest_set_fault_valid(true);
	/* invalid alignment, should assert */
	p = k_heap_aligned_alloc(&k_heap_test, 3, 64, K_NO_WAIT);
	(void)p;
	/*
	 * If calling k_heap_aligned_alloc with invalid alignment didn't result in an assert
	 * then the API isn't working as expected, and the test shall fail.
	 */
	ztest_test_fail();
}

/*
 * should be run last because the double-freeing corrupts memory
 * (hence the prefix z_ in the test's name)
 */
/**
 * @brief Test k_heap_free() API double free edge case.
 *
 * @ingroup k_heap_api_tests
 *
 * @details The test validates that double-freeing a pointer asserts
 *
 * @see k_heap_alloc, k_heap_free()
 */
ZTEST(k_heap_api, test_z_k_heap_double_free)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, timeout);

	zassert_not_null(p, "k_heap_alloc operation failed");

	k_heap_free(&k_heap_test, p);

	ztest_set_fault_valid(true);
	/* Double free: should assert */
	k_heap_free(&k_heap_test, p);
	/*
	 * If calling k_heap_free twice on the same buffer didn't result in an assert
	 * then the API isn't working as expected, and the test shall fail.
	 */
	ztest_test_fail();
}

/**
 * @brief Test initializing a memory heap at run time over a provided region
 *
 * @ingroup k_heap_api_tests
 *
 * @details Initialize a k_heap at run time over a caller-provided memory buffer
 * with k_heap_init(), then confirm the heap is usable: a block can be allocated
 * (and the returned memory lies within the provided region and is writable),
 * an allocation larger than the region fails, and the block can be freed.
 *
 * @see k_heap_init()
 */
ZTEST(k_heap_api, test_k_heap_init)
{
	static char __aligned(8) heap_mem[1024];
	struct k_heap runtime_heap;
	char *p;

	k_heap_init(&runtime_heap, heap_mem, sizeof(heap_mem));

	/* The heap is usable: allocate a modest block from the provided region. */
	p = k_heap_alloc(&runtime_heap, 64, K_NO_WAIT);
	zassert_not_null(p, "allocation from a run-time initialized heap failed");
	zassert_true(p >= heap_mem && (p + 64) <= (heap_mem + sizeof(heap_mem)),
		     "allocated block is outside the provided memory region");

	/* The memory is writable. */
	memset(p, 0xa5, 64);

	/* The heap is bounded by the provided region. */
	zassert_is_null(k_heap_alloc(&runtime_heap, sizeof(heap_mem) * 2, K_NO_WAIT),
			"allocation larger than the heap region should fail");

	k_heap_free(&runtime_heap, p);
}

/* One (size, expectation) pair driving the parameterized alloc-size test. */
struct heap_alloc_case {
	size_t alloc_size;
	bool   expect_success;
};

/**
 * @brief Verify k_heap_alloc() succeeds within capacity and fails beyond it.
 *
 * @ingroup k_heap_api_tests
 *
 * @details
 * Parameterized over a range of block sizes: sizes within HEAP_SIZE must
 * allocate successfully and survive a write/read-back of a fill pattern;
 * sizes beyond heap capacity must return NULL. Each (size, expectation) pair
 * is reported as a separate named Twister case, so a failure pinpoints the
 * exact size rather than a single aggregate result.
 *
 * Test steps:
 * - Read the current (size, expect_success) parameter pair.
 * - Call k_heap_alloc() for that size.
 * - On a success case: assert non-NULL, write a per-byte pattern, read it back
 *   to confirm no corruption, then free the block.
 * - On a failure case: assert the call returned NULL.
 *
 * Expected result:
 * - In-capacity sizes allocate, preserve data, and free cleanly.
 * - Over-capacity sizes return NULL.
 *
 * @see k_heap_alloc()
 * @see k_heap_free()
 */
ZTEST_P(k_heap_api, test_k_heap_alloc_size)
{
	const struct heap_alloc_case *tc =
		ZTEST_GET_PARAM_PTR(struct heap_alloc_case);
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p = (char *)k_heap_alloc(&k_heap_test, tc->alloc_size, timeout);

	if (tc->expect_success) {
		zassert_not_null(p, "k_heap_alloc(%zu) failed unexpectedly",
				 tc->alloc_size);
		for (size_t i = 0; i < tc->alloc_size; i++) {
			p[i] = (char)(i & 0xFFU);
		}
		for (size_t i = 0; i < tc->alloc_size; i++) {
			zassert_equal((uint8_t)p[i], (uint8_t)(i & 0xFFU),
				      "data corruption at offset %zu", i);
		}
		k_heap_free(&k_heap_test, p);
	} else {
		zassert_is_null(p, "k_heap_alloc(%zu) succeeded unexpectedly",
				tc->alloc_size);
	}
}

static const struct heap_alloc_case heap_size_cases[] = {
	/* --- sizes that must succeed (well within HEAP_SIZE=2048) --- */
	{1U,    true},
	{16U,   true},
	{64U,   true},
	{256U,  true},
	{512U,  true},
	{1024U, true},
	{1536U, true},
	/* --- sizes that must fail (exceed heap capacity) --- */
	{2049U, false},
	{4096U, false},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(heap_alloc_sizes, heap_size_cases);
ZTEST_INSTANTIATE_TEST_SUITE_P(sizes, k_heap_api,
			       test_k_heap_alloc_size, heap_alloc_sizes);
