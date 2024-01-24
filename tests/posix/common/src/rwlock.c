/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define N_THR 3

LOG_MODULE_REGISTER(posix_rwlock_test);

static pthread_rwlock_t rwlock;

static void *thread_top(void *p1)
{
	int ret;
	pthread_t id;

	id = (pthread_t)pthread_self();
	ret = pthread_rwlock_tryrdlock(&rwlock);
	if (ret != 0) {
		LOG_DBG("Not able to get RD lock on trying, try again");
		zassert_ok(pthread_rwlock_rdlock(&rwlock), "Failed to acquire write lock");
	}

	LOG_DBG("Thread %d got RD lock", id);
	usleep(USEC_PER_MSEC);
	LOG_DBG("Thread %d releasing RD lock", id);
	zassert_ok(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	LOG_DBG("Thread %d acquiring WR lock", id);
	ret = pthread_rwlock_trywrlock(&rwlock);
	if (ret != 0) {
		zassert_ok(pthread_rwlock_wrlock(&rwlock), "Failed to acquire WR lock");
	}

	LOG_DBG("Thread %d acquired WR lock", id);
	usleep(USEC_PER_MSEC);
	LOG_DBG("Thread %d releasing WR lock", id);
	zassert_ok(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	return NULL;
}

ZTEST(rwlock, test_rw_lock)
{
	int ret;
	pthread_t newthread[N_THR];
	struct timespec time;
	void *status;

	time.tv_sec = 1;
	time.tv_nsec = 0;

	zassert_equal(pthread_rwlock_destroy(&rwlock), EINVAL);
	zassert_equal(pthread_rwlock_rdlock(&rwlock), EINVAL);
	zassert_equal(pthread_rwlock_wrlock(&rwlock), EINVAL);
	zassert_equal(pthread_rwlock_trywrlock(&rwlock), EINVAL);
	zassert_equal(pthread_rwlock_tryrdlock(&rwlock), EINVAL);
	zassert_equal(pthread_rwlock_timedwrlock(&rwlock, &time), EINVAL);
	zassert_equal(pthread_rwlock_timedrdlock(&rwlock, &time), EINVAL);
	zassert_equal(pthread_rwlock_unlock(&rwlock), EINVAL);

	zassert_ok(pthread_rwlock_init(&rwlock, NULL), "Failed to create rwlock");
	LOG_DBG("main acquire WR lock and 3 threads acquire RD lock");
	zassert_ok(pthread_rwlock_timedwrlock(&rwlock, &time), "Failed to acquire write lock");

	/* Creating N preemptive threads in increasing order of priority */
	for (int i = 0; i < N_THR; i++) {
		zassert_ok(pthread_create(&newthread[i], NULL, thread_top, NULL),
			   "Low memory to thread new thread");
	}

	/* Delay to give change to child threads to run */
	usleep(USEC_PER_MSEC);
	LOG_DBG("Parent thread releasing WR lock");
	zassert_ok(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	/* Let child threads acquire RD Lock */
	usleep(USEC_PER_MSEC);
	LOG_DBG("Parent thread acquiring WR lock again");

	time.tv_sec = 2;
	time.tv_nsec = 0;
	ret = pthread_rwlock_timedwrlock(&rwlock, &time);
	if (ret) {
		zassert_ok(pthread_rwlock_wrlock(&rwlock), "Failed to acquire write lock");
	}

	LOG_DBG("Parent thread acquired WR lock again");
	usleep(USEC_PER_MSEC);
	LOG_DBG("Parent thread releasing WR lock again");
	zassert_ok(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	LOG_DBG("3 threads acquire WR lock");
	LOG_DBG("Main thread acquiring RD lock");

	ret = pthread_rwlock_timedrdlock(&rwlock, &time);
	if (ret != 0) {
		zassert_ok(pthread_rwlock_rdlock(&rwlock), "Failed to lock");
	}

	LOG_DBG("Main thread acquired RD lock");
	usleep(USEC_PER_MSEC);
	LOG_DBG("Main thread releasing RD lock");
	zassert_ok(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	for (int i = 0; i < N_THR; i++) {
		zassert_ok(pthread_join(newthread[i], &status), "Failed to join");
	}

	zassert_ok(pthread_rwlock_destroy(&rwlock), "Failed to destroy rwlock");
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(rwlock, NULL, NULL, before, NULL, NULL);
