/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pthread.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define SLEEP_MS 100

static pthread_mutex_t mutex;

static void *normal_mutex_entry(void *p1)
{
	int i, rc;

	/* Sleep for maximum 300 ms as main thread is sleeping for 100 ms */

	for (i = 0; i < 3; i++) {
		rc = pthread_mutex_trylock(&mutex);
		if (rc == 0) {
			break;
		}
		k_msleep(SLEEP_MS);
	}

	zassert_false(rc, "try lock failed");
	TC_PRINT("mutex lock is taken\n");
	zassert_false(pthread_mutex_unlock(&mutex), "mutex unlock is failed");
	return NULL;
}

static void *recursive_mutex_entry(void *p1)
{
	zassert_false(pthread_mutex_lock(&mutex), "mutex is not taken");
	zassert_false(pthread_mutex_lock(&mutex), "mutex is not taken 2nd time");
	TC_PRINT("recursive mutex lock is taken\n");
	zassert_false(pthread_mutex_unlock(&mutex), "mutex is not unlocked");
	zassert_false(pthread_mutex_unlock(&mutex), "mutex is not unlocked");
	return NULL;
}

static void test_mutex_common(int type, void *(*entry)(void *arg))
{
	pthread_t th;
	int protocol;
	int actual_type;
	pthread_mutexattr_t mut_attr;
	struct sched_param schedparam;

	schedparam.sched_priority = 2;

	zassert_ok(pthread_mutexattr_init(&mut_attr));
	zassert_ok(pthread_mutexattr_settype(&mut_attr, type), "setting mutex type is failed");
	zassert_ok(pthread_mutex_init(&mutex, &mut_attr), "mutex initialization is failed");

	zassert_ok(pthread_mutexattr_gettype(&mut_attr, &actual_type),
		   "reading mutex type is failed");
	zassert_not_ok(pthread_mutexattr_getprotocol(NULL, &protocol));
	zassert_not_ok(pthread_mutexattr_getprotocol(&mut_attr, NULL));
	zassert_not_ok(pthread_mutexattr_getprotocol(NULL, NULL));
	zassert_ok(pthread_mutexattr_getprotocol(&mut_attr, &protocol),
		   "reading mutex protocol is failed");
	zassert_ok(pthread_mutexattr_destroy(&mut_attr));

	zassert_ok(pthread_mutex_lock(&mutex));

	zassert_equal(actual_type, type, "mutex type is not normal");
	zassert_equal(protocol, PTHREAD_PRIO_NONE, "mutex protocol is not prio_none");

	zassert_ok(pthread_create(&th, NULL, entry, NULL));

	k_msleep(SLEEP_MS);
	zassert_ok(pthread_mutex_unlock(&mutex));

	zassert_ok(pthread_join(th, NULL));
	zassert_ok(pthread_mutex_destroy(&mutex), "Destroying mutex is failed");
}

/**
 * @brief Test to demonstrate PTHREAD_MUTEX_NORMAL
 *
 * @details Mutex type is setup as normal. pthread_mutex_trylock
 *	    and pthread_mutex_lock are tested with mutex type being
 *	    normal.
 */
ZTEST(mutex, test_mutex_normal)
{
	test_mutex_common(PTHREAD_MUTEX_NORMAL, normal_mutex_entry);
}

/**
 * @brief Test to demonstrate PTHREAD_MUTEX_RECURSIVE
 *
 * @details Mutex type is setup as recursive. mutex will be locked
 *	    twice and unlocked for the same number of time.
 *
 */
ZTEST(mutex, test_mutex_recursive)
{
	test_mutex_common(PTHREAD_MUTEX_RECURSIVE, recursive_mutex_entry);
}

/**
 * @brief Test to demonstrate limited mutex resources
 *
 * @details Exactly CONFIG_MAX_PTHREAD_MUTEX_COUNT can be in use at once.
 */
