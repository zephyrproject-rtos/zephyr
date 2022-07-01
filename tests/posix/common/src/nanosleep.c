/*
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <errno.h>
#include <zephyr/posix/time.h>
#include <stdint.h>
#include <zephyr/sys_clock.h>

/** req and rem are both NULL */
ZTEST(posix_apis, test_nanosleep_NULL_NULL)
{
	int r = nanosleep(NULL, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EFAULT, "actual: %d expected: %d", errno, EFAULT);
}

/**
 * req is NULL, rem is non-NULL (all-zero)
 *
 * Expect rem to be the same when function returns
 */
ZTEST(posix_apis, test_nanosleep_NULL_notNULL)
{
	struct timespec rem = {};

	errno = 0;
	int r = nanosleep(NULL, &rem);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EFAULT, "actual: %d expected: %d",
	errno, EFAULT);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d",
	rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d",
	rem.tv_nsec, 0);
}

/**
 * req is non-NULL (all-zero), rem is NULL
 *
 * Expect req to be the same when function returns
 */
ZTEST(posix_apis, test_nanosleep_notNULL_NULL)
{
	struct timespec req = {};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(req.tv_sec, 0, "actual: %d expected: %d",
		req.tv_sec, 0);
	zassert_equal(req.tv_nsec, 0, "actual: %d expected: %d",
		req.tv_nsec, 0);
	zassert_equal(r, 0, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, 0, "actual: %d expected: %d", errno, 0);
}

/**
 * req is non-NULL (all-zero), rem is non-NULL (all-zero)
 *
 * Expect req & rem to be the same when function returns
 */
ZTEST(posix_apis, test_nanosleep_notNULL_notNULL)
{
	struct timespec req = {};
	struct timespec rem = {};

	errno = 0;
	int r = nanosleep(&req, &rem);

	zassert_equal(r, 0, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, 0, "actual: %d expected: %d", errno, 0);
	zassert_equal(req.tv_sec, 0, "actual: %d expected: %d",
		req.tv_sec, 0);
	zassert_equal(req.tv_nsec, 0, "actual: %d expected: %d",
		req.tv_nsec, 0);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d",
		rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d",
		rem.tv_nsec, 0);
}

/**
 * req and rem point to the same timespec
 *
 * Normative spec says they may be the same.
 * Expect rem to be zero after returning.
 */
ZTEST(posix_apis, test_nanosleep_req_is_rem)
{
	struct timespec ts = {0, 1};

	errno = 0;
	int r = nanosleep(&ts, &ts);

	zassert_equal(r, 0, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, 0, "actual: %d expected: %d", errno, 0);
	zassert_equal(ts.tv_sec, 0, "actual: %d expected: %d",
		ts.tv_sec, 0);
	zassert_equal(ts.tv_nsec, 0, "actual: %d expected: %d",
		ts.tv_nsec, 0);
}

/** req tv_sec is -1 */
ZTEST(posix_apis, test_nanosleep_n1_0)
{
	struct timespec req = {-1, 0};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EINVAL, "actual: %d expected: %d", errno, EFAULT);
}

/** req tv_nsec is -1 */
ZTEST(posix_apis, test_nanosleep_0_n1)
{
	struct timespec req = {0, -1};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EINVAL, "actual: %d expected: %d", errno, EFAULT);
}

/** req tv_sec and tv_nsec are both -1 */
ZTEST(posix_apis, test_nanosleep_n1_n1)
{
	struct timespec req = {-1, -1};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EINVAL, "actual: %d expected: %d", errno, EFAULT);
}

/** req tv_sec is 0 tv_nsec is 10^9 */
ZTEST(posix_apis, test_nanosleep_0_1000000000)
{
	struct timespec req = {0, 1000000000};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EINVAL, "actual: %d expected: %d", errno, EFAULT);
}

static void common(const uint32_t s, uint32_t ns)
{
	uint32_t then;
	uint32_t now;
	int r;
	struct timespec req = {s, ns};
	struct timespec rem = {0, 0};

	errno = 0;
	then = k_cycle_get_32();
	r = nanosleep(&req, &rem);
	now = k_cycle_get_32();

	zassert_equal(r, 0, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, 0, "actual: %d expected: %d", errno, 0);
	zassert_equal(req.tv_sec, s, "actual: %d expected: %d",
		req.tv_sec, s);
	zassert_equal(req.tv_nsec, ns, "actual: %d expected: %d",
		req.tv_nsec, ns);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d",
	rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d",
		rem.tv_nsec, 0);

	uint64_t actual_ns = k_cyc_to_ns_ceil64((now - then));
	uint64_t exp_ns = (uint64_t)s * NSEC_PER_SEC + ns;
	/* round up to the nearest microsecond for k_busy_wait() */
	exp_ns = ceiling_fraction(exp_ns, NSEC_PER_USEC) * NSEC_PER_USEC;

	/* lower bounds check */
	zassert_true(actual_ns >= exp_ns,
		"actual: %llu expected: %llu", actual_ns, exp_ns);

	/* TODO: Upper bounds check when hr timers are available */
}

/** sleep for 1ns */
ZTEST(posix_apis, test_nanosleep_0_1)
{
	common(0, 1);
}

/** sleep for 1us + 1ns */
ZTEST(posix_apis, test_nanosleep_0_1001)
{
	common(0, 1001);
}

/** sleep for 500000000ns */
ZTEST(posix_apis, test_nanosleep_0_500000000)
{
	common(0, 500000000);
}

/** sleep for 1s */
ZTEST(posix_apis, test_nanosleep_1_0)
{
	common(1, 0);
}

/** sleep for 1s + 1ns */
ZTEST(posix_apis, test_nanosleep_1_1)
{
	common(1, 1);
}

/** sleep for 1s + 1us + 1ns */
ZTEST(posix_apis, test_nanosleep_1_1001)
{
	common(1, 1001);
}
