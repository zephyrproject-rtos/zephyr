/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ztest_assert.h>

#include <net/socket.h>

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

static void prepare_sock_v4(const char *addr,
			    u16_t port,
			    int *sock,
			    struct sockaddr_in *sockaddr)
{
	int rv;

	zassert_not_null(addr, "null addr");
	zassert_not_null(sock, "null sock");
	zassert_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(*sock >= 0, "socket open failed");

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	rv = inet_pton(AF_INET, addr, &sockaddr->sin_addr);
	zassert_equal(rv, 1, "inet_pton failed");
}

static void prepare_sock_v6(const char *addr,
			    u16_t port,
			    int *sock,
			    struct sockaddr_in6 *sockaddr)
{
	int rv;

	zassert_not_null(addr, "null addr");
	zassert_not_null(sock, "null sock");
	zassert_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(*sock >= 0, "socket open failed");

	sockaddr->sin6_family = AF_INET6;
	sockaddr->sin6_port = htons(port);
	rv = inet_pton(AF_INET6, addr, &sockaddr->sin6_addr);
	zassert_equal(rv, 1, "inet_pton failed");
}

static void test_sendto_recvfrom(int client_sock,
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
	char rx_buf[30] = {0};

	zassert_not_null(client_addr, "null client addr");
	zassert_not_null(server_addr, "null server addr");

	sent = sendto(client_sock,
		      TEST_STR_SMALL,
		      strlen(TEST_STR_SMALL),
		      0,
		      server_addr,
		      server_addrlen);
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed");

	addrlen = sizeof(addr);
	recved = recvfrom(server_sock,
			  rx_buf,
			  sizeof(rx_buf),
			  0,
			  &addr,
			  &addrlen);
	zassert_true(recved > 0, "recvfrom fail");
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
	zassert_equal(addrlen, client_addrlen, "unexpected addrlen");

	/* Check the client port */
	if (net_sin(client_addr)->sin_port != ANY_PORT) {
		zassert_equal(net_sin(client_addr)->sin_port,
			      net_sin(&addr)->sin_port,
			      "unexpected client port");
	}
}

void test_v4_sendto_recvfrom(void)
{
	int rv;
	int client_sock;
	int server_sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			ANY_PORT,
			&client_sock,
			&client_addr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			SERVER_PORT,
			&server_sock,
			&server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	test_sendto_recvfrom(client_sock,
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

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			ANY_PORT,
			&client_sock,
			&client_addr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			SERVER_PORT,
			&server_sock,
			&server_addr);

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	test_sendto_recvfrom(client_sock,
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

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			CLIENT_PORT,
			&client_sock,
			&client_addr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			SERVER_PORT,
			&server_sock,
			&server_addr);

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	test_sendto_recvfrom(client_sock,
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

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			CLIENT_PORT,
			&client_sock,
			&client_addr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			SERVER_PORT,
			&server_sock,
			&server_addr);

	rv = bind(client_sock,
		  (struct sockaddr *)&client_addr,
		  sizeof(client_addr));
	zassert_equal(rv, 0, "bind failed");

	rv = bind(server_sock,
		  (struct sockaddr *)&server_addr,
		  sizeof(server_addr));
	zassert_equal(rv, 0, "bind failed");

	test_sendto_recvfrom(client_sock,
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
	int len, cmp;

	sock1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sock2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(55555);
	bind(sock1, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	conn_addr.sin_family = AF_INET;
	conn_addr.sin_addr.s_addr = htonl(0xc0000201);
	conn_addr.sin_port = htons(55555);
	connect(sock2, (struct sockaddr *)&conn_addr, sizeof(conn_addr));

	send(sock2, BUF_AND_SIZE(TEST_STR_SMALL), 0);

	len = recv(sock1, buf, sizeof(buf), 0);
	zassert_equal(len, 4, "Invalid recv len");
	cmp = memcmp(buf, TEST_STR_SMALL, STRLEN(TEST_STR_SMALL));
	zassert_equal(cmp, 0, "Invalid recv data");
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
