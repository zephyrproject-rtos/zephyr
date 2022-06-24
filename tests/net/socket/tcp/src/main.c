/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <ztest_assert.h>
#include <fcntl.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/loopback.h>

#include "../../socket_helpers.h"

#define TEST_STR_SMALL "test"

#define ANY_PORT 0
#define SERVER_PORT 4242

#define MAX_CONNS 5

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(3)
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

static void test_shutdown(int sock, int how)
{
	zassert_equal(shutdown(sock, how),
		      0,
		      "shutdown failed");
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
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

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

		recved = recv(new_sock, buffer, chunk_size, 0);

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

void test_v4_send_recv_large(void)
{
	int c_sock;
	int s_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
				&c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
				&s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	(void)k_thread_create(&tcp_server_thread_data, tcp_server_stack_area,
		K_THREAD_STACK_SIZEOF(tcp_server_stack_area),
		tcp_server_block_thread,
		&s_sock, NULL, NULL,
		k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	/* send piece by piece */
	ssize_t total_send = 0;
	int iteration = 0;
	uint8_t buffer[256];

	while (total_send < TEST_LARGE_TRANSFER_SIZE) {
		/* Fill the buffer with a known patern */
		for (int i = 0; i < sizeof(buffer); i++) {
			int total_idx = i + total_send;

			buffer[i] = (total_idx * TEST_PRIME) & 0xff;
		}

		size_t chunk_size = sizeof(buffer);
		size_t remain = TEST_LARGE_TRANSFER_SIZE - total_send;

		if (chunk_size > remain) {
			chunk_size = remain;
		}

		int send_bytes = send(c_sock, buffer, chunk_size, 0);

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

void test_v4_broken_link(void)
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

	rv = setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	test_recv(new_sock, MSG_PEEK);
	test_recv(new_sock, 0);

	/* At this point break the interface */
	zassert_equal(loopback_set_packet_drop_ratio(1.0f), 0,
		"Error setting packet drop rate");

	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	/* At this point break the interface */
	start_time = k_uptime_get_32();

	/* Test the loopback packet loss: message should never arrive */
	recved = recv(new_sock, rx_buf, sizeof(rx_buf), 0);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 500, "Expected timeout after 500ms but "
			"was %dms", time_diff);

	/* Reading from client should indicate the socket has been closed */
	recved = recv(c_sock, rx_buf, sizeof(rx_buf), 0);
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

void test_shutdown_rd_synchronous(void)
{
	/* recv() after shutdown(..., ZSOCK_SHUT_RD) should return 0 (EOF).
	 */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr, s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

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

	shutdown(data->fd, data->how);
}

void test_shutdown_rd_while_recv(void)
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

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

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

	test_context_cleanup();

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

void test_connect_timeout(void)
{
	/* Test if socket connect fails when there is not communication
	 * possible.
	 */
	int count_after = 0;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	int c_sock;
	int rv;

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);

	s_saddr.sin_family = AF_INET;
	s_saddr.sin_port = htons(SERVER_PORT);
	rv = zsock_inet_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &s_saddr.sin_addr);
	zassert_equal(rv, 1, "inet_pton failed");

	loopback_set_packet_drop_ratio(1.0f);

	zassert_equal(connect(c_sock, (struct sockaddr *)&s_saddr,
			    sizeof(s_saddr)),
			    -1, "connect succeed");

	zassert_equal(errno, ETIMEDOUT,
			    "connect should be timed out, got %i", errno);

	test_close(c_sock);

	/* After the client socket closing, the context count should be 0 */
	net_context_foreach(calc_net_context, &count_after);

	zassert_equal(count_after, 0,
			    "net_context still in use");
}

#define TCP_CLOSE_FAILURE_TIMEOUT 60000

void test_close_obstructed(void)
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

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	zassert_equal(connect(c_sock, (struct sockaddr *)&s_saddr,
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

	zassert_equal(wait_for_n_tcp_contexts(0, TCP_TEARDOWN_TIMEOUT),
		      0,
		      "Not all TCP contexts properly cleaned up");

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

	zassert_equal(wait_for_n_tcp_contexts(0, TCP_TEARDOWN_TIMEOUT),
		      0,
		      "Not all TCP contexts properly cleaned up");
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

	test_context_cleanup();
}

void test_so_rcvbuf(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int  retval;
	int optval = UINT16_MAX;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			&sock1, &bind_addr4);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			&sock2, &bind_addr6);

	rv = setsockopt(sock1, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = getsockopt(sock1, SOL_SOCKET, SO_RCVBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = getsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	optval = -1;
	rv = setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	optval = UINT16_MAX + 1;
	rv = setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	test_close(sock1);
	test_close(sock2);

	test_context_cleanup();
}

void test_so_sndbuf(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int retval;
	int optval = UINT16_MAX;
	socklen_t optlen = sizeof(optval);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			&sock1, &bind_addr4);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			&sock2, &bind_addr6);

	rv = setsockopt(sock1, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = getsockopt(sock1, SOL_SOCKET, SO_SNDBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	rv = setsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", rv);
	rv = getsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &retval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", rv);
	zassert_equal(retval, optval, "getsockopt got invalid rcvbuf");
	zassert_equal(optlen, sizeof(optval), "getsockopt got invalid size");

	optval = -1;
	rv = setsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	optval = UINT16_MAX + 1;
	rv = setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	zassert_equal(rv, -1, "setsockopt failed (%d)", rv);

	test_close(sock1);
	test_close(sock2);

	test_context_cleanup();
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

	test_context_cleanup();
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

void test_v4_msg_waitall(void)
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

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

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

	ret = recv(new_sock, rx_buf, sizeof(rx_buf), MSG_WAITALL);
	zassert_equal(ret, sizeof(rx_buf), "Invalid length received");
	zassert_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(rx_buf),
			  "Invalid data received");
	k_work_cancel_delayable(&test_data.tx_work);

	/* MSG_WAITALL + SO_RCVTIMEO - make sure recv returns the amount of data
	 * received so far
	 */
	ret = setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval,
			 sizeof(timeo_optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	memset(rx_buf, 0, sizeof(rx_buf));
	test_data.offset = 0;
	test_data.retries = sizeof(rx_buf) - 1;
	test_data.sock = c_sock;
	k_work_init_delayable(&test_data.tx_work,
			      test_msg_waitall_tx_work_handler);
	k_work_reschedule(&test_data.tx_work, K_MSEC(10));

	ret = recv(new_sock, rx_buf, sizeof(rx_buf) - 1, MSG_WAITALL);
	zassert_equal(ret, sizeof(rx_buf) - 1, "Invalid length received");
	zassert_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(rx_buf) - 1,
			  "Invalid data received");
	k_work_cancel_delayable(&test_data.tx_work);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	test_context_cleanup();
}

void test_v6_msg_waitall(void)
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

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

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

	ret = recv(new_sock, rx_buf, sizeof(rx_buf), MSG_WAITALL);
	zassert_equal(ret, sizeof(rx_buf), "Invalid length received");
	zassert_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(rx_buf),
			  "Invalid data received");
	k_work_cancel_delayable(&test_data.tx_work);

	/* MSG_WAITALL + SO_RCVTIMEO - make sure recv returns the amount of data
	 * received so far
	 */
	ret = setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval,
			 sizeof(timeo_optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	memset(rx_buf, 0, sizeof(rx_buf));
	test_data.offset = 0;
	test_data.retries = sizeof(rx_buf) - 1;
	test_data.sock = c_sock;
	k_work_init_delayable(&test_data.tx_work,
			      test_msg_waitall_tx_work_handler);
	k_work_reschedule(&test_data.tx_work, K_MSEC(10));

	ret = recv(new_sock, rx_buf, sizeof(rx_buf) - 1, MSG_WAITALL);
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
		ztest_user_unit_test(test_shutdown_rd_synchronous),
		ztest_unit_test(test_shutdown_rd_while_recv),
		ztest_unit_test(test_open_close_immediately),
		ztest_unit_test_setup_teardown(test_connect_timeout,
		 restore_packet_loss_ratio, restore_packet_loss_ratio),
		ztest_unit_test_setup_teardown(test_close_obstructed,
		 restore_packet_loss_ratio, restore_packet_loss_ratio),
		ztest_user_unit_test(test_v4_accept_timeout),
		ztest_unit_test(test_so_type),
		ztest_unit_test(test_so_protocol),
		ztest_unit_test(test_so_rcvbuf),
		ztest_unit_test(test_so_sndbuf),
		ztest_unit_test(test_v4_so_rcvtimeo),
		ztest_unit_test(test_v6_so_rcvtimeo),
		ztest_unit_test(test_v4_msg_waitall),
		ztest_unit_test(test_v6_msg_waitall),
		ztest_user_unit_test(test_socket_permission),
		ztest_unit_test(test_v4_send_recv_large),
		ztest_unit_test_setup_teardown(test_v4_send_recv_large,
			set_packet_loss_ratio, restore_packet_loss_ratio),
		ztest_unit_test_setup_teardown(test_v4_broken_link,
			restore_packet_loss_ratio, restore_packet_loss_ratio)
		);

	ztest_run_test_suite(socket_tcp);
}
