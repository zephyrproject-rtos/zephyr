/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thrd.h"

#include <stdint.h>
#include <threads.h>

#include <zephyr/ztest.h>

#define WAIT_TIME_MS 100

static struct libc_cnd_fixture {
	/* shared between threads in tests */
	cnd_t cond;
	mtx_t mutex;

	/* de-duplicate local variables in test cases */
	int res1;
	int res2;
	thrd_t thrd1;
	thrd_t thrd2;
	bool do_timedwait;
	bool is_broadcast;
	struct timespec time_point;
} _libc_cnd_fixture;

ZTEST_F(libc_cnd, test_cnd_init_destroy)
{
	/* degenerate cases */
	if (false) {
		/* pthread_cond_init() pthread_cond_destroy() are not hardened against these */
		zassert_equal(thrd_error, cnd_init(NULL));
		zassert_equal(thrd_error, cnd_init((cnd_t *)BIOS_FOOD));
		cnd_destroy(NULL);
		cnd_destroy((cnd_t *)BIOS_FOOD);
	}

	/* happy path tested in before() / after() */
}

ZTEST_F(libc_cnd, test_cnd_errors)
{
	/* degenerate test cases */
	if (false) {
		/* pthread_cond_*() are not hardened against these */
		zassert_equal(thrd_error, cnd_signal(NULL));
		zassert_equal(thrd_error, cnd_broadcast(NULL));
		zassert_equal(thrd_error, cnd_wait(NULL, NULL));
		zassert_equal(thrd_error, cnd_wait(NULL, &fixture->mutex));
		zassert_equal(thrd_error, cnd_wait(&fixture->cond, NULL));
		zassert_equal(thrd_error, cnd_timedwait(NULL, NULL, NULL));
		zassert_equal(thrd_error, cnd_timedwait(NULL, NULL, &fixture->time_point));
		zassert_equal(thrd_error, cnd_timedwait(NULL, &fixture->mutex, NULL));
		zassert_equal(thrd_error,
			      cnd_timedwait(NULL, &fixture->mutex, &fixture->time_point));
		zassert_equal(thrd_error, cnd_timedwait(&fixture->cond, NULL, NULL));
		zassert_equal(thrd_error,
			      cnd_timedwait(&fixture->cond, NULL, &fixture->time_point));
		zassert_equal(thrd_error, cnd_timedwait(&fixture->cond, &fixture->mutex, NULL));
	}
}

static int test_cnd_thread_fn(void *arg)
{
	int res = thrd_success;
	struct timespec time_point;
	struct libc_cnd_fixture *const fixture = arg;

	if (fixture->do_timedwait) {
		zassume_ok(clock_gettime(CLOCK_REALTIME, &time_point));
		timespec_add_ms(&time_point, WAIT_TIME_MS);
		res = cnd_timedwait(&fixture->cond, &fixture->mutex, &time_point);
	} else {
		res = cnd_wait(&fixture->cond, &fixture->mutex);
	}

	if (fixture->is_broadcast) {
		/* re-signal so that the next thread wakes up too */
		zassert_equal(thrd_success, cnd_signal(&fixture->cond));
	}

	(void)mtx_unlock(&fixture->mutex);

	return res;
}

static void tst_cnd_common(struct libc_cnd_fixture *fixture, size_t wait_ms, bool th2, int exp1,
			   int exp2)
{
	zassert_equal(thrd_success, mtx_lock(&fixture->mutex));

	zassert_equal(thrd_success, thrd_create(&fixture->thrd1, test_cnd_thread_fn, fixture));
	if (th2) {
		zassert_equal(thrd_success,
			      thrd_create(&fixture->thrd2, test_cnd_thread_fn, fixture));
	}

	k_msleep(wait_ms);

	if (fixture->is_broadcast) {
		zassert_equal(thrd_success, cnd_broadcast(&fixture->cond));
	} else {
		zassert_equal(thrd_success, cnd_signal(&fixture->cond));
	}

	zassert_equal(thrd_success, mtx_unlock(&fixture->mutex));

	zassert_equal(thrd_success, thrd_join(fixture->thrd1, &fixture->res1));
	if (th2) {
		zassert_equal(thrd_success, thrd_join(fixture->thrd2, &fixture->res2));
	}

	zassert_equal(exp1, fixture->res1);
	if (th2) {
		zassert_equal(exp2, fixture->res2);
	}
}

ZTEST_F(libc_cnd, test_cnd_signal_wait)
{
	tst_cnd_common(fixture, WAIT_TIME_MS / 2, false, thrd_success, DONT_CARE);
}

ZTEST_F(libc_cnd, test_cnd_signal_timedwait)
{
	fixture->do_timedwait = true;
	tst_cnd_common(fixture, WAIT_TIME_MS / 2, false, thrd_success, DONT_CARE);
}

ZTEST_F(libc_cnd, test_cnd_timedwait_timeout)
{
	fixture->do_timedwait = true;
	tst_cnd_common(fixture, WAIT_TIME_MS * 2, false, thrd_timedout, DONT_CARE);
}

ZTEST_F(libc_cnd, test_cnd_broadcast_wait)
{
	fixture->is_broadcast = true;
	tst_cnd_common(fixture, WAIT_TIME_MS, true, thrd_success, thrd_success);
}

static void *setup(void)
{
	return &_libc_cnd_fixture;
}

static void before(void *arg)
{
	struct libc_cnd_fixture *const fixture = arg;

	*fixture = (struct libc_cnd_fixture){
		.res1 = FORTY_TWO,
		.res2 = SEVENTY_THREE,
	};

	zassert_equal(thrd_success, mtx_init(&fixture->mutex, mtx_plain));
	zassert_equal(thrd_success, cnd_init(&fixture->cond));
}

static void after(void *arg)
{
	struct libc_cnd_fixture *const fixture = arg;

	cnd_destroy(&fixture->cond);
	mtx_destroy(&fixture->mutex);
}

ZTEST_SUITE(libc_cnd, NULL, setup, before, after, NULL);
