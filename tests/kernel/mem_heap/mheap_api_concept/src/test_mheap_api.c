/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel_internal.h>
#include <irq_offload.h>
#include "test_mheap.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define OVERFLOW_SIZE    SIZE_MAX

K_SEM_DEFINE(thread_sem, 0, 1);
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
struct k_thread tdata;

static void tIsr_malloc_and_free(void *data)
{
	ARG_UNUSED(data);
	void *ptr;

	ptr = (char *)z_thread_malloc(BLK_SIZE_MIN);
	zassert_not_null(ptr, "bytes allocation failed from system pool");
	k_free(ptr);
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	void *ptr;

	k_current_get()->resource_pool = NULL;

	ptr = (char *)z_thread_malloc(BLK_SIZE_MIN);
	zassert_is_null(ptr, "bytes allocation failed from system pool");

	k_sem_give(&thread_sem);
}

/*test cases*/

/**
 * @brief Test to demonstrate k_malloc() and k_free() API usage
 *
 * @ingroup kernel_heap_tests
 *
 * @details The test allocates 4 blocks from heap memory pool
 * using k_malloc() API. It also tries to allocate a block of size
 * 64 bytes which fails as all the memory is allocated up. It then
 * validates k_free() API by freeing up all the blocks which were
 * allocated from the heap memory.
 *
 * @see k_malloc()
 */
void test_mheap_malloc_free(void)
{
	void *block[2 * BLK_NUM_MAX], *block_fail;
	int nb;

	for (nb = 0; nb < ARRAY_SIZE(block); nb++) {
		/**
		 * TESTPOINT: This routine provides traditional malloc()
		 * semantics. Memory is allocated from the heap memory pool.
		 */
		block[nb] = k_malloc(BLK_SIZE_MIN);
		if (block[nb] == NULL) {
			break;
		}
	}

	block_fail = k_malloc(BLK_SIZE_MIN);
	/** TESTPOINT: Return NULL if fail.*/
	zassert_is_null(block_fail, NULL);

	for (int i = 0; i < nb; i++) {
		/**
		 * TESTPOINT: This routine provides traditional free()
		 * semantics. The memory being returned must have been allocated
		 * from the heap memory pool.
		 */
		k_free(block[i]);
	}
	/** TESTPOINT: If ptr is NULL, no operation is performed.*/
	k_free(NULL);

	/** TESTPOINT: Return NULL if fail.*/
	block_fail = k_malloc(OVERFLOW_SIZE);
	zassert_is_null(block_fail, NULL);
}

#define NMEMB   8
#define SIZE    16
#define BOUNDS  (NMEMB * SIZE)

/**
 * @brief Test to demonstrate k_calloc() API functionality.
 *
 * @ingroup kernel_heap_tests
 *
 * @details The test validates k_calloc() API. When requesting a
 * huge size of space or a space larger than heap memory,
 * the API will return NULL. The 8 blocks of memory of
 * size 16 bytes are allocated by k_calloc() API. When allocated using
 * k_calloc() the memory buffers have to be zeroed. Check is done, if the
 * blocks are memset to 0 and read/write is allowed. The test is then
 * teared up by freeing all the blocks allocated.
 *
 * @see k_calloc()
 */
void test_mheap_calloc(void)
{
	char *mem;

	/* Requesting a huge size to validate overflow */
	mem = k_calloc(NMEMB, OVERFLOW_SIZE);
	zassert_is_null(mem, "calloc operation failed");

	/* Requesting a space large than heap memory lead to failure */
	mem = k_calloc(NMEMB * 3, SIZE);
	zassert_is_null(mem, "calloc operation failed");

	mem = k_calloc(NMEMB, SIZE);
	zassert_not_null(mem, "calloc operation failed");

	/* Memory should be zeroed and not crash us if we read/write to it */
	for (int i = 0; i < BOUNDS; i++) {
		zassert_equal(mem[i], 0, NULL);
		mem[i] = 1;
	}

	k_free(mem);
}

void test_k_aligned_alloc(void)
{
	void *r;

	/*
	 * Allow sizes that are not necessarily a multiple of the
	 * alignment. The backing allocator would naturally round up to
	 * some minimal block size. This would make k_aligned_alloc()
	 * more like posix_memalign() instead of aligned_alloc(), but
	 * the benefit is that k_malloc() can then just be a wrapper
	 * around k_aligned_alloc().
	 */
	r = k_aligned_alloc(sizeof(void *), 1);
	/* allocation succeeds */
	zassert_not_equal(NULL, r, "aligned alloc of 1 byte failed");
	/* r is suitably aligned */
	zassert_equal(0, (uintptr_t)r % sizeof(void *),
		"%p not %u-byte-aligned",
		r, sizeof(void *));
	k_free(r);

	/* allocate with > 8 byte alignment */
	r = k_aligned_alloc(16, 1);
	/* allocation succeeds */
	zassert_not_equal(NULL, r, "16-byte-aligned alloc failed");
	/* r is suitably aligned */
	zassert_equal(0, (uintptr_t)r % 16,
		"%p not 16-byte-aligned", r);
	k_free(r);
}

/**
 * @brief Validate allocation and free from system heap memory pool.

 * @details Set heap memory as resource pool. It will success when alloc
 * a block of memory smaller than the pool and will fail when alloc
 * a block of memory larger than the pool.
 *
 * @ingroup kernel_heap_tests
 *
 * @see k_thread_system_pool_assign(), z_thread_malloc(), k_free()
 */
void test_sys_heap_mem_pool_assign(void)
{
	void *ptr;

	k_thread_system_pool_assign(k_current_get());
	ptr = (char *)z_thread_malloc(BLK_SIZE_MIN/2);
	zassert_not_null(ptr, "bytes allocation failed from system pool");
	k_free(ptr);

	zassert_is_null((char *)z_thread_malloc(BLK_SIZE_MAX * 2),
						"overflow check failed");
}

/**
 * @brief Validate allocation and free from system heap memory pool
 * in isr context.
 *
 * @details When in isr context, the kernel will successfully alloc a block of
 * memory because in this situation, the kernel will assign the heap memory
 * as resource pool.
 *
 * @ingroup kernel_heap_tests
 *
 * @see z_thread_malloc(), k_free()
 */
void test_malloc_in_isr(void)
{
	irq_offload((irq_offload_routine_t)tIsr_malloc_and_free, NULL);
}

/**
 * @brief Validate allocation and free failure when thread's resource pool
 * is not assigned.
 *
 * @details When a thread's resource pool is not assigned, alloc memory will fail.
 *
 * @ingroup kernel_heap_tests
 *
 * @see z_thread_malloc()
 */
void test_malloc_in_thread(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      0, 0, K_NO_WAIT);

	k_sem_take(&thread_sem, K_FOREVER);

	k_thread_abort(tid);
}
