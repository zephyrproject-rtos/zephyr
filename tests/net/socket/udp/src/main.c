/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <sys/mutex.h>
#include <ztest_assert.h>

#include <net/socket.h>
#include <net/ethernet.h>

#include "ipv6.h"
#include "../../socket_helpers.h"

#if defined(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"
/* More than 256 bytes, to use >1 net_buf. */
#define TEST_STR2 \
	"The Zephyr Project, a Linux Foundation hosted Collaboration " \
	"Project, is an open source collaborative effort uniting leaders " \
	"from across the industry to build a best-in-breed small, scalable, " \
	"real-time operating system (RTOS) optimized for resource-" \
	"constrained devices, across multiple architectures."

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

static ZTEST_BMEM char rx_buf[400];

/* Common routine to communicate packets over pair of sockets. */
static void comm_sendto_recvfrom(int client_sock,
				 struct sockaddr *client_addr,
				 socklen_t client_addrlen,
				 int server_sock,
				 struct sockaddr *server_addr,
				 socklen_t server_addrlen)
{
	ssize_t sent = 0;
	ssize_t recved = 0;
	struct sockaddr addr;
	socklen_t addrlen;
	struct sockaddr addr2;
	socklen_t addrlen2;

	zassert_not_null(client_addr, "null client addr");
	zassert_not_null(server_addr, "null server addr");

	/*
	 * Test client -> server sending
	 */

	sent = sendto(client_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL),
		      0, server_addr, server_addrlen);
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed");

	/* Test recvfrom(MSG_PEEK) */
	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	recved = recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			  MSG_PEEK, &addr, &addrlen);
	zassert_true(recved >= 0, "recvfrom fail");
	zassert_equal(recved, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_mem_equal(rx_buf, BUF_AND_SIZE(TEST_STR_SMALL), "wrong data");
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Test normal recvfrom() */
	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	recved = recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			  0, &addr, &addrlen);
	zassert_true(recved >= 0, "recvfrom fail");
	zassert_equal(recved, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_mem_equal(rx_buf, BUF_AND_SIZE(TEST_STR_SMALL), "wrong data");
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Check the client port */
	if (net_sin(client_addr)->sin_port != ANY_PORT) {
		zassert_equal(net_sin(client_addr)->sin_port,
			      net_sin(&addr)->sin_port,
			      "unexpected client port");
	}

	/*
	 * Test server -> client sending
	 */

	sent = sendto(server_sock, BUF_AND_SIZE(TEST_STR2),
		      0, &addr, addrlen);
	zassert_equal(sent, STRLEN(TEST_STR2), "sendto failed");

	/* Test normal recvfrom() */
	addrlen2 = sizeof(addr);
	clear_buf(rx_buf);
	recved = recvfrom(client_sock, rx_buf, sizeof(rx_buf),
			  0, &addr2, &addrlen2);
	zassert_true(recved >= 0, "recvfrom fail");
	zassert_equal(recved, STRLEN(TEST_STR2),
		      "unexpected received bytes");
	zassert_mem_equal(rx_buf, BUF_AND_SIZE(TEST_STR2), "wrong data");
	zassert_equal(addrlen2, server_addrlen, "unexpected addrlen");

	/* Check the server port */
	zassert_equal(net_sin(server_addr)->sin_port,
		      net_sin(&addr2)->sin_port,
		      "unexpected server port");

	/* Test that unleft leftover data from datagram is discarded. */

	/* Send 2 datagrams */
	sent = sendto(server_sock, BUF_AND_SIZE(TEST_STR2),
		      0, &addr, addrlen);
	zassert_equal(sent, STRLEN(TEST_STR2), "sendto failed");
	sent = sendto(server_sock, BUF_AND_SIZE(TEST_STR_SMALL),
		      0, &addr, addrlen);
	zassert_equal(sent, STRLEN(TEST_STR_SMALL), "sendto failed");

	/* Receive just beginning of 1st datagram */
	addrlen2 = sizeof(addr);
	clear_buf(rx_buf);
	recved = recvfrom(client_sock, rx_buf, 16, 0, &addr2, &addrlen2);
	zassert_true(recved == 16, "recvfrom fail");
	zassert_mem_equal(rx_buf, TEST_STR2, 16, "wrong data");

	/* Make sure that now we receive 2nd datagram */
	addrlen2 = sizeof(addr);
	clear_buf(rx_buf);
	recved = recvfrom(client_sock, rx_buf, 16, 0, &addr2, &addrlen2);
	zassert_true(recved == STRLEN(TEST_STR_SMALL), "recvfrom fail");
	zassert_mem_equal(rx_buf, BUF_AND_SIZE(TEST_STR_SMALL), "wrong data");
}

