/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "memref_testcases.h"

struct container {
	struct k_sem *sem;
};

static void container_cleanup(void *ptr)
{
	struct container *mem = ptr;

	k_sem_give(mem->sem);
}

void memref_test_basic(const struct memref_backend *be)
{
	void *mem;

	mem = memref_alloc(be, 64, NULL);
	zassert_not_null(mem, "memref_alloc failed");
	memref_unref(mem);
}

void memref_test_cleanup_called(const struct memref_backend *be)
{
	struct container *mem;
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	mem = memref_alloc(be, sizeof(*mem), container_cleanup);
	zassert_not_null(mem, "memref_alloc failed");
	mem->sem = &sem;
	memref_unref(mem);
	zassert_ok(k_sem_take(&sem, K_MSEC(100)), "cleanup not called");
}

void memref_test_multi_owners(const struct memref_backend *be)
{
	struct container *mem;
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	mem = memref_alloc(be, sizeof(*mem), container_cleanup);
	zassert_not_null(mem, "memref_alloc failed");
	mem->sem = &sem;
	memref_ref(mem);
	memref_unref(mem);
	zassert_not_equal(k_sem_take(&sem, K_MSEC(100)), 0,
			  "cleanup called when refcount > 0");
	memref_unref(mem);
	zassert_ok(k_sem_take(&sem, K_MSEC(100)), "cleanup not called");
}

void memref_test_calloc_zeroed(const struct memref_backend *be)
{
	uint8_t *mem;

	mem = memref_calloc(be, 8, 12, NULL);
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
		mem = memref_alloc(be, alloc_size, NULL);
		zassert_not_null(mem, "memref_alloc failed");
		memref_unref(mem);
	}
}
