/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include <sys/util.h>
#include <unistd.h>

#include <ztest_assert.h>

#undef read
#define read(fd, buf, len) zsock_recv(fd, buf, len, 0)

#undef write
#define write(fd, buf, len) zsock_send(fd, buf, len, 0)

static void happy_path_inner(
	const unsigned family, const char *family_s,
	const unsigned type, const char *type_s,
	const unsigned proto, const char *proto_s
)
{
	int res;
	int sv[2];

	const char *expected_msg = "Hello, socketpair(2) world!";
	const size_t expected_msg_len = strlen(expected_msg);
	char actual_msg[32];
	size_t actual_msg_len;

	printf("calling socketpair(%s, %s, %s, sv)\n", family_s, type_s,
	       proto_s);
	res = socketpair(family, type, proto, sv);
	zassert_true(res == -1 || res == 0,
		     "socketpair returned an unspecified value");
	zassert_equal(res, 0, "socketpair failed");

	printf("sv: {%d, %d}\n", sv[0], sv[1]);

	socklen_t len;

	/* sockets are bidirectional. test functions from both ends */
	for(int i = 1; i > 0; --i) {

		printf("data direction: %d -> %d\n", sv[i], sv[(!i) & 1]);

		/*
		 * Test with write(2) / read(2)
		 */

		printf("testing write(2)\n");
		res = write(sv[i], expected_msg, expected_msg_len);

		zassert_not_equal(res, -1, "write(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not write entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		printf("testing read(2)\n");
		res = read(sv[(!i) & 1], actual_msg, sizeof(actual_msg));

		zassert_not_equal(res, -1, "read(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(0 == strncmp(expected_msg, actual_msg,
			actual_msg_len),
			"the wrong message was passed through the socketpair");

		/*
		 * Test with send(2) / recv(2)
		 */

		printf("testing send(2)\n");
		res = send(sv[i], expected_msg, expected_msg_len, 0);

		zassert_not_equal(res, -1, "send(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not send entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		printf("testing recv(2)\n");
		res = recv(sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0);

		zassert_not_equal(res, -1, "recv(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(0 == strncmp(expected_msg, actual_msg,
			actual_msg_len),
			"the wrong message was passed through the socketpair");

		/*
		 * Test with sendto(2) / recvfrom(2)
		 */

		printf("testing sendto(2)\n");
		res = sendto(sv[i], expected_msg, expected_msg_len, 0, NULL, 0);

		zassert_not_equal(res, -1, "sendto(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not sendto entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		printf("testing recvfrom(2)\n");
		res = recvfrom(sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0, NULL, &len);

		zassert_not_equal(res, -1, "recvfrom(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(0 == strncmp(expected_msg, actual_msg,
			actual_msg_len),
			"the wrong message was passed through the socketpair");

#if 0
		/*
		 * Test with sendmsg(2) / recvmsg(2)
		 */

		printf("testing sendmsg(2)\n");
		res = sendmsg(sv[i], expected_msg, expected_msg_len, 0);

		zassert_not_equal(res, -1, "sendmsg(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
				  "did not sendmsg entire message");

		memset(actual_msg, 0, sizeof(actual_msg));

		printf("testing recvmsg(2)\n");
		res = recvmsg(sv[(!i) & 1], actual_msg, sizeof(actual_msg), 0);

		zassert_not_equal(res, -1, "recvmsg(2) failed: %d", errno);
		actual_msg_len = res;
		zassert_equal(actual_msg_len, expected_msg_len,
			      "wrong return value");

		zassert_true(0 == strncmp(expected_msg, actual_msg,
			actual_msg_len),
			"the wrong message was passed through the socketpair");
#endif

	}

	printf("closing sv[0]: fd %d\n", sv[0]);
	res = close(sv[0]);
	zassert_equal(res, 0, "close failed");

	printf("closing sv[1]: fd %d\n", sv[1]);
	res = close(sv[1]);
	zassert_equal(res, 0, "close failed");
}

