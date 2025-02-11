/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#include "../../socket_helpers.h"

#define TEST_IPV4_ANY_ADDR "0.0.0.0"
#define TEST_MY_IPV4_ADDR "192.0.2.1"
#define TEST_MY_IPV4_ADDR2 "192.0.2.2"

#define TEST_IPV6_ANY_ADDR "::"
#define TEST_MY_IPV6_ADDR "2001:db8::1"
#define TEST_MY_IPV6_ADDR2 "2001:db8::2"

#define LOCAL_PORT 4242

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(3)

#define SHOULD_SUCCEED true
#define SHOULD_FAIL false

static void test_add_local_ip_address(sa_family_t family, const char *ip)
{
	if (family == AF_INET) {
		struct sockaddr_in addr;
		struct net_if_addr *ifaddr;

		zsock_inet_pton(AF_INET, ip, &addr.sin_addr);

		ifaddr = net_if_ipv4_addr_add(net_if_get_default(),
					      &addr.sin_addr,
					      NET_ADDR_MANUAL,
					      0);
		zassert_not_null(ifaddr,
				 "Cannot add IPv4 address %s", ip);
	} else if (family == AF_INET6) {
		struct sockaddr_in6 addr;

		zsock_inet_pton(AF_INET6, ip, &addr.sin6_addr);

		zassert_not_null(net_if_ipv6_addr_add(net_if_get_default(),
						      &addr.sin6_addr,
						      NET_ADDR_MANUAL,
						      0),
				 "Cannot add IPv6 address %s", ip);
	}
}

static void *setup(void)
{
	/* Make sure that both the specified IPv4 and IPv6 addresses are
	 * added to the network interface.
	 */
	test_add_local_ip_address(AF_INET, TEST_MY_IPV4_ADDR);
	test_add_local_ip_address(AF_INET6, TEST_MY_IPV6_ADDR);

	return NULL;
}

static inline void prepare_sock_tcp(sa_family_t family, const char *ip, uint16_t port,
				       int *sock, struct sockaddr *addr)
{
	if (family == AF_INET) {
		prepare_sock_tcp_v4(ip,
				    port,
				    sock,
				    (struct sockaddr_in *) addr);
	} else if (family == AF_INET6) {
		prepare_sock_tcp_v6(ip,
				    port,
				    sock,
				    (struct sockaddr_in6 *) addr);
	}
}

static inline void prepare_sock_udp(sa_family_t family, const char *ip, uint16_t port,
				       int *sock, struct sockaddr *addr)
{
	if (family == AF_INET) {
		prepare_sock_udp_v4(ip,
				    port,
				    sock,
				    (struct sockaddr_in *) addr);
	} else if (family == AF_INET6) {
		prepare_sock_udp_v6(ip,
				    port,
				    sock,
				    (struct sockaddr_in6 *) addr);
	}
}

static void test_getsocketopt_reuseaddr(int sock, void *optval, socklen_t *optlen)
{
	int ret;

	ret = zsock_getsockopt(sock, SOL_SOCKET, SO_REUSEADDR, optval, optlen);
	zassert_equal(ret, 0, "getsocketopt() failed with error %d", errno);
}

static void test_setsocketopt_reuseaddr(int sock, void *optval, socklen_t optlen)
{
	int ret;

	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, optval, optlen);
	zassert_equal(ret, 0, "setsocketopt() failed with error %d", errno);
}

static void test_enable_reuseaddr(int sock)
{
	int value = 1;

	test_setsocketopt_reuseaddr(sock, &value, sizeof(value));
}

static void test_getsocketopt_reuseport(int sock, void *optval, socklen_t *optlen)
{
	int ret;

	ret = zsock_getsockopt(sock, SOL_SOCKET, SO_REUSEPORT, optval, optlen);
	zassert_equal(ret, 0, "getsocketopt() failed with error %d", errno);
}

static void test_setsocketopt_reuseport(int sock, void *optval, socklen_t optlen)
{
	int ret;

	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, optval, optlen);
	zassert_equal(ret, 0, "setsocketopt() failed with error %d", errno);
}

