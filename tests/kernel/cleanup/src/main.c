/**
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/cleanup/kernel.h>

extern struct k_heap _system_heap;
static size_t free_bytes;

static void *cleanup_setup(void)
{
	struct sys_memory_stats stats;

	zassert_ok(sys_heap_runtime_stats_get(&_system_heap.heap, &stats));

	/* Store the amount of heap bytes usable in tests */
	free_bytes = stats.free_bytes;

	return NULL;
}

static void cleanup_after(void *fixture)
{
	struct sys_memory_stats stats;

	ARG_UNUSED(fixture);

	zassert_ok(sys_heap_runtime_stats_get(&_system_heap.heap, &stats));
	zassert_equal(free_bytes, stats.free_bytes, "Memory leaked in a test");
}

ZTEST(cleanup_api, test_guard_k_mutex)
{
	struct k_mutex lock;
	int ret;

	ret = k_mutex_init(&lock);
	zassert_ok(ret);

	{
		scope_guard(k_mutex)(&lock);
		zexpect_equal(lock.lock_count, 1);
	}

	zexpect_equal(lock.lock_count, 0);
}

ZTEST(cleanup_api, test_defer_k_mutex_unlock)
{
	struct k_mutex lock;
	int ret;

	ret = k_mutex_init(&lock);
	zassert_ok(ret);

	{
		ret = k_mutex_lock(&lock, K_NO_WAIT);
		zassert_ok(ret);
		scope_defer(k_mutex_unlock)(&lock);

		zexpect_equal(lock.lock_count, 1);
	}

	zexpect_equal(lock.lock_count, 0);
}

ZTEST(cleanup_api, test_guard_k_sem)
{
	struct k_sem lock;
	int ret;

	ret = k_sem_init(&lock, 1, 1);
	zassert_ok(ret);

	{
		scope_guard(k_sem)(&lock);
		zexpect_equal(lock.count, 0);
	}

	zexpect_equal(lock.count, 1);
}

ZTEST(cleanup_api, test_defer_k_sem_give)
{
	struct k_sem lock;
	int ret;

	ret = k_sem_init(&lock, 1, 1);
	zassert_ok(ret);

	{
		ret = k_sem_take(&lock, K_NO_WAIT);
		zassert_ok(ret);
		scope_defer(k_sem_give)(&lock);

		zexpect_equal(lock.count, 0);
	}

	zexpect_equal(lock.count, 1);
}

ZTEST(cleanup_api, test_defer_k_free)
{
	void *my_ptr = k_malloc(10);

	scope_defer(k_free)(my_ptr);

	zassert_not_null(my_ptr);

	/* Rely on cleanup_after to check that the ptr is freed */
}

ZTEST(cleanup_api, test_defer_k_heap_free)
{
	void *my_ptr = k_heap_alloc(&_system_heap, 42, K_FOREVER);

	scope_defer(k_heap_free)(&_system_heap, my_ptr);

	zassert_not_null(my_ptr);

	/* Rely on cleanup_after to check that the ptr is freed */
}

K_MEM_SLAB_DEFINE_STATIC(test_slabs, 4, 1, 1);
ZTEST(cleanup_api, test_defer_k_mem_slab_free)
{
	void *ptr;
	int ret;

	zexpect_equal(k_mem_slab_num_used_get(&test_slabs), 0);

	{
		ret = k_mem_slab_alloc(&test_slabs, &ptr, K_NO_WAIT);
		zassert_ok(ret);
		scope_defer(k_mem_slab_free)(&test_slabs, ptr);

		zexpect_equal(k_mem_slab_num_used_get(&test_slabs), 1);
	}

	zexpect_equal(k_mem_slab_num_used_get(&test_slabs), 0);
}

static bool void_function_called;
static void void_function(void)
{
	void_function_called = true;
}
SCOPE_DEFER_DEFINE(void_function);

ZTEST(cleanup_api, test_defer_void_function)
{
	{
		scope_defer(void_function)();

		zexpect_false(void_function_called);
	}

	zexpect_true(void_function_called);
}

struct foo {
	uint8_t *const buf;
	const size_t buf_len;
};

static inline struct foo foo_constructor(size_t len)
{
	return (struct foo){
		.buf = k_malloc(len),
		.buf_len = len,
	};
}

static inline void foo_destructor(struct foo f)
{
	k_free(f.buf);
}

SCOPE_VAR_DEFINE(foo, struct foo, foo_destructor(_T), foo_constructor(len), size_t len);

ZTEST(cleanup_api, test_custom_cleanup_helper)
{
	scope_var(foo, f)(42);

	zexpect_not_null(f.buf);
	zexpect_equal(f.buf_len, 42);

	/* Rely on cleanup_after to check that f is destructed */
}

ZTEST_SUITE(cleanup_api, NULL, cleanup_setup, NULL, cleanup_after, NULL);
