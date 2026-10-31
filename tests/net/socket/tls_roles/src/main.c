/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "../../socket_helpers.h"

#define MY_IPV4_ADDR "127.0.0.1"
#define SERVER_PORT 4242

#define TCP_TEARDOWN_TIMEOUT K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY)

static void check_result(int ret, bool supported)
{
	if (supported) {
		zassert_equal(ret, 0, "Operation failed (%d)", errno);
	} else {
		zassert_equal(ret, -1, "Operation should fail");
		zassert_equal(errno, EOPNOTSUPP, "Unexpected errno %d", errno);
	}
}

ZTEST(net_socket_tls_roles, test_dtls_role_sockopt)
{
	struct net_sockaddr_in addr;
	int role;
	int sock;
	int ret;

	prepare_sock_dtls_v4(MY_IPV4_ADDR, 0, &sock, &addr, NET_IPPROTO_DTLS_1_2);

	role = ZSOCK_TLS_DTLS_ROLE_SERVER;
	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_DTLS_ROLE, &role, sizeof(role));
	check_result(ret, IS_ENABLED(CONFIG_MBEDTLS_SSL_SRV_C));

	role = ZSOCK_TLS_DTLS_ROLE_CLIENT;
	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_DTLS_ROLE, &role, sizeof(role));
	check_result(ret, IS_ENABLED(CONFIG_MBEDTLS_SSL_CLI_C));

	zassert_ok(zsock_close(sock));
}

ZTEST(net_socket_tls_roles, test_tls_listen)
{
	struct net_sockaddr_in addr;
	int sock;
	int ret;

	prepare_sock_tls_v4(MY_IPV4_ADDR, SERVER_PORT, &sock, &addr, NET_IPPROTO_TLS_1_2);
	zassert_ok(zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr)));

	ret = zsock_listen(sock, 1);
	check_result(ret, IS_ENABLED(CONFIG_MBEDTLS_SSL_SRV_C));

	zassert_ok(zsock_close(sock));
	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST(net_socket_tls_roles, test_tls_connect)
{
	struct net_sockaddr_in s_addr;
	struct net_sockaddr_in c_addr;
	int s_sock;
	int c_sock;
	int ret;

	/* Plain TCP listener, so the TCP connection succeeds and the TLS
	 * client role is exercised.
	 */
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_addr);
	zassert_ok(zsock_bind(s_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr)));
	zassert_ok(zsock_listen(s_sock, 1));

	prepare_sock_tls_v4(MY_IPV4_ADDR, 0, &c_sock, &c_addr, NET_IPPROTO_TLS_1_2);

	ret = zsock_connect(c_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, -1, "TLS handshake should not complete");
	if (IS_ENABLED(CONFIG_MBEDTLS_SSL_CLI_C)) {
		zassert_not_equal(errno, EOPNOTSUPP, "Client role should be supported");
	} else {
		zassert_equal(errno, EOPNOTSUPP, "Unexpected errno %d", errno);
	}

	zassert_ok(zsock_close(c_sock));
	zassert_ok(zsock_close(s_sock));
	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST_SUITE(net_socket_tls_roles, NULL, NULL, NULL, NULL, NULL);
