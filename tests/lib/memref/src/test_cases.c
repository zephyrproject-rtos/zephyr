/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "memref_testcases.h"

#define MEMREF_TEST_MAX_BLOCKS 16

void memref_test_basic(memref_test_alloc_t alloc)
{
	void *mem;

	mem = alloc();
	zassert_not_null(mem, "alloc failed");
	memref_unref(mem);
}

void memref_test_payload_rw(memref_test_alloc_t alloc)
{
	uint8_t *mem;
	const size_t len = 16;

	mem = alloc();
	zassert_not_null(mem, "alloc failed");

	for (size_t i = 0; i < len; i++) {
		mem[i] = (uint8_t)(i + 1);
	}
	for (size_t i = 0; i < len; i++) {
		zassert_equal(mem[i], (uint8_t)(i + 1), "payload corrupted at %zu", i);
	}

	memref_unref(mem);
}

void memref_test_alignment(memref_test_alloc_t alloc)
{
	void *mem;

	mem = alloc();
	zassert_not_null(mem, "alloc failed");
	zassert_equal((uintptr_t)mem % sizeof(void *), 0, "payload not aligned");
	memref_unref(mem);
}

void memref_test_reuse_after_free(memref_test_alloc_t alloc)
{
	void *first;
	void *second;

	first = alloc();
	zassert_not_null(first, "first alloc failed");
	memref_unref(first);

	second = alloc();
	zassert_not_null(second, "alloc after free failed");
	memref_unref(second);
}

void memref_test_independent_allocs(memref_test_alloc_t alloc)
{
	const size_t count = 3;
	uint32_t *mem[3];

	for (size_t i = 0; i < count; i++) {
		mem[i] = alloc();
		zassert_not_null(mem[i], "alloc %zu failed", i);
		*mem[i] = (uint32_t)(0xA5A50000u + i);
	}

	for (size_t i = 0; i < count; i++) {
		zassert_equal(*mem[i], (uint32_t)(0xA5A50000u + i),
			      "alloc %zu aliased or corrupted", i);
		memref_unref(mem[i]);
	}
}

void memref_test_unref_null(void)
{
	memref_unref(NULL);
}

void memref_test_stress_allocs(memref_test_alloc_t alloc)
{
	void *mem;
	const size_t iterations = 100;

	for (size_t i = 0; i < iterations; i++) {
		mem = alloc();
		zassert_not_null(mem, "alloc failed");
		memref_unref(mem);
	}
}

void memref_test_exhaustion(memref_test_alloc_t alloc, size_t capacity)
{
	void *mem[MEMREF_TEST_MAX_BLOCKS];

	zassert_true(capacity <= MEMREF_TEST_MAX_BLOCKS, "capacity too large for test");

	for (size_t i = 0; i < capacity; i++) {
		mem[i] = alloc();
		zassert_not_null(mem[i], "alloc %zu failed before exhaustion", i);
	}

	zassert_is_null(alloc(), "alloc succeeded past capacity");

	memref_unref(mem[0]);
	mem[0] = alloc();
	zassert_not_null(mem[0], "alloc failed after freeing a block");

	for (size_t i = 0; i < capacity; i++) {
		memref_unref(mem[i]);
	}
}

void memref_test_full_reuse(memref_test_alloc_t alloc, size_t capacity)
{
	void *mem[MEMREF_TEST_MAX_BLOCKS];

	zassert_true(capacity <= MEMREF_TEST_MAX_BLOCKS, "capacity too large for test");

	for (size_t round = 0; round < 2; round++) {
		for (size_t i = 0; i < capacity; i++) {
			mem[i] = alloc();
			zassert_not_null(mem[i], "round %zu alloc %zu failed", round, i);
		}
		for (size_t i = 0; i < capacity; i++) {
			memref_unref(mem[i]);
		}
	}
}

void memref_test_alloc_overflow(const struct memref_backend *be)
{
	zassert_is_null(memref_alloc(be, SIZE_MAX), "alloc overflow not rejected");
}

void memref_test_zero_size_alloc(const struct memref_backend *be)
{
	void *mem = memref_alloc(be, 0);

	zassert_true(mem == NULL, "zero size alloc should result in no-alloc");
}

void memref_test_varied_sizes(const struct memref_backend *be)
{
	const size_t sizes[] = {1, 7, 31, 64, 200};

	for (size_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		uint8_t *mem = memref_alloc(be, sizes[i]);

		zassert_not_null(mem, "alloc of %zu failed", sizes[i]);
		for (size_t j = 0; j < sizes[i]; j++) {
			mem[j] = (uint8_t)j;
		}
		for (size_t j = 0; j < sizes[i]; j++) {
			zassert_equal(mem[j], (uint8_t)j, "corrupted at %zu", j);
		}
		memref_unref(mem);
	}
}

#ifdef CONFIG_MEMREF_DESTROY_CB
struct container {
	struct k_sem *sem;
};

static void container_destroy(void *ptr)
{
	struct container *mem = ptr;

	k_sem_give(mem->sem);
}

static void *destroyed_ptr;

static void record_destroy(void *ptr)
{
	destroyed_ptr = ptr;
}

static int destroy_count;

static void count_destroy(void *ptr)
{
	ARG_UNUSED(ptr);

	destroy_count++;
}

void memref_test_destroy_called(memref_test_alloc_cb_t alloc_cb)
{
	struct container *mem;
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	mem = alloc_cb(container_destroy);
	zassert_not_null(mem, "alloc_cb failed");
	mem->sem = &sem;
	memref_unref(mem);
	zassert_ok(k_sem_take(&sem, K_MSEC(100)), "destroy not called");
}

void memref_test_multi_owners(memref_test_alloc_cb_t alloc_cb)
{
	struct container *mem;
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	mem = alloc_cb(container_destroy);
	zassert_not_null(mem, "alloc_cb failed");
	mem->sem = &sem;
	memref_ref(mem);
	memref_unref(mem);
	zassert_not_equal(k_sem_take(&sem, K_MSEC(100)), 0,
			  "destroy called when refcount > 0");
	memref_unref(mem);
	zassert_ok(k_sem_take(&sem, K_MSEC(100)), "destroy not called");
}

void memref_test_destroy_pointer_identity(memref_test_alloc_cb_t alloc_cb)
{
	void *mem;

	destroyed_ptr = NULL;
	mem = alloc_cb(record_destroy);
	zassert_not_null(mem, "alloc_cb failed");
	memref_unref(mem);
	zassert_equal(destroyed_ptr, mem, "destroy received a different pointer");
}

void memref_test_ref_depth(memref_test_alloc_cb_t alloc_cb)
{
	const int depth = 5;
	void *mem;

	destroy_count = 0;
	mem = alloc_cb(count_destroy);
	zassert_not_null(mem, "alloc_cb failed");

	for (int i = 0; i < depth; i++) {
		memref_ref(mem);
	}
	for (int i = 0; i < depth; i++) {
		memref_unref(mem);
		zassert_equal(destroy_count, 0, "destroy called while refs remain");
	}

	memref_unref(mem);
	zassert_equal(destroy_count, 1, "destroy not called exactly once");
}
#endif
