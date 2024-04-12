/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_USER_F(net_socketpair, test_write_nonblock)
{
	int res;

	for (size_t i = 0; i < 2; ++i) {
		/* first, fill up the buffer */
		for (size_t k = 0; k < CONFIG_NET_SOCKETPAIR_BUFFER_SIZE;
			++k) {
			res = zsock_send(fixture->sv[i], "x", 1, 0);
			zassert_equal(res, 1, "send() failed: %d", errno);
		}

		/* then set the O_NONBLOCK flag */
		res = zsock_fcntl(fixture->sv[i], F_GETFL, 0);
		zassert_not_equal(res, -1, "fcntl() failed: %d", i, errno);

		res = zsock_fcntl(fixture->sv[i], F_SETFL, res | O_NONBLOCK);
		zassert_not_equal(res, -1, "fcntl() failed: %d", i, errno);

		/* then, try to write one more byte */
		res = zsock_send(fixture->sv[i], "x", 1, 0);
		zassert_equal(res, -1, "expected send to fail");
		zassert_equal(errno, EAGAIN, "errno: expected: EAGAIN "
			"actual: %d", errno);
	}
}

ZTEST_USER_F(net_socketpair, test_read_nonblock)
{
	int res;
	char c;

	for (size_t i = 0; i < 2; ++i) {
		/* set the O_NONBLOCK flag */
		res = zsock_fcntl(fixture->sv[i], F_GETFL, 0);
		zassert_not_equal(res, -1, "fcntl() failed: %d", i, errno);

		res = zsock_fcntl(fixture->sv[i], F_SETFL, res | O_NONBLOCK);
		zassert_not_equal(res, -1, "fcntl() failed: %d", i, errno);

		/* then, try to read one byte */
		res = zsock_recv(fixture->sv[i], &c, 1, 0);
		zassert_equal(res, -1, "expected recv() to fail");
		zassert_equal(errno, EAGAIN, "errno: expected: EAGAIN "
			"actual: %d", errno);
	}
}