void test_v4_sendto_recvfrom(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_v6_sendto_recvfrom(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_v4_bind_sendto(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, CLIENT_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr, sizeof(client_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_v6_bind_sendto(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, CLIENT_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr, sizeof(client_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_send_recv_2_sock(void)
{
	int sock1, sock2;
	struct sockaddr_in bind_addr, conn_addr;
	char buf[10];
	int len, rv;

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock1, &bind_addr);
	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock2, &conn_addr);

	rv = bind(sock1, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = connect(sock2, (struct sockaddr *)&conn_addr, sizeof(conn_addr));
	zassert_equal(rv, 0, "connect failed");

	len = send(sock2, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	clear_buf(buf);
	len = recv(sock1, buf, sizeof(buf), MSG_PEEK);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "Invalid recv len");
	zassert_mem_equal(buf, BUF_AND_SIZE(TEST_STR_SMALL), "Wrong data");

	clear_buf(buf);
	len = recv(sock1, buf, sizeof(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "Invalid recv len");
	zassert_mem_equal(buf, BUF_AND_SIZE(TEST_STR_SMALL), "Wrong data");

	rv = close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock2);
	zassert_equal(rv, 0, "close failed");
}

void test_so_priority(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	uint8_t optval;

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock1, &bind_addr4);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, 55555,
			    &sock2, &bind_addr6);

	rv = bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	optval = 2;
	rv = setsockopt(sock1, SOL_SOCKET, SO_PRIORITY, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval = 8;
	rv = setsockopt(sock2, SOL_SOCKET, SO_PRIORITY, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed");

	rv = close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock2);
	zassert_equal(rv, 0, "close failed");
}

static void comm_sendmsg_recvfrom(int client_sock,
				  struct sockaddr *client_addr,
				  socklen_t client_addrlen,
				  const struct msghdr *client_msg,
				  int server_sock,
				  struct sockaddr *server_addr,
				  socklen_t server_addrlen)
{
	ssize_t sent;
	ssize_t recved;
	struct sockaddr addr;
	socklen_t addrlen;
	int len, i;

	zassert_not_null(client_addr, "null client addr");
	zassert_not_null(server_addr, "null server addr");

	/*
	 * Test client -> server sending
	 */

	sent = sendmsg(client_sock, client_msg, 0);
	zassert_true(sent > 0, "sendmsg failed (%d)", -errno);

	for (i = 0, len = 0; i < client_msg->msg_iovlen; i++) {
		len += client_msg->msg_iov[i].iov_len;
	}

	zassert_equal(sent, len, "iovec len (%d) vs sent (%d)", len, sent);

	/* Test recvfrom(MSG_PEEK) */
	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	recved = recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			  MSG_PEEK, &addr, &addrlen);
	zassert_true(recved >= 0, "recvfrom fail");
	zassert_equal(recved, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(sent, recved, "sent(%d)/received(%d) mismatch",
		      sent, recved);

	zassert_mem_equal(rx_buf, BUF_AND_SIZE(TEST_STR_SMALL),
			  "wrong data (%s)", rx_buf);
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Test normal recvfrom() */
	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	recved = recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			  0, &addr, &addrlen);
	zassert_true(recved >= 0, "recvfrom fail");
	zassert_equal(recved, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_mem_equal(rx_buf, BUF_AND_SIZE(TEST_STR_SMALL), "wrong data");
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Check the client port */
	if (net_sin(client_addr)->sin_port != ANY_PORT) {
		zassert_equal(net_sin(client_addr)->sin_port,
			      net_sin(&addr)->sin_port,
			      "unexpected client port");
	}
}

void test_v4_sendmsg_recvfrom(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(int))];
	} cmsgbuf;

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = sizeof(cmsgbuf.buf);
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &server_addr;
	msg.msg_namelen = sizeof(server_addr);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = 1122;
	*(int *)CMSG_DATA(cmsg) = 42;

	comm_sendmsg_recvfrom(client_sock,
			      (struct sockaddr *)&client_addr,
			      sizeof(client_addr),
			      &msg,
			      server_sock,
			      (struct sockaddr *)&server_addr,
			      sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_v4_sendmsg_recvfrom_no_aux_data(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	struct msghdr msg;
	struct iovec io_vector[1];

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &server_addr;
	msg.msg_namelen = sizeof(server_addr);

	comm_sendmsg_recvfrom(client_sock,
			      (struct sockaddr *)&client_addr,
			      sizeof(client_addr),
			      &msg,
			      server_sock,
			      (struct sockaddr *)&server_addr,
			      sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_v6_sendmsg_recvfrom(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(int))];
	} cmsgbuf;

	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = sizeof(cmsgbuf.buf);
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &server_addr;
	msg.msg_namelen = sizeof(server_addr);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = 1122;
	*(int *)CMSG_DATA(cmsg) = 42;

	comm_sendmsg_recvfrom(client_sock,
			      (struct sockaddr *)&client_addr,
			      sizeof(client_addr),
			      &msg,
			      server_sock,
			      (struct sockaddr *)&server_addr,
			      sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_v4_sendmsg_recvfrom_connected(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(int))];
	} cmsgbuf;

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	rv = connect(client_sock, (struct sockaddr *)&server_addr,
		     sizeof(server_addr));
	zassert_equal(rv, 0, "connect failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = sizeof(cmsgbuf.buf);
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = 1122;
	*(int *)CMSG_DATA(cmsg) = 42;

	comm_sendmsg_recvfrom(client_sock,
			      (struct sockaddr *)&client_addr,
			      sizeof(client_addr),
			      &msg,
			      server_sock,
			      (struct sockaddr *)&server_addr,
			      sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_v6_sendmsg_recvfrom_connected(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(int))];
	} cmsgbuf;

	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &client_sock, &client_addr);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &server_sock, &server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	rv = connect(client_sock, (struct sockaddr *)&server_addr,
		     sizeof(server_addr));
	zassert_equal(rv, 0, "connect failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = sizeof(cmsgbuf.buf);
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = 1122;
	*(int *)CMSG_DATA(cmsg) = 42;

	comm_sendmsg_recvfrom(client_sock,
			      (struct sockaddr *)&client_addr,
			      sizeof(client_addr),
			      &msg,
			      server_sock,
			      (struct sockaddr *)&server_addr,
			      sizeof(server_addr));

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

void test_so_type(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optsize = sizeof(optval);

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock1, &bind_addr4);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, 55555,
			    &sock2, &bind_addr6);

	rv = getsockopt(sock1, SOL_SOCKET, SO_TYPE, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_DGRAM, "getsockopt got invalid type");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = getsockopt(sock2, SOL_SOCKET, SO_TYPE, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_DGRAM, "getsockopt got invalid type");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock2);
	zassert_equal(rv, 0, "close failed");
}