ZTEST(mutex, test_mutex_resource_exhausted)
{
	size_t i;
	pthread_mutex_t m[CONFIG_MAX_PTHREAD_MUTEX_COUNT + 1];

	for (i = 0; i < CONFIG_MAX_PTHREAD_MUTEX_COUNT; ++i) {
		zassert_ok(pthread_mutex_init(&m[i], NULL), "failed to init mutex %zu", i);
	}

	/* try to initialize one more than CONFIG_MAX_PTHREAD_MUTEX_COUNT */
	zassert_equal(i, CONFIG_MAX_PTHREAD_MUTEX_COUNT);
	zassert_not_equal(0, pthread_mutex_init(&m[i], NULL),
			  "should not have initialized mutex %zu", i);

	for (; i > 0; --i) {
		zassert_ok(pthread_mutex_destroy(&m[i - 1]), "failed to destroy mutex %zu", i - 1);
	}
}

/**
 * @brief Test to that there are no mutex resource leaks
 *
 * @details Demonstrate that mutexes may be used over and over again.
 */
ZTEST(mutex, test_mutex_resource_leak)
{
	pthread_mutex_t m;

	for (size_t i = 0; i < 2 * CONFIG_MAX_PTHREAD_MUTEX_COUNT; ++i) {
		zassert_ok(pthread_mutex_init(&m, NULL), "failed to init mutex %zu", i);
		zassert_ok(pthread_mutex_destroy(&m), "failed to destroy mutex %zu", i);
	}
}

#define TIMEDLOCK_TIMEOUT_MS       200
#define TIMEDLOCK_TIMEOUT_DELAY_MS 100

BUILD_ASSERT(TIMEDLOCK_TIMEOUT_DELAY_MS >= 100, "TIMEDLOCK_TIMEOUT_DELAY_MS too small");
BUILD_ASSERT(TIMEDLOCK_TIMEOUT_MS >= 2 * TIMEDLOCK_TIMEOUT_DELAY_MS,
	     "TIMEDLOCK_TIMEOUT_MS too small");

static void timespec_add_ms(struct timespec *ts, uint32_t ms)
{
	bool oflow;

	ts->tv_nsec += ms * NSEC_PER_MSEC;
	oflow = ts->tv_nsec >= NSEC_PER_SEC;
	ts->tv_sec += oflow;
	ts->tv_nsec -= oflow * NSEC_PER_SEC;
}

static void *test_mutex_timedlock_fn(void *arg)
{
	struct timespec time_point;
	pthread_mutex_t *mtx = (pthread_mutex_t *)arg;

	zassume_ok(clock_gettime(CLOCK_MONOTONIC, &time_point));
	timespec_add_ms(&time_point, TIMEDLOCK_TIMEOUT_MS);

	return INT_TO_POINTER(pthread_mutex_timedlock(mtx, &time_point));
}

/** @brief Test to verify @ref pthread_mutex_timedlock returns ETIMEDOUT */
ZTEST(mutex, test_mutex_timedlock)
{
	void *ret;
	pthread_t th;

	zassert_ok(pthread_mutex_init(&mutex, NULL));

	printk("Expecting timedlock with timeout of %d ms to fail\n", TIMEDLOCK_TIMEOUT_MS);
	zassert_ok(pthread_mutex_lock(&mutex));
	zassert_ok(pthread_create(&th, NULL, test_mutex_timedlock_fn, &mutex));
	zassert_ok(pthread_join(th, &ret));
	/* ensure timeout occurs */
	zassert_equal(ETIMEDOUT, POINTER_TO_INT(ret));

	printk("Expecting timedlock with timeout of %d ms to succeed after 100ms\n",
	       TIMEDLOCK_TIMEOUT_MS);
	zassert_ok(pthread_create(&th, NULL, test_mutex_timedlock_fn, &mutex));
	/* unlock before timeout expires */
	k_msleep(TIMEDLOCK_TIMEOUT_DELAY_MS);
	zassert_ok(pthread_mutex_unlock(&mutex));
	zassert_ok(pthread_join(th, &ret));
	/* ensure lock is successful, in spite of delay  */
	zassert_ok(POINTER_TO_INT(ret));

	zassert_ok(pthread_mutex_destroy(&mutex));
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(mutex, NULL, NULL, before, NULL, NULL);
