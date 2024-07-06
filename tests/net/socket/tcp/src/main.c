/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/ztest_assert.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/loopback.h>

#include "../../socket_helpers.h"

#define TEST_STR_SMALL "test"

#define TEST_STR_LONG \
	"The Zephyr Project, a Linux Foundation hosted Collaboration " \
	"Project, is an open source collaborative effort uniting leaders " \
	"from across the industry to build a best-in-breed small, scalable, " \
	"real-time operating system (RTOS) optimized for resource-" \
	"constrained devices, across multiple architectures."

#define MY_IPV4_ADDR "127.0.0.1"
#define MY_IPV6_ADDR "::1"

#define ANY_PORT 0
#define SERVER_PORT 4242

#define MAX_CONNS 5

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(3)
#define THREAD_SLEEP 50 /* ms */

static void test_bind(int sock, struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(zsock_bind(sock, addr, addrlen),
		      0,
		      "bind failed with error %d", errno);
}

static void test_listen(int sock)
{
	zassert_equal(zsock_listen(sock, MAX_CONNS),
		      0,
		      "listen failed with error %d", errno);
}

static void test_connect(int sock, struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(zsock_connect(sock, addr, addrlen),
		      0,
		      "connect failed with error %d", errno);

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
		/* Let the connection proceed */
		k_msleep(THREAD_SLEEP);
	}
}

static void test_send(int sock, const void *buf, size_t len, int flags)
{
	zassert_equal(zsock_send(sock, buf, len, flags),
		      len,
		      "send failed");
}

static void test_sendto(int sock, const void *buf, size_t len, int flags,
			const struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(zsock_sendto(sock, buf, len, flags, addr, addrlen),
		      len,
		      "send failed");
}

static void test_accept(int sock, int *new_sock, struct sockaddr *addr,
			socklen_t *addrlen)
{
	zassert_not_null(new_sock, "null newsock");

	*new_sock = zsock_accept(sock, addr, addrlen);
	zassert_true(*new_sock >= 0, "accept failed");
}

static void test_accept_timeout(int sock, int *new_sock, struct sockaddr *addr,
				socklen_t *addrlen)
{
	zassert_not_null(new_sock, "null newsock");

	*new_sock = zsock_accept(sock, addr, addrlen);
	zassert_equal(*new_sock, -1, "accept succeed");
	zassert_equal(errno, EAGAIN, "");
}

static void test_fcntl(int sock, int cmd, int val)
{
	zassert_equal(zsock_fcntl(sock, cmd, val), 0, "fcntl failed");
}

