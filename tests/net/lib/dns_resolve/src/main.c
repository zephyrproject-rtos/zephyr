/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_DNS_RESOLVER_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dns_resolve.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_DNS_RESOLVER_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define NAME4 "4.zephyr.test"
#define NAME6 "6.zephyr.test"
#define NAME_IPV4 "192.0.2.1"
#define NAME_IPV6 "2001:db8::1"

#define DNS_TIMEOUT 500 /* ms */
#define THREAD_SLEEP 10

#if defined(CONFIG_NET_IPV6)
/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };
#endif

#if defined(CONFIG_NET_IPV4)
/* Interface 1 addresses */
static struct in_addr my_addr2 = { { { 192, 0, 2, 1 } } };
#endif

static struct net_if *iface1;

static bool test_failed;
static bool test_started;
static bool timeout_query;
static struct k_sem wait_data;
static struct k_sem wait_data2;
static uint16_t current_dns_id;
static struct dns_addrinfo addrinfo;

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
#define EXPECTED_SERVER_COUNT CONFIG_DNS_RESOLVER_MAX_SERVERS
#else
#define EXPECTED_SERVER_COUNT (CONFIG_DNS_RESOLVER_MAX_SERVERS / 2)
#endif

/* this must be higher that the DNS_TIMEOUT */
#define WAIT_TIME K_MSEC(DNS_TIMEOUT + 300)

struct net_if_test {
	uint8_t idx;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand8_get();
	}

	memcpy(data->ll_addr.addr, data->mac_addr, sizeof(data->mac_addr));
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);

	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
}

static inline int get_slot_by_id(struct dns_resolve_context *ctx,
				 uint16_t dns_id)
{
	int i;

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (ctx->queries[i].cb && ctx->queries[i].id == dns_id) {
			return i;
		}
	}

	return -1;
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (!timeout_query) {
		struct net_if_test *data = dev->data;
		struct dns_resolve_context *ctx;
		int slot;

		if (net_if_get_by_iface(net_pkt_iface(pkt)) != data->idx) {
			DBG("Invalid interface %d index, expecting %d\n",
			    data->idx, net_if_get_by_iface(net_pkt_iface(pkt)));
			test_failed = true;
		}

		ctx = dns_resolve_get_default();

		slot = get_slot_by_id(ctx, current_dns_id);
		if (slot < 0) {
			DBG("Skipping this query dns id %u\n", current_dns_id);
			goto out;
		}

		/* We need to cancel the query manually so that we
		 * will not get a timeout.
		 */
		k_work_cancel_delayable(&ctx->queries[slot].timer);

		DBG("Calling cb %p with user data %p\n",
		    ctx->queries[slot].cb,
		    ctx->queries[slot].user_data);

		ctx->queries[slot].cb(DNS_EAI_INPROGRESS,
				      &addrinfo,
				      ctx->queries[slot].user_data);
		ctx->queries[slot].cb(DNS_EAI_ALLDONE,
				      NULL,
				      ctx->queries[slot].user_data);

		ctx->queries[slot].cb = NULL;
	}

out:
	return 0;
}

struct net_if_test net_iface1_data;

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

static void *test_init(void)
{
	struct net_if_addr *ifaddr;

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);
	k_sem_init(&wait_data2, 0, UINT_MAX);

	iface1 = net_if_get_by_index(0);
	zassert_is_null(iface1, "iface1");

	iface1 = net_if_get_by_index(1);

	((struct net_if_test *) net_if_get_device(iface1)->data)->idx =
		net_if_get_by_iface(iface1);

#if defined(CONFIG_NET_IPV6)
	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");

		return NULL;
	}

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&ll_addr));
		zassert_not_null(ifaddr, "ll_addr");

		return NULL;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;
#endif

#if defined(CONFIG_NET_IPV4)
	ifaddr = net_if_ipv4_addr_add(iface1, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");

		return NULL;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;
#endif

	net_if_up(iface1);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;

	test_started = true;

	return NULL;
}

void dns_result_cb_dummy(enum dns_resolve_status status,
			 struct dns_addrinfo *info,
			 void *user_data)
{
	return;
}

ZTEST(dns_resolve, test_dns_query_invalid_timeout)
{
	int ret;

	ret = dns_get_addr_info(NAME6,
				DNS_QUERY_TYPE_AAAA,
				NULL,
				dns_result_cb_dummy,
				NULL,
				0);
	zassert_equal(ret, -EINVAL, "Wrong return code for timeout");
}

