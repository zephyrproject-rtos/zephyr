/*
 * Copyright (c) 2023, Harshil Bhatt
 * Copyright (c) 2026, Harshit Kudhial
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define BARRIER_THREAD_COUNT 3
BUILD_ASSERT(BARRIER_THREAD_COUNT <= CONFIG_DYNAMIC_THREAD_POOL_SIZE,
	     "CONFIG_DYNAMIC_THREAD_POOL_SIZE must be >= BARRIER_THREAD_COUNT");

#define BARRIER_CYCLE_COUNT 5

static pthread_barrier_t test_barrier;
static pthread_mutex_t serial_mutex = PTHREAD_MUTEX_INITIALIZER;
static int serial_count;

static void *barrier_cycle_thread(void *arg)
{
	ARG_UNUSED(arg);

	for (int i = 0; i < BARRIER_CYCLE_COUNT; i++) {
		int ret = pthread_barrier_wait(&test_barrier);

		zassert_true(ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD,
			     "pthread_barrier_wait returned unexpected value %d", ret);

		if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
			zassert_ok(pthread_mutex_lock(&serial_mutex));
			serial_count++;
			zassert_ok(pthread_mutex_unlock(&serial_mutex));
		}
	}

	return NULL;
}

ZTEST(posix_barriers, test_barrier)
{
	int ret, pshared;
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
	zassert_equal(ret, EINVAL, "pthread_barrierattr_setpshared should return EINVAL");

	ret = pthread_barrierattr_destroy(&attr);
	zassert_equal(ret, 0, "pthread_barrierattr_destroy failed");
}

ZTEST(posix_barriers, test_pthread_barrier_wait)
{
	pthread_t threads[BARRIER_THREAD_COUNT];
	int ret;

	serial_count = 0;

	ret = pthread_barrier_init(&test_barrier, NULL, BARRIER_THREAD_COUNT + 1);
	zassert_equal(ret, 0, "pthread_barrier_init failed");

	for (int i = 0; i < BARRIER_THREAD_COUNT; i++) {
		ret = pthread_create(&threads[i], NULL, barrier_cycle_thread, NULL);
		zassert_equal(ret, 0, "pthread_create failed for thread %d", i);
	}

	/* Main thread participates as the (N+1)th member of the barrier */
	for (int i = 0; i < BARRIER_CYCLE_COUNT; i++) {
		ret = pthread_barrier_wait(&test_barrier);
		zassert_true(ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD,
			     "main pthread_barrier_wait returned unexpected value %d", ret);

		if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
			zassert_ok(pthread_mutex_lock(&serial_mutex));
			serial_count++;
			zassert_ok(pthread_mutex_unlock(&serial_mutex));
		}
	}

	for (int i = 0; i < BARRIER_THREAD_COUNT; i++) {
		ret = pthread_join(threads[i], NULL);
		zassert_equal(ret, 0, "pthread_join failed for thread %d", i);
	}

	/*
	 * Exactly one thread must receive PTHREAD_BARRIER_SERIAL_THREAD per
	 * cycle, across all BARRIER_CYCLE_COUNT cycles.
	 */
	zassert_equal(serial_count, BARRIER_CYCLE_COUNT,
		      "Expected %d serial thread callbacks, got %d",
		      BARRIER_CYCLE_COUNT, serial_count);

	ret = pthread_barrier_destroy(&test_barrier);
	zassert_equal(ret, 0, "pthread_barrier_destroy failed");
}

ZTEST_SUITE(posix_barriers, NULL, NULL, NULL, NULL, NULL);
