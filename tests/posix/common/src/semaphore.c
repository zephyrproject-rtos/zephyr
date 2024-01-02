/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static sem_t sema;
static void *dummy_sem;

static void *child_func(void *p1)
{
	zassert_equal(sem_post(&sema), 0, "sem_post failed");
	return NULL;
}

ZTEST(posix_apis, test_semaphore)
{
	pthread_t thread1, thread2;
	int val, ret;
	struct timespec abstime;

	/* TESTPOINT: Check if sema value is less than
	 * CONFIG_SEM_VALUE_MAX
	 */
	zassert_equal(sem_init(&sema, 0, (CONFIG_SEM_VALUE_MAX + 1)), -1,
		      "value larger than %d\n", CONFIG_SEM_VALUE_MAX);
	zassert_equal(errno, EINVAL);

	zassert_equal(sem_init(&sema, 0, 0), 0, "sem_init failed");

	/* TESTPOINT: Call sem_post with invalid kobject */
	zassert_equal(sem_post(dummy_sem), -1, "sem_post of"
		      " invalid semaphore object didn't fail");
	zassert_equal(errno, EINVAL);

	/* TESTPOINT: Check if semaphore value is as set */
	zassert_equal(sem_getvalue(&sema, &val), 0);
	zassert_equal(val, 0);

	/* TESTPOINT: Check if sema is acquired when it
	 * is not available
	 */
	zassert_equal(sem_trywait(&sema), -1);
	zassert_equal(errno, EAGAIN);

	ret = pthread_create(&thread1, NULL, child_func, NULL);
	zassert_equal(ret, 0, "Thread creation failed");

	zassert_equal(clock_gettime(CLOCK_REALTIME, &abstime), 0,
		      "clock_gettime failed");

	abstime.tv_sec += 5;

	/* TESPOINT: Wait for 5 seconds and acquire sema given
	 * by thread1
	 */
	zassert_equal(sem_timedwait(&sema, &abstime), 0);

	/* TESTPOINT: Semaphore is already acquired, check if
	 * no semaphore is available
	 */
	zassert_equal(sem_timedwait(&sema, &abstime), -1);
	zassert_equal(errno, ETIMEDOUT);

	/* TESTPOINT: sem_destroy with invalid kobject */
	zassert_equal(sem_destroy(dummy_sem), -1, "invalid"
		      " semaphore is destroyed");
	zassert_equal(errno, EINVAL);

	zassert_equal(sem_destroy(&sema), 0, "semaphore is not destroyed");

	/* TESTPOINT: Initialize sema with 1 */
	zassert_equal(sem_init(&sema, 0, 1), 0, "sem_init failed");
	zassert_equal(sem_getvalue(&sema, &val), 0);
	zassert_equal(val, 1);

	zassert_equal(sem_destroy(&sema), -1, "acquired semaphore"
		      " is destroyed");
	zassert_equal(errno, EBUSY);

	/* TESTPOINT: take semaphore which is initialized with 1 */
	zassert_equal(sem_trywait(&sema), 0);

	zassert_equal(pthread_create(&thread2, NULL, child_func, NULL), 0,
		      "Thread creation failed");

	/* TESTPOINT: Wait and acquire semaphore till thread2 gives */
	zassert_equal(sem_wait(&sema), 0, "sem_wait failed");
}
