/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define N_THR 3
BUILD_ASSERT(N_THR <= CONFIG_DYNAMIC_THREAD_POOL_SIZE, "Insufficient number of dynamic threads");

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

ZTEST(posix_rw_locks, test_rw_lock)
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

	zassert_ok(clock_gettime(CLOCK_REALTIME, &time));
	time.tv_sec += 2;

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

static void test_pthread_rwlockattr_pshared_common(bool set, int pshared)
{
	int tmp_pshared = 4242;
	pthread_rwlockattr_t attr;

	zassert_ok(pthread_rwlockattr_init(&attr));
	zassert_ok(pthread_rwlockattr_getpshared(&attr, &tmp_pshared));
	zassert_equal(tmp_pshared, PTHREAD_PROCESS_PRIVATE);
	if (set) {
		zassert_ok(pthread_rwlockattr_setpshared(&attr, pshared));
		zassert_ok(pthread_rwlockattr_getpshared(&attr, &tmp_pshared));
		zassert_equal(tmp_pshared, pshared);
	}
	zassert_ok(pthread_rwlockattr_destroy(&attr));
}

ZTEST(posix_rw_locks, test_pthread_rwlockattr_getpshared)
{
	test_pthread_rwlockattr_pshared_common(false, 0);
}

ZTEST(posix_rw_locks, test_pthread_rwlockattr_setpshared)
{
	test_pthread_rwlockattr_pshared_common(true, PTHREAD_PROCESS_PRIVATE);
	test_pthread_rwlockattr_pshared_common(true, PTHREAD_PROCESS_SHARED);
}

ZTEST(posix_rw_locks, test_rwlock_static_initializer)
{
	static pthread_rwlock_t static_rw = PTHREAD_RWLOCK_INITIALIZER;
	struct timespec time;

	time.tv_sec = 1;
	time.tv_nsec = 0;

	zassert_ok(pthread_rwlock_rdlock(&static_rw));
	zassert_ok(pthread_rwlock_unlock(&static_rw));

	zassert_ok(pthread_rwlock_tryrdlock(&static_rw));
	zassert_ok(pthread_rwlock_unlock(&static_rw));

	zassert_ok(pthread_rwlock_timedrdlock(&static_rw, &time));
	zassert_ok(pthread_rwlock_unlock(&static_rw));

	zassert_ok(pthread_rwlock_wrlock(&static_rw));
	zassert_ok(pthread_rwlock_unlock(&static_rw));

	zassert_ok(pthread_rwlock_trywrlock(&static_rw));
	zassert_ok(pthread_rwlock_unlock(&static_rw));

	zassert_ok(pthread_rwlock_timedwrlock(&static_rw, &time));
	zassert_ok(pthread_rwlock_unlock(&static_rw));

	zassert_ok(pthread_rwlock_destroy(&static_rw));
}

struct static_rwlock_sync {
	pthread_rwlock_t *rw;
	atomic_t reader1_has_lock;
	atomic_t reader2_got_lock;
	atomic_t writer_verified_busy;
	atomic_t release_readers;
};

static void *static_rwlock_reader1(void *arg)
{
	struct static_rwlock_sync *sync = (struct static_rwlock_sync *)arg;

	zassert_ok(pthread_rwlock_rdlock(sync->rw));
	atomic_set(&sync->reader1_has_lock, 1);

	while (!atomic_get(&sync->release_readers)) {
		usleep(USEC_PER_MSEC);
	}

	zassert_ok(pthread_rwlock_unlock(sync->rw));

	return NULL;
}

static void *static_rwlock_reader2(void *arg)
{
	struct static_rwlock_sync *sync = (struct static_rwlock_sync *)arg;

	while (!atomic_get(&sync->reader1_has_lock)) {
		usleep(USEC_PER_MSEC);
	}

	/* Multiple readers should concurrently acquire shared read lock */
	zassert_ok(pthread_rwlock_tryrdlock(sync->rw));
	atomic_set(&sync->reader2_got_lock, 1);

	while (!atomic_get(&sync->release_readers)) {
		usleep(USEC_PER_MSEC);
	}

	zassert_ok(pthread_rwlock_unlock(sync->rw));

	return NULL;
}

static void *static_rwlock_writer(void *arg)
{
	struct static_rwlock_sync *sync = (struct static_rwlock_sync *)arg;

	while (!atomic_get(&sync->reader1_has_lock) ||
	       !atomic_get(&sync->reader2_got_lock)) {
		usleep(USEC_PER_MSEC);
	}

	/* Verify mutual exclusion: trywrlock must fail with EBUSY */
	zassert_equal(pthread_rwlock_trywrlock(sync->rw), EBUSY,
		      "trywrlock should return EBUSY while readers hold lock");
	atomic_set(&sync->writer_verified_busy, 1);

	/* Signal readers to release their locks */
	atomic_set(&sync->release_readers, 1);

	/* Writer blocks on wrlock until readers release, then acquires */
	zassert_ok(pthread_rwlock_wrlock(sync->rw));
	zassert_ok(pthread_rwlock_unlock(sync->rw));

	return NULL;
}

ZTEST(posix_rw_locks, test_rwlock_static_initializer_multithread)
{
	static pthread_rwlock_t static_rw_mt = PTHREAD_RWLOCK_INITIALIZER;
	pthread_t r1, r2, w;
	struct static_rwlock_sync sync = {
		.rw = &static_rw_mt,
	};

	zassert_ok(pthread_create(&r1, NULL, static_rwlock_reader1, &sync));
	zassert_ok(pthread_create(&r2, NULL, static_rwlock_reader2, &sync));
	zassert_ok(pthread_create(&w, NULL, static_rwlock_writer, &sync));

	zassert_ok(pthread_join(r1, NULL));
	zassert_ok(pthread_join(r2, NULL));
	zassert_ok(pthread_join(w, NULL));

	zassert_equal(atomic_get(&sync.reader2_got_lock), 1,
		      "reader 2 should have acquired shared read lock");
	zassert_equal(atomic_get(&sync.writer_verified_busy), 1,
		      "writer should have verified mutual exclusion with EBUSY");

	zassert_ok(pthread_rwlock_destroy(&static_rw_mt));
}

ZTEST_SUITE(posix_rw_locks, NULL, NULL, NULL, NULL, NULL);