ZTEST(dns_resolve, test_dns_query_invalid_context)
{
	int ret;

	ret = dns_resolve_name(NULL,
			       NAME6,
			       DNS_QUERY_TYPE_AAAA,
			       NULL,
			       dns_result_cb_dummy,
			       NULL,
			       DNS_TIMEOUT);
	zassert_equal(ret, -EINVAL, "Wrong return code for context");
}

ZTEST(dns_resolve, test_dns_query_invalid_callback)
{
	int ret;

	ret = dns_get_addr_info(NAME6,
				DNS_QUERY_TYPE_AAAA,
				NULL,
				NULL,
				NULL,
				DNS_TIMEOUT);
	zassert_equal(ret, -EINVAL, "Wrong return code for callback");
}

ZTEST(dns_resolve, test_dns_query_invalid_query)
{
	int ret;

	ret = dns_get_addr_info(NULL,
				DNS_QUERY_TYPE_AAAA,
				NULL,
				dns_result_cb_dummy,
				NULL,
				DNS_TIMEOUT);
	zassert_equal(ret, -EINVAL, "Wrong return code for query");
}

void dns_result_cb_timeout(enum dns_resolve_status status,
			   struct dns_addrinfo *info,
			   void *user_data)
{
	int expected_status = POINTER_TO_INT(user_data);

	if (expected_status != status) {
		DBG("Result status %d\n", status);
		DBG("Expected status %d\n", expected_status);

		zassert_equal(expected_status, status, "Invalid status");
	}

	k_sem_give(&wait_data);
}

ZTEST(dns_resolve, test_dns_query_server_count)
{
	struct dns_resolve_context *ctx = dns_resolve_get_default();
	int i, count = 0;

	for (i = 0; i < EXPECTED_SERVER_COUNT; i++) {
		if (ctx->state != DNS_RESOLVE_CONTEXT_ACTIVE) {
			continue;
		}

		if (ctx->servers[i].sock < 0) {
			continue;
		}

		count++;
	}

	zassert_equal(count, EXPECTED_SERVER_COUNT,
		      "Invalid number of servers (%d vs %d)",
		      count, EXPECTED_SERVER_COUNT);
}

ZTEST(dns_resolve, test_dns_query_ipv4_server_count)
{
	struct dns_resolve_context *ctx = dns_resolve_get_default();
	int i, count = 0, port = 0;

	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		return;
	}

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS; i++) {
		if (ctx->state != DNS_RESOLVE_CONTEXT_ACTIVE) {
			continue;
		}

		if (ctx->servers[i].sock < 0) {
			continue;
		}

		if (ctx->servers[i].dns_server.sa_family == AF_INET6) {
			continue;
		}

		count++;

		if (net_sin(&ctx->servers[i].dns_server)->sin_port ==
		    ntohs(53)) {
			port++;
		}
	}

	zassert_equal(count, 2, "Invalid number of IPv4 servers");
	zassert_equal(port, 1, "Invalid number of IPv4 servers with port 53");
}

ZTEST(dns_resolve, test_dns_query_ipv6_server_count)
{
	struct dns_resolve_context *ctx = dns_resolve_get_default();
	int i, count = 0, port = 0;

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS; i++) {
		if (ctx->state != DNS_RESOLVE_CONTEXT_ACTIVE) {
			continue;
		}

		if (ctx->servers[i].sock < 0) {
			continue;
		}

		if (ctx->servers[i].dns_server.sa_family == AF_INET) {
			continue;
		}

		count++;

		if (net_sin6(&ctx->servers[i].dns_server)->sin6_port ==
		    ntohs(53)) {
			port++;
		}
	}

#if defined(CONFIG_NET_IPV6)
	zassert_equal(count, 2, "Invalid number of IPv6 servers");
	zassert_equal(port, 1, "Invalid number of IPv6 servers with port 53");
#else
	zassert_equal(count, 0, "Invalid number of IPv6 servers");
	zassert_equal(port, 0, "Invalid number of IPv6 servers with port 53");
#endif
}

ZTEST(dns_resolve, test_dns_query_too_many)
{
	int expected_status = DNS_EAI_CANCELED;
	int ret;

	timeout_query = true;

	ret = dns_get_addr_info(NAME4,
				DNS_QUERY_TYPE_A,
				NULL,
				dns_result_cb_timeout,
				INT_TO_POINTER(expected_status),
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv4 query");

	ret = dns_get_addr_info(NAME4,
				DNS_QUERY_TYPE_A,
				NULL,
				dns_result_cb_dummy,
				INT_TO_POINTER(expected_status),
				DNS_TIMEOUT);
	zassert_equal(ret, -EAGAIN, "Should have run out of space");

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}

	timeout_query = false;
}