static void test_enable_reuseport(int sock)
{
	int value = 1;

	test_setsocketopt_reuseport(sock, &value, sizeof(value));
}

static void test_bind_success(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret;

	ret = zsock_bind(sock, addr, addrlen);
	zassert_equal(ret, 0, "bind() failed with error %d", errno);
}

static void test_bind_fail(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret;

	ret = zsock_bind(sock, addr, addrlen);
	zassert_equal(ret, -1, "bind() succeeded incorrectly");

	zassert_equal(errno, EADDRINUSE, "bind() returned unexpected errno (%d)", errno);
}

static void test_listen(int sock)
{
	zassert_equal(zsock_listen(sock, 0),
		      0,
		      "listen() failed with error %d", errno);
}

static void test_connect_success(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret;

	ret = zsock_connect(sock, addr, addrlen);
	zassert_equal(ret, 0, "connect() failed with error %d", errno);

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
		/* Let the connection proceed */
		k_msleep(50);
	}
}

static void test_connect_fail(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret;

	ret = zsock_connect(sock, addr, addrlen);
	zassert_equal(ret, -1, "connect() succeeded incorrectly");

	zassert_equal(errno, EADDRINUSE, "connect() returned unexpected errno (%d)", errno);
}

static int test_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	int new_sock = zsock_accept(sock, addr, addrlen);

	zassert_not_equal(new_sock, -1, "accept() failed with error %d", errno);

	return new_sock;
}

static void test_sendto(int sock, const void *buf, size_t len, int flags,
		       const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int ret;

	ret = zsock_sendto(sock, buf, len, flags, dest_addr, addrlen);
	zassert_equal(ret, len, "sendto failed with error %d", errno);
}

static void test_recvfrom_success(int sock, void *buf, size_t max_len, int flags,
			 struct sockaddr *src_addr, socklen_t *addrlen)
{
	int ret;

	ret = zsock_recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
	zassert_equal(ret, max_len, "recvfrom failed with error %d", errno);
}

static void test_recvfrom_fail(int sock, void *buf, size_t max_len, int flags,
			 struct sockaddr *src_addr, socklen_t *addrlen)
{
	int ret;

	ret = zsock_recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
	zassert_equal(ret, -1, "recvfrom succeeded incorrectly");

	zassert_equal(errno, EAGAIN, "recvfrom() returned unexpected errno (%d)", errno);
}

static void test_recv_success(int sock, void *buf, size_t max_len, int flags)
{
	int ret;

	ret = zsock_recv(sock, buf, max_len, flags);
	zassert_equal(ret, max_len, "recv failed with error %d", errno);
}

static void test_recv_fail(int sock, void *buf, size_t max_len, int flags)
{
	int ret;

	ret = zsock_recv(sock, buf, max_len, flags);
	zassert_equal(ret, -1, "recvfrom succeeded incorrectly");

	zassert_equal(errno, EAGAIN, "recv() returned unexpected errno (%d)", errno);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_enable_disable)
{
	int server_sock = -1;
	int value = -1;
	socklen_t value_size = sizeof(int);

	server_sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(server_sock >= 0, "socket open failed");

	/* Read initial value */
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, &value_size);
	zassert_equal(value_size, sizeof(int), "incorrect value size returned by getsocketopt()");
	zassert_equal(value, (int) false, "SO_REUSEADDR incorrectly set (expected false)");

	/* Enable option */
	value = 1;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = 2;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = 0x100;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = -1;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	zsock_close(server_sock);
}


