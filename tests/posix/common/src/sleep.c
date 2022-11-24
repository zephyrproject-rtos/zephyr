/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <posix/unistd.h>
#include <ztest.h>

struct waker_work {
	k_tid_t tid;
	struct k_work_delayable dwork;
};
static struct waker_work ww;

static void waker_func(struct k_work *work)
{
	struct waker_work *ww;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	ww = CONTAINER_OF(dwork, struct waker_work, dwork);
	k_wakeup(ww->tid);
}
K_WORK_DELAYABLE_DEFINE(waker, waker_func);

void test_sleep(void)
{
	uint32_t then;
	uint32_t now;
	/* call sleep(10), wakeup after 1s, expect >= 8s left */
	const uint32_t sleep_min_s = 1;
	const uint32_t sleep_max_s = 10;
	const uint32_t sleep_rem_s = 8;

	/* sleeping for 0s should return 0 */
	zassert_ok(sleep(0), NULL);

	/* test that sleeping for 1s sleeps for at least 1s */
	then = k_uptime_get();
	zassert_equal(0, sleep(1), NULL);
	now = k_uptime_get();
	zassert_true((now - then) >= 1 * MSEC_PER_SEC, NULL);

	/* test that sleeping for 2s sleeps for at least 2s */
	then = k_uptime_get();
	zassert_equal(0, sleep(2), NULL);
	now = k_uptime_get();
	zassert_true((now - then) >= 2 * MSEC_PER_SEC, NULL);

	/* test that sleep reports the remainder */
	ww.tid = k_current_get();
	k_work_init_delayable(&ww.dwork, waker_func);
	zassert_equal(1, k_work_schedule(&ww.dwork, K_SECONDS(sleep_min_s)), NULL);
	zassert_true(sleep(sleep_max_s) >= sleep_rem_s, NULL);
}

void test_usleep(void)
{
	uint32_t then;
	uint32_t now;

	/* test usleep works for small values */
	/* Note: k_usleep(), an implementation detail, is a cancellation point */
	zassert_equal(0, usleep(0), NULL);
	zassert_equal(0, usleep(1), NULL);

	/* sleep for the spec limit */
	then = k_uptime_get();
	zassert_equal(0, usleep(USEC_PER_SEC - 1), NULL);
	now = k_uptime_get();
	zassert_true(((now - then) * USEC_PER_MSEC) / (USEC_PER_SEC - 1) >= 1, NULL);

	/* sleep for exactly the limit threshold */
	zassert_equal(-1, usleep(USEC_PER_SEC), NULL);
	zassert_equal(errno, EINVAL, NULL);

	/* sleep for over the spec limit */
	zassert_equal(-1, usleep((useconds_t)ULONG_MAX), NULL);
	zassert_equal(errno, EINVAL, NULL);

	/* test that sleep reports errno = EINTR when woken up */
	ww.tid = k_current_get();
	k_work_init_delayable(&ww.dwork, waker_func);
	zassert_equal(1, k_work_schedule(&ww.dwork, K_USEC(USEC_PER_SEC / 2)), NULL);
	zassert_equal(-1, usleep(USEC_PER_SEC - 1), NULL);
	zassert_equal(EINTR, errno, NULL);
}