void test_so_txtime(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	socklen_t optlen;
	bool optval;

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock1, &bind_addr4);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, 55555,
			    &sock2, &bind_addr6);

	rv = bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	optval = true;
	rv = setsockopt(sock1, SOL_SOCKET, SO_TXTIME, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval = false;
	rv = setsockopt(sock2, SOL_SOCKET, SO_TXTIME, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed");

	optlen = sizeof(optval);
	rv = getsockopt(sock1, SOL_SOCKET, SO_TXTIME, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));
	zassert_equal(optval, true, "getsockopt txtime");

	optlen = sizeof(optval);
	rv = getsockopt(sock2, SOL_SOCKET, SO_TXTIME, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));
	zassert_equal(optval, false, "getsockopt txtime");

	rv = close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock2);
	zassert_equal(rv, 0, "close failed");
}

void test_so_rcvtimeo(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	ssize_t recved = 0;
	struct sockaddr addr;
	socklen_t addrlen;
	uint32_t start_time, time_diff;

	struct timeval optval = {
		.tv_sec = 2,
		.tv_usec = 500000,
	};

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock1, &bind_addr4);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, 55555,
			    &sock2, &bind_addr6);

	rv = bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	rv = setsockopt(sock1, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 0;
	rv = setsockopt(sock2, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	start_time = k_uptime_get_32();
	recved = recvfrom(sock1, rx_buf, sizeof(rx_buf),
			  0, &addr, &addrlen);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2500, "Expected timeout after 2500ms but "
			"was %dms", time_diff);

	start_time = k_uptime_get_32();
	recved = recvfrom(sock2, rx_buf, sizeof(rx_buf),
			  0, &addr, &addrlen);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 2000, "Expected timeout after 2000ms but "
			"was %dms", time_diff);

	rv = close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock2);
	zassert_equal(rv, 0, "close failed");
}

