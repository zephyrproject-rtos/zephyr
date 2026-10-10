/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sem.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net_buf.h>

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

static struct net_sockaddr_in addr_v4;
static struct net_sockaddr_in6 addr_v6;

static int queries_received;
static int queries_received_a;
static int queries_received_aaaa;
static bool answer_queries;
static int expected_query_count =
	CONFIG_NET_SOCKETS_DNS_BACKOFF_INTERVAL >= CONFIG_NET_SOCKETS_DNS_TIMEOUT ?
	2 :
	/* Calculate for both IPv4 and IPv6 so need to double the value */
	2 * (LOG2CEIL(DIV_ROUND_UP(CONFIG_NET_SOCKETS_DNS_TIMEOUT,
				   CONFIG_NET_SOCKETS_DNS_BACKOFF_INTERVAL) + 1));

/* The semaphore is there to wait the data to be received. */
static ZTEST_BMEM struct sys_sem wait_data;

static void reset_query_counters(void)
{
	queries_received = 0;
	queries_received_a = 0;
	queries_received_aaaa = 0;
}

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

	if (qtype == DNS_RR_TYPE_A) {
		queries_received_a++;
	} else if (qtype == DNS_RR_TYPE_AAAA) {
		queries_received_aaaa++;
	}

	/* In this test we are just checking if the query came to us in correct
	 * form, we are not creating a DNS server implementation here.
	 */
	if (strncmp(result->data, QUERY_HOST,
		    sizeof(QUERY_HOST) - 1)) {
		net_buf_unref(result);
		return false;
	}

	net_buf_unref(result);

	return true;
}

static const uint8_t answer_addrs_v4[][4] = {
	{ 192, 0, 2, 10 },
	{ 192, 0, 2, 11 },
};

static const uint8_t answer_addrs_v6[][16] = {
	{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 },
	{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x11 },
};

/* Answer the query in recv_buf with two A or AAAA records */
static void send_answer(int sock, const struct net_sockaddr *addr,
			net_socklen_t addr_len, int len)
{
	uint8_t buf[MAX_BUF_SIZE];
	uint16_t qtype;
	int pos = 12;

	/* Skip the question name */
	while (pos < len && recv_buf[pos] != 0) {
		pos += recv_buf[pos] + 1;
	}

	pos++;
	if (pos + 4 > len) {
		return;
	}

	qtype = sys_get_be16(&recv_buf[pos]);
	pos += 4;

	memcpy(buf, recv_buf, pos);
	buf[2] = 0x81; /* Response, recursion desired */
	buf[3] = 0x80; /* Recursion available */
	sys_put_be16(2, &buf[6]); /* ANCOUNT */
	memset(&buf[8], 0, 4); /* NSCOUNT, ARCOUNT */

	for (int i = 0; i < 2; i++) {
		const uint8_t *rdata = qtype == DNS_RR_TYPE_A ?
				       answer_addrs_v4[i] : answer_addrs_v6[i];
		uint16_t rdlen = qtype == DNS_RR_TYPE_A ?
				 sizeof(answer_addrs_v4[i]) : sizeof(answer_addrs_v6[i]);

		sys_put_be16(0xc00c, &buf[pos]); /* Pointer to the question name */
		sys_put_be16(qtype, &buf[pos + 2]);
		sys_put_be16(DNS_CLASS_IN, &buf[pos + 4]);
		sys_put_be32(60, &buf[pos + 6]); /* TTL */
		sys_put_be16(rdlen, &buf[pos + 10]);
		memcpy(&buf[pos + 12], rdata, rdlen);
		pos += 12 + rdlen;
	}

	(void)zsock_sendto(sock, buf, pos, 0, addr, addr_len);
}

