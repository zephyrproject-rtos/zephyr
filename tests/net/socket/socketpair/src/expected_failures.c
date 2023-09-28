/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_USER_F(net_socketpair, test_expected_failures)
{
	int res;

	/* Use invalid values in fields starting from left to right */

	res = socketpair(AF_INET, SOCK_STREAM, 0, fixture->sv);
	if (res != -1) {
		for (int i = 0; i < 2; ++i) {
			zassert_ok(close(fixture->sv[i]));
			fixture->sv[i] = -1;
		}
	}
	zassert_equal(res, -1, "socketpair should fail with bad address family");
	zassert_equal(errno, EAFNOSUPPORT,
				  "errno should be EAFNOSUPPORT with bad address family");

	res = socketpair(AF_UNIX, 42, 0, fixture->sv);
	if (res != -1) {
		for (int i = 0; i < 2; ++i) {
			zassert_ok(close(fixture->sv[i]));
			fixture->sv[i] = -1;
		}
	}
	zassert_equal(res, -1,
				  "socketpair should fail with unsupported socket type");
	zassert_equal(errno, EPROTOTYPE,
				  "errno should be EPROTOTYPE with bad socket type");

	res = socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_TLS_1_0, fixture->sv);
	if (res != -1) {
		for (int i = 0; i < 2; ++i) {
			zassert_ok(close(fixture->sv[i]));
			fixture->sv[i] = -1;
		}
	}
	zassert_equal(res, -1,
				  "socketpair should fail with unsupported protocol");
	zassert_equal(errno, EPROTONOSUPPORT,
				  "errno should be EPROTONOSUPPORT with bad protocol");

	/* This is not a POSIX requirement, but should be safe */
	res = socketpair(AF_UNIX, SOCK_STREAM, 0, NULL);
	if (res != -1) {
		for (int i = 0; i < 2; ++i) {
			zassert_ok(close(fixture->sv[i]));
			fixture->sv[i] = -1;
		}
	}
	zassert_equal(res, -1,
				  "socketpair should fail with invalid socket vector");
	zassert_equal(errno, EFAULT,
				  "errno should be EFAULT with bad socket vector");
}
