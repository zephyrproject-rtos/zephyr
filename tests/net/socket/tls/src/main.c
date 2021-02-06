/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <ztest_assert.h>
#include <fcntl.h>
#include <net/socket.h>
#include <net/tls_credentials.h>

#include "../../socket_helpers.h"

#define ANY_PORT 0

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(1)

static void test_close(int sock)
{
	zassert_equal(close(sock),
		      0,
		      "close failed");
}

void test_so_type(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tls_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &sock1, &bind_addr4, IPPROTO_TLS_1_2);
	prepare_sock_tls_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &sock2, &bind_addr6, IPPROTO_TLS_1_2);

	rv = getsockopt(sock1, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_STREAM, "getsockopt got invalid type");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = getsockopt(sock2, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_STREAM, "getsockopt got invalid type");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	test_close(sock1);
	test_close(sock2);
	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_so_protocol(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tls_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &sock1, &bind_addr4, IPPROTO_TLS_1_2);
	prepare_sock_tls_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &sock2, &bind_addr6, IPPROTO_TLS_1_1);

	rv = getsockopt(sock1, SOL_SOCKET, SO_PROTOCOL, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_TLS_1_2,
		      "getsockopt got invalid protocol");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = getsockopt(sock2, SOL_SOCKET, SO_PROTOCOL, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_TLS_1_1,
		      "getsockopt got invalid protocol");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	test_close(sock1);
	test_close(sock2);
	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_main(void)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(8));
	}

	ztest_test_suite(
		socket_tls,
		ztest_unit_test(test_so_type),
		ztest_unit_test(test_so_protocol)
		);

	ztest_run_test_suite(socket_tls);
}
