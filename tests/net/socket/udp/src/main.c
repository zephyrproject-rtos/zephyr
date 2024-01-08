/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>

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
/* More than available TX buffers */
static const char test_str_all_tx_bufs[] =
#include <string_all_tx_bufs.inc>
"!"
;

#define MY_IPV4_ADDR "127.0.0.1"
#define MY_IPV6_ADDR "::1"

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

static ZTEST_BMEM char rx_buf[NET_ETH_MTU + 1];

/* Common routine to communicate packets over pair of sockets. */
static void comm_sendto_recvfrom(int client_sock,
				 struct sockaddr *client_addr,
				 socklen_t client_addrlen,
				 int server_sock,
				 struct sockaddr *server_addr,
				 socklen_t server_addrlen)
{
	int avail;
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

	k_msleep(100);

	avail = 42;
	zassert_ok(ioctl(server_sock, ZFD_IOCTL_FIONREAD, &avail));
	zassert_equal(avail, strlen(TEST_STR_SMALL));

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

ZTEST(net_socket_udp, test_02_v4_sendto_recvfrom)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST(net_socket_udp, test_03_v6_sendto_recvfrom)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST(net_socket_udp, test_04_v4_bind_sendto)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	prepare_sock_udp_v4(MY_IPV4_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST(net_socket_udp, test_05_v6_bind_sendto)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST(net_socket_udp, test_01_send_recv_2_sock)
{
	int sock1, sock2;
	struct sockaddr_in bind_addr, conn_addr;
	char buf[10];
	int len, rv;

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock2, &conn_addr);

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

ZTEST(net_socket_udp, test_07_so_priority)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	uint8_t optval;

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

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

ZTEST_USER(net_socket_udp, test_12_v4_sendmsg_recvfrom)
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

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST_USER(net_socket_udp, test_13_v4_sendmsg_recvfrom_no_aux_data)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	struct msghdr msg;
	struct iovec io_vector[1];

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST_USER(net_socket_udp, test_14_v6_sendmsg_recvfrom)
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

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST_USER(net_socket_udp, test_15_v4_sendmsg_recvfrom_connected)
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

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST_USER(net_socket_udp, test_06_v6_sendmsg_recvfrom_connected)
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

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

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

ZTEST(net_socket_udp, test_06_so_type)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optsize = sizeof(optval);

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

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

ZTEST(net_socket_udp, test_08_so_txtime)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	socklen_t optlen;
	bool optval;

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

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

ZTEST(net_socket_udp, test_09_so_rcvtimeo)
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

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

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

ZTEST(net_socket_udp, test_10_so_sndtimeo)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;

	struct timeval optval = {
		.tv_sec = 2,
		.tv_usec = 500000,
	};

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

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

ZTEST(net_socket_udp, test_11_so_protocol)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	int optval;
	socklen_t optsize = sizeof(optval);

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

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
static ZTEST_BMEM struct sockaddr_in6 udp_server_addr;

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
#define MY_IPV6_ADDR_ETH   "2001:db8:100::1"
#define PEER_IPV6_ADDR_ETH "2001:db8:100::2"
#define TEST_TXTIME INT64_MAX
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
	net_time_t txtime;

	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	if (!test_started) {
		return 0;
	}

	txtime = net_pkt_timestamp_ns(pkt);
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

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", NULL, NULL, &eth_fake_data, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs, NET_ETH_MTU);

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_if **my_iface = user_data;

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		if (PART_OF_ARRAY(NET_IF_GET_NAME(eth_fake, 0), iface)) {
			*my_iface = iface;
		}
	}
}

ZTEST(net_socket_udp, test_17_setup_eth)
{
	struct net_if_addr *ifaddr;
	int ret;

	net_if_foreach(iface_cb, &eth_iface);
	zassert_not_null(eth_iface, "No ethernet interface found");

	ifaddr = net_if_ipv6_addr_add(eth_iface, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	net_if_up(eth_iface);

	(void)memset(&udp_server_addr, 0, sizeof(udp_server_addr));
	udp_server_addr.sin6_family = AF_INET6;
	udp_server_addr.sin6_port = htons(1234);
	ret = inet_pton(AF_INET6, PEER_IPV6_ADDR_ETH, &udp_server_addr.sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* In order to avoid neighbor discovery, populate neighbor cache */
	net_ipv6_nbr_add(eth_iface, &udp_server_addr.sin6_addr, &server_link_addr,
			 true, NET_IPV6_NBR_STATE_REACHABLE);
}

ZTEST_USER(net_socket_udp, test_18_v6_sendmsg_with_txtime)
{
	int rv;
	int client_sock;
	bool optval;
	net_time_t txtime;
	struct sockaddr_in6 client_addr;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(uint64_t))];
	} cmsgbuf;

	prepare_sock_udp_v6(MY_IPV6_ADDR_ETH, ANY_PORT, &client_sock, &client_addr);

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
	msg.msg_name = &udp_server_addr;
	msg.msg_namelen = sizeof(udp_server_addr);

	txtime = TEST_TXTIME;

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(txtime));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_TXTIME;
	*(net_time_t *)CMSG_DATA(cmsg) = txtime;

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

