/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_USER_F(net_socketpair, test_fcntl)
{
	int res;
	int flags;

	res = fcntl(fixture->sv[0], F_GETFL, 0);
	zassert_not_equal(res, -1,
		"fcntl(fixture->sv[0], F_GETFL) failed. errno: %d", errno);

	flags = res;
	zassert_equal(res & O_NONBLOCK, 0,
		"socketpair should block by default");

	res = fcntl(fixture->sv[0], F_SETFL, flags | O_NONBLOCK);
	zassert_not_equal(res, -1,
		"fcntl(fixture->sv[0], F_SETFL, flags | O_NONBLOCK) failed. errno: %d",
		errno);

	res = fcntl(fixture->sv[0], F_GETFL, 0);
	zassert_equal(res ^ flags, O_NONBLOCK, "expected O_NONBLOCK set");
}