static void test_recv(int sock, int flags)
{
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	recved = zsock_recv(sock, rx_buf, sizeof(rx_buf), flags);
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

	recved = zsock_recvfrom(sock, rx_buf, sizeof(rx_buf), flags, addr, addrlen);
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_recvmsg(int sock,
			 struct msghdr *msg,
			 int flags,
			 size_t expected,
			 int line)
{
	ssize_t recved;

	recved = zsock_recvmsg(sock, msg, flags);

	zassert_equal(recved, expected,
		      "line %d, unexpected received bytes (%d vs %d)",
		      line, recved, expected);
}

static void test_shutdown(int sock, int how)
{
	zassert_equal(zsock_shutdown(sock, how),
		      0,
		      "shutdown failed");
}

static void test_close(int sock)
{
	zassert_equal(zsock_close(sock),
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
	recved = zsock_recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");

	/* Calling again should be OK. */
	recved = zsock_recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");

	/* Calling when TCP connection is fully torn down should be still OK. */
	k_sleep(TCP_TEARDOWN_TIMEOUT);
	recved = zsock_recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");
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


ZTEST_USER(net_socket_tcp, test_v4_send_recv)
{
	/* Test if send() and recv() work on a ipv4 stream socket. */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recv(new_sock, ZSOCK_MSG_PEEK);
	test_recv(new_sock, 0);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST_USER(net_socket_tcp, test_v6_send_recv)
{
	/* Test if send() and recv() work on a ipv6 stream socket. */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recv(new_sock, ZSOCK_MSG_PEEK);
	test_recv(new_sock, 0);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

/* Test the stack behavior with a resonable sized block data, be sure to have multiple packets */
#define TEST_LARGE_TRANSFER_SIZE 60000
#define TEST_PRIME 811

#define TCP_SERVER_STACK_SIZE 2048

K_THREAD_STACK_DEFINE(tcp_server_stack_area, TCP_SERVER_STACK_SIZE);
struct k_thread tcp_server_thread_data;

/* A thread that receives, while the other part transmits */
void tcp_server_block_thread(void *vps_sock, void *unused2, void *unused3)
{
	int new_sock;
	struct sockaddr addr;
	int *ps_sock = (int *)vps_sock;
	socklen_t addrlen = sizeof(addr);

	test_accept(*ps_sock, &new_sock, &addr, &addrlen);
	zassert_true(addrlen == sizeof(struct sockaddr_in)
		|| addrlen == sizeof(struct sockaddr_in6), "wrong addrlen");

	/* Check the received data */
	ssize_t recved = 0;
	ssize_t total_received = 0;
	int iteration = 0;
	uint8_t buffer[256];

	while (total_received < TEST_LARGE_TRANSFER_SIZE) {
		/* Compute the remaining contents */
		size_t chunk_size = sizeof(buffer);
		size_t remain = TEST_LARGE_TRANSFER_SIZE - total_received;

		if (chunk_size > remain) {
			chunk_size = remain;
		}

		recved = zsock_recv(new_sock, buffer, chunk_size, 0);

		zassert(recved > 0, "received bigger then 0",
			"Error receiving bytes %i bytes, got %i on top of %i in iteration %i, errno %i",
			chunk_size,	recved, total_received, iteration, errno);

		/* Validate the contents */
		for (int i = 0; i < recved; i++) {
			int total_idx = i + total_received;
			uint8_t expValue = (total_idx * TEST_PRIME) & 0xff;

			zassert_equal(buffer[i], expValue, "Unexpected data at %i", total_idx);
		}

		total_received += recved;
		iteration++;
	}

	test_close(new_sock);
}

void test_send_recv_large_common(int tcp_nodelay, int family)
{
	int rv;
	int c_sock;
	int s_sock;
	struct sockaddr *c_saddr = NULL;
	struct sockaddr *s_saddr = NULL;
	size_t addrlen = 0;

	struct sockaddr_in c_saddr_in;
	struct sockaddr_in s_saddr_in;
	struct sockaddr_in6 c_saddr_in6;
	struct sockaddr_in6 s_saddr_in6;

	if (family == AF_INET) {
		prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr_in);
		c_saddr = (struct sockaddr *) &c_saddr_in;
		prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr_in);
		s_saddr = (struct sockaddr *) &s_saddr_in;
		addrlen = sizeof(s_saddr_in);
	} else if (family == AF_INET6) {
		prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr_in6);
		c_saddr = (struct sockaddr *) &c_saddr_in6;
		prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr_in6);
		s_saddr = (struct sockaddr *) &s_saddr_in6;
		addrlen = sizeof(s_saddr_in6);
	} else {
		zassert_unreachable();
	}

	test_bind(s_sock, s_saddr, addrlen);
	test_listen(s_sock);

	(void)k_thread_create(&tcp_server_thread_data, tcp_server_stack_area,
		K_THREAD_STACK_SIZEOF(tcp_server_stack_area),
		tcp_server_block_thread,
		&s_sock, NULL, NULL,
		k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);

	test_connect(c_sock, s_saddr, addrlen);

	rv = zsock_setsockopt(c_sock, IPPROTO_TCP, TCP_NODELAY, (char *) &tcp_nodelay, sizeof(int));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);

	/* send piece by piece */
	ssize_t total_send = 0;
	int iteration = 0;
	uint8_t buffer[256];

	while (total_send < TEST_LARGE_TRANSFER_SIZE) {
		/* Fill the buffer with a known pattern */
		for (int i = 0; i < sizeof(buffer); i++) {
			int total_idx = i + total_send;

			buffer[i] = (total_idx * TEST_PRIME) & 0xff;
		}

		size_t chunk_size = sizeof(buffer);
		size_t remain = TEST_LARGE_TRANSFER_SIZE - total_send;

		if (chunk_size > remain) {
			chunk_size = remain;
		}

		int send_bytes = zsock_send(c_sock, buffer, chunk_size, 0);

		zassert(send_bytes > 0, "send_bytes bigger then 0",
			"Error sending %i bytes on top of %i, got %i in iteration %i, errno %i",
			chunk_size, total_send, send_bytes, iteration, errno);
		total_send += send_bytes;
		iteration++;
	}

	/* join the thread, to wait for the receiving part */
	zassert_equal(k_thread_join(&tcp_server_thread_data, K_SECONDS(60)), 0,
			"Not successfully wait for TCP thread to finish");

	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

/* Control the packet drop ratio at the loopback adapter 8 */
static void set_packet_loss_ratio(void)
{
	/* drop one every 8 packets */
	zassert_equal(loopback_set_packet_drop_ratio(0.125f), 0,
		"Error setting packet drop rate");
}

static void restore_packet_loss_ratio(void)
{
	/* no packet dropping any more */
	zassert_equal(loopback_set_packet_drop_ratio(0.0f), 0,
		"Error setting packet drop rate");
}

ZTEST(net_socket_tcp, test_v4_send_recv_large_normal)
{
	test_send_recv_large_common(0, AF_INET);
}

ZTEST(net_socket_tcp, test_v4_send_recv_large_packet_loss)
{
	set_packet_loss_ratio();
	test_send_recv_large_common(0, AF_INET);
	restore_packet_loss_ratio();
}

ZTEST(net_socket_tcp, test_v4_send_recv_large_no_delay)
{
	set_packet_loss_ratio();
	test_send_recv_large_common(1, AF_INET);
	restore_packet_loss_ratio();
}

ZTEST(net_socket_tcp, test_v6_send_recv_large_normal)
{
	test_send_recv_large_common(0, AF_INET6);
}

ZTEST(net_socket_tcp, test_v6_send_recv_large_packet_loss)
{
	set_packet_loss_ratio();
	test_send_recv_large_common(0, AF_INET6);
	restore_packet_loss_ratio();
}

ZTEST(net_socket_tcp, test_v6_send_recv_large_no_delay)
{
	set_packet_loss_ratio();
	test_send_recv_large_common(1, AF_INET6);
	restore_packet_loss_ratio();
}

ZTEST(net_socket_tcp, test_v4_broken_link)
{
	/* Test if the data stops transmitting after the send returned with a timeout. */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	struct timeval optval = {
		.tv_sec = 0,
		.tv_usec = 500000,
	};

	struct net_stats before;
	struct net_stats after;
	uint32_t start_time, time_diff;
	ssize_t recved;
	int rv;
	uint8_t rx_buf[10];

	restore_packet_loss_ratio();

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	rv = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	test_recv(new_sock, ZSOCK_MSG_PEEK);
	test_recv(new_sock, 0);

	/* At this point break the interface */
	zassert_equal(loopback_set_packet_drop_ratio(1.0f), 0,
		"Error setting packet drop rate");

	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	/* At this point break the interface */
	start_time = k_uptime_get_32();

	/* Test the loopback packet loss: message should never arrive */
	recved = zsock_recv(new_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 500, "Expected timeout after 500ms but "
			"was %dms", time_diff);

	/* Reading from client should indicate the socket has been closed */
	recved = zsock_recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, ETIMEDOUT, "Unexpected errno value: %d", errno);

	/* At this point there should be no traffic any more, get the current counters */
	net_mgmt(NET_REQUEST_STATS_GET_ALL, NULL, &before, sizeof(before));

	k_sleep(K_MSEC(CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT));
	k_sleep(K_MSEC(CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT));

	net_mgmt(NET_REQUEST_STATS_GET_ALL, NULL, &after, sizeof(after));

	zassert_equal(before.ipv4.sent, after.ipv4.sent, "Data sent afer connection timeout");

	test_close(c_sock);
	test_close(new_sock);
	test_close(s_sock);

	restore_packet_loss_ratio();

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST_USER(net_socket_tcp, test_v4_sendto_recvfrom)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recvfrom(new_sock, ZSOCK_MSG_PEEK, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recvfrom(new_sock, 0, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST_USER(net_socket_tcp, test_v6_sendto_recvfrom)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);

	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recvfrom(new_sock, ZSOCK_MSG_PEEK, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recvfrom(new_sock, 0, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST_USER(net_socket_tcp, test_v4_sendto_recvfrom_null_dest)
{
	/* For a stream socket, sendto() should ignore NULL dest address */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

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

ZTEST_USER(net_socket_tcp, test_v6_sendto_recvfrom_null_dest)
{
	/* For a stream socket, sendto() should ignore NULL dest address */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

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

ZTEST_USER(net_socket_tcp, test_v4_sendto_recvmsg)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
#define MAX_BUF_LEN 64
#define SMALL_BUF_LEN (sizeof(TEST_STR_SMALL) - 1 - 2)
	char buf[MAX_BUF_LEN];
	char buf2[SMALL_BUF_LEN];
	char buf3[MAX_BUF_LEN];
	struct iovec io_vector[3];
	struct msghdr msg;
	int i, len;

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	/* Read data first in one chunk */
	io_vector[0].iov_base = buf;
	io_vector[0].iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &addr;
	msg.msg_namelen = addrlen;

	test_recvmsg(new_sock, &msg, ZSOCK_MSG_PEEK, strlen(TEST_STR_SMALL),
		     __LINE__);
	zassert_mem_equal(buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL),
			  "wrong data (%s)", buf);

	/* Then in two chunks */
	io_vector[0].iov_base = buf2;
	io_vector[0].iov_len = sizeof(buf2);
	io_vector[1].iov_base = buf;
	io_vector[1].iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 2;
	msg.msg_name = &addr;
	msg.msg_namelen = addrlen;

	test_recvmsg(new_sock, &msg, 0, strlen(TEST_STR_SMALL), __LINE__);
	zassert_mem_equal(msg.msg_iov[0].iov_base, TEST_STR_SMALL, msg.msg_iov[0].iov_len,
			  "wrong data in %s", "iov[0]");
	zassert_mem_equal(msg.msg_iov[1].iov_base, &TEST_STR_SMALL[msg.msg_iov[0].iov_len],
			  msg.msg_iov[1].iov_len,
			  "wrong data in %s", "iov[1]");

	/* Send larger test buffer */
	test_sendto(c_sock, TEST_STR_LONG, strlen(TEST_STR_LONG), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	/* Verify that the data is truncated */
	io_vector[0].iov_base = buf;
	io_vector[0].iov_len = sizeof(buf);
	io_vector[1].iov_base = buf2;
	io_vector[1].iov_len = sizeof(buf2);
	io_vector[2].iov_base = buf3;
	io_vector[2].iov_len = sizeof(buf3);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 3;
	msg.msg_name = &addr;
	msg.msg_namelen = addrlen;

	for (i = 0, len = 0; i < msg.msg_iovlen; i++) {
		len += msg.msg_iov[i].iov_len;
	}

	test_recvmsg(new_sock, &msg, 0, len, __LINE__);
	zassert_mem_equal(msg.msg_iov[0].iov_base, TEST_STR_LONG, msg.msg_iov[0].iov_len,
			  "wrong data in %s", "iov[0]");
	zassert_mem_equal(msg.msg_iov[1].iov_base, &TEST_STR_LONG[msg.msg_iov[0].iov_len],
			  msg.msg_iov[1].iov_len,
			  "wrong data in %s", "iov[1]");
	zassert_mem_equal(msg.msg_iov[2].iov_base,
			  &TEST_STR_LONG[msg.msg_iov[0].iov_len + msg.msg_iov[1].iov_len],
			  msg.msg_iov[2].iov_len,
			  "wrong data in %s", "iov[2]");

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
	res = zsock_recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(res, -1, "recv() on not connected sock didn't fail");
	zassert_equal(errno, ENOTCONN, "recv() on not connected sock didn't "
				       "lead to ENOTCONN");

	/* Check "server" socket, bound and listen()ed . */
	res = zsock_recv(s_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(res, -1, "recv() on not connected sock didn't fail");
	zassert_equal(errno, ENOTCONN, "recv() on not connected sock didn't "
				       "lead to ENOTCONN");

	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST_USER(net_socket_tcp, test_v4_recv_enotconn)
{
	/* For a stream socket, recv() without connect() or accept()
	 * should lead to ENOTCONN.
	 */
	int c_sock, s_sock;
	struct sockaddr_in c_saddr, s_saddr;

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	_test_recv_enotconn(c_sock, s_sock);
}

ZTEST_USER(net_socket_tcp, test_v6_recv_enotconn)
{
	/* For a stream socket, recv() without connect() or accept()
	 * should lead to ENOTCONN.
	 */
	int c_sock, s_sock;
	struct sockaddr_in6 c_saddr, s_saddr;

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	_test_recv_enotconn(c_sock, s_sock);
}

ZTEST_USER(net_socket_tcp, test_shutdown_rd_synchronous)
{
	/* recv() after shutdown(..., ZSOCK_SHUT_RD) should return 0 (EOF).
	 */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr, s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	/* Connect and accept that connection */
	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);

	/* Shutdown reception */
	test_shutdown(c_sock, ZSOCK_SHUT_RD);

	/* EOF should be notified by recv() */
	test_eof(c_sock);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

struct shutdown_data {
	struct k_work_delayable work;
	int fd;
	int how;
};

static void shutdown_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct shutdown_data *data = CONTAINER_OF(dwork, struct shutdown_data,
						  work);

	zsock_shutdown(data->fd, data->how);
}

ZTEST(net_socket_tcp, test_shutdown_rd_while_recv)
{
	/* Blocking recv() should return EOF after shutdown(..., ZSOCK_SHUT_RD) is
	 * called from another thread.
	 */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr, s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	struct shutdown_data shutdown_work_data;

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	/* Connect and accept that connection */
	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);

	/* Schedule reception shutdown from workqueue */
	k_work_init_delayable(&shutdown_work_data.work, shutdown_work);
	shutdown_work_data.fd = c_sock;
	shutdown_work_data.how = ZSOCK_SHUT_RD;
	k_work_schedule(&shutdown_work_data.work, K_MSEC(10));

	/* Start blocking recv(), which should be unblocked by shutdown() from
	 * another thread and return EOF (0).
	 */
	test_eof(c_sock);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_open_close_immediately)
{
	/* Test if socket closing works if done immediately after
	 * receiving SYN.
	 */
	int count_before = 0, count_after = 0;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	int c_sock;
	int s_sock;

	test_context_cleanup();

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	/* We should have two contexts open now */
	net_context_foreach(calc_net_context, &count_before);

	/* Try to connect to a port that is not accepting connections.
	 * The end result should be that we do not leak net_context.
	 */
	s_saddr.sin_port = htons(SERVER_PORT + 1);

	zassert_not_equal(zsock_connect(c_sock, (struct sockaddr *)&s_saddr,
					sizeof(s_saddr)),
			  0, "connect succeed");

	test_close(c_sock);

	/* Allow for the close communication to finish,
	 * this makes the test success, no longer scheduling dependent
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT / 2));

	/* After the client socket closing, the context count should be 1 */
	net_context_foreach(calc_net_context, &count_after);

	test_close(s_sock);

	/* Although closing a server socket does not require communication,
	 * wait a little to make the test robust to scheduling order
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT / 2));

	zassert_equal(count_before - 1, count_after,
		      "net_context still in use (before %d vs after %d)",
		      count_before - 1, count_after);

	/* No need to wait here, as the test success depends on the socket being closed */
	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_connect_timeout)
{
	/* Test if socket connect fails when there is not communication
	 * possible.
	 */
	int count_after = 0;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	int c_sock;
	int rv;

	restore_packet_loss_ratio();

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);

	s_saddr.sin_family = AF_INET;
	s_saddr.sin_port = htons(SERVER_PORT);
	rv = zsock_inet_pton(AF_INET, MY_IPV4_ADDR, &s_saddr.sin_addr);
	zassert_equal(rv, 1, "inet_pton failed");

	loopback_set_packet_drop_ratio(1.0f);

	zassert_equal(zsock_connect(c_sock, (struct sockaddr *)&s_saddr,
				    sizeof(s_saddr)),
		      -1, "connect succeed");

	zassert_equal(errno, ETIMEDOUT,
			    "connect should be timed out, got %i", errno);

	test_close(c_sock);

	/* Sleep here in order to allow other part of the system to run and
	 * update itself.
	 */
	k_sleep(K_MSEC(10));

	/* After the client socket closing, the context count should be 0 */
	net_context_foreach(calc_net_context, &count_after);

	zassert_equal(count_after, 0,
		      "net_context %d still in use", count_after);

	restore_packet_loss_ratio();
}

#define ASYNC_POLL_TIMEOUT 2000
#define POLL_FDS_NUM 1


ZTEST(net_socket_tcp, test_async_connect_timeout)
{
	/* Test if asynchronous socket connect fails when there is no communication
	 * possible.
	 */
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	int c_sock;
	int rv;
	struct zsock_pollfd poll_fds[POLL_FDS_NUM];

	loopback_set_packet_drop_ratio(1.0f);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	test_fcntl(c_sock, ZVFS_F_SETFL, ZVFS_O_NONBLOCK);
	s_saddr.sin_family = AF_INET;
	s_saddr.sin_port = htons(SERVER_PORT);
	rv = zsock_inet_pton(AF_INET, MY_IPV4_ADDR, &s_saddr.sin_addr);
	zassert_equal(rv, 1, "inet_pton failed");

	rv = zsock_connect(c_sock, (struct sockaddr *)&s_saddr,
			   sizeof(s_saddr));
	zassert_equal(rv, -1, "connect should not succeed");
	zassert_equal(errno, EINPROGRESS,
		      "connect should be in progress, got %i", errno);

	poll_fds[0].fd = c_sock;
	poll_fds[0].events = ZSOCK_POLLOUT;
	int poll_rc = zsock_poll(poll_fds, POLL_FDS_NUM, ASYNC_POLL_TIMEOUT);

	zassert_equal(poll_rc, 1, "poll should return 1, got %i", poll_rc);
	zassert_equal(poll_fds[0].revents, ZSOCK_POLLERR,
		      "poll should set error event");

	test_close(c_sock);

	test_context_cleanup();

	restore_packet_loss_ratio();
}

ZTEST(net_socket_tcp, test_async_connect)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	struct zsock_pollfd poll_fds[1];
	int poll_rc;

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);
	test_fcntl(c_sock, ZVFS_F_SETFL, ZVFS_O_NONBLOCK);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	zassert_equal(zsock_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr)),
		      -1,
		      "connect shouldn't complete right away");

	zassert_equal(errno, EINPROGRESS,
		      "connect should be in progress, got %i", errno);

	poll_fds[0].fd = c_sock;
	poll_fds[0].events = ZSOCK_POLLOUT;
	poll_rc = zsock_poll(poll_fds, 1, ASYNC_POLL_TIMEOUT);
	zassert_equal(poll_rc, 1, "poll should return 1, got %i", poll_rc);
	zassert_equal(poll_fds[0].revents, ZSOCK_POLLOUT,
		      "poll should set POLLOUT");

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "Wrong addrlen");

	test_close(c_sock);
	test_close(s_sock);
	test_close(new_sock);

	test_context_cleanup();
}

