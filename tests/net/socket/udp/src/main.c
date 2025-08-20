/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 * Copyright 2025 NXP
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
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>

#include "ipv6.h"
#include "net_private.h"
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
#define MY_MCAST_IPV4_ADDR "224.0.0.1"
#define MY_MCAST_IPV6_ADDR "ff01::1"

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

	sent = zsock_sendto(client_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL),
			    0, server_addr, server_addrlen);
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed");

	k_msleep(100);

	avail = 42;
	zassert_ok(zsock_ioctl(server_sock, ZFD_IOCTL_FIONREAD, &avail));
	zassert_equal(avail, strlen(TEST_STR_SMALL));

	/* Test recvfrom(MSG_PEEK) */
	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	recved = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
				ZSOCK_MSG_PEEK, &addr, &addrlen);
	zassert_true(recved >= 0, "recvfrom fail");
	zassert_equal(recved, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_mem_equal(rx_buf, BUF_AND_SIZE(TEST_STR_SMALL), "wrong data");
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Test normal recvfrom() */
	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	recved = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
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

	sent = zsock_sendto(server_sock, TEST_STR2, sizeof(TEST_STR2) - 1,
			    0, &addr, addrlen);
	zassert_equal(sent, STRLEN(TEST_STR2), "sendto failed");

	/* Test normal recvfrom() */
	addrlen2 = sizeof(addr);
	clear_buf(rx_buf);
	recved = zsock_recvfrom(client_sock, rx_buf, sizeof(rx_buf),
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
	sent = zsock_sendto(server_sock, TEST_STR2, sizeof(TEST_STR2) - 1,
			    0, &addr, addrlen);
	zassert_equal(sent, STRLEN(TEST_STR2), "sendto failed");
	sent = zsock_sendto(server_sock, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1,
			    0, &addr, addrlen);
	zassert_equal(sent, STRLEN(TEST_STR_SMALL), "sendto failed");

	/* Receive just beginning of 1st datagram */
	addrlen2 = sizeof(addr);
	clear_buf(rx_buf);
	recved = zsock_recvfrom(client_sock, rx_buf, 16, 0, &addr2, &addrlen2);
	zassert_true(recved == 16, "recvfrom fail");
	zassert_mem_equal(rx_buf, TEST_STR2, 16, "wrong data");

	/* Make sure that now we receive 2nd datagram */
	addrlen2 = sizeof(addr);
	clear_buf(rx_buf);
	recved = zsock_recvfrom(client_sock, rx_buf, 16, 0, &addr2, &addrlen2);
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

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr,
			sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(client_sock,
			(struct sockaddr *)&client_addr, sizeof(client_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(client_sock,
			(struct sockaddr *)&client_addr, sizeof(client_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(sock1, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_connect(sock2, (struct sockaddr *)&conn_addr, sizeof(conn_addr));
	zassert_equal(rv, 0, "connect failed");

	len = zsock_send(sock2, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	clear_buf(buf);
	len = zsock_recv(sock1, buf, sizeof(buf), ZSOCK_MSG_PEEK);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "Invalid recv len");
	zassert_mem_equal(buf, BUF_AND_SIZE(TEST_STR_SMALL), "Wrong data");

	clear_buf(buf);
	len = zsock_recv(sock1, buf, sizeof(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "Invalid recv len");
	zassert_mem_equal(buf, BUF_AND_SIZE(TEST_STR_SMALL), "Wrong data");

	rv = zsock_close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock2);
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

	rv = zsock_bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	optval = 2;
	rv = zsock_setsockopt(sock1, SOL_SOCKET, SO_PRIORITY, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval = 6;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_PRIORITY, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed");

	rv = zsock_close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock2);
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

	sent = zsock_sendmsg(client_sock, client_msg, 0);
	zassert_true(sent > 0, "sendmsg failed (%d)", -errno);

	for (i = 0, len = 0; i < client_msg->msg_iovlen; i++) {
		len += client_msg->msg_iov[i].iov_len;
	}

	zassert_equal(sent, len, "iovec len (%d) vs sent (%d)", len, sent);

	/* Test recvfrom(MSG_PEEK) */
	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	recved = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			  ZSOCK_MSG_PEEK, &addr, &addrlen);
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
	recved = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
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

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr,
			sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock,
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

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr,
			sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock,
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

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock,
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

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr,
			sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock,
			(struct sockaddr *)&client_addr,
			sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	rv = zsock_connect(client_sock, (struct sockaddr *)&server_addr,
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

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock,
			(struct sockaddr *)&client_addr,
			sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	rv = zsock_connect(client_sock, (struct sockaddr *)&server_addr,
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

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
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

	rv = zsock_getsockopt(sock1, SOL_SOCKET, SO_TYPE, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_DGRAM, "getsockopt got invalid type");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_getsockopt(sock2, SOL_SOCKET, SO_TYPE, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, SOCK_DGRAM, "getsockopt got invalid type");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock2);
	zassert_equal(rv, 0, "close failed");
}

ZTEST(net_socket_udp, test_08_so_txtime)
{
	struct sockaddr_in bind_addr4;
	struct sockaddr_in6 bind_addr6;
	int sock1, sock2, rv;
	socklen_t optlen;
	int optval;

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

	rv = zsock_bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	optval = true;
	rv = zsock_setsockopt(sock1, SOL_SOCKET, SO_TXTIME, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval = false;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_TXTIME, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed");

	optlen = sizeof(optval);
	rv = zsock_getsockopt(sock1, SOL_SOCKET, SO_TXTIME, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));
	zassert_equal(optval, true, "getsockopt txtime");

	optlen = sizeof(optval);
	rv = zsock_getsockopt(sock2, SOL_SOCKET, SO_TXTIME, &optval, &optlen);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));
	zassert_equal(optval, false, "getsockopt txtime");

	rv = zsock_close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock2);
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
		.tv_sec = 0,
		.tv_usec = 300000,
	};

	prepare_sock_udp_v4(MY_IPV4_ADDR, 55555, &sock1, &bind_addr4);
	prepare_sock_udp_v6(MY_IPV6_ADDR, 55555, &sock2, &bind_addr6);

	rv = zsock_bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_setsockopt(sock1, SOL_SOCKET, SO_RCVTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 400000;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_RCVTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	addrlen = sizeof(addr);
	clear_buf(rx_buf);
	start_time = k_uptime_get_32();
	recved = zsock_recvfrom(sock1, rx_buf, sizeof(rx_buf),
				0, &addr, &addrlen);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 300, "Expected timeout after 300ms but "
			"was %dms", time_diff);

	start_time = k_uptime_get_32();
	recved = zsock_recvfrom(sock2, rx_buf, sizeof(rx_buf),
				0, &addr, &addrlen);
	time_diff = k_uptime_get_32() - start_time;

	zassert_equal(recved, -1, "Unexpected return code");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
	zassert_true(time_diff >= 400, "Expected timeout after 400ms but "
			"was %dms", time_diff);

	rv = zsock_close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock2);
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

	rv = zsock_bind(sock1, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_bind(sock2, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	zassert_equal(rv, 0, "bind failed");

	rv = zsock_setsockopt(sock1, SOL_SOCKET, SO_SNDTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	optval.tv_usec = 0;
	rv = zsock_setsockopt(sock2, SOL_SOCKET, SO_SNDTIMEO, &optval,
			      sizeof(optval));
	zassert_equal(rv, 0, "setsockopt failed");

	rv = zsock_close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock2);
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

	rv = zsock_getsockopt(sock1, SOL_SOCKET, SO_PROTOCOL, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_UDP, "getsockopt got invalid protocol");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_getsockopt(sock2, SOL_SOCKET, SO_PROTOCOL, &optval, &optsize);
	zassert_equal(rv, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, IPPROTO_UDP, "getsockopt got invalid protocol");
	zassert_equal(optsize, sizeof(optval), "getsockopt got invalid size");

	rv = zsock_close(sock1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock2);
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

	sent = zsock_sendmsg(client_sock, client_msg, 0);
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
static struct net_if *lo0;
static ZTEST_BMEM bool test_started;
static ZTEST_BMEM bool test_failed;
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in_addr my_addr2 = { { { 192, 0, 2, 2 } } };
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x3 } } };
static struct in6_addr my_mcast_addr1 = { { { 0xff, 0x01, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in_addr my_mcast_addr2 = { { { 224, 0, 0, 2 } } };
static struct net_linkaddr server_link_addr = {
	.addr = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 },
	.len = NET_ETH_ADDR_LEN,
};
#define MY_IPV6_ADDR_ETH   "2001:db8:100::1"
#define PEER_IPV6_ADDR_ETH "2001:db8:100::2"
#define TEST_TXTIME INT64_MAX
#define WAIT_TIME K_MSEC(250)
#define WAIT_TIME_LONG K_MSEC(1000)

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

	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		lo0 = iface;
		net_if_set_default(iface);
	}
}

ZTEST(net_socket_udp, test_17_setup_eth_for_ipv6)
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
	ret = zsock_inet_pton(AF_INET6, PEER_IPV6_ADDR_ETH, &udp_server_addr.sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* In order to avoid neighbor discovery, populate neighbor cache */
	net_ipv6_nbr_add(eth_iface, &udp_server_addr.sin6_addr, &server_link_addr,
			 true, NET_IPV6_NBR_STATE_REACHABLE);
}

ZTEST_USER(net_socket_udp, test_18_v6_sendmsg_with_txtime)
{
	int rv;
	int client_sock;
	int optval;
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

	rv = zsock_bind(client_sock,
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
	rv = zsock_setsockopt(client_sock, SOL_SOCKET, SO_TXTIME, &optval,
			      sizeof(optval));

	test_started = true;

	comm_sendmsg_with_txtime(client_sock,
				 (struct sockaddr *)&client_addr,
				 sizeof(client_addr),
				 &msg);

	rv = zsock_close(client_sock);
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

	rv = zsock_bind(sock_s, addr_s, addrlen_s);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = zsock_connect(sock_c, addr_s, addrlen_s);
	zassert_equal(rv, 0, "connect failed");

	/* MSG_TRUNC */

	rv = zsock_send(sock_c, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "send failed");

	memset(str_buf, 0, sizeof(str_buf));
	rv = zsock_recv(sock_s, str_buf, 2, ZSOCK_MSG_TRUNC);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "MSG_TRUNC flag failed");
	zassert_mem_equal(str_buf, TEST_STR_SMALL, 2, "invalid rx data");
	zassert_equal(str_buf[2], 0, "received more than requested");

	/* The remaining data should've been discarded */
	rv = zsock_recv(sock_s, str_buf, sizeof(str_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, -1, "consecutive recv should've failed");
	zassert_equal(errno, EAGAIN, "incorrect errno value");

	/* MSG_TRUNC & MSG_PEEK combo */

	rv = zsock_send(sock_c, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "send failed");

	memset(str_buf, 0, sizeof(str_buf));
	rv = zsock_recv(sock_s, str_buf, 2, ZSOCK_MSG_TRUNC | ZSOCK_MSG_PEEK);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1, "MSG_TRUNC flag failed");

	/* The packet should still be available due to MSG_PEEK */
	rv = zsock_recv(sock_s, str_buf, sizeof(str_buf), ZSOCK_MSG_TRUNC);
	zassert_equal(rv, sizeof(TEST_STR_SMALL) - 1,
		      "recv after MSG_PEEK failed");
	zassert_mem_equal(str_buf, BUF_AND_SIZE(TEST_STR_SMALL),
			  "invalid rx data");

	rv = zsock_close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock_s);
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

	rv = zsock_bind(sock_s, addr_s, addrlen_s);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = zsock_connect(sock_c, addr_s, addrlen_s);
	zassert_equal(rv, 0, "connect failed");

	rv = zsock_send(sock_c, buf, buf_size, 0);
	zassert_equal(rv, -1, "send succeeded");
	zassert_equal(errno, ENOMEM, "incorrect errno value");

	rv = zsock_close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock_s);
	zassert_equal(rv, 0, "close failed");
}

