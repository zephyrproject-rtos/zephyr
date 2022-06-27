/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_can.h>

struct test_case {
	int family;
	int type;
	int proto;
};

static const struct test_result {
	struct test_case test_case;
	int result;
	int error;
} expected_result[] = {
	{
		/* 0 */
		.test_case.family = AF_INET,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = IPPROTO_UDP,
		.result = 0,
	},
	{
		/* 1 */
		.test_case.family = AF_INET6,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = IPPROTO_UDP,
		.result = 0,
	},
	{
		/* 2 */
		/* This test will not increase the called func count */
		.test_case.family = AF_UNSPEC,
		.test_case.type = 0,
		.test_case.proto = 0,
		.result = -1,
		.error = EAFNOSUPPORT,
	},
	{
		/* 3 */
		.test_case.family = AF_INET,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = 0,
		.result = 0,
	},
	{
		/* 4 */
		.test_case.family = AF_INET6,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = 0,
		.result = 0,
	},
	{
		/* 5 */
		.test_case.family = AF_INET,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = IPPROTO_UDP,
		.result = 0,
	},
	{
		/* 6 */
		.test_case.family = AF_INET6,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = IPPROTO_UDP,
		.result = 0,
	},
	{
		/* 7 */
		.test_case.family = AF_INET,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = IPPROTO_UDP,
		.result = 0,
	},
	{
		/* 8 */
		.test_case.family = AF_INET6,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = IPPROTO_UDP,
		.result = 0,
	},
	{
		/* 9 */
		.test_case.family = AF_INET6,
		.test_case.type = SOCK_STREAM,
		.test_case.proto = IPPROTO_UDP,
		.result = -1,
		.error = EOPNOTSUPP,
	},
	{
		/* 10 */
		.test_case.family = AF_PACKET,
		.test_case.type = SOCK_RAW,
		.test_case.proto = ETH_P_ALL,
		.result = 0,
	},
	{
		/* 11 */
		.test_case.family = AF_CAN,
		.test_case.type = SOCK_RAW,
		.test_case.proto = CAN_RAW,
		.result = 0,
	},
	{
		/* 12 */
		.test_case.family = AF_INET6,
		.test_case.type = SOCK_STREAM,
		.test_case.proto = IPPROTO_TLS_1_2,
		.result = 0,
	},
	{
		/* 13 */
		.test_case.family = AF_INET,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = IPPROTO_DTLS_1_0,
		.result = 0,
	},
	{
		/* 14 */
		.test_case.family = AF_CAN,
		.test_case.type = SOCK_RAW,
		.test_case.proto = IPPROTO_RAW,
		.result = -1,
		.error = EAFNOSUPPORT,
	},
	{
		/* 15 */
		.test_case.family = AF_INET,
		.test_case.type = SOCK_DGRAM,
		.test_case.proto = 254,
		.result = -1,
		.error = EPROTONOSUPPORT,
	},
};

static int current_test;
static int func_called;
static int failed_family = 2; /* The number of tests that are checked
			       * by generic socket code like the test
			       * number 3
			       */

static int socket_test(int family, int type, int proto)
{
	struct net_context *ctx;
	int ret;

	func_called++;

	ret = net_context_get(family, type, proto, &ctx);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

static int socket_test_ok(int family, int type, int proto)
{
	func_called++;
	return 0;
}

static bool is_tls(int family, int type, int proto)
{
	if ((family == AF_INET || family == AF_INET6) &&
	    (((proto >= IPPROTO_TLS_1_0) && (proto <= IPPROTO_TLS_1_2)) ||
	     (proto >= IPPROTO_DTLS_1_0 && proto <= IPPROTO_DTLS_1_2))) {
		return true;
	}

	return false;
}

static bool is_packet(int family, int type, int proto)
{
	if (type != SOCK_RAW || proto != ETH_P_ALL) {
		return false;
	}

	return true;
}

static bool is_can(int family, int type, int proto)
{
	if (type != SOCK_RAW || proto != CAN_RAW) {
		return false;
	}

	return true;
}

static bool is_ip(int family, int type, int proto)
{
	if (family != AF_INET && family != AF_INET6) {
		return false;
	}

	return true;
}

#define TEST_SOCKET_PRIO 40

NET_SOCKET_REGISTER(af_inet,   TEST_SOCKET_PRIO, AF_INET,   is_ip,      socket_test);
NET_SOCKET_REGISTER(af_inet6,  TEST_SOCKET_PRIO, AF_INET6,  is_ip,      socket_test);
NET_SOCKET_REGISTER(af_can2,   TEST_SOCKET_PRIO, AF_CAN,    is_ip,      socket_test);

/* For these socket families, we return ok always for now */
NET_SOCKET_REGISTER(tls,       TEST_SOCKET_PRIO, AF_UNSPEC, is_tls,    socket_test_ok);
NET_SOCKET_REGISTER(af_packet, TEST_SOCKET_PRIO, AF_PACKET, is_packet, socket_test_ok);
NET_SOCKET_REGISTER(af_can,    TEST_SOCKET_PRIO, AF_CAN,    is_can,    socket_test_ok);

void test_create_sockets(void)
{
	int i, fd, ok_tests = 0, failed_tests = 0;

	for (i = 0; i < ARRAY_SIZE(expected_result); i++, current_test++) {
		errno = 0;

		fd = socket(expected_result[i].test_case.family,
			    expected_result[i].test_case.type,
			    expected_result[i].test_case.proto);

		if (errno == EPROTONOSUPPORT) {
			func_called--;
			continue;
		}

		zassert_equal(fd, expected_result[i].result,
			      "[%d] Invalid result (expecting %d got %d, "
			      "errno %d)", i, expected_result[i].result, fd,
			      errno);
		if (expected_result[i].result < 0) {
			zassert_equal(errno, expected_result[i].error,
				      "[%d] Invalid errno (%d vs %d)", i,
				      errno, expected_result[i].error);
		}

		if (expected_result[i].result == 0) {
			ok_tests++;
		} else {
			failed_tests++;
		}

		if (fd >= 0) {
			close(fd);
		}
	}

	zassert_equal(ok_tests + failed_tests - failed_family, func_called,
		      "Invalid num of tests failed (%d vs %d)",
		      ok_tests + failed_tests - failed_family, func_called);
}

void test_main(void)
{
	ztest_test_suite(socket_register,
			 ztest_unit_test(test_create_sockets));

	ztest_run_test_suite(socket_register);
}
