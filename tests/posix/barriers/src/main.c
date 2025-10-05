/*
 * Copyright (c) 2023, Harshil Bhatt
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <semaphore.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_SPARC)
/*
 * qemu_leon3 asserts when the attribute is not static. No idea why.
 * https://github.com/zephyrproject-rtos/zephyr-testing/actions/runs/18696980774/job/53316894563
 */
#define STATIC static
#else
#define STATIC
#endif

ZTEST(posix_barriers, test_barrier)
{
	int ret, pshared;
	STATIC pthread_barrierattr_t attr;

	ret = pthread_barrierattr_init(&attr);
	zassert_equal(ret, 0, "pthread_barrierattr_init failed");

	ret = pthread_barrierattr_getpshared(&attr, &pshared);
	zassert_equal(ret, 0, "pthread_barrierattr_getpshared failed");
	zassert_equal(pshared, PTHREAD_PROCESS_PRIVATE, "pshared attribute not set correctly");

	ret = pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
	zassert_equal(ret, 0, "pthread_barrierattr_setpshared failed");

	ret = pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	zassert_equal(ret, 0, "pthread_barrierattr_setpshared failed");

	ret = pthread_barrierattr_getpshared(&attr, &pshared);
	zassert_equal(ret, 0, "pthread_barrierattr_getpshared failed");
	zassert_equal(pshared, PTHREAD_PROCESS_SHARED, "pshared attribute not retrieved correctly");

	ret = pthread_barrierattr_setpshared(&attr, 42);
	zassert_equal(ret, -EINVAL, "pthread_barrierattr_setpshared did not return EINVAL");

	ret = pthread_barrierattr_destroy(&attr);
	zassert_equal(ret, 0, "pthread_barrierattr_destroy failed");
}

ZTEST_SUITE(posix_barriers, NULL, NULL, NULL, NULL, NULL);
