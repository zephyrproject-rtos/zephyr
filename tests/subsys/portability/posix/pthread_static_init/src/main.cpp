/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/pthread.h>
#include <zephyr/ztest.h>

/* List initialization also occurs in C++ library synchronization objects. */
ZTEST(pthread_static_init, test_mutex)
{
	pthread_mutex_t mutex{PTHREAD_MUTEX_INITIALIZER};

	zassert_ok(pthread_mutex_lock(&mutex));
	zassert_ok(pthread_mutex_unlock(&mutex));
	zassert_ok(pthread_mutex_destroy(&mutex));
}

ZTEST(pthread_static_init, test_condvar)
{
	pthread_cond_t condvar{PTHREAD_COND_INITIALIZER};

	zassert_ok(pthread_cond_signal(&condvar));
	zassert_ok(pthread_cond_broadcast(&condvar));
	zassert_ok(pthread_cond_destroy(&condvar));
}

ZTEST(pthread_static_init, test_rwlock)
{
	pthread_rwlock_t rwlock{PTHREAD_RWLOCK_INITIALIZER};

	/* The macro must compile in C++; this implementation still needs explicit init. */
	zassert_ok(pthread_rwlock_init(&rwlock, NULL));

	zassert_ok(pthread_rwlock_rdlock(&rwlock));
	zassert_ok(pthread_rwlock_unlock(&rwlock));
	zassert_ok(pthread_rwlock_wrlock(&rwlock));
	zassert_ok(pthread_rwlock_unlock(&rwlock));
	zassert_ok(pthread_rwlock_destroy(&rwlock));
}

ZTEST_SUITE(pthread_static_init, NULL, NULL, NULL, NULL, NULL);