#define TCP_CLOSE_FAILURE_TIMEOUT 90000

ZTEST(net_socket_tcp, test_z_close_obstructed)
{
	/* Test if socket closing even when there is not communication
	 * possible any more
	 */
	int count_before = 0, count_after = 0;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	int c_sock;
	int s_sock;
	int new_sock;
	int dropped_packets_before = 0;
	int dropped_packets_after = 0;

	restore_packet_loss_ratio();

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	zassert_equal(zsock_connect(c_sock, (struct sockaddr *)&s_saddr,
				    sizeof(s_saddr)),
		      0, "connect not succeed");
	test_accept(s_sock, &new_sock, &addr, &addrlen);

	/* We should have two contexts open now */
	net_context_foreach(calc_net_context, &count_before);

	/* Break the communication */
	loopback_set_packet_drop_ratio(1.0f);

	dropped_packets_before = loopback_get_num_dropped_packets();

	test_close(c_sock);

	wait_for_n_tcp_contexts(count_before-1, K_MSEC(TCP_CLOSE_FAILURE_TIMEOUT));

	net_context_foreach(calc_net_context, &count_after);

	zassert_equal(count_before - 1, count_after,
		      "net_context still in use (before %d vs after %d)",
		      count_before - 1, count_after);

	dropped_packets_after = loopback_get_num_dropped_packets();
	int dropped_packets = dropped_packets_after - dropped_packets_before;

	/* At least some packet should have been dropped */
	zassert_equal(dropped_packets,
			CONFIG_NET_TCP_RETRY_COUNT + 1,
			"Incorrect number of FIN retries, got %i, expected %i",
			dropped_packets, CONFIG_NET_TCP_RETRY_COUNT+1);

	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();

	/* After everything is closed, we expect no more dropped packets */
	dropped_packets_before = loopback_get_num_dropped_packets();
	k_sleep(K_SECONDS(2));
	dropped_packets_after = loopback_get_num_dropped_packets();

	zassert_equal(dropped_packets_before, dropped_packets_after,
		      "packets after close");

	restore_packet_loss_ratio();
}

