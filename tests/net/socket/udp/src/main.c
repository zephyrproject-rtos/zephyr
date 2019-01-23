/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>

#include <net/socket.h>

#include "../../socket_helpers.h"

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
	static char rx_buf[400] = {0};

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

void test_main(void)
{
	ztest_test_suite(socket_udp,
			 ztest_unit_test(test_send_recv_2_sock),
			 ztest_unit_test(test_v4_sendto_recvfrom),
			 ztest_unit_test(test_v6_sendto_recvfrom),
			 ztest_unit_test(test_v4_bind_sendto),
			 ztest_unit_test(test_v6_bind_sendto));

	ztest_run_test_suite(socket_udp);
}
