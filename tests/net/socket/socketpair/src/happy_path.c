/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

static void happy_path(
	struct net_socketpair_fixture *fixture,
	const int family, const char *family_s,
	const int type, const char *type_s,
	const int proto, const char *proto_s
)
{
	int res;

	const char *expected_msg = "Hello, socketpair(2) world!";
	const unsigned int expected_msg_len = strlen(expected_msg);
	char actual_msg[32];
	size_t actual_msg_len;
	struct iovec iovec;
	struct msghdr msghdr;
	socklen_t len;

	/* sockets are bidirectional. test functions from both ends */
	for (int i = 0; i < 2; ++i) {

		/*
		 * Test with send() / recv()
		 */

		res = zsock_send(fixture->sv[i], expected_msg, expected_msg_len, 0);

		zassert_not_equal(res, -1, "send() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not send entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		res = zsock_recv(fixture->sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0);

		zassert_not_equal(res, -1, "recv() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(strncmp(expected_msg, actual_msg,
			actual_msg_len) == 0,
			"the wrong message was passed through the socketpair");

		/*
		 * Test with sendto(2) / recvfrom(2)
		 */

		res = zsock_sendto(fixture->sv[i], expected_msg, expected_msg_len, 0, NULL, 0);

		zassert_not_equal(res, -1, "sendto() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not sendto entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		len = 0;
		res = zsock_recvfrom(fixture->sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0,
				     NULL, &len);
		zassert_true(res >= 0, "recvfrom() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(strncmp(expected_msg, actual_msg,
			actual_msg_len) == 0,
			"the wrong message was passed through the socketpair");

		/*
		 * Test with sendmsg(2) / recv(2) - Zephyr lacks recvmsg atm
		 */

		memset(&msghdr, 0, sizeof(msghdr));
		msghdr.msg_iov = &iovec;
		msghdr.msg_iovlen = 1;
		iovec.iov_base = (void *)expected_msg;
		iovec.iov_len = expected_msg_len;

		res = zsock_sendmsg(fixture->sv[i], &msghdr, 0);

		zassert_not_equal(res, -1, "sendmsg() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not sendmsg entire message");

		res = zsock_recv(fixture->sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0);

		zassert_not_equal(res, -1, "recv() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(strncmp(expected_msg, actual_msg,
			actual_msg_len) == 0,
			"the wrong message was passed through the socketpair");
	}
}

ZTEST_USER_F(net_socketpair, test_AF_LOCAL_SOCK_STREAM_0)
{
	happy_path(
		fixture,
		AF_LOCAL, "AF_LOCAL",
		SOCK_STREAM, "SOCK_STREAM",
		0, "0"
	);
}

ZTEST_USER_F(net_socketpair, test_AF_UNIX_SOCK_STREAM_0)
{
	happy_path(
		fixture,
		AF_UNIX, "AF_UNIX",
		SOCK_STREAM, "SOCK_STREAM",
		0, "0"
	);
}