ZTEST_USER(net_socket_tcp, test_v4_accept_timeout)
{
	/* Test if accept() will timeout properly */
	int s_sock;
	int new_sock;
	uint32_t tstamp;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_fcntl(s_sock, ZVFS_F_SETFL, ZVFS_O_NONBLOCK);

	tstamp = k_uptime_get_32();
	test_accept_timeout(s_sock, &new_sock, &addr, &addrlen);
	zassert_true(k_uptime_get_32() - tstamp <= 100, "");

	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

ZTEST(net_socket_tcp, test_so_type)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optlen = sizeof(optval);

	zassert_equal(wait_for_n_tcp_contexts(0, TCP_TEARDOWN_TIMEOUT),
		      0,
		      "Not all TCP contexts properly cleaned up");

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &sock1, &bind_addr4);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &sock2, &bind_addr6);

	rv = zsock_getsockopt(sock1, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_STREAM, "getsockopt got invalid type");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_getsockopt(sock2, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_STREAM, "getsockopt got invalid type");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	test_close(sock1);
	test_close(sock2);

	zassert_equal(wait_for_n_tcp_contexts(0, TCP_TEARDOWN_TIMEOUT),
		      0,
		      "Not all TCP contexts properly cleaned up");
}

ZTEST(net_socket_tcp, test_so_protocol)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &sock1, &bind_addr4);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &sock2, &bind_addr6);

	rv = zsock_getsockopt(sock1, SOL_SOCKET, SO_PROTOCOL, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_TCP, "getsockopt got invalid protocol");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_getsockopt(sock2, SOL_SOCKET, SO_PROTOCOL, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_TCP, "getsockopt got invalid protocol");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	test_close(sock1);
	test_close(sock2);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_so_rcvbuf)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int  retval;
	int optval = UINT16_MAX;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &sock1, &bind_addr4);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &sock2, &bind_addr6);

	rv = zsock_setsockopt(sock1, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = zsock_getsockopt(sock1, SOL_SOCKET, SO_RCVBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = zsock_getsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	optval = -1;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	optval = UINT16_MAX + 1;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	test_close(sock1);
	test_close(sock2);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_so_rcvbuf_win_size)
{
	int rv;
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	char tx_buf[] = TEST_STR_SMALL;
	int buf_optval = sizeof(TEST_STR_SMALL);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	/* Lower server-side RX window size. */
	rv = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVBUF, &buf_optval,
			      sizeof(buf_optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	rv = zsock_send(c_sock, tx_buf, sizeof(tx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, sizeof(tx_buf), "Unexpected return code %d", rv);

	/* Window should've dropped to 0, so the ACK will be delayed - wait for
	 * it to arrive, so that the client is aware of the new window size.
	 */
	k_msleep(150);

	/* Client should not be able to send now (RX window full). */
	rv = zsock_send(c_sock, tx_buf, 1, ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, -1, "Unexpected return code %d", rv);
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);

	test_close(c_sock);
	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_so_sndbuf)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int retval;
	int optval = UINT16_MAX;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &sock1, &bind_addr4);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &sock2, &bind_addr6);

	rv = zsock_setsockopt(sock1, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = zsock_getsockopt(sock1, SOL_SOCKET, SO_SNDBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = zsock_getsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	optval = -1;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	optval = UINT16_MAX + 1;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	test_close(sock1);
	test_close(sock2);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_so_sndbuf_win_size)
{
	int rv;
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	char tx_buf[] = TEST_STR_SMALL;
	int buf_optval = sizeof(TEST_STR_SMALL);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	/* Lower client-side TX window size. */
	rv = zsock_setsockopt(c_sock, SOL_SOCKET, SO_SNDBUF, &buf_optval,
			      sizeof(buf_optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	/* Make sure the ACK from the server does not arrive. */
	loopback_set_packet_drop_ratio(1.0f);

	rv = zsock_send(c_sock, tx_buf, sizeof(tx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, sizeof(tx_buf), "Unexpected return code %d", rv);

	/* Client should not be able to send now (TX window full). */
	rv = zsock_send(c_sock, tx_buf, 1, ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, -1, "Unexpected return code %d", rv);
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);

	restore_packet_loss_ratio();

	test_close(c_sock);
	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_v4_so_rcvtimeo)
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

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	rv = zsock_setsockopt(c_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 0;
	rv = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	start_time = k_uptime_get_32();
	recved = zsock_recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2500, "Expected timeout after 2500ms but "
			"was %dms", time_diff);

	start_time = k_uptime_get_32();
	recved = zsock_recv(new_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2000, "Expected timeout after 2000ms but "
			"was %dms", time_diff);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_v6_so_rcvtimeo)
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

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	rv = zsock_setsockopt(c_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 0;
	rv = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	start_time = k_uptime_get_32();
	recved = zsock_recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2500, "Expected timeout after 2500ms but "
			"was %dms", time_diff);

	start_time = k_uptime_get_32();
	recved = zsock_recv(new_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2000, "Expected timeout after 2000ms but "
			"was %dms", time_diff);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_v4_so_sndtimeo)
{
	int rv;
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	uint32_t start_time, time_diff;
	char tx_buf[] = TEST_STR_SMALL;
	int buf_optval = sizeof(TEST_STR_SMALL);
	struct timeval timeo_optval = {
		.tv_sec = 0,
		.tv_usec = 200000,
	};

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	rv = zsock_setsockopt(c_sock, SOL_SOCKET, SO_SNDTIMEO, &timeo_optval,
			      sizeof(timeo_optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	/* Simulate window full scenario with SO_RCVBUF option. */
	rv = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVBUF, &buf_optval,
			      sizeof(buf_optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	rv = zsock_send(c_sock, tx_buf, sizeof(tx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, sizeof(tx_buf), "Unexpected return code %d", rv);

	/* Wait for ACK (empty window). */
	k_msleep(150);

	/* Client should not be able to send now and time out after SO_SNDTIMEO */
	start_time = k_uptime_get_32();
	rv = zsock_send(c_sock, tx_buf, 1, 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(rv, -1, "Unexpected return code %d", rv);
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 200, "Expected timeout after 200ms but "
			"was %dms", time_diff);

	test_close(c_sock);
	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_v6_so_sndtimeo)
{
	int rv;
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	uint32_t start_time, time_diff;
	char tx_buf[] = TEST_STR_SMALL;
	int buf_optval = sizeof(TEST_STR_SMALL);
	struct timeval timeo_optval = {
		.tv_sec = 0,
		.tv_usec = 500000,
	};

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	rv = zsock_setsockopt(c_sock, SOL_SOCKET, SO_SNDTIMEO, &timeo_optval,
			      sizeof(timeo_optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	/* Simulate window full scenario with SO_RCVBUF option. */
	rv = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVBUF, &buf_optval,
			      sizeof(buf_optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	rv = zsock_send(c_sock, tx_buf, sizeof(tx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, sizeof(tx_buf), "Unexpected return code %d", rv);

	/* Wait for ACK (empty window). */
	k_msleep(150);

	/* Client should not be able to send now and time out after SO_SNDTIMEO */
	start_time = k_uptime_get_32();
	rv = zsock_send(c_sock, tx_buf, 1, 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(rv, -1, "Unexpected return code %d", rv);
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 500, "Expected timeout after 500ms but "
			"was %dms", time_diff);

	test_close(c_sock);
	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();
}

struct test_msg_waitall_data {
	struct k_work_delayable tx_work;
	int sock;
	const uint8_t *data;
	size_t offset;
	int retries;
};

static void test_msg_waitall_tx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct test_msg_waitall_data *test_data =
		CONTAINER_OF(dwork, struct test_msg_waitall_data, tx_work);

	if (test_data->retries > 0) {
		test_send(test_data->sock, test_data->data + test_data->offset, 1, 0);
		test_data->offset++;
		test_data->retries--;
		k_work_reschedule(&test_data->tx_work, K_MSEC(10));
	}
}

ZTEST(net_socket_tcp, test_v4_msg_waitall)
{
	struct test_msg_waitall_data test_data = {
		.data = TEST_STR_SMALL,
	};
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	int ret;
	uint8_t rx_buf[sizeof(TEST_STR_SMALL) - 1] = { 0 };
	struct timeval timeo_optval = {
		.tv_sec = 0,
		.tv_usec = 100000,
	};

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "Wrong addrlen");


	/* Regular MSG_WAITALL - make sure recv returns only after
	 * requested amount is received.
	 */
	test_data.offset = 0;
	test_data.retries = sizeof(rx_buf);
	test_data.sock = c_sock;
	k_work_init_delayable(&test_data.tx_work,
			      test_msg_waitall_tx_work_handler);
	k_work_reschedule(&test_data.tx_work, K_MSEC(10));

	ret = zsock_recv(new_sock, rx_buf, sizeof(rx_buf), ZSOCK_MSG_WAITALL);
	zassert_equal(ret, sizeof(rx_buf), "Invalid length received");
	zassert_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(rx_buf),
			  "Invalid data received");
	k_work_cancel_delayable(&test_data.tx_work);

	/* MSG_WAITALL + SO_RCVTIMEO - make sure recv returns the amount of data
	 * received so far
	 */
	ret = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval,
			       sizeof(timeo_optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	memset(rx_buf, 0, sizeof(rx_buf));
	test_data.offset = 0;
	test_data.retries = sizeof(rx_buf) - 1;
	test_data.sock = c_sock;
	k_work_init_delayable(&test_data.tx_work,
			      test_msg_waitall_tx_work_handler);
	k_work_reschedule(&test_data.tx_work, K_MSEC(10));

	ret = zsock_recv(new_sock, rx_buf, sizeof(rx_buf) - 1, ZSOCK_MSG_WAITALL);
	if (ret < 0) {
		LOG_ERR("receive return val %i", ret);
	}
	zassert_equal(ret, sizeof(rx_buf) - 1, "Invalid length received");
	zassert_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(rx_buf) - 1,
			  "Invalid data received");
	k_work_cancel_delayable(&test_data.tx_work);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_v6_msg_waitall)
{
	struct test_msg_waitall_data test_data = {
		.data = TEST_STR_SMALL,
	};
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	int ret;
	uint8_t rx_buf[sizeof(TEST_STR_SMALL) - 1] = { 0 };
	struct timeval timeo_optval = {
		.tv_sec = 0,
		.tv_usec = 100000,
	};

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "Wrong addrlen");

	/* Regular MSG_WAITALL - make sure recv returns only after
	 * requested amount is received.
	 */
	test_data.offset = 0;
	test_data.retries = sizeof(rx_buf);
	test_data.sock = c_sock;
	k_work_init_delayable(&test_data.tx_work,
			      test_msg_waitall_tx_work_handler);
	k_work_reschedule(&test_data.tx_work, K_MSEC(10));

	ret = zsock_recv(new_sock, rx_buf, sizeof(rx_buf), ZSOCK_MSG_WAITALL);
	zassert_equal(ret, sizeof(rx_buf), "Invalid length received");
	zassert_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(rx_buf),
			  "Invalid data received");
	k_work_cancel_delayable(&test_data.tx_work);

	/* MSG_WAITALL + SO_RCVTIMEO - make sure recv returns the amount of data
	 * received so far
	 */
	ret = zsock_setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval,
			       sizeof(timeo_optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	memset(rx_buf, 0, sizeof(rx_buf));
	test_data.offset = 0;
	test_data.retries = sizeof(rx_buf) - 1;
	test_data.sock = c_sock;
	k_work_init_delayable(&test_data.tx_work,
			      test_msg_waitall_tx_work_handler);
	k_work_reschedule(&test_data.tx_work, K_MSEC(10));

	ret = zsock_recv(new_sock, rx_buf, sizeof(rx_buf) - 1, ZSOCK_MSG_WAITALL);
	zassert_equal(ret, sizeof(rx_buf) - 1, "Invalid length received");
	zassert_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(rx_buf) - 1,
			  "Invalid data received");
	k_work_cancel_delayable(&test_data.tx_work);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	test_context_cleanup();
}

#ifdef CONFIG_USERSPACE
#define CHILD_STACK_SZ		(2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
struct k_thread child_thread;
K_THREAD_STACK_DEFINE(child_stack, CHILD_STACK_SZ);
ZTEST_BMEM volatile int result;

static void child_entry(void *p1, void *p2, void *p3)
{
	int sock = POINTER_TO_INT(p1);

	result = zsock_close(sock);
}

static void spawn_child(int sock)
{
	k_thread_create(&child_thread, child_stack,
			K_THREAD_STACK_SIZEOF(child_stack), child_entry,
			INT_TO_POINTER(sock), NULL, NULL, 0, K_USER,
			K_FOREVER);
}
#endif

ZTEST(net_socket_tcp, test_socket_permission)
{
#ifdef CONFIG_USERSPACE
	int sock;
	struct sockaddr_in saddr;
	struct net_context *ctx;

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &sock, &saddr);

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

static void *setup(void)
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

	return NULL;
}

struct close_data {
	struct k_work_delayable work;
	int fd;
};

static void close_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct close_data *data = CONTAINER_OF(dwork, struct close_data, work);

	zsock_close(data->fd);
}

ZTEST(net_socket_tcp, test_close_while_recv)
{
	/* Blocking recv() should return an error after close() is
	 * called from another thread.
	 */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr, s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	struct close_data close_work_data;
	char rx_buf[1];
	ssize_t ret;

	prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	/* Connect and accept that connection */
	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);

	/* Schedule close() from workqueue */
	k_work_init_delayable(&close_work_data.work, close_work);
	close_work_data.fd = c_sock;
	k_work_schedule(&close_work_data.work, K_MSEC(10));

	/* Start blocking recv(), which should be unblocked by close() from
	 * another thread and return an error.
	 */
	ret = zsock_recv(c_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(ret, -1, "recv did not return error");
	zassert_equal(errno, EINTR, "Unexpected errno value: %d", errno);

	test_close(new_sock);
	test_close(s_sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_close_while_accept)
{
	/* Blocking accept() should return an error after close() is
	 * called from another thread.
	 */
	int s_sock;
	int new_sock;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	struct close_data close_work_data;

	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	/* Schedule close() from workqueue */
	k_work_init_delayable(&close_work_data.work, close_work);
	close_work_data.fd = s_sock;
	k_work_schedule(&close_work_data.work, K_MSEC(10));

	/* Start blocking accept(), which should be unblocked by close() from
	 * another thread and return an error.
	 */
	new_sock = zsock_accept(s_sock, &addr, &addrlen);
	zassert_equal(new_sock, -1, "accept did not return error");
	zassert_equal(errno, EINTR, "Unexpected errno value: %d", errno);

	test_context_cleanup();
}

#undef read
#define read(fd, buf, len) zsock_recv(fd, buf, len, 0)

#undef write
#define write(fd, buf, len) zsock_send(fd, buf, len, 0)

enum test_ioctl_fionread_sockid {
	CLIENT, SERVER, ACCEPT,
};

static void test_ioctl_fionread_setup(int af, int fd[3])
{
	socklen_t addrlen;
	socklen_t addrlen2;
	struct sockaddr_in6 addr;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;

	fd[0] = -1;
	fd[1] = -1;
	fd[2] = -1;

	switch (af) {
	case AF_INET: {
		prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &fd[CLIENT],
				    (struct sockaddr_in *)&c_saddr);
		prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &fd[ACCEPT],
				    (struct sockaddr_in *)&s_saddr);
		addrlen = sizeof(struct sockaddr_in);
	} break;

	case AF_INET6: {
		prepare_sock_tcp_v6(MY_IPV6_ADDR, ANY_PORT, &fd[CLIENT], &c_saddr);
		prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &fd[ACCEPT], &s_saddr);
		addrlen = sizeof(struct sockaddr_in6);
	} break;

	default:
		zassert_true(false);
		return;
	}

	addrlen2 = addrlen;

	test_bind(fd[ACCEPT], (struct sockaddr *)&s_saddr, addrlen);
	test_listen(fd[ACCEPT]);

	test_connect(fd[CLIENT], (struct sockaddr *)&s_saddr, addrlen);
	test_accept(fd[ACCEPT], &fd[SERVER], (struct sockaddr *)&addr, &addrlen2);
}

