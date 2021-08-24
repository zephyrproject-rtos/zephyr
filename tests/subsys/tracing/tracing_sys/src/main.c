/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>

/**
 * @brief Tests for tracing
 * @defgroup subsys_tracing_tests Tracing
 * @ingroup all_tests
 * @{
 * @}
 */

/* size of stack area used by each thread */
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

static struct k_thread thread;
K_THREAD_STACK_DEFINE(thread_stack, STACKSIZE);

static bool area_avil;

/* define semaphore and a mutex */
static struct k_sem sem;
static struct k_mutex mutex;

/* thread handle for switch */
static void thread_handle(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem, K_FOREVER);

	/* try to access area protected by mutex */
	k_mutex_lock(&mutex, K_FOREVER);
	area_avil = true;
	k_mutex_unlock(&mutex);
}

/**
 * @brief Test tracing functionality
 *
 * @details Define and initialize some kernel objects, and
 * spawn a thread to take and give a semaphore, then lock and unlock
 * a mutex, and define a bool value to assert the operation of
 * the subthread is correct. Enable tracing module to check the output
 * from a backend, meanwhile, check the bool value to make sure the
 * tracing functionality doesn't interfere with normal operations
 * of the operating system.
 *
 * @ingroup subsys_tracing_tests
 */
void test_tracing_sys(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	int new_prio = 10;

	k_thread_priority_set(k_current_get(), new_prio);
	k_sem_init(&sem, 0, 1);
	k_mutex_init(&mutex);

	k_thread_create(&thread, thread_stack, STACKSIZE,
		thread_handle, NULL, NULL, NULL,
		K_PRIO_PREEMPT(0), K_INHERIT_PERMS,
		K_NO_WAIT);

	k_sem_give(&sem);
	/* Waiting for enough data generated */
	k_thread_join(&thread, K_FOREVER);

	zassert_true(area_avil, "system operation does not work normally");

	/* revert the priority of main thread */
	k_thread_priority_set(k_current_get(), old_prio);
}

void test_main(void)
{
	ztest_test_suite(test_tracing,
			 ztest_unit_test(test_tracing_sys)
			 );
	ztest_run_test_suite(test_tracing);
}