void test_msg_trunc(int sock_c, int sock_s, struct sockaddr *addr_c,
		    socklen_t addrlen_c, struct sockaddr *addr_s,
		    socklen_t addrlen_s)
{
	int rv;
	uint8_t str_buf[sizeof(TEST_STR_SMALL) - 1];

	rv = bind(sock_s, addr_s, addrlen_s);
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = connect(sock_c, addr_s, addrlen_s);
	zassert_equal(rv, 0, "connect failed");

	/* MSG_TRUNC */

	rv = send(sock_c, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "send failed");

	memset(str_buf, 0, sizeof(str_buf));
	rv = recv(sock_s, str_buf, 2, ZSOCK_MSG_TRUNC);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "MSG_TRUNC flag failed");
	zassert_mem_equal(str_buf, TEST_STR_SMALL, 2, "invalid rx data");
	zassert_equal(str_buf[2], 0, "received more than requested");

	/* The remaining data should've been discarded */
	rv = recv(sock_s, str_buf, sizeof(str_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, -1, "consecutive recv should've failed");
	zassert_equal(errno, EAGAIN, "incorrect errno value");

	/* MSG_TRUNC & MSG_PEEK combo */

	rv = send(sock_c, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "send failed");

	memset(str_buf, 0, sizeof(str_buf));
	rv = recv(sock_s, str_buf, 2, ZSOCK_MSG_TRUNC | ZSOCK_MSG_PEEK);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "MSG_TRUNC flag failed");

	/* The packet should still be available due to MSG_PEEK */
	rv = recv(sock_s, str_buf, sizeof(str_buf), ZSOCK_MSG_TRUNC);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1,
		      "recv after MSG_PEEK failed");
	zassert_mem_equal(str_buf, BUF_AND_SIZE(TEST_STR_SMALL),
			  "invalid rx data");

	rv = close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock_s);
	zassert_equal(rv, 0, "close failed");
}

ZTEST(net_socket_udp, test_19_v4_msg_trunc)
{
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	test_msg_trunc(client_sock, server_sock,
		       (struct sockaddr *)&client_addr, sizeof(client_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr));
}

ZTEST(net_socket_udp, test_20_v6_msg_trunc)
{
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

	test_msg_trunc(client_sock, server_sock,
		       (struct sockaddr *)&client_addr, sizeof(client_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr));
}

static void test_dgram_overflow(int sock_c, int sock_s,
				struct sockaddr *addr_c, socklen_t addrlen_c,
				struct sockaddr *addr_s, socklen_t addrlen_s,
				const void *buf, size_t buf_size)
{
	int rv;

	rv = bind(sock_s, addr_s, addrlen_s);
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = connect(sock_c, addr_s, addrlen_s);
	zassert_equal(rv, 0, "connect failed");

	rv = send(sock_c, buf, buf_size, 0);
	zassert_equal(rv, -1, "send succeeded");
	zassert_equal(errno, ENOMEM, "incorrect errno value");

	rv = close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock_s);
	zassert_equal(rv, 0, "close failed");
}

static void test_dgram_fragmented(int sock_c, int sock_s,
				  struct sockaddr *addr_c, socklen_t addrlen_c,
				  struct sockaddr *addr_s, socklen_t addrlen_s,
				  const void *buf, size_t buf_size)
{
	int rv;

	rv = bind(sock_s, addr_s, addrlen_s);
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = connect(sock_c, addr_s, addrlen_s);
	zassert_equal(rv, 0, "connect failed");

	rv = send(sock_c, buf, buf_size, 0);
	zassert_equal(rv, buf_size, "send failed");

	memset(rx_buf, 0, sizeof(rx_buf));
	rv = recv(sock_s, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(rv, buf_size, "recv failed");
	zassert_mem_equal(rx_buf, buf, buf_size, "wrong data");

	rv = close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock_s);
	zassert_equal(rv, 0, "close failed");
}

ZTEST(net_socket_udp, test_21_v4_dgram_overflow)
{
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	test_dgram_overflow(client_sock, server_sock,
			    (struct sockaddr *)&client_addr, sizeof(client_addr),
			    (struct sockaddr *)&server_addr, sizeof(server_addr),
			    test_str_all_tx_bufs, NET_ETH_MTU + 1);
}