/* note: this is duplicated from tests/net/socket/socketpair/src/fionread.c */
static void test_ioctl_fionread_common(int af)
{
	int avail;
	uint8_t bytes[2];
	/* server fd := 0, client fd := 1, accept fd := 2 */
	enum fde {
		SERVER,
		CLIENT,
		ACCEPT,
	};
	int fd[] = {-1, -1, -1};

	test_ioctl_fionread_setup(af, fd);

	/* both ends should have zero bytes available after being newly created */
	for (enum fde i = SERVER; i <= CLIENT; ++i) {
		avail = 42;
		zassert_ok(zsock_ioctl(fd[i], ZFD_IOCTL_FIONREAD, &avail));
		zassert_equal(0, avail, "exp: %d: act: %d", 0, avail);
	}

	/* write something to one end, check availability from the other end */
	for (enum fde i = SERVER; i <= CLIENT; ++i) {
		enum fde j = (i + 1) % (CLIENT + 1);

		zassert_equal(1, write(fd[i], "\x42", 1));
		zassert_equal(1, write(fd[i], "\x73", 1));
		k_msleep(100);
		zassert_ok(zsock_ioctl(fd[j], ZFD_IOCTL_FIONREAD, &avail));
		zassert_equal(ARRAY_SIZE(bytes), avail, "exp: %d: act: %d", ARRAY_SIZE(bytes),
			      avail);
	}

	/* read the other end, ensure availability is zero again */
	for (enum fde i = SERVER; i <= CLIENT; ++i) {
		int ex = ARRAY_SIZE(bytes);
		int act = read(fd[i], bytes, ARRAY_SIZE(bytes));

		zassert_equal(ex, act, "read() failed: errno: %d exp: %d act: %d", errno, ex, act);
		zassert_ok(zsock_ioctl(fd[i], ZFD_IOCTL_FIONREAD, &avail));
		zassert_equal(0, avail, "exp: %d: act: %d", 0, avail);
	}

	zsock_close(fd[SERVER]);
	zsock_close(fd[CLIENT]);
	zsock_close(fd[ACCEPT]);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_ioctl_fionread_v4)
{
	test_ioctl_fionread_common(AF_INET);
}
ZTEST(net_socket_tcp, test_ioctl_fionread_v6)
{
	test_ioctl_fionread_common(AF_INET6);
}

