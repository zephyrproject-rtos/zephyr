/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test system mutex based on futex
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/mutex.h>

#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PRIORITY  7
#define OPTIONS   (K_USER | K_INHERIT_PERMS)

K_THREAD_STACK_DEFINE(thread_stack_a, STACKSIZE);
K_THREAD_STACK_DEFINE(thread_stack_b, STACKSIZE);
K_THREAD_STACK_DEFINE(thread_stack_c, STACKSIZE);
K_THREAD_STACK_DEFINE(thread_stack_d, STACKSIZE);
K_THREAD_STACK_DEFINE(thread_stack_waiting, STACKSIZE);

static struct k_thread thread_a;
static struct k_thread thread_b;
static struct k_thread thread_c;
static struct k_thread thread_d;
static struct k_thread thread_waiting;

static ZTEST_BMEM int thread_waiting_ret;
static ZTEST_BMEM int thread_contended_ret;

#define ACTION_LOCK_NO_WAIT 1
#define ACTION_LOCK_WAIT    2
#define ACTION_LOCK_FOREVER 3
#define ACTION_UNLOCK       4

static ZTEST_BMEM SYS_MUTEX_DEFINE(mutex_recursive);
static ZTEST_BMEM SYS_MUTEX_DEFINE(mutex_waiting);
static ZTEST_BMEM SYS_MUTEX_DEFINE(mutex_contended);

/*
 * Test recursive locking:
 * Locking an already locked mutex shall succeed, and unlocking should success
 * as many times as the mutex was locked earlier.
 * Unlocking after the last unlock should return -EINVAL.
 */
ZTEST_USER(mutex_futex, test_recursive)
{
	int ret;

	ret = sys_mutex_lock(&mutex_recursive, K_NO_WAIT);
	zassert_true(ret == 0, "mutex first lock failed");
	ret = sys_mutex_lock(&mutex_recursive, K_NO_WAIT);
	zassert_true(ret == 0, "mutex second lock failed");
	ret = sys_mutex_lock(&mutex_recursive, K_NO_WAIT);
	zassert_true(ret == 0, "mutex third lock failed");

	ret = sys_mutex_unlock(&mutex_recursive);
	zassert_true(ret == 0, "mutex first unlock failed");
	ret = sys_mutex_unlock(&mutex_recursive);
	zassert_true(ret == 0, "mutex second unlock failed");
	ret = sys_mutex_unlock(&mutex_recursive);
	zassert_true(ret == 0, "mutex third unlock failed");

	ret = sys_mutex_unlock(&mutex_recursive);
	zassert_true(ret == -EINVAL, "mutex wasn't locked");
}

void thread_entry_other(void *p1, void *p2, void *p3)
{
	int action = (int)(uintptr_t)p1;

	switch (action) {
	case ACTION_LOCK_NO_WAIT:
		thread_waiting_ret = sys_mutex_lock(&mutex_waiting, K_NO_WAIT);
		if (thread_waiting_ret == 0) {
			thread_waiting_ret = sys_mutex_unlock(&mutex_waiting);
		}
		break;
	case ACTION_LOCK_WAIT:
		thread_waiting_ret = sys_mutex_lock(&mutex_waiting, K_MSEC(10));
		if (thread_waiting_ret == 0) {
			thread_waiting_ret = sys_mutex_unlock(&mutex_waiting);
		}
		break;
	case ACTION_LOCK_FOREVER:
		thread_waiting_ret = sys_mutex_lock(&mutex_waiting, K_FOREVER);
		if (thread_waiting_ret == 0) {
			thread_waiting_ret = sys_mutex_unlock(&mutex_waiting);
		}
		break;
	case ACTION_UNLOCK:
		thread_waiting_ret = sys_mutex_unlock(&mutex_waiting);
		break;
	default:
		thread_waiting_ret = -EFAULT;
		break;
	}
}

/*
 * Test waiting on a locked mutex with another thread:
 * K_NO_WAIT shall return -EBUSY.
 * A timeout shall return -EAGAIN.
 * Unlocking a mutex locked by a different thread shall return -EPERM.
 * K_FOREVER shall wait until the mutex is unlocked and then succeed.
 */
