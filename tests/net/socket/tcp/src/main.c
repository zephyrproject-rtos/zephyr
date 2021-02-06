/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <ztest_assert.h>
#include <fcntl.h>
#include <net/socket.h>

#include "../../socket_helpers.h"

#define TEST_STR_SMALL "test"

#define ANY_PORT 0
#define SERVER_PORT 4242

#define MAX_CONNS 5

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(1)
#define THREAD_SLEEP 50 /* ms */

static void test_bind(int sock, struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(bind(sock, addr, addrlen),
		      0,
		      "bind failed");
}

static void test_listen(int sock)
{
	zassert_equal(listen(sock, MAX_CONNS),
		      0,
		      "listen failed");
}

static void test_connect(int sock, struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(connect(sock, addr, addrlen),
		      0,
		      "connect failed");

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
		/* Let the connection proceed */
		k_msleep(THREAD_SLEEP);
	}
}

static void test_send(int sock, const void *buf, size_t len, int flags)
{
	zassert_equal(send(sock, buf, len, flags),
		      len,
		      "send failed");
}

static void test_sendto(int sock, const void *buf, size_t len, int flags,
			const struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(sendto(sock, buf, len, flags, addr, addrlen),
		      len,
		      "send failed");
}

static void test_accept(int sock, int *new_sock, struct sockaddr *addr,
			socklen_t *addrlen)
{
	zassert_not_null(new_sock, "null newsock");

	*new_sock = accept(sock, addr, addrlen);
	zassert_true(*new_sock >= 0, "accept failed");
}

static void test_accept_timeout(int sock, int *new_sock, struct sockaddr *addr,
				socklen_t *addrlen)
{
	zassert_not_null(new_sock, "null newsock");

	*new_sock = accept(sock, addr, addrlen);
	zassert_equal(*new_sock, -1, "accept succeed");
	zassert_equal(errno, EAGAIN, "");
}

static void test_fcntl(int sock, int cmd, int val)
{
	zassert_equal(fcntl(sock, cmd, val), 0, "fcntl failed");
}