static void process_dns(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct zsock_pollfd pollfds[2];
	struct net_sockaddr *addr;
	net_socklen_t addr_len;
	int ret, idx, len;

	NET_DBG("Waiting for IPv4 DNS packets on port %d",
		net_ntohs(addr_v4.sin_port));
	NET_DBG("Waiting for IPv6 DNS packets on port %d",
		net_ntohs(addr_v6.sin6_port));

	while (true) {
		memset(pollfds, 0, sizeof(pollfds));
		pollfds[0].fd = sock_v4;
		pollfds[0].events = ZSOCK_POLLIN;
		pollfds[1].fd = sock_v6;
		pollfds[1].events = ZSOCK_POLLIN;

		NET_DBG("Polling...");

		ret = zsock_poll(pollfds, ARRAY_SIZE(pollfds), -1);
		if (ret <= 0) {
			continue;
		}

		for (idx = 0; idx < ARRAY_SIZE(pollfds); idx++) {
			if (pollfds[idx].revents & ZSOCK_POLLIN) {
				if (pollfds[idx].fd == sock_v4) {
					addr_len = sizeof(addr_v4);
					addr = (struct net_sockaddr *)&addr_v4;
				} else {
					addr_len = sizeof(addr_v6);
					addr = (struct net_sockaddr *)&addr_v6;
				}

				ret = zsock_recvfrom(pollfds[idx].fd,
						     recv_buf, sizeof(recv_buf), 0,
						     addr, &addr_len);
				if (ret < 0) {
					/* Socket error */
					NET_ERR("DNS: Connection error (%d)",
						errno);
					break;
				}

				NET_DBG("Received DNS query");

				len = ret;
				ret = check_dns_query(recv_buf,
						      sizeof(recv_buf));
				if (ret) {
					if (answer_queries) {
						send_answer(pollfds[idx].fd, addr, addr_len, len);
					}

					(void)sys_sem_give(&wait_data);
				}
			}
		}
	}
}

