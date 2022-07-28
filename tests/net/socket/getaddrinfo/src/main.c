/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>
#include <zephyr/sys/sem.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/buf.h>

#include "../../socket_helpers.h"

#include "dns_pack.h"

#define QUERY_HOST "www.zephyrproject.org"

#define ANY_PORT 0
#define MAX_BUF_SIZE 128
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_PRIORITY K_PRIO_COOP(2)
#define WAIT_TIME K_MSEC(250)

static uint8_t recv_buf[MAX_BUF_SIZE];

static int sock_v4;
static int sock_v6;

static struct sockaddr_in addr_v4;
static struct sockaddr_in6 addr_v6;

static int queries_received;

/* The semaphore is there to wait the data to be received. */
static ZTEST_BMEM struct sys_sem wait_data;

NET_BUF_POOL_DEFINE(test_dns_msg_pool, 1, 512, 0, NULL);

static bool check_dns_query(uint8_t *buf, int buf_len)
{
	struct dns_msg_t dns_msg;
	struct net_buf *result;
	enum dns_rr_type qtype;
	enum dns_class qclass;
	int ret, queries;

	/* Store the DNS query name into a temporary net_buf as that is
	 * expected by dns_unpack_query() function. In this test we are
	 * currently not sending any DNS response back as that is not
	 * really needed by these tests.
	 */
	result = net_buf_alloc(&test_dns_msg_pool, K_FOREVER);
	if (!result) {
		ret = -ENOMEM;
		return false;
	}

	dns_msg.msg = buf;
	dns_msg.msg_size = buf_len;

	ret = mdns_unpack_query_header(&dns_msg, NULL);
	if (ret < 0) {
		return false;
	}

	queries = ret;
	queries_received++;

	NET_DBG("Received %d %s", queries,
		queries > 1 ? "queries" : "query");

	(void)memset(result->data, 0, net_buf_tailroom(result));
	result->len = 0U;

	ret = dns_unpack_query(&dns_msg, result, &qtype, &qclass);
	if (ret < 0) {
		net_buf_unref(result);
		return false;
	}

	NET_DBG("[%d] query %s/%s label %s (%d bytes)", queries,
		qtype == DNS_RR_TYPE_A ? "A" : "AAAA", "IN",
		result->data, ret);

	/* In this test we are just checking if the query came to us in correct
	 * form, we are not creating a DNS server implementation here.
	 */
	if (strncmp(result->data + 1, QUERY_HOST,
		    sizeof(QUERY_HOST) - 1)) {
		net_buf_unref(result);
		return false;
	}

	net_buf_unref(result);

	return true;
}

static int process_dns(void)
{
	struct pollfd pollfds[2];
	struct sockaddr *addr;
	socklen_t addr_len;
	int ret, idx;

	NET_DBG("Waiting for IPv4 DNS packets on port %d",
		ntohs(addr_v4.sin_port));
	NET_DBG("Waiting for IPv6 DNS packets on port %d",
		ntohs(addr_v6.sin6_port));

	while (true) {
		memset(pollfds, 0, sizeof(pollfds));
		pollfds[0].fd = sock_v4;
		pollfds[0].events = POLLIN;
		pollfds[1].fd = sock_v6;
		pollfds[1].events = POLLIN;

		NET_DBG("Polling...");

		ret = poll(pollfds, ARRAY_SIZE(pollfds), -1);
		if (ret <= 0) {
			continue;
		}

		for (idx = 0; idx < ARRAY_SIZE(pollfds); idx++) {
			if (pollfds[idx].revents & POLLIN) {
				if (pollfds[idx].fd == sock_v4) {
					addr_len = sizeof(addr_v4);
					addr = (struct sockaddr *)&addr_v4;
				} else {
					addr_len = sizeof(addr_v6);
					addr = (struct sockaddr *)&addr_v6;
				}

				ret = recvfrom(pollfds[idx].fd,
					       recv_buf, sizeof(recv_buf), 0,
					       addr, &addr_len);
				if (ret < 0) {
					/* Socket error */
					NET_ERR("DNS: Connection error (%d)",
						errno);
					break;
				}

				NET_DBG("Received DNS query");

				ret = check_dns_query(recv_buf,
						      sizeof(recv_buf));
				if (ret) {
					(void)sys_sem_give(&wait_data);
				}
			}
		}
	}

	return -errno;
}

