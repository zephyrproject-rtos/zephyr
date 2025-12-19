/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "memref_testcases.h"

#ifdef CONFIG_MEMREF_DESTROY_CB
struct container {
	struct k_sem *sem;
};

static void container_destroy(void *ptr)
{
	struct container *mem = ptr;

	k_sem_give(mem->sem);
}
#endif

void memref_test_basic(const struct memref_backend *be)
{
	void *mem;

	mem = memref_alloc(be, 64);
	zassert_not_null(mem, "memref_alloc failed");
	memref_unref(mem);
}

#ifdef CONFIG_MEMREF_DESTROY_CB
void memref_test_destroy_called(const struct memref_backend *be)
{
	struct container *mem;
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	mem = memref_alloc_cb(be, sizeof(*mem), container_destroy);
	zassert_not_null(mem, "memref_alloc_cb failed");
	mem->sem = &sem;
	memref_unref(mem);
	zassert_ok(k_sem_take(&sem, K_MSEC(100)), "destroy not called");
}

void memref_test_multi_owners(const struct memref_backend *be)
{
	struct container *mem;
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	mem = memref_alloc_cb(be, sizeof(*mem), container_destroy);
	zassert_not_null(mem, "memref_alloc_cb failed");
	mem->sem = &sem;
	memref_ref(mem);
	memref_unref(mem);
	zassert_not_equal(k_sem_take(&sem, K_MSEC(100)), 0,
			  "destroy called when refcount > 0");
	memref_unref(mem);
	zassert_ok(k_sem_take(&sem, K_MSEC(100)), "destroy not called");
}
#endif

void memref_test_calloc_zeroed(const struct memref_backend *be)
{
	uint8_t *mem;

	mem = memref_calloc(be, 8, 12);
	zassert_not_null(mem, "memref_alloc failed");
	for (size_t i = 0; i < 8 * 12; i++) {
		zassert_equal(mem[i], 0, "mem not zeroed");
	}
	memref_unref(mem);
}

void memref_test_stress_allocs(const struct memref_backend *be)
{
	void *mem;
	const size_t alloc_size = 64;
	const size_t iterations = 100;

	for (size_t i = 0; i < iterations; i++) {
		mem = memref_alloc(be, alloc_size);
		zassert_not_null(mem, "memref_alloc failed");
		memref_unref(mem);
	}
}