/* Connect to peer which is not listening the test port and
 * make sure select() returns proper error for the closed
 * connection.
 */
ZTEST(net_socket_tcp, test_connect_and_wait_for_v4_select)
{
	struct sockaddr_in addr = { 0 };
	struct in_addr v4addr;
	int fd, flags, ret, optval;
	socklen_t optlen = sizeof(optval);

	fd = zsock_socket(AF_INET, SOCK_STREAM, 0);

	flags = zsock_fcntl(fd, ZVFS_F_GETFL, 0);
	zsock_fcntl(fd, ZVFS_F_SETFL, flags | ZVFS_O_NONBLOCK);

	zsock_inet_pton(AF_INET, "127.0.0.1", (void *)&v4addr);

	addr.sin_family = AF_INET;
	net_ipaddr_copy(&addr.sin_addr, &v4addr);

	/* There should be nobody serving this port */
	addr.sin_port = htons(8088);

	ret = zsock_connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
	zassert_equal(ret, -1, "connect succeed, %d", errno);
	zassert_equal(errno, EINPROGRESS, "connect succeed, %d", errno);

	/* Wait for the connection (this should fail eventually) */
	while (1) {
		zsock_fd_set wfds;
		struct timeval tv = {
			.tv_sec = 1,
			.tv_usec = 0
		};

		ZSOCK_FD_ZERO(&wfds);
		ZSOCK_FD_SET(fd, &wfds);

		/* Check if the connection is there, this should timeout */
		ret = zsock_select(fd + 1,  NULL, &wfds, NULL, &tv);
		if (ret < 0) {
			break;
		}

		if (ret > 0) {
			if (ZSOCK_FD_ISSET(fd, &wfds)) {
				break;
			}
		}
	}

	zassert_true(ret > 0, "select failed, %d", errno);

	/* Get the reason for the connect */
	ret = zsock_getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed, %d", errno);

	/* If SO_ERROR is 0, then it means that connect succeed. Any
	 * other value (errno) means that it failed.
	 */
	zassert_equal(optval, ECONNREFUSED, "unexpected connect status, %d", optval);

	ret = zsock_close(fd);
	zassert_equal(ret, 0, "close failed, %d", errno);
}

