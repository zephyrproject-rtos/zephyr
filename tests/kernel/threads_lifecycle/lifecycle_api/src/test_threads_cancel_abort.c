/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_threads_lifecycle
 * @{
 * @defgroup t_threads_cancel_abort test_threads_cancel_abort
 * @}
 */
#include <ztest.h>

#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static int execute_flag;

static void thread_entry(void *p1, void *p2, void *p3)
{
	execute_flag = 1;
	k_sleep(100);
	execute_flag = 2;
}

static void thread_entry_abort(void *p1, void *p2, void *p3)
{
	/**TESTPOINT: abort current thread*/
	execute_flag = 1;
	k_thread_abort(k_current_get());
	/*unreachable*/
	execute_flag = 2;
	zassert_true(1 == 0, NULL);
}

/*test cases*/
void test_threads_cancel_undelayed(void)
{
	int cur_prio = k_thread_priority_get(k_current_get());

	/* spawn thread with lower priority */
	int spawn_prio = cur_prio + 1;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      spawn_prio, 0, 0);

	/**TESTPOINT: check cancel retcode when thread is not delayed*/
	int cancel_ret = k_thread_cancel(tid);

	zassert_equal(cancel_ret, -EINVAL, NULL);
	k_thread_abort(tid);
}

void test_threads_cancel_started(void)
{
	int cur_prio = k_thread_priority_get(k_current_get());

	/* spawn thread with lower priority */
	int spawn_prio = cur_prio + 1;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      spawn_prio, 0, 0);

	k_sleep(50);
	/**TESTPOINT: check cancel retcode when thread is started*/
	int cancel_ret = k_thread_cancel(tid);

	zassert_equal(cancel_ret, -EINVAL, NULL);
	k_thread_abort(tid);
}

void test_threads_cancel_delayed(void)
{
	int cur_prio = k_thread_priority_get(k_current_get());

	/* spawn thread with lower priority */
	int spawn_prio = cur_prio + 1;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      spawn_prio, 0, 100);

	k_sleep(50);
	/**TESTPOINT: check cancel retcode when thread is started*/
	int cancel_ret = k_thread_cancel(tid);

	zassert_equal(cancel_ret, 0, NULL);
	k_thread_abort(tid);
}

void test_threads_abort_self(void)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_abort, NULL, NULL, NULL,
				      0, 0, 0);
	k_sleep(100);
	/**TESTPOINT: spawned thread executed but abort itself*/
	zassert_true(execute_flag == 1, NULL);
	k_thread_abort(tid);
}

void test_threads_abort_others(void)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, 0, 0);

	k_thread_abort(tid);
	k_sleep(100);
	/**TESTPOINT: check not-started thread is aborted*/
	zassert_true(execute_flag == 0, NULL);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      0, 0, 0);
	k_sleep(50);
	k_thread_abort(tid);
	/**TESTPOINT: check running thread is aborted*/
	zassert_true(execute_flag == 1, NULL);
	k_sleep(1000);
	zassert_true(execute_flag == 1, NULL);
}
