/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 */

#include "_main.h"

ZTEST_USER_F(net_socketpair, test_close_one_end_and_write_to_the_other)
{
	int res;

	for (size_t i = 0; i < 2; ++i) {
		res = zsock_close(fixture->sv[i]);
		zassert_equal(res, 0, "close(fixture->sv[%u]) failed: %d", i, errno);
		fixture->sv[i] = -1;

		res = zsock_send(fixture->sv[(!i) & 1], "x", 1, 0);
		zassert_equal(res, -1, "expected send() to fail");
		zassert_equal(res, -1, "errno: expected: EPIPE actual: %d",
			errno);

		res = zsock_close(fixture->sv[(!i) & 1]);
		zassert_equal(res, 0, "close(fixture->sv[%u]) failed: %d", i, errno);
		fixture->sv[(!i) & 1] = -1;

		res = zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, fixture->sv);
		zassert_equal(res, 0, "socketpair() failed: %d", errno);
	}
}

ZTEST_USER_F(net_socketpair, test_close_one_end_and_read_from_the_other)
{
	int res;
	char xx[16];

	for (size_t i = 0; i < 2; ++i) {
		/* We want to write some bytes to the closing end of the
		 * socket before it gets shut down, so that we can prove that
		 * reading is possible from the other end still and that data
		 * is not lost.
		 */
		res = zsock_send(fixture->sv[i], "xx", 2, 0);
		zassert_not_equal(res, -1, "send() failed: %d", errno);
		zassert_equal(res, 2, "write() failed to write 2 bytes");

		res = zsock_close(fixture->sv[i]);
		zassert_equal(res, 0, "close(fixture->sv[%u]) failed: %d", i, errno);
		fixture->sv[i] = -1;

		memset(xx, 0, sizeof(xx));
		res = zsock_recv(fixture->sv[(!i) & 1], xx, sizeof(xx), 0);
		zassert_not_equal(res, -1, "read() failed: %d", errno);
		zassert_equal(res, 2, "expected to read 2 bytes but read %d",
			res);

		res = zsock_recv(fixture->sv[(!i) & 1], xx, sizeof(xx), 0);
		zassert_equal(res, 0, "expected read() to succeed but read 0 bytes");

		res = zsock_close(fixture->sv[(!i) & 1]);
		zassert_equal(res, 0, "close(fixture->sv[%u]) failed: %d", i, errno);
		fixture->sv[(!i) & 1] = -1;

		res = zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, fixture->sv);
		zassert_equal(res, 0, "socketpair() failed: %d", errno);
	}
}
