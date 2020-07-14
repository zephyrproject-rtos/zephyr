/*
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <errno.h>
#include <posix/time.h>
#include <stdint.h>
#include <sys_clock.h>

/* TODO: Upper bounds check when hr timers are available */
#define NSEC_PER_TICK \
	(NSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define NSEC_PER_CYCLE \
	(NSEC_PER_SEC / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)

/** req and rem are both NULL */
void test_nanosleep_NULL_NULL(void)
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
void test_nanosleep_NULL_notNULL(void)
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
void test_nanosleep_notNULL_NULL(void)
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
void test_nanosleep_notNULL_notNULL(void)
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
void test_nanosleep_req_is_rem(void)
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
void test_nanosleep_n1_0(void)
{
	struct timespec req = {-1, 0};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EINVAL, "actual: %d expected: %d", errno, EFAULT);
}

/** req tv_nsec is -1 */
void test_nanosleep_0_n1(void)
{
	struct timespec req = {0, -1};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EINVAL, "actual: %d expected: %d", errno, EFAULT);
}

/** req tv_sec and tv_nsec are both -1 */
void test_nanosleep_n1_n1(void)
{
	struct timespec req = {-1, -1};

	errno = 0;
	int r = nanosleep(&req, NULL);

	zassert_equal(r, -1, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, EINVAL, "actual: %d expected: %d", errno, EFAULT);
}

/** req tv_sec is 0 tv_nsec is 10^9 */
void test_nanosleep_0_1000000000(void)
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
	uint32_t dt;
	uint64_t dt_ns;
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

	ns += s * NSEC_PER_SEC;

	dt = now - then;
	dt_ns = k_cyc_to_ns_ceil64(dt);
	if (dt_ns == 0) {
		/* k_cycle_get_32() does not seem to be completely accurate on
		 * some virtual platforms in CI (some function calls to
		 * nanosleep reportedly take 0ns).
		 */
		dt_ns = 1;
	}

	printk("dt_ns: %llu\n", dt_ns);

	zassert_true(dt_ns >= ns, "expected dt_ns >= %d: actual: %d", (int)ns,
		(int)dt_ns);

	/* TODO: Upper bounds check when hr timers are available */
}

/** sleep for 1ns */
void test_nanosleep_0_1(void)
{
	common(0, 1);
}

/** sleep for 500000000ns */
void test_nanosleep_0_500000000(void)
{
	common(0, 500000000);
}

/** sleep for 1s */
void test_nanosleep_1_0(void)
{
	common(1, 0);
}