static void test_dgram_fragmented(int sock_c, int sock_s,
				  struct sockaddr *addr_c, socklen_t addrlen_c,
				  struct sockaddr *addr_s, socklen_t addrlen_s,
				  const void *buf, size_t buf_size)
{
	int rv;

	rv = zsock_bind(sock_s, addr_s, addrlen_s);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = zsock_connect(sock_c, addr_s, addrlen_s);
	zassert_equal(rv, 0, "connect failed");

	rv = zsock_send(sock_c, buf, buf_size, 0);
	zassert_equal(rv, buf_size, "send failed");

	memset(rx_buf, 0, sizeof(rx_buf));
	rv = zsock_recv(sock_s, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(rv, buf_size, "recv failed");
	zassert_mem_equal(rx_buf, buf, buf_size, "wrong data");

	rv = zsock_close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock_s);
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

	rv = zsock_bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = zsock_bind(sock_s1, addr_s1, addrlen_s1);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(sock_s2, addr_s2, addrlen_s2);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_connect(sock_c, addr_s1, addrlen_s1);
	zassert_equal(rv, 0, "connect failed");

	/* Verify that a datagram can be received from the connected address */
	rv = zsock_sendto(sock_s1, &tx_buf, sizeof(tx_buf), 0, addr_c, addrlen_c);
	zassert_equal(rv, sizeof(tx_buf), "send failed %d", errno);

	/* Give the packet a chance to go through the net stack */
	k_msleep(10);

	rx_buf = 0;
	rv = zsock_recv(sock_c, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, sizeof(rx_buf), "recv failed");
	zassert_equal(rx_buf, tx_buf, "wrong data");

	/* Verify that a datagram is not received from other address */
	rv = zsock_sendto(sock_s2, &tx_buf, sizeof(tx_buf), 0, addr_c, addrlen_c);
	zassert_equal(rv, sizeof(tx_buf), "send failed");

	/* Give the packet a chance to go through the net stack */
	k_msleep(10);

	rv = zsock_recv(sock_c, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, -1, "recv should've failed");
	zassert_equal(errno, EAGAIN, "incorrect errno");

	rv = zsock_close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock_s1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock_s2);
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

ZTEST_USER(net_socket_udp, test_26_recvmsg_invalid)
{
	struct msghdr msg;
	struct sockaddr_in6 server_addr;
	struct cmsghdr *cmsg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(int))];
	} cmsgbuf;
	int ret;

	/* Userspace is needed for this test */
	Z_TEST_SKIP_IFNDEF(CONFIG_USERSPACE);

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	ret = zsock_recvmsg(0, NULL, 0);
	zassert_true(ret < 0 && errno == EINVAL, "Wrong errno (%d)", errno);

	/* Set various pointers to NULL or invalid value which should cause failure */
	memset(&msg, 0, sizeof(msg));
	msg.msg_controllen = sizeof(cmsgbuf.buf);

	ret = zsock_recvmsg(0, &msg, 0);
	zassert_true(ret < 0, "recvmsg() succeed");

	msg.msg_control = &cmsgbuf.buf;

	ret = zsock_recvmsg(0, &msg, 0);
	zassert_true(ret < 0 && errno == ENOMEM, "Wrong errno (%d)", errno);

	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = (void *)1;
	msg.msg_namelen = sizeof(server_addr);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = 1122;
	*(int *)CMSG_DATA(cmsg) = 42;

	ret = zsock_recvmsg(0, &msg, 0);
	zassert_true(ret < 0, "recvmsg() succeed");
}

static void comm_sendmsg_recvmsg(int client_sock,
				 struct sockaddr *client_addr,
				 socklen_t client_addrlen,
				 const struct msghdr *client_msg,
				 int server_sock,
				 struct sockaddr *server_addr,
				 socklen_t server_addrlen,
				 struct msghdr *msg,
				 void *cmsgbuf, int cmsgbuf_len,
				 bool expect_control_data)
{
#define MAX_BUF_LEN 64
#define SMALL_BUF_LEN (sizeof(TEST_STR_SMALL) - 1 - 2)
	char buf[MAX_BUF_LEN];
	char buf2[SMALL_BUF_LEN];
	struct iovec io_vector[2];
	ssize_t sent;
	ssize_t recved;
	struct sockaddr addr;
	socklen_t addrlen = server_addrlen;
	int len, i;

	zassert_not_null(client_addr, "null client addr");
	zassert_not_null(server_addr, "null server addr");

	/*
	 * Test client -> server sending
	 */

	sent = zsock_sendmsg(client_sock, client_msg, 0);
	zassert_true(sent > 0, "sendmsg failed, %s (%d)", strerror(errno), -errno);

	/* One negative test with invalid msg_iov */
	memset(msg, 0, sizeof(*msg));
	recved = zsock_recvmsg(server_sock, msg, 0);
	zassert_true(recved < 0 && errno == ENOMEM, "Wrong errno (%d)", errno);

	for (i = 0, len = 0; i < client_msg->msg_iovlen; i++) {
		len += client_msg->msg_iov[i].iov_len;
	}

	zassert_equal(sent, len, "iovec len (%d) vs sent (%d)", len, sent);

	/* Test first with one iovec */
	io_vector[0].iov_base = buf;
	io_vector[0].iov_len = sizeof(buf);

	memset(msg, 0, sizeof(*msg));
	if (cmsgbuf != NULL) {
		memset(cmsgbuf, 0, cmsgbuf_len);
	}
	msg->msg_control = cmsgbuf;
	msg->msg_controllen = cmsgbuf_len;
	msg->msg_iov = io_vector;
	msg->msg_iovlen = 1;
	msg->msg_name = &addr;
	msg->msg_namelen = addrlen;

	/* Test recvmsg(MSG_PEEK) */
	recved = zsock_recvmsg(server_sock, msg, ZSOCK_MSG_PEEK);
	zassert_true(recved > 0, "recvmsg fail, %s (%d)", strerror(errno), -errno);
	zassert_equal(recved, len, "unexpected received bytes (%d vs %d)",
		      recved, len);
	zassert_equal(sent, recved, "sent(%d)/received(%d) mismatch",
		      sent, recved);
	zassert_equal(msg->msg_iovlen, 1, "recvmsg should not modify msg_iovlen");
	zassert_equal(msg->msg_iov[0].iov_len, sizeof(buf),
		      "recvmsg should not modify buffer length");
	zassert_mem_equal(buf, TEST_STR_SMALL, len,
			  "wrong data (%s)", rx_buf);
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Test normal recvmsg() */
	clear_buf(rx_buf);
	recved = zsock_recvmsg(server_sock, msg, 0);
	zassert_true(recved > 0, "recvfrom fail");
	zassert_equal(recved, len, "unexpected received bytes");
	zassert_equal(msg->msg_iovlen, 1, "recvmsg should not modify msg_iovlen");
	zassert_equal(msg->msg_iov[0].iov_len, sizeof(buf),
		      "recvmsg should not modify buffer length");
	zassert_mem_equal(buf, TEST_STR_SMALL, len,
			  "wrong data (%s)", rx_buf);
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Control data should be empty */
	if (!expect_control_data) {
		zassert_equal(msg->msg_controllen, 0,
			      "We received control data (%u vs %zu)",
			      0U, msg->msg_controllen);
	}

	/* Check the client port */
	if (addr.sa_family == AF_INET) {
		if (net_sin(client_addr)->sin_port != ANY_PORT) {
			zassert_equal(net_sin(client_addr)->sin_port,
				      net_sin(&addr)->sin_port,
				      "unexpected client port");
		}
	}

	if (addr.sa_family == AF_INET6) {
		if (net_sin6(client_addr)->sin6_port != ANY_PORT) {
			zassert_equal(net_sin6(client_addr)->sin6_port,
				      net_sin6(&addr)->sin6_port,
				      "unexpected client port");
		}
	}

	/* Then send the message again and verify that we could receive
	 * the full message in smaller chunks too.
	 */
	sent = zsock_sendmsg(client_sock, client_msg, 0);
	zassert_true(sent > 0, "sendmsg failed (%d)", -errno);

	for (i = 0, len = 0; i < client_msg->msg_iovlen; i++) {
		len += client_msg->msg_iov[i].iov_len;
	}

	zassert_equal(sent, len, "iovec len (%d) vs sent (%d)", len, sent);

	/* and then test with two iovec */
	io_vector[0].iov_base = buf2;
	io_vector[0].iov_len = sizeof(buf2);
	io_vector[1].iov_base = buf;
	io_vector[1].iov_len = sizeof(buf);

	memset(msg, 0, sizeof(*msg));
	if (cmsgbuf != NULL) {
		memset(cmsgbuf, 0, cmsgbuf_len);
	}
	msg->msg_control = cmsgbuf;
	msg->msg_controllen = cmsgbuf_len;
	msg->msg_iov = io_vector;
	msg->msg_iovlen = 2;
	msg->msg_name = &addr;
	msg->msg_namelen = addrlen;

	/* Test recvmsg(MSG_PEEK) */
	recved = zsock_recvmsg(server_sock, msg, ZSOCK_MSG_PEEK);
	zassert_true(recved >= 0, "recvfrom fail (errno %d)", errno);
	zassert_equal(recved, len,
		      "unexpected received bytes (%d vs %d)", recved, len);
	zassert_equal(sent, recved, "sent(%d)/received(%d) mismatch",
		      sent, recved);

	zassert_equal(msg->msg_iovlen, 2, "recvmsg should not modify msg_iovlen");
	zassert_equal(msg->msg_iov[0].iov_len, sizeof(buf2),
		      "recvmsg should not modify buffer length");
	zassert_equal(msg->msg_iov[1].iov_len, sizeof(buf),
		      "recvmsg should not modify buffer length");
	zassert_mem_equal(msg->msg_iov[0].iov_base, TEST_STR_SMALL, msg->msg_iov[0].iov_len,
			  "wrong data in %s", "iov[0]");
	zassert_mem_equal(msg->msg_iov[1].iov_base, &TEST_STR_SMALL[msg->msg_iov[0].iov_len],
			  len - msg->msg_iov[0].iov_len,
			  "wrong data in %s", "iov[1]");
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Test normal recvfrom() */
	recved = zsock_recvmsg(server_sock, msg, ZSOCK_MSG_PEEK);
	zassert_true(recved >= 0, "recvfrom fail (errno %d)", errno);
	zassert_equal(recved, len,
		      "unexpected received bytes (%d vs %d)", recved, len);
	zassert_equal(sent, recved, "sent(%d)/received(%d) mismatch",
		      sent, recved);

	zassert_equal(msg->msg_iovlen, 2, "recvmsg should not modify msg_iovlen");
	zassert_equal(msg->msg_iov[0].iov_len, sizeof(buf2),
		      "recvmsg should not modify buffer length");
	zassert_equal(msg->msg_iov[1].iov_len, sizeof(buf),
		      "recvmsg should not modify buffer length");
	zassert_mem_equal(msg->msg_iov[0].iov_base, TEST_STR_SMALL, msg->msg_iov[0].iov_len,
			  "wrong data in %s", "iov[0]");
	zassert_mem_equal(msg->msg_iov[1].iov_base, &TEST_STR_SMALL[msg->msg_iov[0].iov_len],
			  len - msg->msg_iov[0].iov_len,
			  "wrong data in %s", "iov[1]");
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Control data should be empty */
	if (!expect_control_data) {
		zassert_equal(msg->msg_controllen, 0,
			      "We received control data (%u vs %zu)",
			      0U, msg->msg_controllen);
	}

	/* Then check that the trucation flag is set correctly */
	sent = zsock_sendmsg(client_sock, client_msg, 0);
	zassert_true(sent > 0, "sendmsg failed (%d)", -errno);

	for (i = 0, len = 0; i < client_msg->msg_iovlen; i++) {
		len += client_msg->msg_iov[i].iov_len;
	}

	zassert_equal(sent, len, "iovec len (%d) vs sent (%d)", len, sent);

	/* Test first with one iovec */
	io_vector[0].iov_base = buf2;
	io_vector[0].iov_len = sizeof(buf2);

	memset(msg, 0, sizeof(*msg));
	if (cmsgbuf != NULL) {
		memset(cmsgbuf, 0, cmsgbuf_len);
	}
	msg->msg_control = cmsgbuf;
	msg->msg_controllen = cmsgbuf_len;
	msg->msg_iov = io_vector;
	msg->msg_iovlen = 1;
	msg->msg_name = &addr;
	msg->msg_namelen = addrlen;

	/* Test recvmsg */
	recved = zsock_recvmsg(server_sock, msg, 0);
	zassert_true(recved > 0, "recvmsg fail, %s (%d)", strerror(errno), errno);
	zassert_equal(recved, sizeof(buf2),
		      "unexpected received bytes (%d vs %d)",
		      recved, sizeof(buf2));
	zassert_true(msg->msg_flags & ZSOCK_MSG_TRUNC, "Message not truncated");

	zassert_equal(msg->msg_iovlen, 1, "recvmsg should not modify msg_iovlen");
	zassert_equal(msg->msg_iov[0].iov_len, sizeof(buf2),
		      "recvmsg should not modify buffer length");
	zassert_mem_equal(buf2, TEST_STR_SMALL, sizeof(buf2),
			  "wrong data (%s)", buf2);
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Control data should be empty */
	if (!expect_control_data) {
		zassert_equal(msg->msg_controllen, 0,
			      "We received control data (%u vs %zu)",
			      0U, msg->msg_controllen);
	}
}

