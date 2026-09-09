/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/mpipe/mpipe_thread.h>

static struct k_sem thread_ran_sem;
static atomic_t thread_run_count;

static void simple_thread_func(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_inc(&thread_run_count);
	k_sem_give((struct k_sem *)p1);
}

struct mpipe_thread_api_fixture {
	struct mpipe_thread thread;
};

static void *thread_suite_setup(void)
{
	static struct mpipe_thread_api_fixture fixture;

	return &fixture;
}

static void thread_before(void *f)
{
	struct mpipe_thread_api_fixture *fix = f;

	memset(&fix->thread, 0, sizeof(fix->thread));
	fix->thread.stack_id = -1;

	k_sem_init(&thread_ran_sem, 0, 1);
	atomic_set(&thread_run_count, 0);
}

ZTEST_SUITE(mpipe_thread_api, NULL, thread_suite_setup, thread_before, NULL, NULL);

ZTEST_F(mpipe_thread_api, test_create)
{
	k_tid_t tid =
		mpipe_thread_create(&fixture->thread, simple_thread_func, &thread_ran_sem, NULL,
				    NULL, CONFIG_MPIPE_THREAD_DEFAULT_PRIORITY, K_NO_WAIT);

	zassert_not_null(tid, "mpipe_thread_create returned NULL");
	zassert_true(fixture->thread.stack_id >= 0 &&
			     fixture->thread.stack_id < CONFIG_MPIPE_THREADS_NUM,
		     "stack_id %d out of range [0, %d)", fixture->thread.stack_id,
		     CONFIG_MPIPE_THREADS_NUM);

	int ret = k_sem_take(&thread_ran_sem, K_MSEC(1000));

	zassert_equal(ret, 0, "entry function not called within 1s");
	zassert_equal(atomic_get(&thread_run_count), 1, "entry function call count != 1");
}