void test_socket_socketpair_happy_path(void)
{
	struct unstr {
		unsigned u;
		const char *s;
	};
	static const struct unstr address_family[] = {
		{ AF_LOCAL, "AF_LOCAL" },
		{ AF_UNIX,  "AF_UNIX"  },
	};
	static const struct unstr socket_type[] = {
		{ SOCK_STREAM, "SOCK_STREAM" },
		//{ SOCK_RAW,    "SOCK_RAW"    },
	};
	static const struct unstr protocol[] = {
		{ 0, "0" },
	};

	for(size_t i = 0; i < ARRAY_SIZE(address_family); ++i) {
		for(size_t j = 0; j < ARRAY_SIZE(socket_type); ++j) {
			for(size_t k = 0; k < ARRAY_SIZE(protocol); ++k) {
				happy_path_inner(
					address_family[i].u, address_family[i].s,
					socket_type[j].u, socket_type[j].s,
					protocol[k].u, protocol[k].s
				);
			}
		}
	}
}

void test_socket_socketpair_expected_failures(void)
{
	int res;
	int sv[2];

	/* Use invalid values in fields starting from left to right */

	res = socketpair(AF_INET, SOCK_STREAM, 0, sv);
	zassert_equal(res, -1, "socketpair with fail with bad address family");
	zassert_equal(errno, EAFNOSUPPORT,
				  "errno should be EAFNOSUPPORT with bad adddress family");
	if (res != -1) {
		close(sv[0]);
		close(sv[1]);
	}

	res = socketpair(AF_UNIX, 42, 0, sv);
	zassert_equal(res, -1,
				  "socketpair should fail with unsupported socket type");
	zassert_equal(errno, EPROTOTYPE,
				  "errno should be EPROTOTYPE with bad socket type");
	if (res != -1) {
		close(sv[0]);
		close(sv[1]);
	}

	res = socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_TLS_1_0, sv);
	zassert_equal(res, -1,
				  "socketpair should fail with unsupported protocol");
	zassert_equal(errno, EPROTONOSUPPORT,
				  "errno should be EPROTONOSUPPORT with bad protocol");
	if (res != -1) {
		close(sv[0]);
		close(sv[1]);
	}

	/* This is not a POSIX requirement, but should be safe */
	res = socketpair(AF_UNIX, SOCK_STREAM, 0, NULL);
	zassert_equal(res, -1,
				  "socketpair should fail with invalid socket vector");
	zassert_equal(errno, EINVAL,
				  "errno should be EINVAL with bad socket vector");
}

void test_socket_socketpair_unsupported_calls(void)
{
	int res;
	int sv[2];
	struct sockaddr_un addr = {
		.sun_family = AF_UNIX,
	};
	socklen_t len = sizeof(addr);

	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_equal(res, 0, "socketpair(AF_UNIX, SOCK_STREAM, 0, sv) failed");


	for(size_t i = 0; i < 2; ++i) {

		res = bind(sv[i], (struct sockaddr *)&addr, len);
		zassert_equal(res, -1, "bind should fail on a socketpair endpoint");
		zassert_equal(errno, EISCONN, "bind should set errno to EISCONN");

		res = connect(sv[i], (struct sockaddr *)&addr, len);
		zassert_equal(res, -1, "connect should fail on a socketpair endpoint");
		zassert_equal(errno, EISCONN, "connect should set errno to EISCONN");

		res = listen(sv[i], 1);
		zassert_equal(res, -1, "listen should fail on a socketpair endpoint");
		zassert_equal(errno, EINVAL, "listen should set errno to EINVAL");

		res = accept(sv[i], (struct sockaddr *)&addr, &len);
		zassert_equal(res, -1, "accept should fail on a socketpair endpoint");
		zassert_equal(errno, EOPNOTSUPP, "accept should set errno to EOPNOTSUPP");
	}

	res = close(sv[0]);
	zassert_equal(res, 0, "close failed");

	res = close(sv[1]);
	zassert_equal(res, 0, "close failed");
}

void test_socket_socketpair_select(void) {
	/* TODO */
}

void test_socket_socketpair_poll(void) {
	/* TODO */
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(
		socket_socketpair,
		ztest_user_unit_test(test_socket_socketpair_happy_path),
		ztest_user_unit_test(test_socket_socketpair_expected_failures),
		ztest_user_unit_test(test_socket_socketpair_unsupported_calls),
		ztest_user_unit_test(test_socket_socketpair_select),
		ztest_user_unit_test(test_socket_socketpair_poll));

	ztest_run_test_suite(socket_socketpair);
}