ZTEST_USER(net_socket_udp, test_27_recvmsg_user)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	struct msghdr msg, server_msg;
	struct iovec io_vector[1];

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr,
			sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock,
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

	comm_sendmsg_recvmsg(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     &msg,
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr),
			     &server_msg, NULL, 0, false);

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

static void run_ancillary_recvmsg_test(int client_sock,
				       struct sockaddr *client_addr,
				       int client_addr_len,
				       int server_sock,
				       struct sockaddr *server_addr,
				       int server_addr_len)
{
	int rv;
	int opt;
	int ifindex = 0;
	socklen_t optlen;
	struct sockaddr addr = { 0 };
	struct net_if *iface;
	struct msghdr msg;
	struct msghdr server_msg;
	struct iovec io_vector[1];
	struct cmsghdr *cmsg, *prevcmsg;
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	} cmsgbuf;
#define SMALL_BUF_LEN (sizeof(TEST_STR_SMALL) - 1 - 2)
	char buf[MAX_BUF_LEN];

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_CONTEXT_RECV_PKTINFO);

	rv = zsock_bind(server_sock, server_addr, server_addr_len);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock, client_addr, client_addr_len);
	zassert_equal(rv, 0, "client bind failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&cmsgbuf, 0, sizeof(cmsgbuf));

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = server_addr;
	msg.msg_namelen = server_addr_len;

	comm_sendmsg_recvmsg(client_sock,
			     client_addr,
			     client_addr_len,
			     &msg,
			     server_sock,
			     server_addr,
			     server_addr_len,
			     &server_msg,
			     &cmsgbuf.buf,
			     sizeof(cmsgbuf.buf),
			     true);

	for (prevcmsg = NULL, cmsg = CMSG_FIRSTHDR(&server_msg);
	     cmsg != NULL && prevcmsg != cmsg;
	     prevcmsg = cmsg, cmsg = CMSG_NXTHDR(&server_msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP &&
		    cmsg->cmsg_type == IP_PKTINFO) {
			net_sin(&addr)->sin_addr = ((struct in_pktinfo *)CMSG_DATA(cmsg))->ipi_addr;
			break;
		}
	}

	/* As we have not set the socket option, the address should not be set */
	if (server_addr->sa_family == AF_INET) {
		zassert_equal(net_sin(&addr)->sin_addr.s_addr, INADDR_ANY, "Source address set!");
	}

	if (server_addr->sa_family == AF_INET6) {
		zassert_true(net_sin6(&addr)->sin6_addr.s6_addr32[0] == 0 &&
			     net_sin6(&addr)->sin6_addr.s6_addr32[1] == 0 &&
			     net_sin6(&addr)->sin6_addr.s6_addr32[2] == 0 &&
			     net_sin6(&addr)->sin6_addr.s6_addr32[3] == 0,
			     "Source address set!");
	}

	opt = 1;
	optlen = sizeof(opt);
	if (server_addr->sa_family == AF_INET) {
		rv = zsock_setsockopt(server_sock, IPPROTO_IP, IP_PKTINFO, &opt, optlen);
	} else {
		rv = zsock_setsockopt(server_sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &opt, optlen);
	}
	zassert_equal(rv, 0, "setsockopt failed (%d)", -errno);

	memset(&cmsgbuf, 0, sizeof(cmsgbuf));

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = server_addr;
	msg.msg_namelen = server_addr_len;

	comm_sendmsg_recvmsg(client_sock,
			     client_addr,
			     client_addr_len,
			     &msg,
			     server_sock,
			     server_addr,
			     server_addr_len,
			     &server_msg,
			     &cmsgbuf.buf,
			     sizeof(cmsgbuf.buf),
			     true);

	for (cmsg = CMSG_FIRSTHDR(&server_msg); cmsg != NULL;
	     cmsg = CMSG_NXTHDR(&server_msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP &&
		    cmsg->cmsg_type == IP_PKTINFO) {
			net_sin(&addr)->sin_addr =
				((struct in_pktinfo *)CMSG_DATA(cmsg))->ipi_addr;
			ifindex = ((struct in_pktinfo *)CMSG_DATA(cmsg))->ipi_ifindex;
			break;
		}

		if (cmsg->cmsg_level == IPPROTO_IPV6 &&
		    cmsg->cmsg_type == IPV6_PKTINFO) {
			net_ipaddr_copy(&net_sin6(&addr)->sin6_addr,
					&((struct in6_pktinfo *)CMSG_DATA(cmsg))->ipi6_addr);
			ifindex = ((struct in6_pktinfo *)CMSG_DATA(cmsg))->ipi6_ifindex;
			break;
		}
	}

	if (server_addr->sa_family == AF_INET) {
		zassert_equal(net_sin(&addr)->sin_addr.s_addr,
			      net_sin(server_addr)->sin_addr.s_addr,
			      "Source address not set properly!");
	}

	if (server_addr->sa_family == AF_INET6) {
		zassert_mem_equal(&net_sin6(&addr)->sin6_addr,
				  &net_sin6(server_addr)->sin6_addr,
				  sizeof(struct in6_addr),
				  "Source address not set properly!");
	}

	if (!k_is_user_context()) {
		iface = net_if_get_default();
		zassert_equal(ifindex, net_if_get_by_iface(iface));
	}

	/* Make sure that the recvmsg() fails if control area is too small */
	rv = zsock_sendto(client_sock, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0,
			  server_addr, server_addr_len);
	zassert_equal(rv, STRLEN(TEST_STR_SMALL), "sendto failed (%d)", -errno);

	io_vector[0].iov_base = buf;
	io_vector[0].iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = 1; /* making sure the control buf is always too small */
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;

	rv = zsock_recvmsg(server_sock, &msg, 0);
	zassert_true(rv, "recvmsg succeed (%d)", rv);

	zassert_true(msg.msg_flags & ZSOCK_MSG_CTRUNC, "Control message not truncated");

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

ZTEST_USER(net_socket_udp, test_28_recvmsg_ancillary_ipv4_pktinfo_data_user)
{
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	int client_sock;
	int server_sock;

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	run_ancillary_recvmsg_test(client_sock,
				   (struct sockaddr *)&client_addr,
				   sizeof(client_addr),
				   server_sock,
				   (struct sockaddr *)&server_addr,
				   sizeof(server_addr));
}

ZTEST_USER(net_socket_udp, test_29_recvmsg_ancillary_ipv6_pktinfo_data_user)
{
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;
	int client_sock;
	int server_sock;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

	run_ancillary_recvmsg_test(client_sock,
				   (struct sockaddr *)&client_addr,
				   sizeof(client_addr),
				   server_sock,
				   (struct sockaddr *)&server_addr,
				   sizeof(server_addr));
}

ZTEST(net_socket_udp, test_30_setup_eth_for_ipv4)
{
	struct net_if_addr *ifaddr;

	net_if_foreach(iface_cb, &eth_iface);
	zassert_not_null(eth_iface, "No ethernet interface found");

	net_if_down(eth_iface);

	ifaddr = net_if_ipv4_addr_add(eth_iface, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");
	}

	net_if_up(eth_iface);
}

static int bind_socket(int sock, struct net_if *iface)
{
	struct sockaddr_ll addr;

	memset(&addr, 0, sizeof(addr));

	addr.sll_ifindex = net_if_get_by_iface(iface);
	addr.sll_family = AF_PACKET;

	return zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
}

static void test_check_ttl(int sock_c, int sock_s, int sock_p,
			   struct sockaddr *addr_c, socklen_t addrlen_c,
			   struct sockaddr *addr_s, socklen_t addrlen_s,
			   struct sockaddr *addr_sendto, socklen_t addrlen_sendto,
			   sa_family_t family, uint8_t expected_ttl,
			   uint8_t expected_mcast_ttl)
{
	uint8_t tx_buf = 0xab;
	uint8_t rx_buf;
	int ret, count = 10, opt;
#define IPV4_HDR_SIZE sizeof(struct net_ipv4_hdr)
#define IPV6_HDR_SIZE sizeof(struct net_ipv6_hdr)
#define UDP_HDR_SIZE sizeof(struct net_udp_hdr)
#define V4_HDR_SIZE (IPV4_HDR_SIZE + UDP_HDR_SIZE)
#define V6_HDR_SIZE (IPV6_HDR_SIZE + UDP_HDR_SIZE)
#define MAX_HDR_SIZE (IPV6_HDR_SIZE + UDP_HDR_SIZE)
	uint8_t data_to_receive[sizeof(tx_buf) + MAX_HDR_SIZE];
	struct sockaddr_ll src;
	socklen_t addrlen;
	char ifname[CONFIG_NET_INTERFACE_NAME_LEN];
	struct ifreq ifreq = { 0 };
	struct timeval timeo_optval = {
		.tv_sec = 0,
		.tv_usec = 100000,
	};
#if defined(CONFIG_NET_STATISTICS)
	struct net_stats_ip ipv4_stats_before, ipv4_stats_after;
	struct net_stats_ip ipv6_stats_before, ipv6_stats_after;
#endif

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_INTERFACE_NAME);

	ret = zsock_bind(sock_c, addr_c, addrlen_c);
	zassert_equal(ret, 0, "client bind failed");

	ret = net_if_get_name(lo0, ifname, sizeof(ifname));
	zassert_true(ret > 0, "cannot get interface name (%d)", ret);

	strncpy(ifreq.ifr_name, ifname, sizeof(ifreq.ifr_name));
	ret = zsock_setsockopt(sock_c, SOL_SOCKET, SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	zassert_equal(ret, 0, "SO_BINDTODEVICE failed, %d", -errno);

	ret = zsock_connect(sock_c, addr_s, addrlen_s);
	zassert_equal(ret, 0, "connect failed");

	ret = zsock_setsockopt(sock_s, SOL_SOCKET, SO_RCVTIMEO,
			       &timeo_optval, sizeof(timeo_optval));
	zassert_equal(ret, 0, "Cannot set receive timeout (%d)", -errno);

	while (count > 0) {
		ret = zsock_sendto(sock_c, &tx_buf, sizeof(tx_buf), 0,
				   addr_sendto, addrlen_sendto);
		zassert_equal(ret, sizeof(tx_buf), "send failed (%d)", -errno);

		ret = zsock_recv(sock_s, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT);
		if (ret > 0) {
			zassert_equal(ret, sizeof(rx_buf), "recv failed (%d)", ret);
			zassert_equal(rx_buf, tx_buf, "wrong data");
		}

		ret = zsock_recvfrom(sock_p, data_to_receive, sizeof(data_to_receive), 0,
				     (struct sockaddr *)&src, &addrlen);
		if (ret > 0) {
			int hdr_size = family == AF_INET ?
				V4_HDR_SIZE : V6_HDR_SIZE;
			zassert_equal(ret, sizeof(tx_buf) + hdr_size,
				      "Cannot receive all data (%d vs %zd) (%d)",
				      ret, sizeof(tx_buf), -errno);
			zassert_mem_equal(&data_to_receive[hdr_size], &tx_buf,
					  sizeof(tx_buf),
					  "Sent and received buffers do not match");

			if (family == AF_INET) {
				struct net_ipv4_hdr *ipv4 =
					(struct net_ipv4_hdr *)&data_to_receive[0];

				if (expected_ttl > 0) {
					zassert_equal(ipv4->ttl, expected_ttl,
						      "Invalid ttl (%d vs %d)",
						      ipv4->ttl, expected_ttl);
				} else if (expected_mcast_ttl > 0) {
					zassert_equal(ipv4->ttl, expected_mcast_ttl,
						      "Invalid mcast ttl (%d vs %d)",
						      ipv4->ttl, expected_mcast_ttl);
				}
			} else if (family == AF_INET6) {
				struct net_ipv6_hdr *ipv6 =
					(struct net_ipv6_hdr *)&data_to_receive[0];

				if (expected_ttl > 0) {
					zassert_equal(ipv6->hop_limit, expected_ttl,
						      "Invalid hop limit (%d vs %d)",
						      ipv6->hop_limit, expected_ttl);
				} else if (expected_mcast_ttl > 0) {
					zassert_equal(ipv6->hop_limit, expected_mcast_ttl,
						      "Invalid mcast hop limit (%d vs %d)",
						      ipv6->hop_limit, expected_mcast_ttl);
				}
			} else {
				zassert_true(false, "Invalid address family (%d)",
					     family);
			}

			break;
		}

		count--;
	}

	zassert_true(count > 0, "timeout while waiting data");

	if (family == AF_INET) {
		/* Set TTL to 0 and make sure the packet is dropped and not
		 * received
		 */
		int option;

		if (expected_ttl > 0) {
			option = IP_TTL;
		} else {
			option = IP_MULTICAST_TTL;
		}

		opt = 0;
		ret = zsock_setsockopt(sock_c, IPPROTO_IP, option, &opt, sizeof(opt));
		zassert_equal(ret, 0, "Cannot set %s TTL (%d)",
			      option == IP_TTL ? "unicast" : "multicast",
			      -errno);

#if defined(CONFIG_NET_STATISTICS)
		/* Get IPv4 stats and verify they are updated for dropped
		 * packets.
		 */
		net_mgmt(NET_REQUEST_STATS_GET_IPV4, lo0,
			 &ipv4_stats_before, sizeof(ipv4_stats_before));
#endif
		ret = zsock_sendto(sock_c, &tx_buf, sizeof(tx_buf), 0,
				   addr_sendto, addrlen_sendto);
		zassert_equal(ret, sizeof(tx_buf), "send failed (%d)", -errno);

#if defined(CONFIG_NET_STATISTICS)
		net_mgmt(NET_REQUEST_STATS_GET_IPV4, lo0,
			 &ipv4_stats_after, sizeof(ipv4_stats_after));

		zassert_equal(ipv4_stats_before.drop + 1,
			      ipv4_stats_after.drop,
			      "Dropped statistics not updated (%d vs %d)",
			      ipv4_stats_before.drop + 1,
			      ipv4_stats_after.drop);
#endif
		ret = zsock_recv(sock_s, &rx_buf, sizeof(rx_buf), 0);
		zassert_true(ret < 0 && errno == EAGAIN, "recv succeed (%d)", -errno);
	}

	if (family == AF_INET6) {
		/* Set hoplimit to 0 and make sure the packet is dropped and not
		 * received.
		 */
		int option;

		if (expected_ttl > 0) {
			option = IPV6_UNICAST_HOPS;
		} else {
			option = IPV6_MULTICAST_HOPS;
		}

		opt = 0;
		ret = zsock_setsockopt(sock_c, IPPROTO_IPV6, option,
				       &opt, sizeof(opt));
		zassert_equal(ret, 0, "Cannot set %s hops (%d)",
			      option == IPV6_UNICAST_HOPS ? "unicast" : "multicast",
			      -errno);

#if defined(CONFIG_NET_STATISTICS)
		/* Get IPv6 stats and verify they are updated for dropped
		 * packets.
		 */
		net_mgmt(NET_REQUEST_STATS_GET_IPV6, lo0,
			 &ipv6_stats_before, sizeof(ipv6_stats_before));
#endif
		ret = zsock_sendto(sock_c, &tx_buf, sizeof(tx_buf), 0,
				   addr_sendto, addrlen_sendto);
		zassert_equal(ret, sizeof(tx_buf), "send failed (%d)", -errno);

#if defined(CONFIG_NET_STATISTICS)
		net_mgmt(NET_REQUEST_STATS_GET_IPV6, lo0,
			 &ipv6_stats_after, sizeof(ipv6_stats_after));

		zassert_equal(ipv6_stats_before.drop + 1,
			      ipv6_stats_after.drop,
			      "Dropped statistics not updated (%d vs %d)",
			      ipv6_stats_before.drop + 1,
			      ipv6_stats_after.drop);
#endif
		ret = zsock_recv(sock_s, &rx_buf, sizeof(rx_buf), 0);
		zassert_true(ret < 0 && errno == EAGAIN, "recv succeed (%d)", -errno);

	}

	ret = zsock_close(sock_c);
	zassert_equal(ret, 0, "close failed");
	ret = zsock_close(sock_s);
	zassert_equal(ret, 0, "close failed");
	ret = zsock_close(sock_p);
	zassert_equal(ret, 0, "close failed");
}

ZTEST(net_socket_udp, test_31_v4_ttl)
{
	int ret;
	int client_sock;
	int server_sock;
	int packet_sock;
	int ttl, verify;
	socklen_t optlen;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_SOCKETS_PACKET);

	prepare_sock_udp_v4(MY_IPV4_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	packet_sock = zsock_socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(packet_sock >= 0, "Cannot create packet socket (%d)", -errno);

	ret = bind_socket(packet_sock, lo0);
	zassert_equal(ret, 0, "packet socket bind failed");

	zassert_not_null(lo0->config.ip.ipv4,
			 "Interface %d (%p) no IPv4 configured",
			 net_if_get_by_iface(lo0), lo0);

	ttl = 16;
	net_if_ipv4_set_ttl(lo0, ttl);
	verify = net_if_ipv4_get_ttl(lo0);
	zassert_equal(verify, ttl, "Different TTLs (%d vs %d)", ttl, verify);

	ttl = 128;
	ret = zsock_setsockopt(client_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
	zassert_equal(ret, 0, "Cannot set unicast TTL (%d)", -errno);

	optlen = sizeof(verify);
	ret = zsock_getsockopt(client_sock, IPPROTO_IP, IP_TTL, &verify, &optlen);
	zassert_equal(ret, 0, "Cannot get unicast TTL (%d)", -errno);
	zassert_equal(verify, ttl, "Different unicast TTL (%d vs %d)",
		      ttl, verify);

	test_check_ttl(client_sock, server_sock, packet_sock,
		       (struct sockaddr *)&client_addr, sizeof(client_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr),
		       AF_INET, ttl, 0);
}

ZTEST(net_socket_udp, test_32_v4_mcast_ttl)
{
	int ret;
	int client_sock;
	int server_sock;
	int packet_sock;
	int mcast_ttl, verify;
	socklen_t optlen;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	struct sockaddr_in sendto_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_SOCKETS_PACKET);

	prepare_sock_udp_v4(MY_IPV4_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	packet_sock = zsock_socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(packet_sock >= 0, "Cannot create packet socket (%d)", -errno);

	ret = bind_socket(packet_sock, lo0);
	zassert_equal(ret, 0, "packet socket bind failed");

	zassert_not_null(lo0->config.ip.ipv4,
			 "Interface %d (%p) no IPv4 configured",
			 net_if_get_by_iface(lo0), lo0);

	mcast_ttl = 8;
	ret = zsock_setsockopt(client_sock, IPPROTO_IP, IP_MULTICAST_TTL, &mcast_ttl,
			       sizeof(mcast_ttl));
	zassert_equal(ret, 0, "Cannot set multicast ttl (%d)", -errno);

	optlen = sizeof(verify);
	ret = zsock_getsockopt(client_sock, IPPROTO_IP, IP_MULTICAST_TTL, &verify,
			       &optlen);
	zassert_equal(ret, 0, "Cannot get multicast ttl (%d)", -errno);
	zassert_equal(verify, mcast_ttl, "Different multicast TTLs (%d vs %d)",
		      mcast_ttl, verify);

	ret = net_addr_pton(AF_INET, MY_MCAST_IPV4_ADDR, &sendto_addr.sin_addr);
	zassert_equal(ret, 0, "Cannot get IPv4 address (%d)", ret);

	test_check_ttl(client_sock, server_sock, packet_sock,
		       (struct sockaddr *)&client_addr, sizeof(client_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr),
		       (struct sockaddr *)&sendto_addr, sizeof(sendto_addr),
		       AF_INET, 0, mcast_ttl);
}

ZTEST(net_socket_udp, test_33_v6_mcast_hops)
{
	int ret;
	int client_sock;
	int server_sock;
	int packet_sock;
	int mcast_hops, if_mcast_hops;
	int verify, opt;
	socklen_t optlen;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;
	struct sockaddr_in6 sendto_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_SOCKETS_PACKET);

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

	packet_sock = zsock_socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(packet_sock >= 0, "Cannot create packet socket (%d)", -errno);

	ret = bind_socket(packet_sock, lo0);
	zassert_equal(ret, 0, "packet socket bind failed");

	zassert_not_null(lo0->config.ip.ipv6,
			 "Interface %d (%p) no IPv6 configured",
			 net_if_get_by_iface(lo0), lo0);

	/* First make sure setting hop limit to -1 works as expected (route default
	 * value should be used).
	 */
	if_mcast_hops = net_if_ipv6_get_mcast_hop_limit(lo0);

	opt = -1;
	ret = zsock_setsockopt(client_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &opt,
			       sizeof(opt));
	zassert_equal(ret, 0, "Cannot set multicast hop limit (%d)", -errno);

	optlen = sizeof(verify);
	ret = zsock_getsockopt(client_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &verify,
			       &optlen);
	zassert_equal(ret, 0, "Cannot get multicast hop limit (%d)", -errno);
	zassert_equal(verify, if_mcast_hops, "Different multicast hop limit (%d vs %d)",
		      if_mcast_hops, verify);

	/* Then test the normal case where we set the value */
	mcast_hops = 8;
	ret = zsock_setsockopt(client_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &mcast_hops,
			       sizeof(mcast_hops));
	zassert_equal(ret, 0, "Cannot set multicast hop limit (%d)", -errno);

	optlen = sizeof(verify);
	ret = zsock_getsockopt(client_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &verify,
			       &optlen);
	zassert_equal(ret, 0, "Cannot get multicast hop limit (%d)", -errno);
	zassert_equal(verify, mcast_hops, "Different multicast hop limit (%d vs %d)",
		      mcast_hops, verify);

	ret = net_addr_pton(AF_INET6, MY_MCAST_IPV6_ADDR, &sendto_addr.sin6_addr);
	zassert_equal(ret, 0, "Cannot get IPv6 address (%d)", ret);

	test_check_ttl(client_sock, server_sock, packet_sock,
		       (struct sockaddr *)&client_addr, sizeof(client_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr),
		       (struct sockaddr *)&sendto_addr, sizeof(sendto_addr),
		       AF_INET6, 0, mcast_hops);
}

ZTEST(net_socket_udp, test_34_v6_hops)
{
	int ret;
	int client_sock;
	int server_sock;
	int packet_sock;
	int hops, verify;
	socklen_t optlen;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_SOCKETS_PACKET);

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

	packet_sock = zsock_socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(packet_sock >= 0, "Cannot create packet socket (%d)", -errno);

	ret = bind_socket(packet_sock, lo0);
	zassert_equal(ret, 0, "packet socket bind failed");

	zassert_not_null(lo0->config.ip.ipv6,
			 "Interface %d (%p) no IPv6 configured",
			 net_if_get_by_iface(lo0), lo0);

	hops = 16;
	net_if_ipv6_set_hop_limit(lo0, hops);
	verify = net_if_ipv6_get_hop_limit(lo0);
	zassert_equal(verify, hops, "Different hop limit (%d vs %d)", hops, verify);

	hops = 8;
	ret = zsock_setsockopt(client_sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops,
			       sizeof(hops));
	zassert_equal(ret, 0, "Cannot set unicast hops (%d)", -errno);

	optlen = sizeof(verify);
	ret = zsock_getsockopt(client_sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &verify,
			       &optlen);
	zassert_equal(ret, 0, "Cannot get unicast hops (%d)", -errno);
	zassert_equal(verify, hops, "Different unicast hops (%d vs %d)",
		      hops, verify);

	test_check_ttl(client_sock, server_sock, packet_sock,
		       (struct sockaddr *)&client_addr, sizeof(client_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr),
		       (struct sockaddr *)&server_addr, sizeof(server_addr),
		       AF_INET6, hops, 0);
}