static void test_recv(int sock, int flags)
{
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	recved = recv(sock, rx_buf, sizeof(rx_buf), flags);
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_recvfrom(int sock,
			  int flags,
			  struct sockaddr *addr,
			  socklen_t *addrlen)
{
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	recved = recvfrom(sock,
			  rx_buf,
			  sizeof(rx_buf),
			  flags,
			  addr,
			  addrlen);
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_close(int sock)
{
	zassert_equal(close(sock),
		      0,
		      "close failed");
}

/* Test that EOF handling works correctly. Should be called with socket
 * whose peer socket was closed.
 */
static void test_eof(int sock)
{
	char rx_buf[1];
	ssize_t recved;

	/* Test that EOF properly detected. */
	recved = recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");

	/* Calling again should be OK. */
	recved = recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");

	/* Calling when TCP connection is fully torn down should be still OK. */
	k_sleep(TCP_TEARDOWN_TIMEOUT);
	recved = recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");
}

void test_v4_send_recv(void)
{
	/* Test if send() and recv() work on a ipv4 stream socket. */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recv(new_sock, MSG_PEEK);
	test_recv(new_sock, 0);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_send_recv(void)
{
	/* Test if send() and recv() work on a ipv6 stream socket. */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recv(new_sock, MSG_PEEK);
	test_recv(new_sock, 0);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v4_sendto_recvfrom(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recvfrom(new_sock, MSG_PEEK, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recvfrom(new_sock, 0, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_sendto_recvfrom(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recvfrom(new_sock, MSG_PEEK, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recvfrom(new_sock, 0, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v4_sendto_recvfrom_null_dest(void)
{
	/* For a stream socket, sendto() should ignore NULL dest address */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recvfrom(new_sock, 0, NULL, NULL);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_sendto_recvfrom_null_dest(void)
{
	/* For a stream socket, sendto() should ignore NULL dest address */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recvfrom(new_sock, 0, NULL, NULL);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void _test_recv_enotconn(int c_sock, int s_sock)
{
	char rx_buf[1] = {0};
	int res;

	test_listen(s_sock);

	/* Check "client" socket, just created. */
	res = recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(res, -1, "recv() on not connected sock didn't fail");
	zassert_equal(errno, ENOTCONN, "recv() on not connected sock didn't "
				       "lead to ENOTCONN");

	/* Check "server" socket, bound and listen()ed . */
	res = recv(s_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(res, -1, "recv() on not connected sock didn't fail");
	zassert_equal(errno, ENOTCONN, "recv() on not connected sock didn't "
				       "lead to ENOTCONN");

	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v4_recv_enotconn(void)
{
	/* For a stream socket, recv() without connect() or accept()
	 * should lead to ENOTCONN.
	 */
	int c_sock, s_sock;
	struct sockaddr_in c_saddr, s_saddr;

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	_test_recv_enotconn(c_sock, s_sock);
}

void test_v6_recv_enotconn(void)
{
	/* For a stream socket, recv() without connect() or accept()
	 * should lead to ENOTCONN.
	 */
	int c_sock, s_sock;
	struct sockaddr_in6 c_saddr, s_saddr;

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	_test_recv_enotconn(c_sock, s_sock);
}

static void calc_net_context(struct net_context *context, void *user_data)
{
	int *count = user_data;

	(*count)++;
}

void test_open_close_immediately(void)
{
	/* Test if socket closing works if done immediately after
	 * receiving SYN.
	 */
	int count_before = 0, count_after = 0;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	int c_sock;
	int s_sock;

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	/* We should have two contexts open now */
	net_context_foreach(calc_net_context, &count_before);

	/* Try to connect to a port that is not accepting connections.
	 * The end result should be that we do not leak net_context.
	 */
	s_saddr.sin_port = htons(SERVER_PORT + 1);

	zassert_not_equal(connect(c_sock, (struct sockaddr *)&s_saddr,
				  sizeof(s_saddr)),
			  0, "connect succeed");
	test_close(c_sock);

	/* After the client socket closing, the context count should be 1 */
	net_context_foreach(calc_net_context, &count_after);

	test_close(s_sock);

	zassert_equal(count_before - 1, count_after,
		      "net_context still in use (before %d vs after %d)",
		      count_before - 1, count_after);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v4_accept_timeout(void)
{
	/* Test if accept() will timeout properly */
	int s_sock;
	int new_sock;
	uint32_t tstamp;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_fcntl(s_sock, F_SETFL, O_NONBLOCK);

	tstamp = k_uptime_get_32();
	test_accept_timeout(s_sock, &new_sock, &addr, &addrlen);
	zassert_true(k_uptime_get_32() - tstamp <= 100, "");

	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_so_type(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &sock1, &bind_addr4);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &sock2, &bind_addr6);

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

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &sock1, &bind_addr4);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &sock2, &bind_addr6);

	rv = getsockopt(sock1, SOL_SOCKET, SO_PROTOCOL, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_TCP, "getsockopt got invalid protocol");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = getsockopt(sock2, SOL_SOCKET, SO_PROTOCOL, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_TCP, "getsockopt got invalid protocol");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	test_close(sock1);
	test_close(sock2);
	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v4_so_rcvtimeo(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	int rv;
	uint32_t start_time, time_diff;
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	struct timeval optval = {
		.tv_sec = 2,
		.tv_usec = 500000,
	};

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	rv = setsockopt(c_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 0;
	rv = setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	start_time = k_uptime_get_32();
	recved = recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2500, "Expected timeout after 2500ms but "
			"was %dms", time_diff);

	start_time = k_uptime_get_32();
	recved = recv(new_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2000, "Expected timeout after 2000ms but "
			"was %dms", time_diff);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_so_rcvtimeo(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	int rv;
	uint32_t start_time, time_diff;
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	struct timeval optval = {
		.tv_sec = 2,
		.tv_usec = 500000,
	};

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	rv = setsockopt(c_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 0;
	rv = setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	start_time = k_uptime_get_32();
	recved = recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2500, "Expected timeout after 2500ms but "
			"was %dms", time_diff);

	start_time = k_uptime_get_32();
	recved = recv(new_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2000, "Expected timeout after 2000ms but "
			"was %dms", time_diff);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

#ifdef CONFIG_USERSPACE
#define CHILD_STACK_SZ		(2048 + CONFIG_TEST_EXTRA_STACKSIZE)
struct k_thread child_thread;
K_THREAD_STACK_DEFINE(child_stack, CHILD_STACK_SZ);
ZTEST_BMEM volatile int result;

static void child_entry(void *p1, void *p2, void *p3)
{
	int sock = POINTER_TO_INT(p1);

	result = close(sock);
}

static void spawn_child(int sock)
{
	k_thread_create(&child_thread, child_stack,
			K_THREAD_STACK_SIZEOF(child_stack), child_entry,
			INT_TO_POINTER(sock), NULL, NULL, 0, K_USER,
			K_FOREVER);
}
#endif

void test_socket_permission(void)
{
#ifdef CONFIG_USERSPACE
	int sock;
	struct sockaddr_in saddr;
	struct net_context *ctx;

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &sock, &saddr);

	ctx = zsock_get_context_object(sock);
	zassert_not_null(ctx, "zsock_get_context_object() failed");

	/* Spawn a child thread which doesn't inherit our permissions,
	 * it will try to perform a socket operation and fail due to lack
	 * of permissions on it.
	 */
	spawn_child(sock);
	k_thread_start(&child_thread);
	k_thread_join(&child_thread, K_FOREVER);

	zassert_not_equal(result, 0, "child succeeded with no permission");

	/* Now spawn the same child thread again, but this time we grant
	 * permission on the net_context before we start it, and the
	 * child should now succeed.
	 */
	spawn_child(sock);
	k_object_access_grant(ctx, &child_thread);
	k_thread_start(&child_thread);
	k_thread_join(&child_thread, K_FOREVER);

	zassert_equal(result, 0, "child failed with permissions");
#else
	ztest_test_skip();
#endif /* CONFIG_USERSPACE */
}

void test_main(void)
{
#ifdef CONFIG_USERSPACE
	/* ztest thread inherit permissions from main */
	k_thread_access_grant(k_current_get(), &child_thread, child_stack);
#endif

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(8));
	}

	ztest_test_suite(
		socket_tcp,
		ztest_user_unit_test(test_v4_send_recv),
		ztest_user_unit_test(test_v6_send_recv),
		ztest_user_unit_test(test_v4_sendto_recvfrom),
		ztest_user_unit_test(test_v6_sendto_recvfrom),
		ztest_user_unit_test(test_v4_sendto_recvfrom_null_dest),
		ztest_user_unit_test(test_v6_sendto_recvfrom_null_dest),
		ztest_user_unit_test(test_v4_recv_enotconn),
		ztest_user_unit_test(test_v6_recv_enotconn),
		ztest_unit_test(test_open_close_immediately),
		ztest_user_unit_test(test_v4_accept_timeout),
		ztest_unit_test(test_so_type),
		ztest_unit_test(test_so_protocol),
		ztest_unit_test(test_v4_so_rcvtimeo),
		ztest_unit_test(test_v6_so_rcvtimeo),
		ztest_user_unit_test(test_socket_permission)
		);

	ztest_run_test_suite(socket_tcp);
}
