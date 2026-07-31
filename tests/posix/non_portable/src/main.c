/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <pthread.h>
#include <unistd.h>

#define SLEEP_DURATION_MS 200

static void *timedjoin_thread(void *p1)
{
	int sleep_duration_ms = POINTER_TO_INT(p1);

	usleep(USEC_PER_MSEC * sleep_duration_ms);
	return NULL;
}

ZTEST(posix_non_portable, test_pthread_tryjoin_np)
{
	pthread_t th = {0};
	void *retval;

	/* Creating a thread that exits after 200ms*/
	zassert_ok(pthread_create(&th, NULL, timedjoin_thread, INT_TO_POINTER(SLEEP_DURATION_MS)));

	/* Attempting to join, when thread is still running, should fail */
	usleep(USEC_PER_MSEC * SLEEP_DURATION_MS / 2);
	zassert_equal(pthread_tryjoin_np(th, &retval), EBUSY);

	/* Sleep so thread will exit */
	usleep(USEC_PER_MSEC * SLEEP_DURATION_MS);

	/* Attempting to join without blocking should succeed now */
	zassert_ok(pthread_tryjoin_np(th, &retval));
}

ZTEST(posix_non_portable, test_pthread_timedjoin_np)
{
	pthread_t th = {0};
	void *ret;
	struct timespec not_done;
	struct timespec done;
	struct timespec invalid[] = {
		{.tv_nsec = -1},
		{.tv_nsec = NSEC_PER_SEC},
	};

	/* setup timespecs when the thread is still running and when it is done */
	clock_gettime(CLOCK_REALTIME, &not_done);
	clock_gettime(CLOCK_REALTIME, &done);
	not_done.tv_nsec += SLEEP_DURATION_MS / 2 * NSEC_PER_MSEC;
	done.tv_nsec += SLEEP_DURATION_MS * 1.5 * NSEC_PER_MSEC;
	while (not_done.tv_nsec >= NSEC_PER_SEC) {
		not_done.tv_sec++;
		not_done.tv_nsec -= NSEC_PER_SEC;
	}
	while (done.tv_nsec >= NSEC_PER_SEC) {
		done.tv_sec++;
		done.tv_nsec -= NSEC_PER_SEC;
	}

	/* Creating a thread that exits after 200ms*/
	zassert_ok(pthread_create(&th, NULL, timedjoin_thread, INT_TO_POINTER(SLEEP_DURATION_MS)));

	/* pthread_timedjoin-np must return EINVAL for invalid struct timespecs */
	zassert_equal(pthread_timedjoin_np(th, &ret, NULL), EINVAL);
	for (size_t i = 0; i < ARRAY_SIZE(invalid); ++i) {
		zassert_equal(pthread_timedjoin_np(th, &ret, &invalid[i]), EINVAL);
	}

	/* Attempting to join with a timeout, when the thread is still running should fail */
	zassert_equal(pthread_timedjoin_np(th, &ret, &not_done), ETIMEDOUT);

	/* Attempting to join with a timeout, when the thread is done, should succeed */
	zassert_ok(pthread_timedjoin_np(th, &ret, &done));
}

ZTEST(posix_non_portable, test_pthread_get_set_name_np)
{
	pthread_t th = {0};
	char thr_name_buf[CONFIG_THREAD_MAX_NAME_LEN];
	const char *thr_name = "test_thread";
	int ret;

	/* TESTPOINT: Try getting name of NULL thread (aka uninitialized
	 * thread var).
	 */
	ret = pthread_getname_np(PTHREAD_INVALID, thr_name_buf, sizeof(thr_name_buf));
	zassert_equal(ret, ESRCH, "uninitialized getname!");

	/* TESTPOINT: Try setting name of NULL thread (aka uninitialized
	 * thread var).
	 */
	ret = pthread_setname_np(PTHREAD_INVALID, thr_name);
	zassert_equal(ret, ESRCH, "uninitialized setname!");

	zassert_ok(pthread_create(&th, NULL, timedjoin_thread, INT_TO_POINTER(SLEEP_DURATION_MS)));

	/* TESTPOINT: Try getting thread name with no buffer */
	ret = pthread_getname_np(th, NULL, sizeof(thr_name_buf));
	zassert_equal(ret, EINVAL, "uninitialized getname!");

	/* TESTPOINT: Try setting thread name with no buffer */
	ret = pthread_setname_np(th, NULL);
	zassert_equal(ret, EINVAL, "uninitialized setname!");

	/* TESTPOINT: Try setting thread name */
	ret = pthread_setname_np(th, thr_name);
	zassert_false(ret, "Set thread name failed!");

	/* TESTPOINT: Try getting thread name */
	ret = pthread_getname_np(th, thr_name_buf, sizeof(thr_name_buf));
	zassert_false(ret, "Get thread name failed!");

	/* TESTPOINT: Thread names match */
	ret = strncmp(thr_name, thr_name_buf, MIN(strlen(thr_name), strlen(thr_name_buf)));
	zassert_false(ret, "Thread names don't match!");

	/* Join to clean up */
	zassert_ok(pthread_join(th, NULL));
}

ZTEST_SUITE(posix_non_portable, NULL, NULL, NULL, NULL, NULL);