ZTEST_USER(net_socket_udp, test_35_recvmsg_msg_controllen_update)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	struct msghdr msg, server_msg;
	struct iovec io_vector[1];
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	} cmsgbuf;

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	rv = zsock_bind(server_sock,
			(struct sockaddr *)&server_addr,
			sizeof(server_addr));
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock,
			(struct sockaddr *)&client_addr,
			sizeof(client_addr));
	zassert_equal(rv, 0, "client bind failed");

	memset(&cmsgbuf, 0, sizeof(cmsgbuf));

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &server_addr;
	msg.msg_namelen = sizeof(server_addr);

	comm_sendmsg_recvmsg(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     &msg,
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr),
			     &server_msg,
			     &cmsgbuf.buf,
			     sizeof(cmsgbuf.buf),
			     false);

	rv = zsock_close(client_sock);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(server_sock);
	zassert_equal(rv, 0, "close failed");
}

ZTEST(net_socket_udp, test_36_v6_address_removal)
{
	int ret;
	bool status;
	int client_sock;
	struct sockaddr_in6 client_addr;
	struct net_if_addr *ifaddr;
	struct net_if *iface;

	if (!IS_ENABLED(CONFIG_NET_IPV6_PE)) {
		return;
	}

	ifaddr = net_if_ipv6_addr_lookup(&my_addr1, &iface);
	zassert_equal(ifaddr->atomic_ref, 1, "Ref count is wrong (%ld vs %d)",
		      ifaddr->atomic_ref, 1);

	prepare_sock_udp_v6(MY_IPV6_ADDR_ETH, CLIENT_PORT, &client_sock, &client_addr);

	ret = zsock_bind(client_sock,
			 (struct sockaddr *)&client_addr,
			 sizeof(client_addr));
	zassert_equal(ret, 0, "client bind failed");

	status = net_if_ipv6_addr_rm(eth_iface, &my_addr1);
	zassert_false(status, "Address could be removed");

	ifaddr = net_if_ipv6_addr_lookup(&my_addr1, &iface);
	zassert_not_null(ifaddr, "Address %s not found",
			 net_sprint_ipv6_addr(&my_addr1));

	ret = zsock_close(client_sock);
	zassert_equal(ret, 0, "close failed");

	ifaddr = net_if_ipv6_addr_lookup(&my_addr1, &iface);
	zassert_equal(iface, eth_iface, "Invalid interface %p vs %p",
		      iface, eth_iface);
	zassert_is_null(ifaddr, "Address %s found",
			net_sprint_ipv6_addr(&my_addr1));
}

