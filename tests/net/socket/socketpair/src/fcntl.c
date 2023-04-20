/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_USER(net_socketpair, test_fcntl)
{
	int res;
	int sv[2] = {-1, -1};
	int flags;

	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_equal(res, 0,
		"socketpair(AF_UNIX, SOCK_STREAM, 0, sv) failed");

	res = fcntl(sv[0], F_GETFL, 0);
	zassert_not_equal(res, -1,
		"fcntl(sv[0], F_GETFL) failed. errno: %d", errno);

	flags = res;
	zassert_equal(res & O_NONBLOCK, 0,
		"socketpair should block by default");

	res = fcntl(sv[0], F_SETFL, flags | O_NONBLOCK);
	zassert_not_equal(res, -1,
		"fcntl(sv[0], F_SETFL, flags | O_NONBLOCK) failed. errno: %d",
		errno);

	res = fcntl(sv[0], F_GETFL, 0);
	zassert_equal(res ^ flags, O_NONBLOCK, "expected O_NONBLOCK set");

	close(sv[0]);
	close(sv[1]);
}