static void test_reuseaddr_unspecified_specified_common(sa_family_t family,
							char const *first_ip,
							char const *second_ip,
							bool should_succeed)
{
	int server_sock1 = -1;
	int server_sock2 = -1;

	struct sockaddr bind_addr1;
	struct sockaddr bind_addr2;

	/* Create the sockets */
	prepare_sock_tcp(family, first_ip, LOCAL_PORT, &server_sock1, &bind_addr1);
	prepare_sock_tcp(family, second_ip, LOCAL_PORT, &server_sock2, &bind_addr2);

	/* Bind the first socket */
	test_bind_success(server_sock1, &bind_addr1, sizeof(bind_addr1));

	/* Try to bind the second socket, should fail */
	test_bind_fail(server_sock2, &bind_addr2, sizeof(bind_addr2));

	/* Enable SO_REUSEADDR option for the second socket */
	test_enable_reuseaddr(server_sock2);

	/* Try to bind the second socket again */
	if (should_succeed) {
		test_bind_success(server_sock2, &bind_addr2, sizeof(bind_addr2));
	} else {
		test_bind_fail(server_sock2, &bind_addr2, sizeof(bind_addr2));
	}

	zsock_close(server_sock1);
	zsock_close(server_sock2);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv4_first_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_MY_IPV4_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv6_first_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_MY_IPV6_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv4_second_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET,
						    TEST_MY_IPV4_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv6_second_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET6,
						    TEST_MY_IPV6_ADDR,
						    TEST_IPV6_ANY_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv4_both_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv6_both_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_IPV6_ANY_ADDR,
						    SHOULD_FAIL);
}


static void test_reuseaddr_tcp_listening_common(sa_family_t family,
						char const *first_ip,
						char const *second_ip)
{
	int server_sock1 = -1;
	int server_sock2 = -1;

	struct sockaddr bind_addr1;
	struct sockaddr bind_addr2;

	/* Create the sockets */
	prepare_sock_tcp(family, first_ip, LOCAL_PORT, &server_sock1, &bind_addr1);
	prepare_sock_tcp(family, second_ip, LOCAL_PORT, &server_sock2, &bind_addr2);

	/* Bind the first socket */
	test_bind_success(server_sock1, &bind_addr1, sizeof(bind_addr1));

	/* Set the first socket to LISTEN state */
	test_listen(server_sock1);

	/* Enable SO_REUSEADDR option for the second socket */
	test_enable_reuseaddr(server_sock2);

	/* Try to bind the second socket, should fail */
	test_bind_fail(server_sock2, (struct sockaddr *) &bind_addr2, sizeof(bind_addr2));

	zsock_close(server_sock1);
	zsock_close(server_sock2);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv4_tcp_unspecified_listening)
{
	test_reuseaddr_tcp_listening_common(AF_INET,
					    TEST_IPV4_ANY_ADDR,
					    TEST_MY_IPV4_ADDR);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv6_tcp_unspecified_listening)
{
	test_reuseaddr_tcp_listening_common(AF_INET6,
					    TEST_IPV6_ANY_ADDR,
					    TEST_MY_IPV6_ADDR);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv4_tcp_specified_listening)
{
	test_reuseaddr_tcp_listening_common(AF_INET,
					    TEST_MY_IPV4_ADDR,
					    TEST_IPV4_ANY_ADDR);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv6_tcp_specified_listening)
{
	test_reuseaddr_tcp_listening_common(AF_INET6,
					    TEST_MY_IPV6_ADDR,
					    TEST_IPV6_ANY_ADDR);
}