static void check_ipv6_address_preferences(struct net_if *iface,
					   uint16_t preference,
					   const struct in6_addr *addr,
					   const struct in6_addr *dest)
{
	const struct in6_addr *selected;
	size_t optlen;
	int optval;
	int sock;
	int ret;

	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	optval = preference;
	ret = zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_ADDR_PREFERENCES,
			       &optval, sizeof(optval));
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);

	optval = 0; optlen = 0U;
	ret = zsock_getsockopt(sock, IPPROTO_IPV6, IPV6_ADDR_PREFERENCES,
			       &optval, &optlen);
	zassert_equal(ret, 0, "setsockopt failed (%d)", errno);
	zassert_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));
	zassert_equal(optval, preference,
		      "getsockopt address preferences");

	selected = net_if_ipv6_select_src_addr_hint(iface,
						    dest,
						    preference);
	ret = net_ipv6_addr_cmp(addr, selected);
	zassert_true(ret, "Wrong address %s selected, expected %s",
		     net_sprint_ipv6_addr(selected),
		     net_sprint_ipv6_addr(addr));

	ret = zsock_close(sock);
	zassert_equal(sock, 0, "Cannot close socket (%d)", -errno);
}

ZTEST(net_socket_udp, test_37_ipv6_src_addr_select)
{
	struct net_if_addr *ifaddr;
	const struct in6_addr dest = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x2 } } };

	net_if_foreach(iface_cb, &eth_iface);
	zassert_not_null(eth_iface, "No ethernet interface found");

	ifaddr = net_if_ipv6_addr_add(eth_iface, &my_addr1,
				      NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	ifaddr->is_temporary = false;

	ifaddr = net_if_ipv6_addr_add(eth_iface, &my_addr3,
				      NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "addr1");
	}

	ifaddr->is_temporary = true;

	net_if_up(eth_iface);

	check_ipv6_address_preferences(NULL, IPV6_PREFER_SRC_PUBLIC,
				       &my_addr1, &dest);
	check_ipv6_address_preferences(NULL, IPV6_PREFER_SRC_TMP,
				       &my_addr3, &dest);
}

