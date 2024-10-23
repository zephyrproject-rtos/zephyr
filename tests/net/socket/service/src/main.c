/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/net/socket_service.h>

#include "../../socket_helpers.h"

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

#define MY_IPV6_ADDR "::1"

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(3)

K_SEM_DEFINE(wait_data, 0, UINT_MAX);
K_SEM_DEFINE(wait_data_tcp, 0, UINT_MAX);
#define WAIT_TIME 500

static void server_handler(struct k_work *work)
{
	struct net_socket_service_event *pev =
		CONTAINER_OF(work, struct net_socket_service_event, work);

	ARG_UNUSED(pev);

	k_sem_give(&wait_data);
}

static void tcp_server_handler(struct k_work *work)
{
	struct net_socket_service_event *pev =
		CONTAINER_OF(work, struct net_socket_service_event, work);

	ARG_UNUSED(pev);

	k_sem_give(&wait_data_tcp);

	k_yield();

	Z_SPIN_DELAY(100);
}

NET_SOCKET_SERVICE_SYNC_DEFINE(udp_service_sync, server_handler, 2);
NET_SOCKET_SERVICE_SYNC_DEFINE(tcp_service_small_sync, tcp_server_handler, 1);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(tcp_service_sync, tcp_server_handler, 2);


void run_test_service(const struct net_socket_service_desc *udp_service,
		      const struct net_socket_service_desc *tcp_service_small,
		      const struct net_socket_service_desc *tcp_service)
{
	int ret;
	int c_sock_udp;
	int s_sock_udp;
	int c_sock_tcp;
	int s_sock_tcp;
	int new_sock;
	struct sockaddr_in6 c_addr;
	struct sockaddr_in6 s_addr;
	ssize_t len;
	char buf[10];
	struct zsock_pollfd sock[2] = {
		[0] = { .fd = -1 },
		[1] = { .fd = -1 },
	};

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock_udp, &c_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock_udp, &s_addr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock_tcp, &c_addr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock_tcp, &s_addr);

	sock[0].fd = s_sock_udp;
	sock[0].events = ZSOCK_POLLIN;

	ret = net_socket_service_register(udp_service, sock, ARRAY_SIZE(sock), NULL);
	zassert_equal(ret, 0, "Cannot register udp service (%d)", ret);

	sock[0].fd = s_sock_tcp;
	sock[0].events = ZSOCK_POLLIN;

	ret = net_socket_service_register(tcp_service_small, sock, ARRAY_SIZE(sock) + 1, NULL);
	zassert_equal(ret, -ENOMEM, "Could register tcp service (%d)", ret);

	ret = net_socket_service_register(tcp_service, sock, ARRAY_SIZE(sock), NULL);
	zassert_equal(ret, 0, "Cannot register tcp service (%d)", ret);

	ret = bind(s_sock_udp, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "bind failed");

	ret = connect(c_sock_udp, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "connect failed");

	/* Send pkt for s_sock_udp and poll with timeout of 10 */
	len = send(c_sock_udp, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting callback");
	}

	/* Recv pkt from s_sock_udp and ensure no poll events happen */
	len = recv(s_sock_udp, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	ret = bind(s_sock_tcp, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "bind failed (%d)", -errno);
	ret = listen(s_sock_tcp, 0);
	zassert_equal(ret, 0, "");

	ret = connect(c_sock_tcp, (const struct sockaddr *)&s_addr,
		      sizeof(s_addr));
	zassert_equal(ret, 0, "");

	/* Let the network stack run */
	k_msleep(10);

	len = send(c_sock_tcp, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	if (k_sem_take(&wait_data_tcp, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting callback");
	}

	new_sock = accept(s_sock_tcp, NULL, NULL);
	zassert_true(new_sock >= 0, "");

	sock[1].fd = new_sock;
	sock[1].events = ZSOCK_POLLIN;

	ret = net_socket_service_register(tcp_service, sock, ARRAY_SIZE(sock), NULL);
	zassert_equal(ret, 0, "Cannot register tcp service (%d)", ret);

	if (k_sem_take(&wait_data_tcp, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting callback");
	}

	len = recv(new_sock, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	ret = net_socket_service_unregister(tcp_service);
	zassert_equal(ret, 0, "Cannot unregister tcp service (%d)", ret);

	ret = net_socket_service_unregister(udp_service);
	zassert_equal(ret, 0, "Cannot unregister tcp service (%d)", ret);

	ret = net_socket_service_unregister(tcp_service_small);
	zassert_equal(ret, 0, "Cannot unregister tcp service (%d)", ret);

	ret = close(new_sock);
	zassert_equal(ret, 0, "close failed");

	ret = close(c_sock_tcp);
	zassert_equal(ret, 0, "close failed");

	ret = close(s_sock_tcp);
	zassert_equal(ret, 0, "close failed");

	ret = close(c_sock_udp);
	zassert_equal(ret, 0, "close failed");

	ret = close(s_sock_udp);
	zassert_equal(ret, 0, "close failed");

	/* Let the stack close the TCP sockets properly */
	k_msleep(100);
}

ZTEST(net_socket_service, test_service_sync)
{
	run_test_service(&udp_service_sync, &tcp_service_small_sync,
			 &tcp_service_sync);
}

ZTEST_SUITE(net_socket_service, NULL, NULL, NULL, NULL, NULL);
