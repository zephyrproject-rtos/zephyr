/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sem.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net_buf.h>

#include "../../socket_helpers.h"

#include "dns_pack.h"

#define QUERY_HOST "www.zephyrproject.org"

#define ANY_PORT 0
#define MAX_BUF_SIZE 128
/* Question plus up to four AAAA records */
#define ANSWER_BUF_SIZE 256
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_PRIORITY K_PRIO_COOP(2)
#define WAIT_TIME K_MSEC(250)

#define PREFERRED_FAMILY (IS_ENABLED(CONFIG_NET_SOCKETS_DNS_PREFER_IPV4) ? \
			  NET_AF_INET : NET_AF_INET6)

static uint8_t recv_buf[MAX_BUF_SIZE];

static int sock_v4;
static int sock_v6;

static struct net_sockaddr_in addr_v4;
static struct net_sockaddr_in6 addr_v6;

static int queries_received;
static enum dns_rr_type first_qtype;
static bool answer_queries;
static int answer_count_v4 = 2;
static int answer_count_v6 = 2;
static int expected_query_count =
	CONFIG_NET_SOCKETS_DNS_BACKOFF_INTERVAL >= CONFIG_NET_SOCKETS_DNS_TIMEOUT ?
	2 :
	/* Calculate for both IPv4 and IPv6 so need to double the value */
	2 * (LOG2CEIL(DIV_ROUND_UP(CONFIG_NET_SOCKETS_DNS_TIMEOUT,
				   CONFIG_NET_SOCKETS_DNS_BACKOFF_INTERVAL) + 1));

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

	if (first_qtype == 0) {
		first_qtype = qtype;
	}

	NET_DBG("[%d] query %s/%s label %s (%d bytes)", queries,
		qtype == DNS_RR_TYPE_A ? "A" : "AAAA", "IN",
		result->data, ret);

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
	{ 192, 0, 2, 12 },
	{ 192, 0, 2, 13 },
};

static const uint8_t answer_addrs_v6[][16] = {
	{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 },
	{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x11 },
	{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x12 },
	{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x13 },
};

/* Answer the query in recv_buf with answer_count_v4 A or answer_count_v6
 * AAAA records
 */