ZTEST(net_socket_udp, test_38_ipv6_multicast_ifindex)
{
	struct sockaddr_in6 saddr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(SERVER_PORT),
		.sin6_addr = my_mcast_addr1,
	};
	struct net_if_mcast_addr *ifmaddr;
	struct net_if_addr *ifaddr;
	int server_sock;
	size_t addrlen;
	size_t optlen;
	int ifindex;
	int optval;
	int sock;
	int ret;
	int err;

	net_if_foreach(iface_cb, &eth_iface);
	zassert_not_null(eth_iface, "No ethernet interface found");

	ifmaddr = net_if_ipv6_maddr_add(eth_iface, &my_mcast_addr1);
	if (!ifmaddr) {
		DBG("Cannot add IPv6 multicast address %s\n",
		       net_sprint_ipv6_addr(&my_mcast_addr1));
		zassert_not_null(ifmaddr, "mcast_addr1");
	}

	ifaddr = net_if_ipv6_addr_add(eth_iface, &my_addr3,
				      NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "addr1");
	}

	net_if_up(eth_iface);

	/* Check that we get the default interface */
	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	optval = 0; optlen = 0U;
	ret = zsock_getsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			       &optval, &optlen);
	zexpect_equal(ret, 0, "setsockopt failed (%d)", errno);
	zexpect_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));
	ifindex = net_if_get_by_iface(net_if_get_default());
	zexpect_equal(optval, ifindex,
		      "getsockopt multicast ifindex (expected %d got %d)",
		      ifindex, optval);

	ret = zsock_close(sock);
	zassert_equal(sock, 0, "Cannot close socket (%d)", -errno);

	/* Check failure for IPv4 socket */
	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	optval = 0; optlen = 0U;
	ret = zsock_getsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			       &optval, &optlen);
	err = -errno;
	zexpect_equal(ret, -1, "setsockopt failed (%d)", errno);
	zexpect_equal(err, -EAFNOSUPPORT, "setsockopt failed (%d)", errno);
	zexpect_equal(optlen, 0U, "setsockopt optlen (%d)", optlen);

	ret = zsock_close(sock);
	zassert_equal(sock, 0, "Cannot close socket (%d)", -errno);

	/* Check that we can set the interface */
	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	/* Clear any existing interface value by setting it to 0 */
	optval = 0; optlen = sizeof(int);
	ret = zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			       &optval, optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));

	/* Set the output multicast packet interface to the default interface */
	optval = net_if_get_by_iface(net_if_get_default()); optlen = sizeof(int);
	ret = zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			       &optval, optlen);
	zexpect_equal(ret, 0, "setsockopt failed (%d)", errno);
	zexpect_equal(optlen, sizeof(optval), "invalid optlen %d vs %d",
		      optlen, sizeof(optval));

	optval = 0; optlen = 0U;
	ret = zsock_getsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			       &optval, &optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(int), "setsockopt optlen (%d)", optlen);
	zexpect_equal(optval, net_if_get_by_iface(net_if_get_default()),
		      "getsockopt multicast ifindex (expected %d got %d)",
		      net_if_get_by_iface(net_if_get_default()), optval);

	server_sock = prepare_listen_sock_udp_v6(&saddr6);
	zassert_not_equal(server_sock, -1, "Cannot create server socket (%d)", -errno);

	test_started = true;
	loopback_enable_address_swap(false);

	ret = zsock_sendto(sock, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0,
			   (struct sockaddr *)&saddr6, sizeof(saddr6));
	zexpect_equal(ret, STRLEN(TEST_STR_SMALL),
		      "invalid send len (was %d expected %d) (%d)",
		      ret, STRLEN(TEST_STR_SMALL), -errno);

	/* Test that the sent data is received from default interface and
	 * not the Ethernet one.
	 */
	addrlen = sizeof(saddr6);
	ret = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&saddr6, &addrlen);
	zexpect_true(ret >= 0, "recvfrom fail");
	zexpect_equal(ret, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zexpect_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1,
			  "wrong data");

	ret = zsock_close(sock);
	zassert_equal(ret, 0, "Cannot close socket (%d)", -errno);

	ret = zsock_close(server_sock);
	zassert_equal(ret, 0, "Cannot close socket (%d)", -errno);

	test_started = false;
	loopback_enable_address_swap(true);
}

ZTEST(net_socket_udp, test_39_ipv4_multicast_ifindex)
{
	struct sockaddr_in saddr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(SERVER_PORT),
		.sin_addr = my_mcast_addr2,
	};
	struct sockaddr_in dst_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(SERVER_PORT),
		.sin_addr = my_mcast_addr2,
	};
	struct net_if_mcast_addr *ifmaddr;
	struct net_if_addr *ifaddr;
	struct in_addr addr = { 0 };
	struct ip_mreqn mreqn;
	struct ip_mreq mreq;
	struct net_if *iface;
	int server_sock;
	size_t addrlen;
	size_t optlen;
	int ifindex;
	int sock;
	int ret;
	int err;

	net_if_foreach(iface_cb, &eth_iface);
	zassert_not_null(eth_iface, "No ethernet interface found");

	ifmaddr = net_if_ipv4_maddr_add(eth_iface, &my_mcast_addr2);
	if (!ifmaddr) {
		DBG("Cannot add IPv4 multicast address %s\n",
		       net_sprint_ipv4_addr(&my_mcast_addr2));
		zassert_not_null(ifmaddr, "mcast_addr2");
	}

	ifaddr = net_if_ipv4_addr_add(eth_iface, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");
	}

	net_if_up(eth_iface);

	/* Check that we get the default interface */
	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	optlen = sizeof(addr);
	ret = zsock_getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &addr, &optlen);
	zexpect_equal(ret, 0, "setsockopt failed (%d)", errno);
	zexpect_equal(optlen, sizeof(addr), "invalid optlen %d vs %d",
		      optlen, sizeof(addr));
	ifindex = net_if_get_by_iface(net_if_get_default());
	ret = net_if_ipv4_addr_lookup_by_index(&addr);
	zexpect_equal(ret, ifindex,
		      "getsockopt multicast ifindex (expected %d got %d)",
		      ifindex, ret);

	ret = zsock_close(sock);
	zassert_equal(sock, 0, "Cannot close socket (%d)", -errno);

	/* Check failure for IPv6 socket */
	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	optlen = 0U;
	ret = zsock_getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &addr, &optlen);
	err = -errno;
	zexpect_equal(ret, -1, "setsockopt failed (%d)", errno);
	zexpect_equal(err, -EAFNOSUPPORT, "setsockopt failed (%d)", errno);
	zexpect_equal(optlen, 0U, "setsockopt optlen (%d)", optlen);

	ret = zsock_close(sock);
	zassert_equal(sock, 0, "Cannot close socket (%d)", -errno);

	/* Check that we can set the interface */
	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	/* Clear any existing interface value by setting it to 0 */
	optlen = sizeof(mreqn);
	mreqn.imr_ifindex = 0;
	mreqn.imr_address.s_addr = 0;

	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &mreqn, optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	/* Verify that we get the empty value */
	optlen = sizeof(addr);
	ret = zsock_getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &addr, &optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(addr), "setsockopt optlen (%d)", optlen);

	/* Set the output multicast packet interface to the default interface */
	optlen = sizeof(mreqn);
	mreqn.imr_ifindex = net_if_get_by_iface(net_if_get_default());
	mreqn.imr_address.s_addr = 0;

	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &mreqn, optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	/* Verify that we get the default interface */
	optlen = sizeof(addr);
	memset(&addr, 0, sizeof(addr));
	ret = zsock_getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &addr, &optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(addr), "setsockopt optlen (%d)", optlen);

	ifaddr = net_if_ipv4_addr_lookup(&addr, &iface);
	zexpect_not_null(ifaddr, "Address %s not found",
			 net_sprint_ipv4_addr(&addr));
	zexpect_equal(net_if_get_by_iface(iface),
		      net_if_get_by_iface(net_if_get_default()),
		      "Invalid interface %d vs %d",
		      net_if_get_by_iface(iface),
		      net_if_get_by_iface(net_if_get_default()));

	/* Now send a packet and verify that it is sent via the default
	 * interface instead of the Ethernet interface.
	 */
	server_sock = prepare_listen_sock_udp_v4(&saddr4);
	zassert_not_equal(server_sock, -1, "Cannot create server socket (%d)", -errno);

	test_started = true;
	loopback_enable_address_swap(false);

	ret = zsock_sendto(sock, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0,
			   (struct sockaddr *)&dst_addr, sizeof(dst_addr));
	zexpect_equal(ret, STRLEN(TEST_STR_SMALL),
		      "invalid send len (was %d expected %d) (%d)",
		      ret, STRLEN(TEST_STR_SMALL), -errno);

	/* Test that the sent data is received from Ethernet interface. */
	addrlen = sizeof(saddr4);
	ret = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&saddr4, &addrlen);
	zexpect_true(ret >= 0, "recvfrom fail");
	zexpect_equal(ret, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zexpect_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1,
			  "wrong data");

	/* Clear the old interface value by setting it to 0 */
	optlen = sizeof(mreqn);
	mreqn.imr_ifindex = 0;
	mreqn.imr_address.s_addr = 0;

	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &mreqn, optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	/* Then do it the other way around, set the address but leave the
	 * interface number unassigned.
	 */
	optlen = sizeof(mreqn);
	mreqn.imr_ifindex = 0;

	/* Get the address of default interface and set it as a target
	 * interface.
	 */
	ifaddr = net_if_ipv4_addr_get_first_by_index(net_if_get_by_iface(net_if_get_default()));
	zexpect_not_null(ifaddr, "No address found for interface %d",
			 net_if_get_by_iface(net_if_get_default()));
	mreqn.imr_address.s_addr = ifaddr->address.in_addr.s_addr;

	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &mreqn, optlen);
	zexpect_equal(ret, 0, "setsockopt failed (%d)", errno);

	/* Verify that we get the default interface address */
	optlen = sizeof(struct in_addr);
	ret = zsock_getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &addr, &optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(struct in_addr), "setsockopt optlen (%d)", optlen);
	ret = net_if_ipv4_addr_lookup_by_index(&addr);
	zexpect_equal(ret, net_if_get_by_iface(net_if_get_default()),
		      "getsockopt multicast ifindex (expected %d got %d)",
		      net_if_get_by_iface(net_if_get_default()), ret);
	zexpect_equal(ifaddr->address.in_addr.s_addr,
		      addr.s_addr,
		      "getsockopt iface address mismatch (expected %s got %s)",
		      net_sprint_ipv4_addr(&ifaddr->address.in_addr),
		      net_sprint_ipv4_addr(&addr));

	ret = zsock_sendto(sock, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0,
			   (struct sockaddr *)&dst_addr, sizeof(dst_addr));
	zexpect_equal(ret, STRLEN(TEST_STR_SMALL),
		      "invalid send len (was %d expected %d) (%d)",
		      ret, STRLEN(TEST_STR_SMALL), -errno);

	/* Test that the sent data is received from default interface. */
	addrlen = sizeof(saddr4);
	ret = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&saddr4, &addrlen);
	zexpect_true(ret >= 0, "recvfrom fail");
	zexpect_equal(ret, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zexpect_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1,
			  "wrong data");

	/* Then use mreq structure to set the interface */
	optlen = sizeof(mreq);
	ifaddr = net_if_ipv4_addr_get_first_by_index(net_if_get_by_iface(net_if_get_default()));
	zexpect_not_null(ifaddr, "No address found for interface %d",
			 net_if_get_by_iface(net_if_get_default()));
	mreq.imr_interface.s_addr = ifaddr->address.in_addr.s_addr;

	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &mreq, optlen);
	zexpect_equal(ret, 0, "setsockopt failed (%d)", errno);

	ret = zsock_sendto(sock, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1, 0,
			   (struct sockaddr *)&dst_addr, sizeof(dst_addr));
	zexpect_equal(ret, STRLEN(TEST_STR_SMALL),
		      "invalid send len (was %d expected %d) (%d)",
		      ret, STRLEN(TEST_STR_SMALL), -errno);

	/* Test that the sent data is received from default interface. */
	addrlen = sizeof(saddr4);
	ret = zsock_recvfrom(server_sock, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&saddr4, &addrlen);
	zexpect_true(ret >= 0, "recvfrom fail");
	zexpect_equal(ret, strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zexpect_mem_equal(rx_buf, TEST_STR_SMALL, sizeof(TEST_STR_SMALL) - 1,
			  "wrong data");

	ret = zsock_close(sock);
	zassert_equal(ret, 0, "Cannot close socket (%d)", -errno);

	ret = zsock_close(server_sock);
	zassert_equal(ret, 0, "Cannot close socket (%d)", -errno);

	test_started = false;
	loopback_enable_address_swap(true);
}