/* Connect to peer which is not listening the test port and
 * make sure poll() returns proper error for the closed
 * connection.
 */
ZTEST(net_socket_tcp, test_connect_and_wait_for_v4_poll)
{
	struct sockaddr_in addr = { 0 };
	struct zsock_pollfd fds[1];
	struct in_addr v4addr;
	int fd, flags, ret, optval;
	bool closed = false;
	socklen_t optlen = sizeof(optval);

	fd = zsock_socket(AF_INET, SOCK_STREAM, 0);

	flags = zsock_fcntl(fd, ZVFS_F_GETFL, 0);
	zsock_fcntl(fd, ZVFS_F_SETFL, flags | ZVFS_O_NONBLOCK);

	zsock_inet_pton(AF_INET, "127.0.0.1", (void *)&v4addr);

	addr.sin_family = AF_INET;
	net_ipaddr_copy(&addr.sin_addr, &v4addr);

	/* There should be nobody serving this port */
	addr.sin_port = htons(8088);

	ret = zsock_connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
	zassert_equal(ret, -1, "connect succeed, %d", errno);
	zassert_equal(errno, EINPROGRESS, "connect succeed, %d", errno);

	/* Wait for the connection (this should fail eventually) */
	while (1) {
		memset(fds, 0, sizeof(fds));
		fds[0].fd = fd;
		fds[0].events = ZSOCK_POLLOUT;

		/* Check if the connection is there, this should timeout */
		ret = zsock_poll(fds, 1, 10);
		if (ret < 0) {
			break;
		}

		if (fds[0].revents > 0) {
			if (fds[0].revents & ZSOCK_POLLERR) {
				closed = true;
				break;
			}
		}
	}

	zassert_true(closed, "poll failed, %d", errno);

	/* Get the reason for the connect */
	ret = zsock_getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed, %d", errno);

	/* If SO_ERROR is 0, then it means that connect succeed. Any
	 * other value (errno) means that it failed.
	 */
	zassert_equal(optval, ECONNREFUSED, "unexpected connect status, %d", optval);

	ret = zsock_close(fd);
	zassert_equal(ret, 0, "close failed, %d", errno);
}