K_THREAD_DEFINE(dns_server_thread_id, STACK_SIZE,
		process_dns, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

static void *test_getaddrinfo_setup(void)
{
	char str[INET6_ADDRSTRLEN], *addr_str;
	struct sockaddr addr;
	int ret;

	ret = net_ipaddr_parse(CONFIG_DNS_SERVER1,
			       sizeof(CONFIG_DNS_SERVER1) - 1,
			       &addr);
	zassert_true(ret, "Cannot parse IP address %s", CONFIG_DNS_SERVER1);

	if (addr.sa_family == AF_INET) {
		memcpy(&addr_v4, net_sin(&addr), sizeof(struct sockaddr_in));
	} else if (addr.sa_family == AF_INET6) {
		memcpy(&addr_v6, net_sin6(&addr), sizeof(struct sockaddr_in6));
	}

	ret = net_ipaddr_parse(CONFIG_DNS_SERVER2,
			       sizeof(CONFIG_DNS_SERVER2) - 1,
			       &addr);
	zassert_true(ret, "Cannot parse IP address %s", CONFIG_DNS_SERVER2);

	if (addr.sa_family == AF_INET) {
		memcpy(&addr_v4, net_sin(&addr), sizeof(struct sockaddr_in));
	} else if (addr.sa_family == AF_INET6) {
		memcpy(&addr_v6, net_sin6(&addr), sizeof(struct sockaddr_in6));
	}

	addr_str = inet_ntop(AF_INET, &addr_v4.sin_addr, str, sizeof(str));
	NET_DBG("v4: [%s]:%d", addr_str, ntohs(addr_v4.sin_port));

	sock_v4 = prepare_listen_sock_udp_v4(&addr_v4);
	zassert_true(sock_v4 >= 0, "Invalid IPv4 socket");

	addr_str = inet_ntop(AF_INET6, &addr_v6.sin6_addr, str, sizeof(str));
	NET_DBG("v6: [%s]:%d", addr_str, ntohs(addr_v6.sin6_port));

	sock_v6 = prepare_listen_sock_udp_v6(&addr_v6);
	zassert_true(sock_v6 >= 0, "Invalid IPv6 socket");

	sys_sem_init(&wait_data, 0, INT_MAX);

	k_thread_start(dns_server_thread_id);

	k_thread_priority_set(dns_server_thread_id,
			      k_thread_priority_get(k_current_get()));
	k_yield();

	return NULL;
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_ok)
{
	struct addrinfo *res = NULL;

	queries_received = 0;

	/* This check simulates a local query that we will catch
	 * in process_dns() function. So we do not check the res variable
	 * as that will currently not contain anything useful. We just check
	 * that the query triggered a function call to process_dns() function
	 * and that it could parse the DNS query.
	 */
	(void)getaddrinfo(QUERY_HOST, NULL, NULL, &res);

	if (sys_sem_count_get(&wait_data) != 2) {
		zassert_true(false, "Did not receive all queries");
	}

	(void)sys_sem_take(&wait_data, K_NO_WAIT);
	(void)sys_sem_take(&wait_data, K_NO_WAIT);

	zassert_equal(queries_received, 2,
		      "Did not receive both IPv4 and IPv6 query");

	freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_cancelled)
{
	struct addrinfo *res = NULL;
	int ret;

	ret = getaddrinfo(QUERY_HOST, NULL, NULL, &res);

	if (sys_sem_count_get(&wait_data) != 2) {
		zassert_true(false, "Did not receive all queries");
	}

	(void)sys_sem_take(&wait_data, K_NO_WAIT);
	(void)sys_sem_take(&wait_data, K_NO_WAIT);

	/* Without a local DNS server this request will be canceled. */
	zassert_equal(ret, DNS_EAI_CANCELED, "Invalid result");

	freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_no_host)
{
	struct addrinfo *res = NULL;
	int ret;

	ret = getaddrinfo(NULL, NULL, NULL, &res);

	zassert_equal(ret, DNS_EAI_SYSTEM, "Invalid result");
	zassert_equal(errno, EINVAL, "Invalid errno");
	zassert_is_null(res, "ai_addr is not NULL");

	freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_num_ipv4)
{
	struct zsock_addrinfo *res = NULL;
	struct sockaddr_in *saddr;
	int ret;

	struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	ret = zsock_getaddrinfo("1.2.3.255", "65534", NULL, &res);

	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET, "");
	zassert_equal(res->ai_socktype, SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_TCP, "");
	zsock_freeaddrinfo(res);

	ret = zsock_getaddrinfo("1.2.3.255", "65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET, "");
	zassert_equal(res->ai_socktype, SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_TCP, "");
	zsock_freeaddrinfo(res);

	hints.ai_socktype = SOCK_DGRAM;
	ret = zsock_getaddrinfo("1.2.3.255", "65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET, "");
	zassert_equal(res->ai_socktype, SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_UDP, "");

	saddr = (struct sockaddr_in *)res->ai_addr;
	zassert_equal(saddr->sin_family, AF_INET, "");
	zassert_equal(saddr->sin_port, htons(65534), "");
	zassert_equal(saddr->sin_addr.s4_addr[0], 1, "");
	zassert_equal(saddr->sin_addr.s4_addr[1], 2, "");
	zassert_equal(saddr->sin_addr.s4_addr[2], 3, "");
	zassert_equal(saddr->sin_addr.s4_addr[3], 255, "");
	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_num_ipv6)
{
	struct zsock_addrinfo *res = NULL;
	struct sockaddr_in6 *saddr;
	int ret;

	struct zsock_addrinfo hints = {
		.ai_family = AF_INET6,
		.ai_socktype = SOCK_STREAM
	};

	ret = zsock_getaddrinfo("[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]",
			"65534", NULL, &res);

	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET6, "");
	zassert_equal(res->ai_socktype, SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_TCP, "");

	saddr = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, AF_INET6, "");
	zassert_equal(saddr->sin6_port, htons(65534), "");
	zassert_equal(saddr->sin6_addr.s6_addr[0], 0xFE, "");
	zassert_equal(saddr->sin6_addr.s6_addr[1], 0xDC, "");
	zassert_equal(saddr->sin6_addr.s6_addr[2], 0xBA, "");
	zassert_equal(saddr->sin6_addr.s6_addr[3], 0x98, "");
	zassert_equal(saddr->sin6_addr.s6_addr[4], 0x76, "");
	zassert_equal(saddr->sin6_addr.s6_addr[5], 0x54, "");
	zassert_equal(saddr->sin6_addr.s6_addr[6], 0x32, "");
	zassert_equal(saddr->sin6_addr.s6_addr[7], 0x10, "");
	zassert_equal(saddr->sin6_addr.s6_addr[8], 0xFE, "");
	zassert_equal(saddr->sin6_addr.s6_addr[9], 0xDC, "");
	zassert_equal(saddr->sin6_addr.s6_addr[10], 0xBA, "");
	zassert_equal(saddr->sin6_addr.s6_addr[11], 0x98, "");
	zassert_equal(saddr->sin6_addr.s6_addr[12], 0x76, "");
	zassert_equal(saddr->sin6_addr.s6_addr[13], 0x54, "");
	zassert_equal(saddr->sin6_addr.s6_addr[14], 0x32, "");
	zassert_equal(saddr->sin6_addr.s6_addr[15], 0x10, "");
	zsock_freeaddrinfo(res);


	ret = zsock_getaddrinfo("[1080:0:0:0:8:800:200C:417A]",
			"65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET6, "");
	zassert_equal(res->ai_socktype, SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_TCP, "");

	saddr = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, AF_INET6, "");
	zassert_equal(saddr->sin6_port, htons(65534), "");
	zassert_equal(saddr->sin6_addr.s6_addr[0], 0x10, "");
	zassert_equal(saddr->sin6_addr.s6_addr[1], 0x80, "");
	zassert_equal(saddr->sin6_addr.s6_addr[2], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[3], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[4], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[5], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[6], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[7], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[8], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[9], 0x08, "");
	zassert_equal(saddr->sin6_addr.s6_addr[10], 0x08, "");
	zassert_equal(saddr->sin6_addr.s6_addr[11], 0x00, "");
	zassert_equal(saddr->sin6_addr.s6_addr[12], 0x20, "");
	zassert_equal(saddr->sin6_addr.s6_addr[13], 0x0C, "");
	zassert_equal(saddr->sin6_addr.s6_addr[14], 0x41, "");
	zassert_equal(saddr->sin6_addr.s6_addr[15], 0x7A, "");
	zsock_freeaddrinfo(res);


	hints.ai_socktype = SOCK_DGRAM;
	ret = zsock_getaddrinfo("[3ffe:2a00:100:7031::1]",
			"65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET6, "");
	zassert_equal(res->ai_socktype, SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_UDP, "");

	saddr = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, AF_INET6, "");
	zassert_equal(saddr->sin6_port, htons(65534), "");
	zassert_equal(saddr->sin6_addr.s6_addr[0], 0x3f, "");
	zassert_equal(saddr->sin6_addr.s6_addr[1], 0xfe, "");
	zassert_equal(saddr->sin6_addr.s6_addr[2], 0x2a, "");
	zassert_equal(saddr->sin6_addr.s6_addr[3], 0x00, "");
	zassert_equal(saddr->sin6_addr.s6_addr[4], 0x01, "");
	zassert_equal(saddr->sin6_addr.s6_addr[5], 0x00, "");
	zassert_equal(saddr->sin6_addr.s6_addr[6], 0x70, "");
	zassert_equal(saddr->sin6_addr.s6_addr[7], 0x31, "");
	zassert_equal(saddr->sin6_addr.s6_addr[8], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[9], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[10], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[11], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[12], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[13], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[14], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[15], 0x1, "");
	zsock_freeaddrinfo(res);


	ret = zsock_getaddrinfo("[1080::8:800:200C:417A]",
			"65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");

	saddr = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, AF_INET6, "");
	zassert_equal(saddr->sin6_port, htons(65534), "");
	zassert_equal(saddr->sin6_addr.s6_addr[0], 0x10, "");
	zassert_equal(saddr->sin6_addr.s6_addr[1], 0x80, "");
	zassert_equal(saddr->sin6_addr.s6_addr[2], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[3], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[4], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[5], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[6], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[7], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[8], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[9], 0x8, "");
	zassert_equal(saddr->sin6_addr.s6_addr[10], 0x08, "");
	zassert_equal(saddr->sin6_addr.s6_addr[11], 0x00, "");
	zassert_equal(saddr->sin6_addr.s6_addr[12], 0x20, "");
	zassert_equal(saddr->sin6_addr.s6_addr[13], 0x0C, "");
	zassert_equal(saddr->sin6_addr.s6_addr[14], 0x41, "");
	zassert_equal(saddr->sin6_addr.s6_addr[15], 0x7A, "");
	zsock_freeaddrinfo(res);


	ret = zsock_getaddrinfo("[::192.9.5.5]", "65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");

	saddr = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, AF_INET6, "");
	zassert_equal(saddr->sin6_port, htons(65534), "");
	zassert_equal(saddr->sin6_addr.s6_addr[0], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[1], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[2], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[3], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[4], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[5], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[6], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[7], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[8], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[9], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[10], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[11], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[12], 192, "");
	zassert_equal(saddr->sin6_addr.s6_addr[13], 9, "");
	zassert_equal(saddr->sin6_addr.s6_addr[14], 5, "");
	zassert_equal(saddr->sin6_addr.s6_addr[15], 5, "");
	zsock_freeaddrinfo(res);


	ret = zsock_getaddrinfo("[::FFFF:129.144.52.38]",
			"65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");

	saddr = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, AF_INET6, "");
	zassert_equal(saddr->sin6_port, htons(65534), "");
	zassert_equal(saddr->sin6_addr.s6_addr[0], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[1], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[2], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[3], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[4], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[5], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[6], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[7], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[8], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[9], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[10], 0xFF, "");
	zassert_equal(saddr->sin6_addr.s6_addr[11], 0xFF, "");
	zassert_equal(saddr->sin6_addr.s6_addr[12], 129, "");
	zassert_equal(saddr->sin6_addr.s6_addr[13], 144, "");
	zassert_equal(saddr->sin6_addr.s6_addr[14], 52, "");
	zassert_equal(saddr->sin6_addr.s6_addr[15], 38, "");
	zsock_freeaddrinfo(res);


	ret = zsock_getaddrinfo("[2010:836B:4179::836B:4179]",
			"65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");

	saddr = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, AF_INET6, "");
	zassert_equal(saddr->sin6_port, htons(65534), "");
	zassert_equal(saddr->sin6_addr.s6_addr[0], 0x20, "");
	zassert_equal(saddr->sin6_addr.s6_addr[1], 0x10, "");
	zassert_equal(saddr->sin6_addr.s6_addr[2], 0x83, "");
	zassert_equal(saddr->sin6_addr.s6_addr[3], 0x6B, "");
	zassert_equal(saddr->sin6_addr.s6_addr[4], 0x41, "");
	zassert_equal(saddr->sin6_addr.s6_addr[5], 0x79, "");
	zassert_equal(saddr->sin6_addr.s6_addr[6], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[7], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[8], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[9], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[10], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[11], 0x0, "");
	zassert_equal(saddr->sin6_addr.s6_addr[12], 0x83, "");
	zassert_equal(saddr->sin6_addr.s6_addr[13], 0x6B, "");
	zassert_equal(saddr->sin6_addr.s6_addr[14], 0x41, "");
	zassert_equal(saddr->sin6_addr.s6_addr[15], 0x79, "");
	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_flags_numerichost)
{
	int ret;
	struct zsock_addrinfo *res = NULL;
	struct zsock_addrinfo hints = {
		.ai_flags = AI_NUMERICHOST,
	};

	ret = zsock_getaddrinfo("foo.bar", "65534", &hints, &res);
	zassert_equal(ret, DNS_EAI_FAIL, "Invalid result");
	zassert_is_null(res, "");

	ret = zsock_getaddrinfo("1.2.3.4", "65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");

	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_ipv4_hints_ipv6)
{
	struct zsock_addrinfo *res = NULL;
	struct zsock_addrinfo hints = {
		.ai_family = AF_INET6,
	};
	int ret;

	ret = zsock_getaddrinfo("192.0.2.1", NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_ADDRFAMILY, "Invalid result (%d)", ret);
	zassert_is_null(res, "");
	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_ipv6_hints_ipv4)
{
	struct zsock_addrinfo *res = NULL;
	struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
	};
	int ret;

	ret = zsock_getaddrinfo("2001:db8::1", NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_ADDRFAMILY, "Invalid result (%d)", ret);
	zassert_is_null(res, "");
	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_port_invalid)
{
	int ret;
	struct zsock_addrinfo *res = NULL;
	ret = zsock_getaddrinfo("192.0.2.1", "70000", NULL, &res);
	zassert_equal(ret, DNS_EAI_NONAME, "Invalid result (%d)", ret);
	zassert_is_null(res, "");
	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_null_host)
{
	struct sockaddr_in *saddr;
	struct sockaddr_in6 *saddr6;
	struct zsock_addrinfo *res = NULL;
	struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_PASSIVE
	};
	int ret;

	/* Test IPv4 TCP */
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET, "");
	zassert_equal(res->ai_socktype, SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_TCP, "");
	saddr = net_sin(res->ai_addr);
	zassert_equal(saddr->sin_family, AF_INET, "");
	zassert_equal(saddr->sin_port, htons(80), "");
	zassert_equal(saddr->sin_addr.s_addr, INADDR_ANY, "");
	zsock_freeaddrinfo(res);

	/* Test IPv6 TCP */
	hints.ai_family = AF_INET6;
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET6, "");
	zassert_equal(res->ai_socktype, SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_TCP, "");
	saddr6 = net_sin6(res->ai_addr);
	zassert_equal(saddr6->sin6_family, AF_INET6, "");
	zassert_equal(saddr6->sin6_port, htons(80), "");
	zassert_equal(0, memcmp(&saddr6->sin6_addr, &in6addr_any, sizeof(in6addr_any)), "");
	zsock_freeaddrinfo(res);

	/* Test IPv6 UDP */
	hints.ai_socktype = SOCK_DGRAM;
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET6, "");
	zassert_equal(res->ai_socktype, SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_UDP, "");
	saddr6 = (struct sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr6->sin6_family, AF_INET6, "");
	zassert_equal(saddr6->sin6_port, htons(80), "");
	zsock_freeaddrinfo(res);

	/* Test IPv4 UDP */
	hints.ai_family = AF_INET;
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, AF_INET, "");
	zassert_equal(res->ai_socktype, SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_UDP, "");
	saddr = (struct sockaddr_in *)res->ai_addr;
	zassert_equal(saddr->sin_family, AF_INET, "");
	zassert_equal(saddr->sin_port, htons(80), "");
	zsock_freeaddrinfo(res);
}

ZTEST_SUITE(net_socket_getaddrinfo, NULL, test_getaddrinfo_setup, NULL, NULL, NULL);
