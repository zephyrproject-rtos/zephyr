/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/mp/core/mp_thread.h>

static struct k_sem thread_ran_sem;
static atomic_t thread_run_count;

static void simple_thread_func(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_inc(&thread_run_count);
	k_sem_give((struct k_sem *)p1);
}

extern struct k_heap _system_heap;

struct mp_thread_api_fixture {
	struct mp_thread thread;
	struct sys_memory_stats mem_before;
};

static void *thread_suite_setup(void)
{
	static struct mp_thread_api_fixture fixture;

	return &fixture;
}

static void thread_before(void *f)
{
	struct mp_thread_api_fixture *fix = f;

	memset(&fix->thread, 0, sizeof(fix->thread));
	fix->thread.stack_id = -1;

	k_sem_init(&thread_ran_sem, 0, 1);
	atomic_set(&thread_run_count, 0);

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->mem_before);
}

static void thread_after(void *f)
{
	struct mp_thread_api_fixture *fix = f;
	struct sys_memory_stats mem_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &mem_after);
	zassert_equal(fix->mem_before.allocated_bytes, mem_after.allocated_bytes,
		      "Memory leak detected: before=%zu after=%zu", fix->mem_before.allocated_bytes,
		      mem_after.allocated_bytes);
}

ZTEST_SUITE(mp_thread_api, NULL, thread_suite_setup, thread_before, thread_after, NULL);

ZTEST_F(mp_thread_api, test_create)
{
	k_tid_t tid = mp_thread_create(&fixture->thread, simple_thread_func, &thread_ran_sem, NULL,
				       NULL, CONFIG_MP_THREAD_DEFAULT_PRIORITY, K_NO_WAIT);

	zassert_not_null(tid, "mp_thread_create returned NULL");
	zassert_true(fixture->thread.stack_id >= 0 &&
			     fixture->thread.stack_id < CONFIG_MP_THREADS_NUM,
		     "stack_id %d out of range [0, %d)", fixture->thread.stack_id,
		     CONFIG_MP_THREADS_NUM);

	int ret = k_sem_take(&thread_ran_sem, K_MSEC(1000));

	zassert_equal(ret, 0, "entry function not called within 1s");
	zassert_equal(atomic_get(&thread_run_count), 1, "entry function call count != 1");
}
