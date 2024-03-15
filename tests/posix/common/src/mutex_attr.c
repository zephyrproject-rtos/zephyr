/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pthread.h>

#include <zephyr/ztest.h>

ZTEST(mutex_attr, test_pthread_mutexattr_init)
{
	pthread_mutexattr_t attr;

	/* degenerate cases */
	{
		zassert_equal(EINVAL, pthread_mutexattr_init(NULL));
	}

	zassert_ok(pthread_mutexattr_init(&attr));
	zassert_ok(pthread_mutexattr_destroy(&attr));
}

ZTEST(mutex_attr, test_pthread_mutexattr_destroy)
{
	pthread_mutexattr_t attr;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(EINVAL, pthread_mutexattr_destroy(&attr));
		}
		zassert_equal(EINVAL, pthread_mutexattr_destroy(NULL));
	}

	zassert_ok(pthread_mutexattr_init(&attr));
	zassert_ok(pthread_mutexattr_destroy(&attr));
	if (false) {
		/* undefined behaviour */
		zassert_equal(EINVAL, pthread_mutexattr_destroy(&attr));
	}
}

ZTEST(mutex_attr, test_pthread_mutexattr_gettype)
{
	int type;
	pthread_mutexattr_t attr;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(EINVAL, pthread_mutexattr_gettype(&attr, &type));
		}
		zassert_equal(EINVAL, pthread_mutexattr_gettype(NULL, NULL));
		zassert_equal(EINVAL, pthread_mutexattr_gettype(NULL, &type));
		zassert_equal(EINVAL, pthread_mutexattr_gettype(&attr, NULL));
	}

	zassert_ok(pthread_mutexattr_init(&attr));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_DEFAULT);
	zassert_ok(pthread_mutexattr_destroy(&attr));
}

ZTEST(mutex_attr, test_pthread_mutexattr_settype)
{
	int type;
	pthread_mutexattr_t attr;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(EINVAL,
				      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT));
		}
		zassert_equal(EINVAL, pthread_mutexattr_settype(NULL, 42));
		zassert_equal(EINVAL, pthread_mutexattr_settype(NULL, PTHREAD_MUTEX_NORMAL));
		zassert_equal(EINVAL, pthread_mutexattr_settype(&attr, 42));
	}

	zassert_ok(pthread_mutexattr_init(&attr));

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_DEFAULT);

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_NORMAL);

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_RECURSIVE);

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_ERRORCHECK);

	zassert_ok(pthread_mutexattr_destroy(&attr));
}

ZTEST_SUITE(mutex_attr, NULL, NULL, NULL, NULL, NULL);