ZTEST(net_socket_tcp, test_so_keepalive)
{
	struct sockaddr_in bind_addr4;
	int sock, ret;
	int optval;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &sock, &bind_addr4);

	/* Keep-alive should be disabled by default. */
	ret = zsock_getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, 0, "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	/* Enable keep-alive. */
	optval = 1;
	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	ret = zsock_getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, 1, "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	/* Check keep-alive parameters defaults. */
	ret = zsock_getsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, CONFIG_NET_TCP_KEEPIDLE_DEFAULT,
		      "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	ret = zsock_getsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, CONFIG_NET_TCP_KEEPINTVL_DEFAULT,
		      "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	ret = zsock_getsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, CONFIG_NET_TCP_KEEPCNT_DEFAULT,
		      "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	/* Check keep-alive parameters update. */
	optval = 123;
	ret = zsock_setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	optval = 10;
	ret = zsock_setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	optval = 2;
	ret = zsock_setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	ret = zsock_getsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, 123, "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	ret = zsock_getsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, 10, "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	ret = zsock_getsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, 2, "getsockopt got invalid value");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	test_close(sock);

	test_context_cleanup();
}

ZTEST(net_socket_tcp, test_keepalive_timeout)
{
	struct sockaddr_in c_saddr, s_saddr;
	int c_sock, s_sock, new_sock;
	uint8_t rx_buf;
	int optval;
	int ret;

	prepare_sock_tcp_v4(MY_IPV4_ADDR, ANY_PORT, &c_sock, &c_saddr);
	prepare_sock_tcp_v4(MY_IPV4_ADDR, SERVER_PORT, &s_sock, &s_saddr);

	/* Enable keep-alive on both ends and set timeouts/retries to minimum */
	optval = 1;
	ret = zsock_setsockopt(c_sock, SOL_SOCKET, SO_KEEPALIVE,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);
	ret = zsock_setsockopt(s_sock, SOL_SOCKET, SO_KEEPALIVE,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	optval = 1;
	ret = zsock_setsockopt(c_sock, IPPROTO_TCP, TCP_KEEPIDLE,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);
	ret = zsock_setsockopt(s_sock, IPPROTO_TCP, TCP_KEEPIDLE,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	optval = 1;
	ret = zsock_setsockopt(c_sock, IPPROTO_TCP, TCP_KEEPINTVL,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);
	ret = zsock_setsockopt(s_sock, IPPROTO_TCP, TCP_KEEPINTVL,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	optval = 1;
	ret = zsock_setsockopt(c_sock, IPPROTO_TCP, TCP_KEEPCNT,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);
	ret = zsock_setsockopt(s_sock, IPPROTO_TCP, TCP_KEEPCNT,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	/* Establish connection */
	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);
	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_accept(s_sock, &new_sock, NULL, NULL);

	/* Kill communication - expect that connection will be closed after
	 * a timeout period.
	 */
	loopback_set_packet_drop_ratio(1.0f);

	ret = zsock_recv(c_sock, &rx_buf, sizeof(rx_buf), 0);
	zassert_equal(ret, -1, "recv() should've failed");
	zassert_equal(errno, ETIMEDOUT, "wrong errno value, %d", errno);

	/* Same on the other end. */
	ret = zsock_recv(new_sock, &rx_buf, sizeof(rx_buf), 0);
	zassert_equal(ret, -1, "recv() should've failed");
	zassert_equal(errno, ETIMEDOUT, "wrong errno value, %d", errno);

	test_close(c_sock);
	test_close(new_sock);
	test_close(s_sock);

	loopback_set_packet_drop_ratio(0.0f);
	test_context_cleanup();
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	for (int i = 0; i < CONFIG_ZVFS_OPEN_MAX; ++i) {
		(void)zsock_close(i);
	}
}

ZTEST_SUITE(net_socket_tcp, NULL, setup, NULL, after, NULL);
