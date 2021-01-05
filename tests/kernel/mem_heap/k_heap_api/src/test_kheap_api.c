/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include "test_kheap.h"

K_HEAP_DEFINE(k_heap_test, HEAP_SIZE);

#define ALLOC_SIZE_1 1024
#define ALLOC_SIZE_2 1536
#define ALLOC_SIZE_3 2049

static void tIsr_kheap_alloc_nowait(void *data)
{
	ARG_UNUSED(data);

	char *p = (char *)k_heap_alloc(&k_heap_test, ALLOC_SIZE_1, K_NO_WAIT);

	zassert_not_null(p, "k_heap_alloc operation failed");
	k_heap_free(&k_heap_test, p);
}

/*test cases*/

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
void test_k_heap_alloc(void)
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
void test_k_heap_alloc_fail(void)
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
void test_k_heap_free(void)
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
 * @ingroup kernel_heap_tests
 */
void test_kheap_alloc_in_isr_nowait(void)
{
	irq_offload((irq_offload_routine_t)tIsr_kheap_alloc_nowait, NULL);
}
