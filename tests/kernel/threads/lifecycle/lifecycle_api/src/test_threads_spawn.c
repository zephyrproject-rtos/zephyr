/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_threads_lifecycle
 * @{
 * @defgroup t_threads_spawn test_thread_spawn
 * @brief TestPurpose: verify basic thread spawn relevant apis
 * @}
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

/*test cases*/
void test_threads_spawn_params(void)
{
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_params,
			(void *)tp1, (void *)tp2, (void *)tp3, 0,
			K_USER, 0);
	k_sleep(100);
}

void test_threads_spawn_priority(void)
{
	/* spawn thread with higher priority */
	spawn_prio = k_thread_priority_get(k_current_get()) - 1;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_priority,
			NULL, NULL, NULL, spawn_prio, K_USER, 0);
	k_sleep(100);
}

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