#if defined(CONFIG_NET_CONTEXT_CLAMP_PORT_RANGE)

#define PORT_RANGE(lower, upper) \
	(uint32_t)(((uint16_t)(upper) << 16) | (uint16_t)(lower))

static void check_port_range(struct sockaddr *my_addr,
			     size_t my_addr_len,
			     struct sockaddr *local_addr,
			     size_t local_addr_len)
{
	sa_family_t family = AF_UNSPEC;
	uint32_t optval;
	size_t addr_len;
	size_t optlen;
	int sock;
	int ret, err;

	addr_len = local_addr_len;

	if (my_addr->sa_family == AF_INET) {
		family = AF_INET;
	} else if (my_addr->sa_family == AF_INET6) {
		family = AF_INET6;
	} else {
		zassert_true(false, "Invalid address family %d",
			     my_addr->sa_family);
	}

	sock = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	optval = PORT_RANGE(1024, 1500);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	optval = 0; optlen = 0U;
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, optlen);
	err = -errno;
	zexpect_equal(ret, -1, "setsockopt failed (%d)", err);

	optval = 0; optlen = sizeof(uint64_t);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, optlen);
	err = -errno;
	zexpect_equal(ret, -1, "setsockopt failed (%d)", err);

	optval = PORT_RANGE(0, 0);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	/* Linux allows setting the invalid port range but that is not
	 * then taken into use when we bind the socket.
	 */
	optval = PORT_RANGE(1024, 0);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	optval = PORT_RANGE(0, 1024);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	/* Then set a valid range and verify that bound socket is using it */
	optval = PORT_RANGE(10000, 10010);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	optval = 0; optlen = sizeof(optval);
	ret = zsock_getsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, &optlen);
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optval, PORT_RANGE(10000, 10010), "Invalid port range");

	ret = zsock_bind(sock, my_addr, my_addr_len);
	err = -errno;
	zexpect_equal(ret, 0, "bind failed (%d)", err);

	ret = zsock_getsockname(sock, local_addr, &addr_len);
	err = -errno;
	zexpect_equal(ret, 0, "getsockname failed (%d)", err);

	/* The port should be in the range */
	zexpect_true(ntohs(net_sin(local_addr)->sin_port) >= 10000 &&
		     ntohs(net_sin(local_addr)->sin_port) <= 10010,
		     "Invalid port %d", ntohs(net_sin(local_addr)->sin_port));

	(void)zsock_close(sock);

	/* Try setting invalid range and verify that we do not net a port from that
	 * range.
	 */
	sock = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);

	optval = PORT_RANGE(1001, 1000);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	err = -errno;
	zexpect_equal(ret, -1, "setsockopt failed (%d)", err);
	zexpect_equal(err, -EINVAL, "Invalid errno (%d)", -err);

	/* Port range cannot be just one port */
	optval = PORT_RANGE(1001, 1001);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	err = -errno;
	zexpect_equal(ret, -1, "setsockopt failed (%d)", err);
	zexpect_equal(err, -EINVAL, "Invalid errno (%d)", -err);

	optval = PORT_RANGE(0, 1000);
	ret = zsock_setsockopt(sock, IPPROTO_IP, IP_LOCAL_PORT_RANGE,
			       &optval, sizeof(optval));
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);

	ret = zsock_bind(sock, my_addr, my_addr_len);
	err = -errno;
	zexpect_equal(ret, 0, "bind failed (%d)", err);

	addr_len = local_addr_len;
	ret = zsock_getsockname(sock, local_addr, &addr_len);
	err = -errno;
	zexpect_equal(ret, 0, "getsockname failed (%d)", err);

	/* The port should not be in the range */
	zexpect_false(ntohs(net_sin(local_addr)->sin_port) >= 1000 &&
		      ntohs(net_sin(local_addr)->sin_port) <= 1001,
		      "Invalid port %d", ntohs(net_sin(local_addr)->sin_port));

	(void)zsock_close(sock);
}
#endif

ZTEST(net_socket_udp, test_40_clamp_udp_tcp_port_range)
{
#if defined(CONFIG_NET_CONTEXT_CLAMP_PORT_RANGE)
	struct sockaddr_in my_addr4 = {
		.sin_family = AF_INET,
		.sin_port = 0,
		.sin_addr = { { { 192, 0, 2, 2 } } },
	};
	struct sockaddr_in6 my_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
		.sin6_addr = in6addr_loopback,
	};
	struct sockaddr_in local_addr4;
	struct sockaddr_in6 local_addr6;

	/* First try with a IPv4 socket */
	check_port_range((struct sockaddr *)&my_addr4, sizeof(my_addr4),
			 (struct sockaddr *)&local_addr4, sizeof(local_addr4));

	/* Finally try with a IPv6 socket */
	check_port_range((struct sockaddr *)&my_addr6, sizeof(my_addr6),
			 (struct sockaddr *)&local_addr6, sizeof(local_addr6));
#else
	ztest_test_skip();
#endif
}

static void test_dgram_peer_addr_reset(int sock_c, int sock_s1, int sock_s2,
				       struct sockaddr *addr_c, socklen_t addrlen_c,
				       struct sockaddr *addr_s1, socklen_t addrlen_s1,
				       struct sockaddr *addr_s2, socklen_t addrlen_s2)
{
	uint8_t tx_buf = 0xab;
	uint8_t rx_buf;
	struct sockaddr_storage unspec = {
		.ss_family = AF_UNSPEC,
	};
	int rv;

	rv = zsock_bind(sock_c, addr_c, addrlen_c);
	zassert_equal(rv, 0, "client bind failed");

	rv = zsock_bind(sock_s1, addr_s1, addrlen_s1);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(sock_s2, addr_s2, addrlen_s2);
	zassert_equal(rv, 0, "server bind failed");

	/* Connect client socket to a specific peer address. */
	rv = zsock_connect(sock_c, addr_s1, addrlen_s1);
	zassert_equal(rv, 0, "connect failed");

	/* Verify that a datagram is not received from other address */
	rv = zsock_sendto(sock_s2, &tx_buf, sizeof(tx_buf), 0, addr_c, addrlen_c);
	zassert_equal(rv, sizeof(tx_buf), "send failed");

	/* Give the packet a chance to go through the net stack */
	k_msleep(10);

	rv = zsock_recv(sock_c, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, -1, "recv should've failed");
	zassert_equal(errno, EAGAIN, "incorrect errno");

	/* Reset peer address */
	rv = zsock_connect(sock_c, (struct sockaddr *)&unspec, sizeof(unspec));
	zassert_equal(rv, 0, "connect failed");

	/* Verify that a datagram can be received from other address */
	rv = zsock_sendto(sock_s2, &tx_buf, sizeof(tx_buf), 0, addr_c, addrlen_c);
	zassert_equal(rv, sizeof(tx_buf), "send failed");

