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

ZTEST_SUITE(mutex_attr, NULL, NULL, NULL, NULL, NULL);