ZTEST(dns_resolve, test_dns_query_ipv4_timeout)
{
	int expected_status = DNS_EAI_CANCELED;
	int ret;

	timeout_query = true;

	ret = dns_get_addr_info(NAME4,
				DNS_QUERY_TYPE_A,
				NULL,
				dns_result_cb_timeout,
				INT_TO_POINTER(expected_status),
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv4 query");

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}

	timeout_query = false;
}

ZTEST(dns_resolve, test_dns_query_ipv6_timeout)
{
	int expected_status = DNS_EAI_CANCELED;
	int ret;

	timeout_query = true;

	ret = dns_get_addr_info(NAME6,
				DNS_QUERY_TYPE_AAAA,
				NULL,
				dns_result_cb_timeout,
				INT_TO_POINTER(expected_status),
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv6 query");

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}

	timeout_query = false;
}

static void verify_cancelled(void)
{
	struct dns_resolve_context *ctx = dns_resolve_get_default();
	int i, count = 0, timer_not_stopped = 0;

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (ctx->queries[i].cb) {
			count++;
		}

		if (k_work_delayable_busy_get(&ctx->queries[i].timer) != 0) {
			timer_not_stopped++;
		}
	}

	zassert_equal(count, 0, "Not all pending queries vere cancelled");
	zassert_equal(timer_not_stopped, 0, "Not all timers vere cancelled");
}

ZTEST(dns_resolve, test_dns_query_ipv4_cancel)
{
	int expected_status = DNS_EAI_CANCELED;
	uint16_t dns_id;
	int ret;

	timeout_query = true;

	ret = dns_get_addr_info(NAME4,
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb_timeout,
				INT_TO_POINTER(expected_status),
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv4 query");

	ret = dns_cancel_addr_info(dns_id);
	zassert_equal(ret, 0, "Cannot cancel IPv4 query");

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}

	verify_cancelled();
}

ZTEST(dns_resolve, test_dns_query_ipv6_cancel)
{
	int expected_status = DNS_EAI_CANCELED;
	uint16_t dns_id;
	int ret;

	timeout_query = true;

	ret = dns_get_addr_info(NAME6,
				DNS_QUERY_TYPE_AAAA,
				&dns_id,
				dns_result_cb_timeout,
				INT_TO_POINTER(expected_status),
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv6 query");

	ret = dns_cancel_addr_info(dns_id);
	zassert_equal(ret, 0, "Cannot cancel IPv6 query");

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}

	verify_cancelled();
}

struct expected_status {
	int status1;
	int status2;
	const char *caller;
};

void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	struct expected_status *expected = user_data;

	if (status != expected->status1 && status != expected->status2) {
		DBG("Result status %d\n", status);
		DBG("Expected status1 %d\n", expected->status1);
		DBG("Expected status2 %d\n", expected->status2);
		DBG("Caller %s\n", expected->caller);

		zassert_true(false, "Invalid status");
	}

	k_sem_give(&wait_data2);
}