static void test_reuseaddr_tcp_tcp_time_wait_common(sa_family_t family,
						    char const *first_ip,
						    char const *second_ip)
{
	int server_sock = -1;
	int client_sock = -1;
	int accept_sock = -1;

	struct sockaddr bind_addr;
	struct sockaddr conn_addr;

	struct sockaddr accept_addr;
	socklen_t accept_addrlen = sizeof(accept_addr);

	prepare_sock_tcp(family, first_ip, LOCAL_PORT, &server_sock, &bind_addr);
	prepare_sock_tcp(family, second_ip, LOCAL_PORT, &client_sock, &conn_addr);

	/* Bind the server socket */
	test_bind_success(server_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));

	/* Start listening on the server socket */
	test_listen(server_sock);

	/* Connect the client */
	test_connect_success(client_sock, &conn_addr, sizeof(conn_addr));

	/* Accept the client */
	accept_sock = test_accept(server_sock, &accept_addr, &accept_addrlen);

	/* Close the server socket */
	zsock_close(server_sock);

	/* Close the accepted socket */
	zsock_close(accept_sock);

	/* Wait a short time for the accept socket to enter TIME_WAIT state*/
	k_msleep(50);

	/* Recreate the server socket */
	prepare_sock_tcp(family, first_ip, LOCAL_PORT, &server_sock, &bind_addr);

	/* Bind the server socket, should fail */
	test_bind_fail(server_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));

	/* Enable SO_REUSEADDR option for the new server socket */
	test_enable_reuseaddr(server_sock);

	/* Try to bind the new server socket again, should work now */
	test_bind_success(server_sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));

	zsock_close(client_sock);
	zsock_close(server_sock);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv4_tcp_time_wait_unspecified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET,
						TEST_IPV4_ANY_ADDR,
						TEST_MY_IPV4_ADDR);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv6_tcp_time_wait_unspecified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET6,
						TEST_IPV6_ANY_ADDR,
						TEST_MY_IPV6_ADDR);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv4_tcp_time_wait_specified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET,
						TEST_MY_IPV4_ADDR,
						TEST_MY_IPV4_ADDR);
}

ZTEST_USER(socket_reuseaddr_test_suite, test_ipv6_tcp_time_wait_specified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET6,
						TEST_MY_IPV6_ADDR,
						TEST_MY_IPV6_ADDR);
}


ZTEST_SUITE(socket_reuseaddr_test_suite, NULL, setup, NULL, NULL, NULL);