	/* Give the packet a chance to go through the net stack */
	k_msleep(10);

	rx_buf = 0;
	rv = zsock_recv(sock_c, &rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, sizeof(rx_buf), "recv failed %d", errno);
	zassert_equal(rx_buf, tx_buf, "wrong data");

	rv = zsock_close(sock_c);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock_s1);
	zassert_equal(rv, 0, "close failed");
	rv = zsock_close(sock_s2);
	zassert_equal(rv, 0, "close failed");
}

ZTEST(net_socket_udp, test_41_v4_dgram_peer_addr_reset)
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

	test_dgram_peer_addr_reset(client_sock, server_sock_1, server_sock_2,
				   (struct sockaddr *)&client_addr, sizeof(client_addr),
				   (struct sockaddr *)&server_addr_1, sizeof(server_addr_1),
				   (struct sockaddr *)&server_addr_2, sizeof(server_addr_2));
}

ZTEST(net_socket_udp, test_42_v6_dgram_peer_addr_reset)
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

	test_dgram_peer_addr_reset(client_sock, server_sock_1, server_sock_2,
				   (struct sockaddr *)&client_addr, sizeof(client_addr),
				   (struct sockaddr *)&server_addr_1, sizeof(server_addr_1),
				   (struct sockaddr *)&server_addr_2, sizeof(server_addr_2));
}

static void comm_sendmsg_recvmsg_hop_limit(int client_sock,
					   struct sockaddr *client_addr,
					   socklen_t client_addrlen,
					   const struct msghdr *client_msg,
					   int server_sock,
					   struct sockaddr *server_addr,
					   socklen_t server_addrlen,
					   struct msghdr *msg,
					   void *cmsgbuf, int cmsgbuf_len,
					   bool expect_control_data)
{
#define MAX_BUF_LEN 64
	char buf[MAX_BUF_LEN];
	struct iovec io_vector[1];
	ssize_t sent;
	ssize_t recved;
	struct sockaddr addr;
	socklen_t addrlen = server_addrlen;
	int len, i;

	zassert_not_null(client_addr, "null client addr");
	zassert_not_null(server_addr, "null server addr");

	/*
	 * Test client -> server sending
	 */

	sent = zsock_sendmsg(client_sock, client_msg, 0);
	zassert_true(sent > 0, "sendmsg failed, %s (%d)", strerror(errno), -errno);

	/* One negative test with invalid msg_iov */
	memset(msg, 0, sizeof(*msg));
	recved = zsock_recvmsg(server_sock, msg, 0);
	zassert_true(recved < 0 && errno == ENOMEM, "Wrong errno (%d)", errno);

	for (i = 0, len = 0; i < client_msg->msg_iovlen; i++) {
		len += client_msg->msg_iov[i].iov_len;
	}

	zassert_equal(sent, len, "iovec len (%d) vs sent (%d)", len, sent);

	/* Test first with one iovec */
	io_vector[0].iov_base = buf;
	io_vector[0].iov_len = sizeof(buf);

	memset(msg, 0, sizeof(*msg));
	if (cmsgbuf != NULL) {
		memset(cmsgbuf, 0, cmsgbuf_len);
	}
	msg->msg_control = cmsgbuf;
	msg->msg_controllen = cmsgbuf_len;
	msg->msg_iov = io_vector;
	msg->msg_iovlen = 1;
	msg->msg_name = &addr;
	msg->msg_namelen = addrlen;

	/* Test normal recvmsg() */
	clear_buf(rx_buf);
	recved = zsock_recvmsg(server_sock, msg, 0);
	zassert_true(recved > 0, "recvfrom fail");
	zassert_equal(recved, len, "unexpected received bytes");
	zassert_equal(msg->msg_iovlen, 1, "recvmsg should not modify msg_iovlen");
	zassert_equal(msg->msg_iov[0].iov_len, sizeof(buf),
		      "recvmsg should not modify buffer length");
	zassert_mem_equal(buf, TEST_STR_SMALL, len,
			  "wrong data (%s)", rx_buf);
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Control data should be empty */
	if (!expect_control_data) {
		zassert_equal(msg->msg_controllen, 0,
			      "We received control data (%u vs %zu)",
			      0U, msg->msg_controllen);
	}
}

static void run_ancillary_recvmsg_hoplimit_test(int client_sock,
						struct sockaddr *client_addr,
						int client_addr_len,
						int server_sock,
						struct sockaddr *server_addr,
						int server_addr_len)
{
	int rv;
	int opt;
	socklen_t optlen;
	struct msghdr msg;
	struct msghdr server_msg;
	struct iovec io_vector[1];
	struct cmsghdr *cmsg, *prevcmsg;
	int send_hop_limit = 90, recv_hop_limit = 0;
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(int))];
	} cmsgbuf;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_CONTEXT_RECV_HOPLIMIT);

	rv = zsock_bind(server_sock, server_addr, server_addr_len);
	zassert_equal(rv, 0, "server bind failed");

	rv = zsock_bind(client_sock, client_addr, client_addr_len);
	zassert_equal(rv, 0, "client bind failed");

	io_vector[0].iov_base = TEST_STR_SMALL;
	io_vector[0].iov_len = strlen(TEST_STR_SMALL);

	memset(&cmsgbuf, 0, sizeof(cmsgbuf));

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = server_addr;
	msg.msg_namelen = server_addr_len;
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = sizeof(cmsgbuf.buf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	if (client_addr->sa_family == AF_INET) {
		cmsg->cmsg_level = IPPROTO_IP;
		cmsg->cmsg_type = IP_TTL;
	} else {
		cmsg->cmsg_level = IPPROTO_IPV6;
		cmsg->cmsg_type = IPV6_HOPLIMIT;
	}

	*(int *)CMSG_DATA(cmsg) = send_hop_limit;

	comm_sendmsg_recvmsg_hop_limit(client_sock,
				       client_addr,
				       client_addr_len,
				       &msg,
				       server_sock,
				       server_addr,
				       server_addr_len,
				       &server_msg,
				       &cmsgbuf.buf,
				       sizeof(cmsgbuf.buf),
				       true);

	for (prevcmsg = NULL, cmsg = CMSG_FIRSTHDR(&server_msg);
	     cmsg != NULL && prevcmsg != cmsg;
	     prevcmsg = cmsg, cmsg = CMSG_NXTHDR(&server_msg, cmsg)) {
		if (client_addr->sa_family == AF_INET) {
			if (cmsg->cmsg_level == IPPROTO_IP &&
			    cmsg->cmsg_type == IP_TTL) {
				recv_hop_limit = *(int *)CMSG_DATA(cmsg);
				break;
			}
		} else {
			if (cmsg->cmsg_level == IPPROTO_IPV6 &&
			    cmsg->cmsg_type == IPV6_HOPLIMIT) {
				recv_hop_limit = *(int *)CMSG_DATA(cmsg);
				break;
			}
		}
	}

	/* As we have not set the socket option, the hop_limit should not be set */
	zassert_equal(recv_hop_limit, 0, "Hop limit set!");

	opt = 1;
	optlen = sizeof(opt);
	if (server_addr->sa_family == AF_INET) {
		rv = zsock_setsockopt(server_sock, IPPROTO_IP, IP_RECVTTL, &opt, optlen);
	} else {
		rv = zsock_setsockopt(server_sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &opt, optlen);
	}
	zassert_equal(rv, 0, "setsockopt failed (%d)", -errno);

	memset(&cmsgbuf, 0, sizeof(cmsgbuf));

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = server_addr;
	msg.msg_namelen = server_addr_len;
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_control = &cmsgbuf.buf;
	msg.msg_controllen = sizeof(cmsgbuf.buf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	if (client_addr->sa_family == AF_INET) {
		cmsg->cmsg_level = IPPROTO_IP;
		cmsg->cmsg_type = IP_TTL;
	} else {
		cmsg->cmsg_level = IPPROTO_IPV6;
		cmsg->cmsg_type = IPV6_HOPLIMIT;
	}

	*(int *)CMSG_DATA(cmsg) = send_hop_limit;

	comm_sendmsg_recvmsg_hop_limit(client_sock,
				       client_addr,
				       client_addr_len,
				       &msg,
				       server_sock,
				       server_addr,
				       server_addr_len,
				       &server_msg,
				       &cmsgbuf.buf,
				       sizeof(cmsgbuf.buf),
				       true);

	for (prevcmsg = NULL, cmsg = CMSG_FIRSTHDR(&server_msg);
	     cmsg != NULL && prevcmsg != cmsg;
	     prevcmsg = cmsg, cmsg = CMSG_NXTHDR(&server_msg, cmsg)) {
		if (client_addr->sa_family == AF_INET) {
			if (cmsg->cmsg_level == IPPROTO_IP &&
			    cmsg->cmsg_type == IP_TTL) {
				recv_hop_limit = *(int *)CMSG_DATA(cmsg);
				break;
			}
		} else {
			if (cmsg->cmsg_level == IPPROTO_IPV6 &&
			    cmsg->cmsg_type == IPV6_HOPLIMIT) {
				recv_hop_limit = *(int *)CMSG_DATA(cmsg);
				break;
			}
		}
	}

	zassert_equal(send_hop_limit, recv_hop_limit, "Hop limit not parsed correctly");
}

ZTEST_USER(net_socket_udp, test_43_recvmsg_ancillary_ipv4_hoplimit_data_user)
{
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	int client_sock;
	int server_sock;

	prepare_sock_udp_v4(MY_IPV4_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v4(MY_IPV4_ADDR, SERVER_PORT, &server_sock, &server_addr);

	run_ancillary_recvmsg_hoplimit_test(client_sock,
					    (struct sockaddr *)&client_addr,
					    sizeof(client_addr),
					    server_sock,
					    (struct sockaddr *)&server_addr,
					    sizeof(server_addr));
}

ZTEST_USER(net_socket_udp, test_44_recvmsg_ancillary_ipv6_hoplimit_data_user)
{
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;
	int client_sock;
	int server_sock;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &server_sock, &server_addr);

	run_ancillary_recvmsg_hoplimit_test(client_sock,
					    (struct sockaddr *)&client_addr,
					    sizeof(client_addr),
					    server_sock,
					    (struct sockaddr *)&server_addr,
					    sizeof(server_addr));
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	for (int i = 0; i < CONFIG_ZVFS_OPEN_MAX; ++i) {
		(void)zsock_close(i);
	}
}

ZTEST_SUITE(net_socket_udp, NULL, NULL, NULL, after, NULL);