ZTEST(net_socket_udp, test_22_v6_dgram_fragmented_or_overflow)
{
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

	if (IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT)) {
		test_dgram_fragmented(client_sock, server_sock,
				      (struct sockaddr *)&client_addr, sizeof(client_addr),
				      (struct sockaddr *)&server_addr, sizeof(server_addr),
				      test_str_all_tx_bufs, NET_ETH_MTU + 1);
	} else {
		test_dgram_overflow(client_sock, server_sock,
				    (struct sockaddr *)&client_addr, sizeof(client_addr),
				    (struct sockaddr *)&server_addr, sizeof(server_addr),
				    test_str_all_tx_bufs, NET_ETH_MTU + 1);
	}
}

ZTEST(net_socket_udp, test_23_v6_dgram_overflow)
{
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

	test_dgram_overflow(client_sock, server_sock,
			    (struct sockaddr *)&client_addr, sizeof(client_addr),
			    (struct sockaddr *)&server_addr, sizeof(server_addr),
			    BUF_AND_SIZE(test_str_all_tx_bufs));
}

static void test_dgram_connected(int sock_c, int sock_s1, int sock_s2,
				 struct sockaddr *addr_c, socklen_t addrlen_c,
				 struct sockaddr *addr_s1, socklen_t addrlen_s1,
				 struct sockaddr *addr_s2, socklen_t addrlen_s2)
{
	uint8_t tx_buf = 0xab;
	uint8_t rx_buf;
	int rv;

	rv = bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = bind(sock_s1, addr_s1, addrlen_s1);
	zassert_equal(rv, 0, "server bind failed");

	rv = bind(sock_s2, addr_s2, addrlen_s2);
	zassert_equal(rv, 0, "server bind failed");

	rv = connect(sock_c, addr_s1, addrlen_s1);
	zassert_equal(rv, 0, "connect failed");

	/* Verify that a datagram can be received from the connected address */
	rv = sendto(sock_s1, &tx_buf, sizeof(tx_buf), 0, addr_c, addrlen_c);
	zassert_equal(rv, sizeof(tx_buf), "send failed %d", errno);

	/* Give the packet a chance to go through the net stack */
	k_msleep(10);

	rx_buf = 0;
	rv = recv(sock_c, &rx_buf, sizeof(rx_buf), MSG_DONTWAIT);
	zassert_equal(rv, sizeof(rx_buf), "recv failed");
	zassert_equal(rx_buf, tx_buf, "wrong data");

	/* Verify that a datagram is not received from other address */
	rv = sendto(sock_s2, &tx_buf, sizeof(tx_buf), 0, addr_c, addrlen_c);
	zassert_equal(rv, sizeof(tx_buf), "send failed");

	/* Give the packet a chance to go through the net stack */
	k_msleep(10);

	rv = recv(sock_c, &rx_buf, sizeof(rx_buf), MSG_DONTWAIT);
	zassert_equal(rv, -1, "recv should've failed");
	zassert_equal(errno, EAGAIN, "incorrect errno");

	rv = close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock_s1);
	zassert_equal(rv, 0, "close failed");
	rv = close(sock_s2);
	zassert_equal(rv, 0, "close failed");
}

ZTEST(net_socket_udp, test_24_v4_dgram_connected)
{
	int client_sock;
	int server_sock_1;
	int server_sock_2;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr_1;
	struct sockaddr_in server_addr_2;

	prepare_sock_udp_v4(MY_IPV4_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock_1, &server_addr_1);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT + 1, &server_sock_2, &server_addr_2);

	test_dgram_connected(client_sock, server_sock_1, server_sock_2,
			     (struct sockaddr *)&client_addr, sizeof(client_addr),
			     (struct sockaddr *)&server_addr_1, sizeof(server_addr_1),
			     (struct sockaddr *)&server_addr_2, sizeof(server_addr_2));
}

ZTEST(net_socket_udp, test_25_v6_dgram_connected)
{
	int client_sock;
	int server_sock_1;
	int server_sock_2;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr_1;
	struct sockaddr_in6 server_addr_2;

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock_1, &server_addr_1);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT + 1, &server_sock_2, &server_addr_2);

	test_dgram_connected(client_sock, server_sock_1, server_sock_2,
			     (struct sockaddr *)&client_addr, sizeof(client_addr),
			     (struct sockaddr *)&server_addr_1, sizeof(server_addr_1),
			     (struct sockaddr *)&server_addr_2, sizeof(server_addr_2));
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	for (int i = 0; i < CONFIG_POSIX_MAX_FDS; ++i) {
		(void)zsock_close(i);
	}
}

ZTEST_SUITE(net_socket_udp, NULL, NULL, NULL, after, NULL);