ZTEST_USER(socket_reuseport_test_suite, test_enable_disable)
{
	int server_sock = -1;

	int value = -1;
	socklen_t value_size = sizeof(int);

	server_sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(server_sock >= 0, "socket open failed");

	/* Read initial value */
	test_getsocketopt_reuseport(server_sock, (void *)&value, &value_size);
	zassert_equal(value_size, sizeof(int), "incorrect value size returned by getsocketopt()");
	zassert_equal(value, (int) false, "SO_REUSEPORT incorrectly set (expected false)");

	/* Enable option */
	value = 1;
	test_setsocketopt_reuseport(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseport(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEPORT not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = 2;
	test_setsocketopt_reuseport(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseport(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEPORT not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = 0x100;
	test_setsocketopt_reuseport(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseport(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEPORT not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = -1;
	test_setsocketopt_reuseport(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseport(server_sock, (void *)&value, &value_size);
	zassert_equal(value, (int) true, "SO_REUSEPORT not correctly set, returned %d", value);

	zsock_close(server_sock);
}


static void test_reuseport_unspecified_specified_common(sa_family_t family,
							char const *first_ip,
							char const *second_ip,
							bool should_succeed)
{
	int server_sock1 = -1;
	int server_sock2 = -1;

	struct sockaddr bind_addr1;
	struct sockaddr bind_addr2;

	/* Create the sockets */
	prepare_sock_tcp(family, first_ip, LOCAL_PORT, &server_sock1, &bind_addr1);
	prepare_sock_tcp(family, second_ip, LOCAL_PORT, &server_sock2, &bind_addr2);

	/* Depending on the expected result, we enable SO_REUSEPORT for the first socket */
	if (should_succeed) {
		test_enable_reuseport(server_sock1);
	}

	/* Bind the first socket */
	test_bind_success(server_sock1, &bind_addr1, sizeof(bind_addr1));

	/* Try to bind the second socket, should fail */
	test_bind_fail(server_sock2, &bind_addr2, sizeof(bind_addr2));

	/* Enable SO_REUSEPORT option for the second socket */
	test_enable_reuseport(server_sock2);

	/* Try to bind the second socket again */
	if (should_succeed) {
		test_bind_success(server_sock2, &bind_addr2, sizeof(bind_addr2));
	} else {
		test_bind_fail(server_sock2, &bind_addr2, sizeof(bind_addr2));
	}

	zsock_close(server_sock1);
	zsock_close(server_sock2);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_both_unspecified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_both_unspecified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_IPV6_ANY_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_both_unspecified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_both_unspecified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_IPV6_ANY_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_both_specified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_MY_IPV4_ADDR,
						    TEST_MY_IPV4_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_both_specified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_MY_IPV6_ADDR,
						    TEST_MY_IPV6_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_both_specified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_MY_IPV4_ADDR,
						    TEST_MY_IPV4_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_both_specified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_MY_IPV6_ADDR,
						    TEST_MY_IPV6_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_first_unspecified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_MY_IPV4_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_first_unspecified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_MY_IPV6_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_first_unspecified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_MY_IPV4_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_first_unspecified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_MY_IPV6_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_second_unspecified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_MY_IPV4_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_second_unspecified_bad)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_MY_IPV6_ADDR,
						    TEST_IPV6_ANY_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_second_unspecified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET,
						    TEST_MY_IPV4_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_second_unspecified_good)
{
	test_reuseport_unspecified_specified_common(AF_INET6,
						    TEST_MY_IPV6_ADDR,
						    TEST_IPV6_ANY_ADDR,
						    SHOULD_SUCCEED);
}


enum sockets_reuseport_enabled {
	NONE_SET = 0,
	FIRST_SET,
	SECOND_SET,
	BOTH_SET
};

static void test_reuseport_udp_server_client_common(sa_family_t family,
						    char const *ip,
						    enum sockets_reuseport_enabled setup)
{
	int server_sock = -1;
	int client_sock = -1;
	int accept_sock = 1;

	struct sockaddr server_addr;
	struct sockaddr client_addr;

	struct sockaddr accept_addr;
	socklen_t accept_addr_len = sizeof(accept_addr);

	char tx_buf = 0x55;
	char rx_buf;

	/* Create sockets */
	prepare_sock_udp(family, ip, LOCAL_PORT, &server_sock, &server_addr);
	prepare_sock_udp(family, ip, 0, &client_sock, &client_addr);

	/* Make sure we can bind to the address:port */
	if (setup == FIRST_SET || setup == BOTH_SET) {
		test_enable_reuseport(server_sock);
	}

	/* Bind server socket */
	test_bind_success(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr));

	/* Bind client socket (on a random port) */
	test_bind_success(client_sock, (struct sockaddr *) &client_addr, sizeof(client_addr));

	/* Send message from client to server */
	test_sendto(client_sock, &tx_buf, sizeof(tx_buf), 0, &server_addr, sizeof(server_addr));

	/* Give the packet a chance to go through the net stack */
	k_msleep(50);

	/* Receive data from the client */
	rx_buf = 0;
	test_recvfrom_success(server_sock, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT,
		       &accept_addr, &accept_addr_len);
	zassert_equal(rx_buf, tx_buf, "wrong data");

	/* Create a more specific socket to have a direct connection to the new client */
	accept_sock = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(accept_sock >= 0, "socket open failed");

	/* Make sure we can bind to the address:port */
	if (setup == SECOND_SET || setup == BOTH_SET) {
		test_enable_reuseport(accept_sock);
	}

	/* Try to bind new client socket */
	if (setup == BOTH_SET) {
		/* Should succeed */
		test_bind_success(accept_sock, (struct sockaddr *) &server_addr,
				  sizeof(server_addr));
	} else {
		/* Should fail */
		test_bind_fail(accept_sock, (struct sockaddr *) &server_addr,
			       sizeof(server_addr));
	}

	/* Connect the client to set remote address and remote port */
	test_connect_success(accept_sock, &accept_addr, sizeof(accept_addr));

	/* Send another message from client to server */
	test_sendto(client_sock, &tx_buf, sizeof(tx_buf), 0, &server_addr, sizeof(server_addr));

	/* Give the packet a chance to go through the net stack */
	k_msleep(50);

	/* Receive the data */
	if (setup == BOTH_SET) {
		/* We should receive data on the new specific socket, not on the general one */
		rx_buf = 0;
		test_recvfrom_fail(server_sock, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT,
			&accept_addr, &accept_addr_len);

		rx_buf = 0;
		test_recv_success(accept_sock, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT);
	} else {
		/* We should receive data on the general server socket */
		rx_buf = 0;
		test_recvfrom_success(server_sock, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT,
			&accept_addr, &accept_addr_len);

		rx_buf = 0;
		test_recv_fail(accept_sock, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT);
	}

	zsock_close(accept_sock);
	zsock_close(client_sock);
	zsock_close(server_sock);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_udp_bad_both_not_set)
{
	test_reuseport_udp_server_client_common(AF_INET,
						TEST_MY_IPV4_ADDR,
						NONE_SET);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_udp_bad_both_not_set)
{
	test_reuseport_udp_server_client_common(AF_INET6,
						TEST_MY_IPV6_ADDR,
						NONE_SET);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_udp_bad_first_not_set)
{
	test_reuseport_udp_server_client_common(AF_INET,
						TEST_MY_IPV4_ADDR,
						SECOND_SET);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_udp_bad_first_not_set)
{
	test_reuseport_udp_server_client_common(AF_INET6,
						TEST_MY_IPV6_ADDR,
						SECOND_SET);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_udp_bad_second_not_set)
{
	test_reuseport_udp_server_client_common(AF_INET,
						TEST_MY_IPV4_ADDR,
						FIRST_SET);
}


ZTEST_USER(socket_reuseport_test_suite, test_ipv6_udp_bad_second_not_set)
{
	test_reuseport_udp_server_client_common(AF_INET6,
						TEST_MY_IPV6_ADDR,
						FIRST_SET);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_udp_good)
{
	test_reuseport_udp_server_client_common(AF_INET,
						TEST_MY_IPV4_ADDR,
						BOTH_SET);
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv6_udp_good)
{
	test_reuseport_udp_server_client_common(AF_INET6,
						TEST_MY_IPV6_ADDR,
						BOTH_SET);
}


static void test_reuseport_tcp_identical_clients_common(sa_family_t family,
							char const *server_ip,
							char const *client_ip)
{
	int server_sock = -1;
	int client_sock1 = -1;
	int client_sock2 = -1;
	int accept_sock = 1;

	struct sockaddr server_addr;
	struct sockaddr client_addr;
	struct sockaddr connect_addr;

	struct sockaddr accept_addr;
	socklen_t accept_addr_len = sizeof(accept_addr);

	/* Create sockets */
	prepare_sock_tcp(family, server_ip, LOCAL_PORT, &server_sock, &server_addr);
	prepare_sock_tcp(family, client_ip, LOCAL_PORT + 1, &client_sock1, &client_addr);
	prepare_sock_tcp(family, client_ip, LOCAL_PORT, &client_sock2, &connect_addr);

	/* Enable SO_REUSEPORT option for the two sockets */
	test_enable_reuseport(client_sock1);
	test_enable_reuseport(client_sock2);

	/* Bind server socket */
	test_bind_success(server_sock, &server_addr, sizeof(server_addr));

	/* Start listening on the server socket */
	test_listen(server_sock);

	/* Bind the client sockets */
	test_bind_success(client_sock1, &client_addr, sizeof(client_addr));
	test_bind_success(client_sock2, &client_addr, sizeof(client_addr));

	/* Connect the first client */
	test_connect_success(client_sock1, &connect_addr, sizeof(connect_addr));

	/* Accept the first client */
	accept_sock = test_accept(server_sock, &accept_addr, &accept_addr_len);

	/* Connect the second client, should fail */
	test_connect_fail(client_sock2, (struct sockaddr *)&connect_addr, sizeof(connect_addr));

	zsock_close(accept_sock);
	zsock_close(client_sock1);
	zsock_close(client_sock2);
	zsock_close(server_sock);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

ZTEST_USER(socket_reuseport_test_suite, test_ipv4_tcp_identical_clients)
{
	test_reuseport_tcp_identical_clients_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_MY_IPV4_ADDR);
}


ZTEST_USER(socket_reuseport_test_suite, test_ipv6_tcp_identical_clients)
{
	test_reuseport_tcp_identical_clients_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_MY_IPV6_ADDR);
}

ZTEST_SUITE(socket_reuseport_test_suite, NULL, setup, NULL, NULL, NULL);