K_THREAD_DEFINE(dns_server_thread_id, STACK_SIZE,
		process_dns, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

static void *test_getaddrinfo_setup(void)
{
	char str[NET_INET6_ADDRSTRLEN], *addr_str;
	struct net_sockaddr addr;
	int ret;

	ret = net_ipaddr_parse(CONFIG_DNS_SERVER1,
			       sizeof(CONFIG_DNS_SERVER1) - 1,
			       &addr);
	zassert_true(ret, "Cannot parse IP address %s", CONFIG_DNS_SERVER1);

	if (addr.sa_family == NET_AF_INET) {
		memcpy(&addr_v4, net_sin(&addr), sizeof(struct net_sockaddr_in));
	} else if (addr.sa_family == NET_AF_INET6) {
		memcpy(&addr_v6, net_sin6(&addr), sizeof(struct net_sockaddr_in6));
	}

	ret = net_ipaddr_parse(CONFIG_DNS_SERVER2,
			       sizeof(CONFIG_DNS_SERVER2) - 1,
			       &addr);
	zassert_true(ret, "Cannot parse IP address %s", CONFIG_DNS_SERVER2);

	if (addr.sa_family == NET_AF_INET) {
		memcpy(&addr_v4, net_sin(&addr), sizeof(struct net_sockaddr_in));
	} else if (addr.sa_family == NET_AF_INET6) {
		memcpy(&addr_v6, net_sin6(&addr), sizeof(struct net_sockaddr_in6));
	}

	addr_str = zsock_inet_ntop(NET_AF_INET, &addr_v4.sin_addr, str, sizeof(str));
	NET_DBG("v4: [%s]:%d", addr_str, net_ntohs(addr_v4.sin_port));

	sock_v4 = prepare_listen_sock_udp_v4(&addr_v4);
	zassert_true(sock_v4 >= 0, "Invalid IPv4 socket");

	addr_str = zsock_inet_ntop(NET_AF_INET6, &addr_v6.sin6_addr, str, sizeof(str));
	NET_DBG("v6: [%s]:%d", addr_str, net_ntohs(addr_v6.sin6_port));

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
	struct zsock_addrinfo *res = NULL;

	queries_received = 0;

	/* This check simulates a local query that we will catch
	 * in process_dns() function. So we do not check the res variable
	 * as that will currently not contain anything useful. We just check
	 * that the query triggered a function call to process_dns() function
	 * and that it could parse the DNS query.
	 */
	(void)zsock_getaddrinfo(QUERY_HOST, NULL, NULL, &res);

	(void)sys_sem_take(&wait_data, K_NO_WAIT);
	(void)sys_sem_take(&wait_data, K_NO_WAIT);

	zassert_equal(queries_received, expected_query_count,
		      "Did not receive both IPv4 and IPv6 query (got %d, expected %d)",
		      queries_received, expected_query_count);

	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_cancelled)
{
	struct zsock_addrinfo *res = NULL;
	int ret;

	ret = zsock_getaddrinfo(QUERY_HOST, NULL, NULL, &res);

	(void)sys_sem_take(&wait_data, K_NO_WAIT);
	(void)sys_sem_take(&wait_data, K_NO_WAIT);

	zassert_equal(queries_received, expected_query_count,
		      "Did not receive both IPv4 and IPv6 query (got %d, expected %d)",
		      queries_received, expected_query_count);

	/* Without a local DNS server this request will be canceled. */
	zassert_equal(ret, DNS_EAI_CANCELED, "Invalid result");

	zsock_freeaddrinfo(res);
}

static void check_result_order(struct zsock_addrinfo *res, int first)
{
	int second = first == NET_AF_INET6 ? NET_AF_INET : NET_AF_INET6;
	int families[] = { first, first, second, second };
	struct zsock_addrinfo *ai = res;

	for (int i = 0; i < ARRAY_SIZE(families); i++) {
		zassert_not_null(ai, "Missing result %d", i);
		zassert_equal(ai->ai_family, families[i], "Invalid family for result %d", i);
		zassert_equal_ptr(ai->ai_addr, net_sad(&ai->_ai_addr),
				  "Invalid address pointer for result %d", i);

		/* Results of a family keep the order of the DNS answer */
		if (ai->ai_family == NET_AF_INET) {
			zassert_mem_equal(&net_sin(ai->ai_addr)->sin_addr,
					  answer_addrs_v4[i % 2], 4, "Invalid address %d", i);
		} else {
			zassert_mem_equal(&net_sin6(ai->ai_addr)->sin6_addr,
					  answer_addrs_v6[i % 2], 16, "Invalid address %d", i);
		}

		ai = ai->ai_next;
	}

	zassert_is_null(ai, "Too many results");
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_result_order)
{
	struct net_in_addr addr4 = { { { 192, 0, 2, 1 } } };
	struct net_in6_addr addr6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	const bool prefer_ipv4 = IS_ENABLED(CONFIG_NET_SOCKETS_DNS_PREFER_IPV4);
	const int preferred = prefer_ipv4 ? NET_AF_INET : NET_AF_INET6;
	const int other = prefer_ipv4 ? NET_AF_INET6 : NET_AF_INET;
	struct zsock_addrinfo *res = NULL;
	struct net_if *iface = net_if_get_default();
	int ret;

	answer_queries = true;

	/* No global address: results of the preferred family come first */
	ret = zsock_getaddrinfo(QUERY_HOST, NULL, NULL, &res);
	zassert_equal(ret, 0, "Invalid result (%d)", ret);
	check_result_order(res, preferred);
	zsock_freeaddrinfo(res);

	/* Only the other family is configured: its results come first */
	if (prefer_ipv4) {
		zassert_not_null(net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0),
				 "Cannot add IPv6 address");
	} else {
		zassert_not_null(net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0),
				 "Cannot add IPv4 address");
	}

	ret = zsock_getaddrinfo(QUERY_HOST, NULL, NULL, &res);
	zassert_equal(ret, 0, "Invalid result (%d)", ret);
	check_result_order(res, other);
	zsock_freeaddrinfo(res);

	if (prefer_ipv4) {
		zassert_true(net_if_ipv6_addr_rm(iface, &addr6), "Cannot remove IPv6 address");
	} else {
		zassert_true(net_if_ipv4_addr_rm(iface, &addr4), "Cannot remove IPv4 address");
	}

	answer_queries = false;
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_addrconfig)
{
	struct net_in_addr addr4 = { { { 192, 0, 2, 1 } } };
	struct net_in6_addr addr6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	struct zsock_addrinfo hints = {
		.ai_flags = ZSOCK_AI_ADDRCONFIG,
	};
	struct zsock_addrinfo *res = NULL;
	struct net_if *iface = net_if_get_default();
	int ret;

	/* Only loopback addresses: nothing is queried */
	ret = zsock_getaddrinfo(QUERY_HOST, NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_ADDRFAMILY, "Invalid result (%d)", ret);
	zassert_is_null(res, "");
	zassert_equal(queries_received, 0, "Unexpected queries (%d)", queries_received);

	/* IPv4 address: only A is queried */
	zassert_not_null(net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv4 address");
	reset_query_counters();

	ret = zsock_getaddrinfo(QUERY_HOST, NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_CANCELED, "Invalid result (%d)", ret);
	zassert_equal(queries_received_a, expected_query_count / 2,
		      "Invalid A query count (%d)", queries_received_a);
	zassert_equal(queries_received_aaaa, 0,
		      "Unexpected AAAA queries (%d)", queries_received_aaaa);
	zsock_freeaddrinfo(res);

	/* IPv4 and IPv6 addresses: both are queried */
	zassert_not_null(net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv6 address");
	reset_query_counters();

	ret = zsock_getaddrinfo(QUERY_HOST, NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_CANCELED, "Invalid result (%d)", ret);
	zassert_equal(queries_received_a, expected_query_count / 2,
		      "Invalid A query count (%d)", queries_received_a);
	zassert_equal(queries_received_aaaa, expected_query_count / 2,
		      "Invalid AAAA query count (%d)", queries_received_aaaa);
	zsock_freeaddrinfo(res);

	/* IPv6 address only: only AAAA is queried */
	zassert_true(net_if_ipv4_addr_rm(iface, &addr4), "Cannot remove IPv4 address");
	reset_query_counters();

	ret = zsock_getaddrinfo(QUERY_HOST, NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_CANCELED, "Invalid result (%d)", ret);
	zassert_equal(queries_received_a, 0,
		      "Unexpected A queries (%d)", queries_received_a);
	zassert_equal(queries_received_aaaa, expected_query_count / 2,
		      "Invalid AAAA query count (%d)", queries_received_aaaa);
	zsock_freeaddrinfo(res);

	zassert_true(net_if_ipv6_addr_rm(iface, &addr6), "Cannot remove IPv6 address");
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_no_host)
{
	struct zsock_addrinfo *res = NULL;
	int ret;

	ret = zsock_getaddrinfo(NULL, NULL, NULL, &res);

	zassert_equal(ret, DNS_EAI_SYSTEM, "Invalid result");
	zassert_equal(errno, EINVAL, "Invalid errno");
	zassert_is_null(res, "ai_addr is not NULL");

	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_num_ipv4)
{
	struct zsock_addrinfo *res = NULL;
	struct net_sockaddr_in *saddr;
	int ret;

	struct zsock_addrinfo hints = {
		.ai_family = NET_AF_INET,
		.ai_socktype = NET_SOCK_STREAM
	};

	ret = zsock_getaddrinfo("1.2.3.255", "65534", NULL, &res);

	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET, "");
	zassert_equal(res->ai_socktype, NET_SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_TCP, "");
	zsock_freeaddrinfo(res);

	ret = zsock_getaddrinfo("1.2.3.255", "65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET, "");
	zassert_equal(res->ai_socktype, NET_SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_TCP, "");
	zsock_freeaddrinfo(res);

	hints.ai_socktype = NET_SOCK_DGRAM;
	ret = zsock_getaddrinfo("1.2.3.255", "65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET, "");
	zassert_equal(res->ai_socktype, NET_SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_UDP, "");

	saddr = (struct net_sockaddr_in *)res->ai_addr;
	zassert_equal(saddr->sin_family, NET_AF_INET, "");
	zassert_equal(saddr->sin_port, net_htons(65534), "");
	zassert_equal(saddr->sin_addr.s4_addr[0], 1, "");
	zassert_equal(saddr->sin_addr.s4_addr[1], 2, "");
	zassert_equal(saddr->sin_addr.s4_addr[2], 3, "");
	zassert_equal(saddr->sin_addr.s4_addr[3], 255, "");
	zsock_freeaddrinfo(res);
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_num_ipv6)
{
	struct zsock_addrinfo *res = NULL;
	struct net_sockaddr_in6 *saddr;
	int ret;

	struct zsock_addrinfo hints = {
		.ai_family = NET_AF_INET6,
		.ai_socktype = NET_SOCK_STREAM
	};

	ret = zsock_getaddrinfo("[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]",
			"65534", NULL, &res);

	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET6, "");
	zassert_equal(res->ai_socktype, NET_SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_TCP, "");

	saddr = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr->sin6_port, net_htons(65534), "");
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
	zassert_equal(res->ai_family, NET_AF_INET6, "");
	zassert_equal(res->ai_socktype, NET_SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_TCP, "");

	saddr = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr->sin6_port, net_htons(65534), "");
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


	hints.ai_socktype = NET_SOCK_DGRAM;
	ret = zsock_getaddrinfo("[3ffe:2a00:100:7031::1]",
			"65534", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET6, "");
	zassert_equal(res->ai_socktype, NET_SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_UDP, "");

	saddr = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr->sin6_port, net_htons(65534), "");
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

	saddr = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr->sin6_port, net_htons(65534), "");
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

	saddr = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr->sin6_port, net_htons(65534), "");
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

	saddr = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr->sin6_port, net_htons(65534), "");
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

	saddr = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr->sin6_port, net_htons(65534), "");
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
		.ai_flags = ZSOCK_AI_NUMERICHOST,
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
		.ai_family = NET_AF_INET6,
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
		.ai_family = NET_AF_INET,
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
	struct net_sockaddr_in *saddr;
	struct net_sockaddr_in6 *saddr6;
	struct zsock_addrinfo *res = NULL;
	struct zsock_addrinfo hints = {
		.ai_family = NET_AF_INET,
		.ai_socktype = NET_SOCK_STREAM,
		.ai_flags = ZSOCK_AI_PASSIVE
	};
	int ret;

	/* Test IPv4 TCP */
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET, "");
	zassert_equal(res->ai_socktype, NET_SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_TCP, "");
	saddr = net_sin(res->ai_addr);
	zassert_equal(saddr->sin_family, NET_AF_INET, "");
	zassert_equal(saddr->sin_port, net_htons(80), "");
	zassert_equal(saddr->sin_addr.s_addr, NET_INADDR_ANY, "");
	zsock_freeaddrinfo(res);

	/* Test IPv6 TCP */
	hints.ai_family = NET_AF_INET6;
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET6, "");
	zassert_equal(res->ai_socktype, NET_SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_TCP, "");
	saddr6 = net_sin6(res->ai_addr);
	zassert_equal(saddr6->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr6->sin6_port, net_htons(80), "");
	zassert_equal(0, memcmp(&saddr6->sin6_addr, &net_in6addr_any, sizeof(net_in6addr_any)), "");
	zsock_freeaddrinfo(res);

	/* Test IPv6 UDP */
	hints.ai_socktype = NET_SOCK_DGRAM;
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET6, "");
	zassert_equal(res->ai_socktype, NET_SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_UDP, "");
	saddr6 = (struct net_sockaddr_in6 *)res->ai_addr;
	zassert_equal(saddr6->sin6_family, NET_AF_INET6, "");
	zassert_equal(saddr6->sin6_port, net_htons(80), "");
	zsock_freeaddrinfo(res);

	/* Test IPv4 UDP */
	hints.ai_family = NET_AF_INET;
	ret = zsock_getaddrinfo(NULL, "80", &hints, &res);
	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");
	zassert_equal(res->ai_family, NET_AF_INET, "");
	zassert_equal(res->ai_socktype, NET_SOCK_DGRAM, "");
	zassert_equal(res->ai_protocol, NET_IPPROTO_UDP, "");
	saddr = (struct net_sockaddr_in *)res->ai_addr;
	zassert_equal(saddr->sin_family, NET_AF_INET, "");
	zassert_equal(saddr->sin_port, net_htons(80), "");
	zsock_freeaddrinfo(res);
}

static void test_getaddrinfo_before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_query_counters();
}

ZTEST_SUITE(net_socket_getaddrinfo, NULL, test_getaddrinfo_setup, test_getaddrinfo_before,
	    NULL, NULL);
