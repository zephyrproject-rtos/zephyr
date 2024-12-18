/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include "test_kheap.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
struct k_thread tdata;

K_HEAP_DEFINE(k_heap_test, HEAP_SIZE);

#define ALLOC_SIZE_1 1024
#define ALLOC_SIZE_2 1536
#define ALLOC_SIZE_3 2049
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

/** @brief Test a minimum-size static k_heap
 *  @ingroup kernel_kheap_api_tests
 *
 * @details Create a minimum size (1-byte) static heap, verify that it
 * works to allocate that byte at runtime and that it doesn't overflow
 * its memory bounds.
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
 * @brief Test to demonstrate k_heap_alloc() and k_heap_free() API usage
 *
 * @ingroup kernel_kheap_api_tests
 *
 * @details The test allocates 1024 bytes from 2048 byte heap,
 * and checks if allocation is successful or not
 *
 * @see k_heap_malloc(), k_heap_Free()
 */
ZTEST(k_heap_api, test_k_heap_alloc)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, timeout);

	zassert_not_null(p, "k_heap_alloc operation failed");

	for (int i = 0; i < ALLOC_SIZE_1; i++) {
		p[i] = '0';
	}
	k_heap_free(&k_heap_test, p);
}


/**
 * @brief Test to demonstrate k_heap_alloc() and k_heap_free() API usage
 *
 * @ingroup kernel_kheap_api_tests
 *
 * @details The test allocates 2049 bytes, which is greater than the heap
 * size(2048 bytes), and checks for NULL return from k_heap_alloc
 *
 * @see k_heap_malloc(), k_heap_Free()
 */
ZTEST(k_heap_api, test_k_heap_alloc_fail)
{

	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_3, timeout);

	zassert_is_null(p, NULL);

	k_heap_free(&k_heap_test, p);
}


/**
 * @brief Test to demonstrate k_heap_free() API functionality.
 *
 * @ingroup kernel_kheap_api_tests
 *
 * @details The test validates k_heap_free()
 * API, by using below steps
 * 1. allocate the memory from the heap,
 * 2. free the allocated memory
 * 3. allocate  memory more than the first allocation.
 * the allocation in the 3rd step should succeed if k_heap_free()
 * works as expected
 *
 * @see k_heap_alloc, k_heap_free()
 */
ZTEST(k_heap_api, test_k_heap_free)
{
	k_timeout_t timeout = Z_TIMEOUT_US(TIMEOUT);
	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, timeout);

	zassert_not_null(p, "k_heap_alloc operation failed");
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
 * @ingroup kernel_heap_tests
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
 * @ingroup kernel_heap_tests
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
 * @ingroup kernel_heap_tests
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
 * @brief Test to demonstrate k_heap_calloc() and k_heap_free() API usage
 *
 * @ingroup kernel_kheap_api_tests
 *
 * @details The test allocates 256 unsigned integers of 4 bytes for a
 * total of 1024 bytes from the 2048 byte heap. It checks if allocation
 * and initialization are successful or not
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
}
