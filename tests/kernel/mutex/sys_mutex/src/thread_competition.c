/*
 * Copyright (c) 2021 intel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>
#include <sys/mutex.h>

#define HIGH_T1	 0xaaa
#define HIGH_T2	 0xbbb
#define LOW_PRO	 0xccc

#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

static K_THREAD_STACK_DEFINE(thread_low_stack, STACKSIZE);
static struct k_thread thread_low_data;
static K_THREAD_STACK_DEFINE(thread_high_stack1, STACKSIZE);
static struct k_thread thread_high_data1;
static K_THREAD_STACK_DEFINE(thread_high_stack2, STACKSIZE);
static struct k_thread thread_high_data2;
SYS_MUTEX_DEFINE(mutex);
static uint32_t flag;

static void low_prio_wait_for_mutex(void *p1, void *p2, void *p3)
{
	struct sys_mutex *pmutex = p1;

	sys_mutex_lock(pmutex, K_FOREVER);

	flag = LOW_PRO;
	/* Keep mutex for a while */
	k_sleep(K_MSEC(20));

	sys_mutex_unlock(pmutex);
}

static void high_prio_t1_wait_for_mutex(void *p1, void *p2, void *p3)
{
	struct sys_mutex *pmutex = p1;

	sys_mutex_lock(pmutex, K_FOREVER);

	flag = HIGH_T1;
	/* Keep mutex for a while */
	k_sleep(K_MSEC(30));

	sys_mutex_unlock(pmutex);
}

static void high_prio_t2_wait_for_mutex(void *p1, void *p2, void *p3)
{
	struct sys_mutex *pmutex = p1;

	sys_mutex_lock(pmutex, K_FOREVER);

	flag = HIGH_T2;
	/* Keep mutex for a while */
	k_sleep(K_MSEC(20));

	sys_mutex_unlock(pmutex);
}

/**
 * @brief Test multi-threads to take mutex.
 *
 * @details Define three threads, and set a higher priority for two of them,
 * and set a lower priority for the last one. Then Add a delay between
 * creating the two high priority threads.
 * Test point:
 * 1. Any number of threads may wait on a mutex locked by others
 * simultaneously.
 * 2. When the mutex is released, it is took by the highest priority
 * thread that has waited longest.
 *
 * @ingroup kernel_mutex_tests
 */
void test_mutex_multithread_competition(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	int prio = 10;

	sys_mutex_lock(&mutex, K_NO_WAIT);

	k_thread_priority_set(k_current_get(), prio);

	k_thread_create(&thread_high_data1, thread_high_stack1, STACKSIZE,
			high_prio_t1_wait_for_mutex,
			&mutex, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Thread thread_high_data1 wait more time than thread_high_data2 */
	k_sleep(K_MSEC(10));

	k_thread_create(&thread_low_data, thread_low_stack, STACKSIZE,
			low_prio_wait_for_mutex,
			&mutex, NULL, NULL,
			prio + 4, 0, K_NO_WAIT);

	k_thread_create(&thread_high_data2, thread_high_stack2, STACKSIZE,
			high_prio_t2_wait_for_mutex,
			&mutex, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Release mutex by current thread */
	sys_mutex_unlock(&mutex);
	/* 0 ~ 30ms, the mutex should be keep by high prio t1 thread */
	k_sleep(K_MSEC(15));
	zassert_true(flag == HIGH_T1,
	"The highest priority thread failed to take the mutex.");

	/* 30ms ~ 50ms, the mutex should be keep by high prio t2 thread */
	k_sleep(K_MSEC(15));
	zassert_true(flag == HIGH_T2,
	"The highest priority thread failed to take the mutex.");

	/* 50ms ~ 70ms, the mutex should be keep by low prio thread */
	k_sleep(K_MSEC(15));
	zassert_true(flag == LOW_PRO,
	"The low priority thread failed to take the mutex.");

	/* Wait for thread exiting */
	k_thread_join(&thread_low_data, K_FOREVER);
	k_thread_join(&thread_high_data1, K_FOREVER);
	k_thread_join(&thread_high_data2, K_FOREVER);

	/* Revert priority of the main thread */
	k_thread_priority_set(k_current_get(), old_prio);
}
