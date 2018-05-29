/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>

#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_EXTERN(tstack);
extern struct k_thread tdata;

static char tp1[8];
static int tp2 = 100;
static struct k_sema *tp3;
static int spawn_prio;

static void thread_entry_params(void *p1, void *p2, void *p3)
{
	/* checkpoint: check parameter 1, 2, 3 */
	zassert_equal((char *)p1, tp1, NULL);
	zassert_equal((int)p2, tp2, NULL);
	zassert_equal((struct k_sema *)p3, tp3, NULL);
}

static void thread_entry_priority(void *p1, void *p2, void *p3)
{
	/* checkpoint: check priority */
	zassert_equal(k_thread_priority_get(k_current_get()), spawn_prio, NULL);
}

static void thread_entry_delay(void *p1, void *p2, void *p3)
{
	tp2 = 100;
}

/* test cases */

/**
 * @ingroup kernel_thread_tests
 * @brief Check the parameters passed to thread entry function
 *
 * @details Create an user thread and pass 2 variables and a
 * semaphore to a thread entry function. Check for the correctness
 * of the parameters passed.
 *
 * @see k_thread_create()
 */
void test_threads_spawn_params(void)
{
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_params,
			(void *)tp1, (void *)tp2, (void *)tp3, 0,
			K_USER, 0);
	k_sleep(100);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Spawn thread with higher priority
 *
 * @details Create an user thread with priority greater than
 * current thread and check its behavior.
 *
 * @see k_thread_create()
 */
void test_threads_spawn_priority(void)
{
	/* spawn thread with higher priority */
	spawn_prio = k_thread_priority_get(k_current_get()) - 1;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_priority,
			NULL, NULL, NULL, spawn_prio, K_USER, 0);
	k_sleep(100);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Spawn thread with a delay
 *
 * @details Create a user thread with delay and check if the
 * thread entry function is executed only after the timeout occurs.
 *
 * @see k_thread_create()
 */
void test_threads_spawn_delay(void)
{
	/* spawn thread with higher priority */
	tp2 = 10;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_delay,
			NULL, NULL, NULL, 0, K_USER, 120);
	/* 100 < 120 ensure spawn thread not start */
	k_sleep(100);
	/* checkpoint: check spawn thread not execute */
	zassert_true(tp2 == 10, NULL);
	/* checkpoint: check spawn thread executed */
	k_sleep(100);
	zassert_true(tp2 == 100, NULL);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Spawn thread with forever delay and highest priority
 *
 * @details Create an user thread with forever delay and yield
 * the current thread. Even though the current thread has yielded,
 * the thread will not be put in ready queue since it has forever delay,
 * the thread is explicitly started using k_thread_start() and checked
 * if thread has started executing.
 *
 * @see k_thread_create()
 */
void test_threads_spawn_forever(void)
{
	/* spawn thread with highest priority. It will run immediately once
	 * started.
	 */
	tp2 = 10;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_delay, NULL, NULL, NULL,
				      K_HIGHEST_THREAD_PRIO,
				      K_USER, K_FOREVER);
	k_yield();
	/* checkpoint: check spawn thread not execute */
	zassert_true(tp2 == 10, NULL);
	/* checkpoint: check spawn thread executed */
	k_thread_start(tid);
	k_yield();
	zassert_true(tp2 == 100, NULL);
	k_thread_abort(tid);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Validate behavior of multiple calls to k_thread_start()
 *
 * @details Call k_thread_start() on an already terminated thread
 *
 * @see k_thread_start()
 */
void test_thread_start(void)
{
	tp2 = 5;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_delay, NULL, NULL, NULL,
				      K_HIGHEST_THREAD_PRIO,
				      K_USER, K_FOREVER);

	k_thread_start(tid);
	k_yield();
	zassert_true(tp2 == 100, NULL);

	/* checkpoint: k_thread_start() should not start the
	 * terminated thread
	 */

	tp2 = 50;
	k_thread_start(tid);
	k_yield();
	zassert_false(tp2 == 100, NULL);
}