void test_so_sndtimeo(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;

	struct timeval optval = {
		.tv_sec = 2,
		.tv_usec = 500000,
	};

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock1, &bind_addr4);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, 55555,
			    &sock2, &bind_addr6);

	rv = bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	rv = setsockopt(sock1, SOL_SOCKET, SO_SNDTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 0;
	rv = setsockopt(sock2, SOL_SOCKET, SO_SNDTIMEO, &optval,
			sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed");

	rv = close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock2);
	zassert_equal(rv, 0, "close failed");
}

void test_so_protocol(void)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optsize = sizeof(optval);

	prepare_sock_udp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, 55555,
			    &sock1, &bind_addr4);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, 55555,
			    &sock2, &bind_addr6);

	rv = getsockopt(sock1, SOL_SOCKET, SO_PROTOCOL, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_UDP, "getsockopt got invalid protocol");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = getsockopt(sock2, SOL_SOCKET, SO_PROTOCOL, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_UDP, "getsockopt got invalid protocol");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock2);
	zassert_equal(rv, 0, "close failed");
}

static void comm_sendmsg_with_txtime(int client_sock,
				     struct sockaddr *client_addr,
				     socklen_t client_addrlen,
				     const struct msghdr *client_msg)
{
	ssize_t sent;
	int len, i;

	zassert_not_null(client_addr, "null client addr");

	/*
	 * Test client -> server sending
	 */

	sent = sendmsg(client_sock, client_msg, 0);
	zassert_true(sent > 0, "sendmsg failed (%d)", -errno);

	for (i = 0, len = 0; i < client_msg->msg_iovlen; i++) {
		len += client_msg->msg_iov[i].iov_len;
	}

	zassert_equal(sent, len, "iovec len (%d) vs sent (%d)", len, sent);
}

/* In order to verify that the network device driver is able to receive
 * the TXTIME option, create a separate network device and catch the packets
 * we are sending.
 */
struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_address[6];
};

static struct eth_fake_context eth_fake_data;
static ZTEST_BMEM struct sockaddr_in6 server_addr;

/* The semaphore is there to wait the data to be received. */
static ZTEST_BMEM SYS_MUTEX_DEFINE(wait_data);

static struct net_if *eth_iface;
static ZTEST_BMEM bool test_started;
static ZTEST_BMEM bool test_failed;
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static uint8_t server_lladdr[] = { 0x01, 0x02, 0x03, 0xff, 0xfe,
				0x04, 0x05, 0x06 };
