/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>
#include <sys/mutex.h>
#include <net/socket.h>
#include <net/dns_resolve.h>
#include <net/buf.h>

#include "../../socket_helpers.h"

#include "dns_pack.h"

#define QUERY_HOST "www.zephyrproject.org"

#define ANY_PORT 0
#define MAX_BUF_SIZE 128
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREAD_PRIORITY K_PRIO_COOP(8)
#define WAIT_TIME 250

static u8_t recv_buf[MAX_BUF_SIZE];

static int sock_v4;
static int sock_v6;

static struct sockaddr_in addr_v4;
static struct sockaddr_in6 addr_v6;

static int queries_received;

/* The mutex is there to wait the data to be received. */
static ZTEST_BMEM SYS_MUTEX_DEFINE(wait_data);

NET_BUF_POOL_DEFINE(test_dns_msg_pool, 1, 512, 0, NULL);

static bool check_dns_query(u8_t *buf, int buf_len)
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
		log_strdup(result->data), ret);

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

	sys_mutex_lock(&wait_data, K_FOREVER);

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
					sys_mutex_unlock(&wait_data);
				}
			}
		}
	}

	return -errno;
}

K_THREAD_DEFINE(dns_server_thread_id, STACK_SIZE,
		process_dns, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, K_FOREVER);

void test_getaddrinfo_setup(void)
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
	NET_DBG("v4: [%s]:%d", log_strdup(addr_str), ntohs(addr_v4.sin_port));

	sock_v4 = prepare_listen_sock_udp_v4(&addr_v4);
	zassert_true(sock_v4 >= 0, "Invalid IPv4 socket");

	addr_str = inet_ntop(AF_INET6, &addr_v6.sin6_addr, str, sizeof(str));
	NET_DBG("v6: [%s]:%d", log_strdup(addr_str), ntohs(addr_v6.sin6_port));

	sock_v6 = prepare_listen_sock_udp_v6(&addr_v6);
	zassert_true(sock_v6 >= 0, "Invalid IPv6 socket");

	k_thread_start(dns_server_thread_id);

	k_yield();
}

void test_getaddrinfo_ok(void)
{
	struct addrinfo *res = NULL;

	queries_received = 0;

	/* This check simulates a local query that we will catch
	 * in dns_process() function. So we do not check the res variable
	 * as that will currently not contain anything useful. We just check
	 * that the query triggered a function call to dns_process() function
	 * and that it could parse the DNS query.
	 */
	(void)getaddrinfo(QUERY_HOST, NULL, NULL, &res);

	if (sys_mutex_lock(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout DNS query not received");
	}

	zassert_equal(queries_received, 2,
		      "Did not receive both IPv4 and IPv6 query");

	freeaddrinfo(res);
}

void test_getaddrinfo_cancelled(void)
{
	struct addrinfo *res = NULL;
	int ret;

	ret = getaddrinfo(QUERY_HOST, NULL, NULL, &res);

	/* Without a local DNS server this request will be canceled. */
	zassert_equal(ret, DNS_EAI_CANCELED, "Invalid result");

	freeaddrinfo(res);
}

void test_getaddrinfo_no_host(void)
{
	struct addrinfo *res = NULL;
	int ret;

	ret = getaddrinfo(NULL, NULL, NULL, &res);

	zassert_equal(ret, DNS_EAI_SYSTEM, "Invalid result");
	zassert_equal(errno, EINVAL, "Invalid errno");
	zassert_is_null(res, "ai_addr is not NULL");

	freeaddrinfo(res);
}

void test_getaddrinfo_num_ipv4(void)
{
	struct zsock_addrinfo *res = NULL;
	struct sockaddr_in *saddr;
	int ret;

	ret = zsock_getaddrinfo("1.2.3.255", "65534", NULL, &res);

	zassert_equal(ret, 0, "Invalid result");
	zassert_not_null(res, "");
	zassert_is_null(res->ai_next, "");

	zassert_equal(res->ai_family, AF_INET, "");
	zassert_equal(res->ai_socktype, SOCK_STREAM, "");
	zassert_equal(res->ai_protocol, IPPROTO_TCP, "");

	saddr = (struct sockaddr_in *)res->ai_addr;
	zassert_equal(saddr->sin_family, AF_INET, "");
	zassert_equal(saddr->sin_port, htons(65534), "");
	zassert_equal(saddr->sin_addr.s4_addr[0], 1, "");
	zassert_equal(saddr->sin_addr.s4_addr[1], 2, "");
	zassert_equal(saddr->sin_addr.s4_addr[2], 3, "");
	zassert_equal(saddr->sin_addr.s4_addr[3], 255, "");

	zsock_freeaddrinfo(res);
}

void test_getaddrinfo_flags_numerichost(void)
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

static void test_getaddrinfo_ipv4_hints_ipv6(void)
{
	struct zsock_addrinfo *res = NULL;
	struct zsock_addrinfo hints = {
		.ai_family = AF_INET6,
	};
	int ret;

	ret = zsock_getaddrinfo("192.0.2.1", NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_ADDRFAMILY, "Invalid result (%d)", ret);
	zassert_is_null(res, "");
}

static void test_getaddrinfo_ipv6_hints_ipv4(void)
{
	struct zsock_addrinfo *res = NULL;
	struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
	};
	int ret;

	ret = zsock_getaddrinfo("2001:db8::1", NULL, &hints, &res);
	zassert_equal(ret, DNS_EAI_ADDRFAMILY, "Invalid result (%d)", ret);
	zassert_is_null(res, "");
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_getaddrinfo,
			 ztest_unit_test(test_getaddrinfo_setup),
			 ztest_unit_test(test_getaddrinfo_ok),
			 ztest_unit_test(test_getaddrinfo_cancelled),
			 ztest_unit_test(test_getaddrinfo_no_host),
			 ztest_unit_test(test_getaddrinfo_num_ipv4),
			 ztest_unit_test(test_getaddrinfo_flags_numerichost),
			 ztest_unit_test(test_getaddrinfo_ipv4_hints_ipv6),
			 ztest_unit_test(test_getaddrinfo_ipv6_hints_ipv4));

	ztest_run_test_suite(socket_getaddrinfo);
}