static void send_answer(int sock, const struct net_sockaddr *addr,
			net_socklen_t addr_len, int len)
{
	/* Static: too large for the server thread stack */
	static uint8_t buf[ANSWER_BUF_SIZE];
	uint16_t qtype;
	int count;
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
	count = qtype == DNS_RR_TYPE_A ? answer_count_v4 : answer_count_v6;

	memcpy(buf, recv_buf, pos);
	buf[2] = 0x81; /* Response, recursion desired */
	buf[3] = 0x80; /* Recursion available */
	sys_put_be16(count, &buf[6]); /* ANCOUNT */
	memset(&buf[8], 0, 4); /* NSCOUNT, ARCOUNT */

	for (int i = 0; i < count; i++) {
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

/* Keep the DNS replies on loopback, whatever the routes of other interfaces */
static void bind_to_loopback(int sock)
{
	struct net_ifreq ifreq = { .ifr_name = "lo" };
	int ret;

	ret = zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
			       &ifreq, sizeof(ifreq));
	zassert_equal(ret, 0, "Cannot bind to loopback (%d)", errno);
}

static void *test_getaddrinfo_setup(void)
{
	char str[NET_INET6_ADDRSTRLEN], *addr_str;
	struct net_sockaddr addr;
	int ret;

	/* Skip the interface name after '%' */
	ret = net_ipaddr_parse(CONFIG_DNS_SERVER1,
			       strcspn(CONFIG_DNS_SERVER1, "%"),
			       &addr);
	zassert_true(ret, "Cannot parse IP address %s", CONFIG_DNS_SERVER1);

	if (addr.sa_family == NET_AF_INET) {
		memcpy(&addr_v4, net_sin(&addr), sizeof(struct net_sockaddr_in));
	} else if (addr.sa_family == NET_AF_INET6) {
		memcpy(&addr_v6, net_sin6(&addr), sizeof(struct net_sockaddr_in6));
	}

	/* Skip the interface name after '%' */
	ret = net_ipaddr_parse(CONFIG_DNS_SERVER2,
			       strcspn(CONFIG_DNS_SERVER2, "%"),
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
	bind_to_loopback(sock_v4);

	addr_str = zsock_inet_ntop(NET_AF_INET6, &addr_v6.sin6_addr, str, sizeof(str));
	NET_DBG("v6: [%s]:%d", addr_str, net_ntohs(addr_v6.sin6_port));

	sock_v6 = prepare_listen_sock_udp_v6(&addr_v6);
	zassert_true(sock_v6 >= 0, "Invalid IPv6 socket");
	bind_to_loopback(sock_v6);

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

/* Check the families of the results. Results of a family keep the order of
 * the DNS answer, starting with its first record.
 */
static void check_results(struct zsock_addrinfo *res, const int *families, int count)
{
	struct zsock_addrinfo *ai = res;
	int idx_v4 = 0;
	int idx_v6 = 0;

	for (int i = 0; i < count; i++) {
		zassert_not_null(ai, "Missing result %d", i);
		zassert_equal(ai->ai_family, families[i], "Invalid family for result %d", i);
		zassert_equal_ptr(ai->ai_addr, net_sad(&ai->_ai_addr),
				  "Invalid address pointer for result %d", i);

		if (ai->ai_family == NET_AF_INET) {
			zassert_mem_equal(&net_sin(ai->ai_addr)->sin_addr,
					  answer_addrs_v4[idx_v4++], 4, "Invalid address %d", i);
		} else {
			zassert_mem_equal(&net_sin6(ai->ai_addr)->sin6_addr,
					  answer_addrs_v6[idx_v6++], 16, "Invalid address %d", i);
		}

		ai = ai->ai_next;
	}

	zassert_is_null(ai, "Too many results");
}

static void check_result_order(struct zsock_addrinfo *res, int first, int count)
{
	int second = first == NET_AF_INET6 ? NET_AF_INET : NET_AF_INET6;
	const int families[] = { first, first, second, second };

	check_results(res, families, count);
}

static void check_getaddrinfo_family(int family, int first)
{
	struct zsock_addrinfo hints = { .ai_family = family };
	struct zsock_addrinfo *res = NULL;
	/* The preferred family is queried first, regardless of reachability */
	int queried = family == NET_AF_UNSPEC ? PREFERRED_FAMILY : family;
	int ret;

	first_qtype = 0;
	ret = zsock_getaddrinfo(QUERY_HOST, NULL, &hints, &res);
	zassert_equal(ret, 0, "Invalid result (%d)", ret);
	zassert_equal(first_qtype, queried == NET_AF_INET6 ? DNS_RR_TYPE_AAAA : DNS_RR_TYPE_A,
		      "Invalid first query type (%d)", first_qtype);
	check_result_order(res, first, family == NET_AF_UNSPEC ? 4 : 2);
	zsock_freeaddrinfo(res);
}

static void check_getaddrinfo_order(int first)
{
	check_getaddrinfo_family(NET_AF_UNSPEC, first);
}

/* Fake Ethernet interface: it resolves next hops with ARP, unlike loopback */
static void eth_fake_iface_init(struct net_if *iface)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	uint8_t mac_addr[6] = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 };

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static const struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", NULL, NULL, NULL, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs, NET_ETH_MTU);

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_result_order)
{
	/* Not on the subnet of the DNS answers with the default netmask */
	struct net_in_addr addr4 = { { { 198, 51, 100, 1 } } };
	struct net_in_addr gw4 = { { { 198, 51, 100, 254 } } };
	struct net_in_addr onlink4 = { { { 192, 0, 2, 1 } } };
	struct net_in_addr no_gw4 = { 0 };
	struct net_in6_addr addr6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	struct net_in6_addr prefix6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct net_in6_addr router6 = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0xfe } } };
	const int preferred = PREFERRED_FAMILY;
	struct net_if *eth = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	struct net_if *lo = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	struct net_if_router *router;
	struct net_if_router *router_v4;

	zassert_not_null(eth, "No Ethernet interface");
	zassert_not_null(lo, "No loopback interface");

	answer_queries = true;

	/* No reachable destination: the preferred family comes first */
	check_getaddrinfo_order(preferred);

	/* IPv6 address without a route, IPv4 address with a gateway: IPv4 first */
	zassert_not_null(net_if_ipv6_addr_add(eth, &addr6, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv6 address");
	zassert_not_null(net_if_ipv4_addr_add(eth, &addr4, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv4 address");
	net_if_ipv4_set_gw(eth, &gw4);
	check_getaddrinfo_order(NET_AF_INET);

	/* IPv6 default router: both reachable, the preferred family comes first */
	router = net_if_ipv6_router_add(eth, &router6, true, 0);
	zassert_not_null(router, "Cannot add IPv6 router");
	check_getaddrinfo_order(preferred);

	/* No IPv4 gateway on an ARP interface: only IPv6 reachable */
	net_if_ipv4_set_gw(eth, &no_gw4);
	check_getaddrinfo_order(NET_AF_INET6);

	/* IPv4 default router instead of a gateway: both reachable */
	router_v4 = net_if_ipv4_router_add(eth, &gw4, true, 0);
	zassert_not_null(router_v4, "Cannot add IPv4 router");
	check_getaddrinfo_order(preferred);
	zassert_true(net_if_ipv4_router_rm(router_v4), "Cannot remove IPv4 router");

	/* No IPv6 router but an on-link IPv6 prefix: only IPv6 reachable */
	zassert_true(net_if_ipv6_router_rm(router), "Cannot remove IPv6 router");
	zassert_not_null(net_if_ipv6_prefix_add(eth, &prefix6, 64, UINT32_MAX),
			 "Cannot add IPv6 prefix");
	check_getaddrinfo_order(NET_AF_INET6);

	/* IPv4 address on the subnet of the DNS answers: both reachable */
	zassert_true(net_if_ipv4_addr_rm(eth, &addr4), "Cannot remove IPv4 address");
	zassert_not_null(net_if_ipv4_addr_add(eth, &onlink4, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv4 address");
	check_getaddrinfo_order(preferred);

	zassert_true(net_if_ipv6_prefix_rm(eth, &prefix6, 64), "Cannot remove IPv6 prefix");
	zassert_true(net_if_ipv4_addr_rm(eth, &onlink4), "Cannot remove IPv4 address");
	zassert_true(net_if_ipv6_addr_rm(eth, &addr6), "Cannot remove IPv6 address");

	/* Off-link IPv4 without a gateway on an interface without ARP, as on
	 * PPP: only IPv4 reachable
	 */
	zassert_not_null(net_if_ipv4_addr_add(lo, &addr4, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv4 address");
	check_getaddrinfo_order(NET_AF_INET);

	/* Same with an on-link IPv6 prefix: both reachable */
	zassert_not_null(net_if_ipv6_addr_add(lo, &addr6, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv6 address");
	zassert_not_null(net_if_ipv6_prefix_add(lo, &prefix6, 64, UINT32_MAX),
			 "Cannot add IPv6 prefix");
	check_getaddrinfo_order(preferred);

	zassert_true(net_if_ipv6_prefix_rm(lo, &prefix6, 64), "Cannot remove IPv6 prefix");
	zassert_true(net_if_ipv6_addr_rm(lo, &addr6), "Cannot remove IPv6 address");
	zassert_true(net_if_ipv4_addr_rm(lo, &addr4), "Cannot remove IPv4 address");

	answer_queries = false;
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_family_hint)
{
	answer_queries = true;

	/* Only the requested family is queried and returned */
	check_getaddrinfo_family(NET_AF_INET6, NET_AF_INET6);
	check_getaddrinfo_family(NET_AF_INET, NET_AF_INET);

	answer_queries = false;
}

static void check_getaddrinfo_slots(int count_preferred, int count_other,
				    const int *families, int count)
{
	struct zsock_addrinfo *res = NULL;
	int ret;

	answer_count_v4 = PREFERRED_FAMILY == NET_AF_INET ? count_preferred : count_other;
	answer_count_v6 = PREFERRED_FAMILY == NET_AF_INET6 ? count_preferred : count_other;

	ret = zsock_getaddrinfo(QUERY_HOST, NULL, NULL, &res);
	zassert_equal(ret, 0, "Invalid result (%d)", ret);
	check_results(res, families, count);
	zsock_freeaddrinfo(res);
}

/* Make destinations of the DNS answers of a family reachable, or not */
static void set_family_reachable(int family, bool reachable)
{
	struct net_in_addr addr4 = { { { 192, 0, 2, 1 } } };
	struct net_in6_addr addr6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	struct net_in6_addr prefix6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	if (family == NET_AF_INET && reachable) {
		zassert_not_null(net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0),
				 "Cannot add IPv4 address");
	} else if (family == NET_AF_INET) {
		zassert_true(net_if_ipv4_addr_rm(iface, &addr4), "Cannot remove IPv4 address");
	} else if (reachable) {
		zassert_not_null(net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0),
				 "Cannot add IPv6 address");
		zassert_not_null(net_if_ipv6_prefix_add(iface, &prefix6, 64, UINT32_MAX),
				 "Cannot add IPv6 prefix");
	} else {
		zassert_true(net_if_ipv6_prefix_rm(iface, &prefix6, 64),
			     "Cannot remove IPv6 prefix");
		zassert_true(net_if_ipv6_addr_rm(iface, &addr6), "Cannot remove IPv6 address");
	}
}

ZTEST(net_socket_getaddrinfo, test_getaddrinfo_slots)
{
	const int pref = PREFERRED_FAMILY;
	const int other = pref == NET_AF_INET6 ? NET_AF_INET : NET_AF_INET6;

	BUILD_ASSERT(CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES == 4, "The test assumes 4 slots");

	answer_queries = true;

	/* The other family takes back half of the slots from the preferred one */
	check_getaddrinfo_slots(4, 2, (const int[]){ pref, pref, other, other }, 4);

	/* It only takes back the slots it needs */
	check_getaddrinfo_slots(4, 1, (const int[]){ pref, pref, pref, other }, 4);

	/* Slots left over by one family go to the other one */
	check_getaddrinfo_slots(4, 0, (const int[]){ pref, pref, pref, pref }, 4);
	check_getaddrinfo_slots(0, 4, (const int[]){ other, other, other, other }, 4);
	check_getaddrinfo_slots(1, 4, (const int[]){ pref, other, other, other }, 4);

	/* Only the other family is reachable: its addresses are still returned */
	set_family_reachable(other, true);
	check_getaddrinfo_slots(4, 2, (const int[]){ other, other, pref, pref }, 4);
	set_family_reachable(other, false);

	answer_count_v4 = 2;
	answer_count_v6 = 2;
	answer_queries = false;
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

ZTEST_SUITE(net_socket_getaddrinfo, NULL, test_getaddrinfo_setup, NULL, NULL, NULL);