static struct net_linkaddr server_link_addr = {
	.addr = server_lladdr,
	.len = sizeof(server_lladdr),
};
#define MY_IPV6_ADDR "2001:db8:100::1"
#define PEER_IPV6_ADDR "2001:db8:100::2"
#define TEST_TXTIME 0xff112233445566ff
#define WAIT_TIME K_MSEC(250)

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	uint64_t txtime;

	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	if (!test_started) {
		return 0;
	}

	txtime = net_pkt_txtime(pkt);
	if (txtime != TEST_TXTIME) {
		test_failed = true;
	} else {
		test_failed = false;
	}

	sys_mutex_unlock(&wait_data);

	return 0;
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

static int eth_fake_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", eth_fake_init, device_pm_control_nop,
		    &eth_fake_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

static void test_setup_eth(void)
{
	struct net_if_addr *ifaddr;
	int ret;

	eth_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	zassert_not_null(eth_iface, "No ethernet interface found");

	ifaddr = net_if_ipv6_addr_add(eth_iface, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	net_if_up(eth_iface);

	(void)memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(1234);
	ret = inet_pton(AF_INET6, PEER_IPV6_ADDR, &server_addr.sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* In order to avoid neighbor discovery, populate neighbor cache */
	net_ipv6_nbr_add(eth_iface, &server_addr.sin6_addr, &server_link_addr,
			 true, NET_IPV6_NBR_STATE_REACHABLE);
}

void test_v6_sendmsg_with_txtime(void)
{
	int rv;
	int client_sock;
	bool optval;
	uint64_t txtime;
	struct sockaddr_in6 client_addr;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(uint64_t))];
	} cmsgbuf;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock,
			    &client_addr);

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = sizeof(cmsgbuf.buf);
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &server_addr;
	msg.msg_namelen = sizeof(server_addr);

	txtime = TEST_TXTIME;

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(txtime));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_TXTIME;
	*(uint64_t *)CMSG_DATA(cmsg) = txtime;

	optval = true;
	rv = setsockopt(client_sock, SOL_SOCKET, SO_TXTIME, &optval,
			sizeof(optval));

	test_started = true;

	comm_sendmsg_with_txtime(client_sock,
				 (struct sockaddr *)&client_addr,
				 sizeof(client_addr),
				 &msg);

	rv = close(client_sock);
	zassert_equal(rv, 0, "close failed");

	if (sys_mutex_lock(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout DNS query not received");
	}

	zassert_false(test_failed, "Invalid txtime received");

	test_started = false;
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_udp,
			 ztest_unit_test(test_send_recv_2_sock),
			 ztest_unit_test(test_v4_sendto_recvfrom),
			 ztest_unit_test(test_v6_sendto_recvfrom),
			 ztest_unit_test(test_v4_bind_sendto),
			 ztest_unit_test(test_v6_bind_sendto),
			 ztest_unit_test(test_so_type),
			 ztest_unit_test(test_so_priority),
			 ztest_unit_test(test_so_txtime),
			 ztest_unit_test(test_so_rcvtimeo),
			 ztest_unit_test(test_so_sndtimeo),
			 ztest_unit_test(test_so_protocol),
			 ztest_unit_test(test_v4_sendmsg_recvfrom),
			 ztest_user_unit_test(test_v4_sendmsg_recvfrom),
			 ztest_unit_test(test_v4_sendmsg_recvfrom_no_aux_data),
			 ztest_user_unit_test(test_v4_sendmsg_recvfrom_no_aux_data),
			 ztest_unit_test(test_v6_sendmsg_recvfrom),
			 ztest_user_unit_test(test_v6_sendmsg_recvfrom),
			 ztest_unit_test(test_v4_sendmsg_recvfrom_connected),
			 ztest_user_unit_test(test_v4_sendmsg_recvfrom_connected),
			 ztest_unit_test(test_v6_sendmsg_recvfrom_connected),
			 ztest_user_unit_test(test_v6_sendmsg_recvfrom_connected),
			 ztest_unit_test(test_setup_eth),
			 ztest_unit_test(test_v6_sendmsg_with_txtime),
			 ztest_user_unit_test(test_v6_sendmsg_with_txtime)
		);

	ztest_run_test_suite(socket_udp);
}
