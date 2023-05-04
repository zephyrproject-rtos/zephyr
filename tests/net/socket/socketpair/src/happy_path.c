/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

static void happy_path(
	const int family, const char *family_s,
	const int type, const char *type_s,
	const int proto, const char *proto_s
)
{
	int res;
	int sv[2] = {-1, -1};

	const char *expected_msg = "Hello, socketpair(2) world!";
	const unsigned int expected_msg_len = strlen(expected_msg);
	char actual_msg[32];
	size_t actual_msg_len;
	struct iovec iovec;
	struct msghdr msghdr;

	LOG_DBG("calling socketpair(%u, %u, %u, %p)", family, type, proto, sv);
	res = socketpair(family, type, proto, sv);
	zassert_true(res == -1 || res == 0,
		     "socketpair returned an unspecified value");
	zassert_equal(res, 0, "socketpair failed");
	LOG_DBG("sv: {%d, %d}", sv[0], sv[1]);

	socklen_t len;

	/* sockets are bidirectional. test functions from both ends */
	for (int i = 0; i < 2; ++i) {

		/*
		 * Test with send() / recv()
		 */

		res = send(sv[i], expected_msg, expected_msg_len, 0);

		zassert_not_equal(res, -1, "send() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not send entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		res = recv(sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0);

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

		res = sendto(sv[i], expected_msg, expected_msg_len, 0, NULL, 0);

		zassert_not_equal(res, -1, "sendto() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not sendto entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		len = 0;
		res = recvfrom(sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0,
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

		res = sendmsg(sv[i], &msghdr, 0);

		zassert_not_equal(res, -1, "sendmsg() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not sendmsg entire message");

		res = recv(sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0);

		zassert_not_equal(res, -1, "recv() failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(strncmp(expected_msg, actual_msg,
			actual_msg_len) == 0,
			"the wrong message was passed through the socketpair");
	}

	res = close(sv[0]);
	zassert_equal(res, 0, "close failed");

	res = close(sv[1]);
	zassert_equal(res, 0, "close failed");
}

ZTEST_USER(net_socketpair, test_AF_LOCAL__SOCK_STREAM__0)
{
	happy_path(
		AF_LOCAL, "AF_LOCAL",
		SOCK_STREAM, "SOCK_STREAM",
		0, "0"
	);
}

ZTEST_USER(net_socketpair, test_AF_UNIX__SOCK_STREAM__0)
{
	happy_path(
		AF_UNIX, "AF_UNIX",
		SOCK_STREAM, "SOCK_STREAM",
		0, "0"
	);
}