ZTEST(dns_resolve, test_dns_query_ipv4)
{
	struct expected_status status = {
		.status1 = DNS_EAI_INPROGRESS,
		.status2 = DNS_EAI_ALLDONE,
		.caller = __func__,
	};
	int ret;

	timeout_query = false;

	ret = dns_get_addr_info(NAME4,
				DNS_QUERY_TYPE_A,
				&current_dns_id,
				dns_result_cb,
				&status,
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv4 query");

	DBG("Query id %u\n", current_dns_id);

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	if (k_sem_take(&wait_data2, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}
}

ZTEST(dns_resolve, test_dns_query_ipv6)
{
	struct expected_status status = {
		.status1 = DNS_EAI_INPROGRESS,
		.status2 = DNS_EAI_ALLDONE,
		.caller = __func__,
	};
	int ret;

	timeout_query = false;

	ret = dns_get_addr_info(NAME6,
				DNS_QUERY_TYPE_AAAA,
				&current_dns_id,
				dns_result_cb,
				&status,
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv6 query");

	DBG("Query id %u\n", current_dns_id);

	k_yield(); /* mandatory so that net_if send func gets to run */

	if (k_sem_take(&wait_data2, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}
}

struct expected_addr_status {
	struct sockaddr addr;
	int status1;
	int status2;
	const char *caller;
};

void dns_result_numeric_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info,
			   void *user_data)
{
	struct expected_addr_status *expected = user_data;

	if (status != expected->status1 && status != expected->status2) {
		DBG("Result status %d\n", status);
		DBG("Expected status1 %d\n", expected->status1);
		DBG("Expected status2 %d\n", expected->status2);
		DBG("Caller %s\n", expected->caller);

		zassert_true(false, "Invalid status");
	}

	if (info && info->ai_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		if (net_ipv4_addr_cmp(&net_sin(&info->ai_addr)->sin_addr,
				      &my_addr2) != true) {
			zassert_true(false, "IPv4 address does not match");
		}
#endif
	}

	if (info && info->ai_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		if (net_ipv6_addr_cmp(&net_sin6(&info->ai_addr)->sin6_addr,
				      &my_addr3) != true) {
			zassert_true(false, "IPv6 address does not match");
		}
#endif
	}

	k_sem_give(&wait_data2);
}

ZTEST(dns_resolve, test_dns_query_ipv4_numeric)
{
	struct expected_addr_status status = {
		.status1 = DNS_EAI_INPROGRESS,
		.status2 = DNS_EAI_ALLDONE,
		.caller = __func__,
	};
	int ret;

	timeout_query = false;

	ret = dns_get_addr_info(NAME_IPV4,
				DNS_QUERY_TYPE_A,
				&current_dns_id,
				dns_result_numeric_cb,
				&status,
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv4 numeric query");

	DBG("Query id %u\n", current_dns_id);

	k_yield(); /* mandatory so that net_if send func gets to run */

	if (k_sem_take(&wait_data2, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}
}

ZTEST(dns_resolve, test_dns_query_ipv6_numeric)
{
	struct expected_addr_status status = {
		.status1 = DNS_EAI_INPROGRESS,
		.status2 = DNS_EAI_ALLDONE,
		.caller = __func__,
	};
	int ret;

	timeout_query = false;

	ret = dns_get_addr_info(NAME_IPV6,
				DNS_QUERY_TYPE_AAAA,
				&current_dns_id,
				dns_result_numeric_cb,
				&status,
				DNS_TIMEOUT);
	zassert_equal(ret, 0, "Cannot create IPv6 query");

	DBG("Query id %u\n", current_dns_id);

	k_yield(); /* mandatory so that net_if send func gets to run */

	if (k_sem_take(&wait_data2, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}
}

#define MDNS_IPV4_ADDR "224.0.0.251:5353"
#define MDNS_IPV6_ADDR "[ff02::fb]:5353"

ZTEST(dns_resolve, test_mdns_ipv4_igmp_group)
{
	struct net_if_mcast_addr *maddr;
	struct sockaddr_in addr4;
	bool st;

	/* Skip this test if mDNS responder is enabled because it will join
	 * multicast group automatically.
	 */
	Z_TEST_SKIP_IFDEF(CONFIG_MDNS_RESPONDER);
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4);
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_IGMP);

	st = net_ipaddr_parse(MDNS_IPV4_ADDR, sizeof(MDNS_IPV4_ADDR) - 1,
			      (struct sockaddr *)&addr4);
	zassert_true(st, "Cannot parse IPv4 address");

	maddr = net_if_ipv4_maddr_lookup(&addr4.sin_addr, NULL);
	zassert_not_null(maddr, "IPv4 mDNS address not found");

	st = net_if_ipv4_maddr_is_joined(maddr);
	zassert_true(st, "IPv4 mDNS group not joined");
}

ZTEST(dns_resolve, test_mdns_ipv6_mld_group)
{
	struct net_if_mcast_addr *maddr;
	struct sockaddr_in6 addr6;
	bool st;

	/* Skip this test if mDNS responder is enabled because it will join
	 * multicast group automatically.
	 */
	Z_TEST_SKIP_IFDEF(CONFIG_MDNS_RESPONDER);
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6);
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_MLD);

	st = net_ipaddr_parse(MDNS_IPV6_ADDR, sizeof(MDNS_IPV6_ADDR) - 1,
			      (struct sockaddr *)&addr6);
	zassert_true(st, "Cannot parse IPv6 address");

	maddr = net_if_ipv6_maddr_lookup(&addr6.sin6_addr, NULL);
	zassert_not_null(maddr, "IPv6 mDNS address not found");

	st = net_if_ipv6_maddr_is_joined(maddr);
	zassert_true(st, "IPv6 mDNS group not joined");
}

ZTEST_SUITE(dns_resolve, NULL, test_init, NULL, NULL, NULL);