ZTEST_USER(mutex_futex, test_waiting)
{
	int ret;

	ret = sys_mutex_lock(&mutex_waiting, K_NO_WAIT);
	zassert_true(ret == 0, "mutex lock failed");

	k_thread_create(&thread_waiting, thread_stack_waiting,
			K_THREAD_STACK_SIZEOF(thread_stack_waiting), thread_entry_other,
			(void *)(uintptr_t)ACTION_LOCK_NO_WAIT, NULL, NULL, PRIORITY, OPTIONS,
			K_NO_WAIT);
	ret = k_thread_join(&thread_waiting, K_FOREVER);
	zassert_true(ret == 0, "thread join failed");
	zassert_true(thread_waiting_ret == -EBUSY, "mutex was locked without wait");

	k_thread_create(&thread_waiting, thread_stack_waiting,
			K_THREAD_STACK_SIZEOF(thread_stack_waiting), thread_entry_other,
			(void *)(uintptr_t)ACTION_LOCK_WAIT, NULL, NULL, PRIORITY, OPTIONS,
			K_NO_WAIT);
	ret = k_thread_join(&thread_waiting, K_FOREVER);
	zassert_true(ret == 0, "thread join failed");
	zassert_true(thread_waiting_ret == -EAGAIN, "mutex was locked with wait");

	k_thread_create(&thread_waiting, thread_stack_waiting,
			K_THREAD_STACK_SIZEOF(thread_stack_waiting), thread_entry_other,
			(void *)(uintptr_t)ACTION_UNLOCK, NULL, NULL, PRIORITY, OPTIONS, K_NO_WAIT);
	ret = k_thread_join(&thread_waiting, K_FOREVER);
	zassert_true(ret == 0, "thread join failed");
	zassert_true(thread_waiting_ret == -EPERM, "mutex was locked by different thread");

	k_thread_create(&thread_waiting, thread_stack_waiting,
			K_THREAD_STACK_SIZEOF(thread_stack_waiting), thread_entry_other,
			(void *)(uintptr_t)ACTION_LOCK_FOREVER, NULL, NULL, PRIORITY, OPTIONS,
			K_NO_WAIT);
	ret = sys_mutex_unlock(&mutex_waiting);
	zassert_true(ret == 0, "mutex unlock failed");
	ret = k_thread_join(&thread_waiting, K_FOREVER);
	zassert_true(ret == 0, "thread join failed");
	zassert_true(thread_waiting_ret == 0, "other thread mutex lock failed");
}

void thread_entry_contended(void *p1, void *p2, void *p3)
{
	char *name = (char *)p1;
	int ret, iterations, sleep_iterations;

	iterations = 1000;
	sleep_iterations = 20;

	TC_PRINT("Thread %s contended mutex test entry\n", name);

	for (int i = 0; i < iterations; i++) {
		ret = sys_mutex_lock(&mutex_contended, K_FOREVER);
		if (ret != 0) {
			TC_ERROR("Thread %s failed locking mutex at %d\n", name, i);
			thread_contended_ret = 1;
			break;
		}
		if (i < sleep_iterations) {
			k_msleep(1);
		} else {
			k_yield();
		}
		ret = sys_mutex_unlock(&mutex_contended);
		if (ret != 0) {
			TC_ERROR("Thread %s failed unlocking mutex at %d\n", name, i);
			thread_contended_ret = 1;
			break;
		}
	}

	TC_PRINT("Thread %s contended mutex test exit\n", name);
}

/*
 * Test contested mutex path with multiple threads. Each tries locking and unlocking
 * in a loop. For the first few iterations threads wait before unlocking, so that the
 * others can queue up. For the remainder they just yield to try locking and unlocking
 * in quick succession.
 */
ZTEST_USER(mutex_futex, test_contended)
{
	int ret;

	k_thread_create(&thread_a, thread_stack_a, K_THREAD_STACK_SIZEOF(thread_stack_a),
			thread_entry_contended, "a", NULL, NULL, PRIORITY, OPTIONS, K_FOREVER);
	k_thread_create(&thread_b, thread_stack_b, K_THREAD_STACK_SIZEOF(thread_stack_b),
			thread_entry_contended, "b", NULL, NULL, PRIORITY, OPTIONS, K_FOREVER);
	k_thread_create(&thread_c, thread_stack_c, K_THREAD_STACK_SIZEOF(thread_stack_c),
			thread_entry_contended, "c", NULL, NULL, PRIORITY, OPTIONS, K_FOREVER);
	k_thread_create(&thread_d, thread_stack_d, K_THREAD_STACK_SIZEOF(thread_stack_d),
			thread_entry_contended, "d", NULL, NULL, PRIORITY, OPTIONS, K_FOREVER);

	k_thread_start(&thread_a);
	k_thread_start(&thread_b);
	k_thread_start(&thread_c);
	k_thread_start(&thread_d);

	ret = k_thread_join(&thread_a, K_MSEC(5000));
	zassert_true(ret == 0, "thread join failed");
	ret = k_thread_join(&thread_b, K_MSEC(5000));
	zassert_true(ret == 0, "thread join failed");
	ret = k_thread_join(&thread_c, K_MSEC(5000));
	zassert_true(ret == 0, "thread join failed");
	ret = k_thread_join(&thread_d, K_MSEC(5000));
	zassert_true(ret == 0, "thread join failed");
	zassert_true(thread_contended_ret == 0, "contended mutex failed");
}

static void *setup(void)
{
	k_thread_access_grant(k_current_get(), &thread_stack_a, &thread_stack_b, &thread_stack_c,
			      &thread_stack_d, &thread_stack_waiting, &thread_a, &thread_b,
			      &thread_c, &thread_d, &thread_waiting);
	return NULL;
}

ZTEST_SUITE(mutex_futex, NULL, setup, NULL, NULL, NULL);
