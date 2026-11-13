/*
 * Copyright (c) 2023, Harshil Bhatt
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(posix_barriers, test_barrier)
{
	int ret;
	int pshared;
	pthread_barrierattr_t attr;

	ret = pthread_barrierattr_init(&attr);
	zassert_equal(ret, 0, "pthread_barrierattr_init failed");

	ret = pthread_barrierattr_getpshared(&attr, &pshared);
	zassert_equal(ret, 0, "pthread_barrierattr_getpshared failed");
	zassert_equal(pshared, PTHREAD_PROCESS_PRIVATE, "pshared attribute not set correctly");

	ret = pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
	zassert_equal(ret, 0, "pthread_barrierattr_setpshared failed");

	ret = pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_PUBLIC);
	zassert_equal(ret, 0, "pthread_barrierattr_setpshared failed");

	ret = pthread_barrierattr_getpshared(&attr, &pshared);
	zassert_equal(ret, 0, "pthread_barrierattr_getpshared failed");
	zassert_equal(pshared, PTHREAD_PROCESS_PUBLIC, "pshared attribute not retrieved correctly");

	ret = pthread_barrierattr_setpshared(&attr, 42);
	zassert_equal(ret, EINVAL, "pthread_barrierattr_setpshared did not return EINVAL");

	ret = pthread_barrierattr_destroy(&attr);
	zassert_equal(ret, 0, "pthread_barrierattr_destroy failed");
}

ZTEST(posix_barriers, test_barrier_attr_invalid_params)
{
	int pshared;
	pthread_barrierattr_t attr;

	zassert_equal(pthread_barrierattr_init(NULL), EINVAL);
	zassert_equal(pthread_barrierattr_destroy(NULL), EINVAL);
	zassert_equal(pthread_barrierattr_setpshared(NULL, PTHREAD_PROCESS_PRIVATE), EINVAL);
	zassert_equal(pthread_barrierattr_getpshared(NULL, &pshared), EINVAL);

	zassert_ok(pthread_barrierattr_init(&attr));
	zassert_equal(pthread_barrierattr_getpshared(&attr, NULL), EINVAL);
	zassert_ok(pthread_barrierattr_destroy(&attr));
}

ZTEST(posix_barriers, test_barrier_invalid_params)
{
	pthread_barrier_t barrier;
	pthread_barrier_t uninit_barrier = 0;
	pthread_barrierattr_t attr;

	zassert_equal(pthread_barrier_init(NULL, NULL, 1), EINVAL);
	zassert_equal(pthread_barrier_init(&barrier, NULL, 0), EINVAL);

	zassert_ok(pthread_barrierattr_init(&attr));
	attr.pshared = 42;
	zassert_equal(pthread_barrier_init(&barrier, &attr, 1), EINVAL);
	zassert_ok(pthread_barrierattr_destroy(&attr));

	zassert_equal(pthread_barrier_destroy(NULL), EINVAL);
	zassert_equal(pthread_barrier_wait(NULL), EINVAL);
	zassert_equal(pthread_barrier_destroy(&uninit_barrier), EINVAL);
	zassert_equal(pthread_barrier_wait(&uninit_barrier), EINVAL);
}

ZTEST(posix_barriers, test_barrier_lifecycle)
{
	pthread_barrier_t barrier;

	zassert_ok(pthread_barrier_init(&barrier, NULL, 1));
	zassert_equal(pthread_barrier_wait(&barrier), PTHREAD_BARRIER_SERIAL_THREAD);
	zassert_equal(pthread_barrier_wait(&barrier), PTHREAD_BARRIER_SERIAL_THREAD);
	zassert_ok(pthread_barrier_destroy(&barrier));
	zassert_equal(pthread_barrier_destroy(&barrier), EINVAL);
}

ZTEST_SUITE(posix_barriers, NULL, NULL, NULL, NULL, NULL);
