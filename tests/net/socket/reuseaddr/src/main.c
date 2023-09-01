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

static inline void prepare_sock_tcp(sa_family_t family, const char *addr, uint16_t port,
				       int *sock, struct sockaddr *sockaddr)
{
	if (family == AF_INET) {
		prepare_sock_tcp_v4(addr,
				    port,
				    sock,
				    (struct sockaddr_in *) sockaddr);
	} else if (family == AF_INET6) {
		prepare_sock_tcp_v6(addr,
				    port,
				    sock,
				    (struct sockaddr_in6 *) sockaddr);
	}
}

static void test_getsocketopt_reuseaddr(int sock, void *optval, socklen_t *optlen)
{
	zassert_equal(getsockopt(sock, SOL_SOCKET, SO_REUSEADDR, optval, optlen),
		      0,
		      "getsocketopt() failed with error %d", errno);
}

static void test_setsocketopt_reuseaddr(int sock, void *optval, socklen_t optlen)
{
	zassert_equal(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, optval, optlen),
		      0,
		      "setsocketopt() failed with error %d", errno);
}

static void test_enable_reuseaddr(int sock)
{
	int value = 1;

	test_setsocketopt_reuseaddr(sock, &value, sizeof(value));
}

static void test_add_local_ip_address(sa_family_t family, const char *ip)
{
	if (family == AF_INET) {
		struct sockaddr_in addr;

		zsock_inet_pton(AF_INET, ip, &addr.sin_addr);

		zassert_not_null(net_if_ipv4_addr_add(net_if_get_default(),
						      &addr.sin_addr,
						      NET_ADDR_MANUAL,
						      0),
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

static void test_bind_success(int sock, const struct sockaddr * addr, socklen_t addrlen)
{
	zassert_equal(bind(sock, addr, addrlen),
		      0,
		      "bind() failed with error %d", errno);
}

static void test_bind_fail(int sock, const struct sockaddr * addr, socklen_t addrlen)
{
	zassert_equal(bind(sock, addr, addrlen),
		      -1,
		      "bind() succeeded incorrectly");

	zassert_equal(errno, EADDRINUSE, "bind() returned unexpected errno (%d)", errno);
}

static void test_listen(int sock)
{
	zassert_equal(listen(sock, 0),
		      0,
		      "listen() failed with error %d", errno);
}

static void test_connect(int sock, const struct sockaddr * addr, socklen_t addrlen)
{
	zassert_equal(connect(sock, addr, addrlen),
		      0,
		      "connect() failed with error %d", errno);

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
		/* Let the connection proceed */
		k_msleep(50);
	}
}

static int test_accept(int sock, struct sockaddr * addr, socklen_t * addrlen)
{
	int new_sock = accept(sock, addr, addrlen);

	zassert_not_equal(new_sock, -1, "accept() failed with error %d", errno);

	return new_sock;
}

static void calc_net_context(struct net_context *context, void *user_data)
{
	int *count = user_data;

	(*count)++;
}

/* Wait until the number of TCP contexts reaches a certain level
 *   exp_num_contexts : The number of contexts to wait for
 *   timeout :		The time to wait for
 */
int wait_for_n_tcp_contexts(int exp_num_contexts, k_timeout_t timeout)
{
	uint32_t start_time = k_uptime_get_32();
	uint32_t time_diff;
	int context_count = 0;

	/* After the client socket closing, the context count should be 1 less */
	net_context_foreach(calc_net_context, &context_count);

	time_diff = k_uptime_get_32() - start_time;

	/* Eventually the client socket should be cleaned up */
	while (context_count != exp_num_contexts) {
		context_count = 0;
		net_context_foreach(calc_net_context, &context_count);
		k_sleep(K_MSEC(50));
		time_diff = k_uptime_get_32() - start_time;

		if (K_MSEC(time_diff).ticks > timeout.ticks) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void test_context_cleanup(void)
{
	zassert_equal(wait_for_n_tcp_contexts(0, TCP_TEARDOWN_TIMEOUT),
		      0,
		      "Not all TCP contexts properly cleaned up");
}


ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_enable_disable)
{
	int server_sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(server_sock >= 0, "socket open failed");

	int value = -1;
	size_t value_size = sizeof(int);

	/* Read initial value */
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, (socklen_t *)&value_size);
	zassert_equal(value_size, sizeof(int), "incorrect value size returned by getsocketopt()");
	zassert_equal(value, (int) false, "SO_REUSEADDR incorrectly set (expected false)");

	/* Enable option */
	value = 1;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, (socklen_t *)&value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = 2;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, (socklen_t *)&value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = 0x100;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, (socklen_t *)&value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	/* Enable option (with other value as linux takes any int here) */
	value = -1;
	test_setsocketopt_reuseaddr(server_sock, (void *)&value, sizeof(value));
	test_getsocketopt_reuseaddr(server_sock, (void *)&value, (socklen_t *)&value_size);
	zassert_equal(value, (int) true, "SO_REUSEADDR not correctly set, returned %d", value);

	close(server_sock);

	test_context_cleanup();
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

	close(server_sock1);
	close(server_sock2);

	test_context_cleanup();
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv4_first_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_MY_IPV4_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv6_first_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET6,
						    TEST_IPV6_ANY_ADDR,
						    TEST_MY_IPV6_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv4_second_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET,
						    TEST_MY_IPV4_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv6_second_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET6,
						    TEST_MY_IPV6_ADDR,
						    TEST_IPV6_ANY_ADDR,
						    SHOULD_SUCCEED);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv4_both_unspecified)
{
	test_reuseaddr_unspecified_specified_common(AF_INET,
						    TEST_IPV4_ANY_ADDR,
						    TEST_IPV4_ANY_ADDR,
						    SHOULD_FAIL);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv6_both_unspecified)
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

	close(server_sock1);
	close(server_sock2);

	test_context_cleanup();
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv4_tcp_unspecified_listening)
{
	test_reuseaddr_tcp_listening_common(AF_INET,
					    TEST_IPV4_ANY_ADDR,
					    TEST_MY_IPV4_ADDR);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv6_tcp_unspecified_listening)
{
	test_reuseaddr_tcp_listening_common(AF_INET6,
					    TEST_IPV6_ANY_ADDR,
					    TEST_MY_IPV6_ADDR);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv4_tcp_specified_listening)
{
	test_reuseaddr_tcp_listening_common(AF_INET,
					    TEST_MY_IPV4_ADDR,
					    TEST_IPV4_ANY_ADDR);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv6_tcp_specified_listening)
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
	test_connect(client_sock, &conn_addr, sizeof(conn_addr));

	/* Accept the client */
	accept_sock = test_accept(server_sock, &accept_addr, &accept_addrlen);

	/* Close the server socket */
	close(server_sock);

	/* Close the accepted socket */
	close(accept_sock);

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

	close(client_sock);
	close(server_sock);

	test_context_cleanup();
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv4_tcp_time_wait_unspecified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET,
						TEST_IPV4_ANY_ADDR,
						TEST_MY_IPV4_ADDR);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv6_tcp_time_wait_unspecified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET6,
						TEST_IPV6_ANY_ADDR,
						TEST_MY_IPV6_ADDR);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv4_tcp_time_wait_specified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET,
						TEST_MY_IPV4_ADDR,
						TEST_MY_IPV4_ADDR);
}

ZTEST_USER(socket_reuseaddr_reuseport_test_suite, test_reuseaddr_ipv6_tcp_time_wait_specified)
{
	test_reuseaddr_tcp_tcp_time_wait_common(AF_INET6,
						TEST_MY_IPV6_ADDR,
						TEST_MY_IPV6_ADDR);
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


ZTEST_SUITE(socket_reuseaddr_reuseport_test_suite, NULL, setup, NULL, NULL, NULL);
