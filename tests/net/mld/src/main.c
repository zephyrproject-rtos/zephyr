/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV6_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>

#include <zephyr/net/mld.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

#include <zephyr/random/random.h>

#include "icmpv6.h"
#include "ipv6.h"
#include "route_ipv6.h"

#define THREAD_SLEEP 50 /* ms */
#define MLD_REPORT_ADDR_COUNT 8

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IPV6_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

struct mld_report_mcast_record {
	uint8_t record_type;
	uint8_t aux_data_len;
	uint16_t num_of_sources;
	struct net_in6_addr mcast_addr;
} __packed;

struct mld_report_info {
	uint16_t records_count;
	struct mld_report_mcast_record records[MLD_REPORT_ADDR_COUNT];
};

typedef void (*mld_report_callback)(struct net_pkt *pkt, void *user_data);

struct mld_report_handler {
	mld_report_callback fn;
	void *user_data;
};

static struct net_in6_addr my_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct net_in6_addr peer_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x2 } } };
/* A transient site-local group, MLD is not run for scope 0 */
static const struct net_in6_addr mcast_addr = { { { 0xff, 0x15, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static const struct net_in6_addr mldv2_routers_addr = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
							0, 0, 0, 0, 0, 0, 0, 0x16 } } };

static struct net_in6_addr exp_mcast_group_storage;
static const struct net_in6_addr *exp_mcast_group;
static struct net_if *net_iface;
static bool is_group_joined;
static bool is_group_left;
static bool is_join_msg_ok;
static bool is_leave_msg_ok;
static bool is_query_received;
static bool is_report_sent;
static bool is_v1_report_sent;
static bool is_v1_done_sent;
static int report_count;

static struct mld_report_handler *report_handler;

K_SEM_DEFINE(wait_data, 0, UINT_MAX);
K_SEM_DEFINE(wait_joined, 0, UINT_MAX);
K_SEM_DEFINE(wait_left, 0, UINT_MAX);
K_SEM_DEFINE(wait_report, 0, UINT_MAX);

#define WAIT_TIME 500
#define WAIT_TIME_LONG MSEC_PER_SEC
#define MY_PORT 1969
#define PEER_PORT 13856

struct net_test_mld {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_mld *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand8_get();
	}

	return context->mac_addr;
}

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static struct net_icmp_hdr *get_icmp_hdr(struct net_pkt *pkt)
{
	net_pkt_cursor_init(pkt);

	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
		     net_pkt_ipv6_ext_len(pkt));

	return (struct net_icmp_hdr *)net_pkt_cursor_get_pos(pkt);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_icmp_hdr *icmp;

	if (!pkt->buffer) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	icmp = get_icmp_hdr(pkt);

	if (icmp->type == NET_ICMPV6_MLDv2) {
		/* FIXME, add more checks here */

		NET_DBG("Received something....");
		is_join_msg_ok = true;
		is_leave_msg_ok = true;
		is_report_sent = true;
		report_count++;

		if (report_handler) {
			report_handler->fn(pkt, report_handler->user_data);
		}

		k_sem_give(&wait_data);
	} else if (icmp->type == NET_ICMPV6_MLDv1_REPORT) {
		NET_DBG("Received MLDv1 report....");
		is_v1_report_sent = true;
		is_report_sent = true;
		report_count++;
		k_sem_give(&wait_data);
	} else if (icmp->type == NET_ICMPV6_MLDv1_DONE) {
		NET_DBG("Received MLDv1 done....");
		is_v1_done_sent = true;
		k_sem_give(&wait_data);
	}

	return 0;
}

static int tester_null_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

struct net_test_mld net_test_data;
struct net_test_mld net_test_null_data;

static struct dummy_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = tester_send,
};

static void init_null_iface(struct net_if *iface)
{
	struct net_test_mld *context = net_if_get_device(iface)->data;

	memset(&context->mac_addr, 0, sizeof(context->mac_addr));

	net_if_set_link_addr(iface, context->mac_addr, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static struct dummy_api net_test_null_if_api = {
	.iface_api.init = init_null_iface,
	.send = tester_null_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test_mld, "net_test_mld",
		net_test_dev_init, NULL, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

/* Interface without a device or API, only for usage of `struct net_if` */
NET_DEVICE_INIT(net_test_null_iface, "net_test_null_iface", net_test_dev_init, NULL,
		&net_test_null_data, NULL, 99, &net_test_null_if_api, _ETH_L2_LAYER,
		_ETH_L2_CTX_TYPE, 127);

static void test_iface_down_up(void)
{
	zassert_ok(net_if_down(net_iface), "Failed to bring iface down");
	k_msleep(10);
	zassert_ok(net_if_up(net_iface), "Failed to bring iface up");
}

static void test_iface_down_up_delayed_carrier(void)
{
	zassert_ok(net_if_down(net_iface), "Failed to bring iface down");
	k_msleep(10);
	net_if_carrier_off(net_iface);
	zassert_ok(net_if_up(net_iface), "Failed to bring iface up");
	k_msleep(10);
	net_if_carrier_on(net_iface);
}

static void test_iface_carrier_off_on(void)
{
	net_if_carrier_off(net_iface);
	k_msleep(10);
	net_if_carrier_on(net_iface);
}

static void group_joined(struct net_mgmt_event_callback *cb,
			 uint64_t nm_event, struct net_if *iface)
{
	const struct net_in6_addr *group = cb->info;

	ARG_UNUSED(iface);

	if (nm_event != NET_EVENT_IPV6_MCAST_JOIN) {
		/* Spurious callback. */
		return;
	}

	if (exp_mcast_group == NULL) {
		is_group_joined = true;

		k_sem_give(&wait_joined);
		return;
	}

	if (group == NULL || cb->info_length != sizeof(*group)) {
		return;
	}

	if (net_ipv6_addr_cmp(exp_mcast_group, group)) {
		is_group_joined = true;

		k_sem_give(&wait_joined);
	}
}

static void group_left(struct net_mgmt_event_callback *cb,
		       uint64_t nm_event, struct net_if *iface)
{
	const struct net_in6_addr *group = cb->info;

	ARG_UNUSED(iface);

	if (nm_event != NET_EVENT_IPV6_MCAST_LEAVE) {
		/* Spurious callback. */
		return;
	}

	if (exp_mcast_group == NULL) {
		is_group_left = true;

		k_sem_give(&wait_left);
		return;
	}

	if (group == NULL || cb->info_length != sizeof(*group)) {
		return;
	}

	if (net_ipv6_addr_cmp(exp_mcast_group, group)) {
		is_group_left = true;

		k_sem_give(&wait_left);
	}
}

static struct mgmt_events {
	uint64_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
} mgmt_events[] = {
	{ .event = NET_EVENT_IPV6_MCAST_JOIN, .handler = group_joined },
	{ .event = NET_EVENT_IPV6_MCAST_LEAVE, .handler = group_left },
	{ 0 }
};

static void setup_mgmt_events(void)
{
	int i;

	for (i = 0; mgmt_events[i].event; i++) {
		net_mgmt_init_event_callback(&mgmt_events[i].cb,
					     mgmt_events[i].handler,
					     mgmt_events[i].event);

		net_mgmt_add_event_callback(&mgmt_events[i].cb);
	}
}

static void *test_mld_setup(void)
{
	struct net_if_addr *ifaddr;

	setup_mgmt_events();

	net_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	zassert_not_null(net_iface, "Interface is NULL");

	ifaddr = net_if_ipv6_addr_add(net_iface, &my_addr,
				      NET_ADDR_MANUAL, 0);

	zassert_not_null(ifaddr, "Cannot add IPv6 address");

	return NULL;
}

/* The unsolicited report of a join is retransmitted after a random delay,
 * which would disturb the tests that count reports. Cancel the pending
 * retransmissions and verify them in their own tests.
 */
static void cancel_retransmits(struct net_if *iface)
{
	ARRAY_FOR_EACH_PTR(iface->config.ip.ipv6->mcast, mcast) {
		mcast->mld_retx_left = 0;
		mcast->mld_retx_timeout = sys_timepoint_calc(K_FOREVER);
	}
}

/* Forget about MLDv1 queriers heard by a previous test */
static void test_mld_before(void *fixture)
{
	struct net_if_ipv6 *ipv6 = net_iface->config.ip.ipv6;

	ARG_UNUSED(fixture);

	report_handler = NULL;
	exp_mcast_group = NULL;

	ipv6->mld_v1_querier_timeout = sys_timepoint_calc(K_NO_WAIT);
	ipv6->mld_general_timeout = sys_timepoint_calc(K_FOREVER);
	ipv6->mld_version = 0;
	ARRAY_FOR_EACH(ipv6->mcast, i) {
		ipv6->mcast[i].mld_resp_timeout = sys_timepoint_calc(K_FOREVER);
	}
	cancel_retransmits(net_iface);

	is_v1_report_sent = false;
	is_v1_done_sent = false;
	report_count = 0;
}

static void test_join_group(void)
{
	int ret;

	ret = net_ipv6_mld_join(net_iface, &mcast_addr);
	zassert_equal(ret, 0, "Cannot join IPv6 multicast group");
	cancel_retransmits(net_iface);

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void test_leave_group(void)
{
	int ret;

	ret = net_ipv6_mld_leave(net_iface, &mcast_addr);

	zassert_equal(ret, 0, "Cannot leave IPv6 multicast group");

	k_msleep(THREAD_SLEEP);
}

static void test_catch_join_group(void)
{
	is_group_joined = false;

	test_join_group();

	if (k_sem_take(&wait_joined, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_group_joined) {
		zassert_true(0, "Did not catch join event");
	}

	is_group_joined = false;
}

static void test_catch_leave_group(void)
{
	is_group_joined = false;

	test_leave_group();

	if (k_sem_take(&wait_left, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting leave event");
	}

	if (!is_group_left) {
		zassert_true(0, "Did not catch leave event");
	}

	is_group_left = false;
}

static void test_verify_join_group(void)
{
	is_join_msg_ok = false;

	test_join_group();

	if (k_sem_take(&wait_joined, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_join_msg_ok) {
		zassert_true(0, "Join msg invalid");
	}

	is_join_msg_ok = false;
}

static void test_verify_leave_group(void)
{
	is_leave_msg_ok = false;

	test_leave_group();

	if (k_sem_take(&wait_left, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting leave event");
	}

	if (!is_leave_msg_ok) {
		zassert_true(0, "Leave msg invalid");
	}

	is_leave_msg_ok = false;
}

struct mld_query_opts {
	/* Source address, a link-local peer when NULL */
	const struct net_in6_addr *src;
	/* Destination, the link-scope all-nodes group when NULL */
	const struct net_in6_addr *dst;
	/* Multicast Address field, unspecified when NULL */
	const struct net_in6_addr *group;
	/* Maximum Response Code */
	uint16_t max_resp;
	/* Number of (unspecified) sources appended to an MLDv2 query */
	uint16_t sources;
	/* An MLDv1 query of 24 octets instead of an MLDv2 one */
	bool v1;
	/* Leave out the Hop-by-Hop header carrying the Router Alert option */
	bool no_router_alert;
	/* Put the padding before the Router Alert option instead of after it */
	bool pad_first;
	/* Carry the Router Alert option in a Destination Options header */
	bool dest_opts;
	/* Hop limit, 1 when 0 */
	uint8_t hop_limit;
};

/* Inject a Multicast Listener Query built from opts */
static void send_mld_query(struct net_if *iface, const struct mld_query_opts *opts)
{
	static const struct net_in6_addr peer_ll_addr = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
							      0, 0, 0, 0, 0, 0, 0, 0x2 } } };
	const struct net_in6_addr *src = opts->src != NULL ? opts->src : &peer_ll_addr;
	const struct net_in6_addr *group =
		opts->group != NULL ? opts->group : net_ipv6_unspecified_address();
	struct net_in6_addr all_nodes;
	const struct net_in6_addr *dst;
	struct net_pkt *pkt;
	int ret;

	net_ipv6_addr_create_ll_allnodes_mcast(&all_nodes);
	dst = opts->dst != NULL ? opts->dst : &all_nodes;

	pkt = net_pkt_alloc_with_buffer(iface, 128 + opts->sources * sizeof(struct net_in6_addr),
					NET_AF_INET6, NET_IPPROTO_ICMPV6, K_FOREVER);
	zassert_not_null(pkt, "Cannot allocate pkt");

	net_pkt_set_ipv6_hop_limit(pkt, opts->hop_limit != 0 ? opts->hop_limit : 1);
	ret = net_ipv6_create(pkt, src, dst);
	zassert_false(ret, "Cannot create ipv6 pkt");

	if (!opts->no_router_alert) {
		/* Hop-by-Hop header with the Router Alert option of RFC 2711 and
		 * a PadN option, in either order.
		 */
		zassert_ok(net_pkt_write_u8(pkt, NET_IPPROTO_ICMPV6), "Failed to write");
		zassert_ok(net_pkt_write_u8(pkt, 0), "Failed to write"); /* 8 octets */
		if (opts->pad_first) {
			zassert_ok(net_pkt_write_be16(pkt, 0x0100), "Failed to write");
		}
		zassert_ok(net_pkt_write_be16(pkt, 0x0502), "Failed to write");
		zassert_ok(net_pkt_write_be16(pkt, 0), "Failed to write"); /* MLD */
		if (!opts->pad_first) {
			zassert_ok(net_pkt_write_be16(pkt, 0x0100), "Failed to write");
		}
		net_pkt_set_ipv6_ext_len(pkt, 8);
		net_pkt_set_ipv6_next_hdr(pkt, opts->dest_opts ? NET_IPV6_NEXTHDR_DESTO
							       : NET_IPV6_NEXTHDR_HBHO);
	} else {
		net_pkt_set_ipv6_next_hdr(pkt, NET_IPPROTO_ICMPV6);
	}

	ret = net_icmpv6_create(pkt, NET_ICMPV6_MLD_QUERY, 0);
	zassert_false(ret, "Cannot create icmpv6 pkt");

	zassert_ok(net_pkt_write_be16(pkt, opts->max_resp), "Failed to write");
	zassert_ok(net_pkt_write_be16(pkt, 0), "Failed to write"); /* reserved */
	zassert_ok(net_pkt_write(pkt, group, sizeof(struct net_in6_addr)), "Failed to write");

	if (!opts->v1) {
		zassert_ok(net_pkt_write_be16(pkt, 0), "Failed to write"); /* Resv, S, QRV, QQIC */
		zassert_ok(net_pkt_write_be16(pkt, opts->sources), "Failed to write");

		for (uint16_t i = 0; i < opts->sources; i++) {
			zassert_ok(net_pkt_write(pkt, net_ipv6_unspecified_address(),
						 sizeof(struct net_in6_addr)),
				   "Failed to write");
		}
	}

	net_pkt_cursor_init(pkt);
	ret = net_ipv6_finalize(pkt, NET_IPPROTO_ICMPV6);
	zassert_false(ret, "Failed to finalize ipv6 packet");

	net_pkt_cursor_init(pkt);

	ret = net_recv_data(iface, pkt);
	zassert_false(ret, "Failed to receive data");
}

/* A General Query from a link-local peer */
static void send_query(struct net_if *iface)
{
	const struct mld_query_opts opts = { .max_resp = 3 };

	send_mld_query(iface, &opts);
}

/* interface needs to join the MLDv2-capable routers multicast group before it
 * can receive MLD queries
 */
static void join_mldv2_capable_routers_group(void)
{
	struct net_if *iface;
	int ret;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	ret = net_ipv6_mld_join(iface, &mldv2_routers_addr);

	zassert_true(ret == 0 || ret == -EALREADY,
		     "Cannot join MLDv2-capable routers multicast group");
	cancel_retransmits(iface);

	k_msleep(THREAD_SLEEP);

}

static void leave_mldv2_capable_routers_group(void)
{
	struct net_if *iface;
	int ret;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	ret = net_ipv6_mld_leave(iface, &mldv2_routers_addr);

	zassert_equal(ret, 0,
		      "Cannot leave MLDv2-capable routers multicast group");

	k_msleep(THREAD_SLEEP);
}

/* We are not really interested to parse the query at this point */
static enum net_verdict handle_mld_query(struct net_icmp_ctx *ctx,
					 struct net_pkt *pkt,
					 struct net_icmp_ip_hdr *hdr,
					 struct net_icmp_hdr *icmp_hdr,
					 void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	is_query_received = true;

	NET_DBG("Handling MLD query");

	return NET_DROP;
}

static void test_catch_query(void)
{
	struct net_icmp_ctx ctx;
	int ret;

	join_mldv2_capable_routers_group();

	is_query_received = false;

	ret = net_icmp_init_ctx(&ctx, NET_AF_INET6, NET_ICMPV6_MLD_QUERY,
				0, handle_mld_query);
	zassert_equal(ret, 0, "Cannot register %s handler (%d)",
		      STRINGIFY(NET_ICMPV6_MLD_QUERY), ret);

	send_query(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)));

	k_msleep(THREAD_SLEEP);

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting query event");
	}

	if (!is_query_received) {
		zassert_true(0, "Query msg invalid");
	}

	is_query_received = false;

	leave_mldv2_capable_routers_group();

	net_icmp_cleanup_ctx(&ctx);
}

static void test_verify_send_report(void)
{
	join_mldv2_capable_routers_group();

	is_query_received = false;
	is_report_sent = false;

	k_sem_reset(&wait_data);

	test_join_group();

	k_yield();

	/* Did we send a report? */
	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting for report");
	}

	k_sem_reset(&wait_data);

	is_report_sent = false;
	send_query(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)));

	k_yield();

	/* Did we send a report? */
	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting for report");
	}

	zassert_true(is_report_sent, "Report not sent");

	leave_mldv2_capable_routers_group();
	test_leave_group();
}

/* This value should be longer that the one in net_if.c when DAD timeouts */
#define DAD_TIMEOUT (MSEC_PER_SEC / 5U)

ZTEST(net_mld_test_suite, test_allnodes)
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct net_in6_addr addr;

	net_ipv6_addr_create_ll_allnodes_mcast(&addr);

	/* Let the DAD succeed so that the multicast address will be there */
	k_sleep(K_MSEC(DAD_TIMEOUT));

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);

	zassert_not_null(ifmaddr, "Interface does not contain "
			"allnodes multicast address");
}

static void expect_exclude_mcast_report(struct net_pkt *pkt, void *user_data)
{
	struct mld_report_mcast_record record;
	uint16_t records_count;
	uint16_t res_bytes;
	bool *report_sent = user_data;

	zassert_not_null(exp_mcast_group, "Expected mcast group not sent");

	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, sizeof(struct net_icmp_hdr));

	zassert_ok(net_pkt_read_be16(pkt, &res_bytes), "Failed to read reserved bytes");
	zassert_equal(0, res_bytes, "Reserved bytes must be zeroed");

	zassert_ok(net_pkt_read_be16(pkt, &records_count), "Failed to read addr count");
	zexpect_equal(records_count, 1, "Incorrect record count ");

	net_pkt_read(pkt, &record, sizeof(struct mld_report_mcast_record));

	if (record.record_type == NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE &&
	    net_ipv6_addr_cmp_raw((const uint8_t *)exp_mcast_group,
				  (const uint8_t *)&record.mcast_addr)) {
		*report_sent = true;

		k_sem_give(&wait_report);
	}
}

/* No MLD message is ever sent for the link-scope all-nodes address
 * (RFC 3810 ch 6): fail on a report that carries it.
 */
static void reject_mcast_report(struct net_pkt *pkt, void *user_data)
{
	struct mld_report_mcast_record record;
	uint16_t records_count;
	uint16_t res_bytes;

	ARG_UNUSED(user_data);

	zassert_not_null(exp_mcast_group, "Expected mcast group not set");

	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, sizeof(struct net_icmp_hdr));

	zassert_ok(net_pkt_read_be16(pkt, &res_bytes), "Failed to read reserved bytes");
	zassert_ok(net_pkt_read_be16(pkt, &records_count), "Failed to read addr count");

	for (uint16_t i = 0; i < records_count; i++) {
		zassert_ok(net_pkt_read(pkt, &record, sizeof(record)), "Failed to read record");
		zassert_false(net_ipv6_addr_cmp_raw((const uint8_t *)exp_mcast_group,
						    (const uint8_t *)&record.mcast_addr),
			      "Group %s must not be reported",
			      net_sprint_ipv6_addr(exp_mcast_group));
	}
}

static void verify_allnodes_on_iface_event(void (*action)(void))
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct net_in6_addr addr;
	struct mld_report_handler handler = {
		.fn = reject_mcast_report,
	};

	net_ipv6_addr_create_ll_allnodes_mcast(&addr);
	k_sem_reset(&wait_joined);

	is_group_joined = false;
	exp_mcast_group_storage = addr;
	exp_mcast_group = &exp_mcast_group_storage;
	report_handler = &handler;

	action();

	zassert_ok(k_sem_take(&wait_joined, K_MSEC(WAIT_TIME)),
		   "Timeout while waiting for an event");
	cancel_retransmits(net_iface);

	/* Let the reports for the other groups go out and be checked */
	k_msleep(THREAD_SLEEP);

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);
	zassert_not_null(ifmaddr, "Interface does not contain "
			"allnodes multicast address");

	zassert_true(is_group_joined, "Did not join mcast group");
}

/* Verify that mcast all nodes is present after interface admin state toggle */
ZTEST(net_mld_test_suite, test_allnodes_after_iface_up)
{
	verify_allnodes_on_iface_event(test_iface_down_up);
}

/* Verify that mcast all nodes is present after delayed carrier on */
ZTEST(net_mld_test_suite, test_allnodes_after_iface_up_carrier_delayed)
{
	verify_allnodes_on_iface_event(test_iface_down_up_delayed_carrier);
}

/* Verify that mcast all nodes is present after carrier toggle */
ZTEST(net_mld_test_suite, test_allnodes_after_carrier_toggle)
{
	verify_allnodes_on_iface_event(test_iface_carrier_off_on);
}

ZTEST(net_mld_test_suite, test_solicit_node)
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct net_in6_addr addr;

	net_ipv6_addr_create_solicited_node(&my_addr, &addr);

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);

	zassert_not_null(ifmaddr, "Interface does not contain "
			"solicit node multicast address");
}

static void verify_solicit_node_on_iface_event(void (*action)(void))
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct net_in6_addr addr;
	bool exclude_report_sent = false;
	struct mld_report_handler handler = {
		.fn = expect_exclude_mcast_report,
		.user_data = &exclude_report_sent
	};

	net_ipv6_addr_create_solicited_node(&my_addr, &addr);
	k_sem_reset(&wait_joined);

	is_group_joined = false;
	exp_mcast_group_storage = addr;
	exp_mcast_group = &exp_mcast_group_storage;
	report_handler = &handler;

	action();

	zassert_ok(k_sem_take(&wait_joined, K_MSEC(WAIT_TIME)),
		   "Timeout while waiting for an event");
	cancel_retransmits(net_iface);

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);
	zassert_not_null(ifmaddr, "Interface does not contain "
			"solicit node multicast address");

	zassert_true(is_group_joined, "Did not join mcast group");
	zassert_true(exclude_report_sent, "Did not send report");
}

/* Verify that mcast solicited node is present after interface admin state toggle */
ZTEST(net_mld_test_suite, test_solicit_node_after_iface_up)
{
	verify_solicit_node_on_iface_event(test_iface_down_up);
}

/* Verify that mcast solicited node is present after delayed carrier on */
ZTEST(net_mld_test_suite, test_solicit_node_after_iface_up_carrier_delayed)
{
	verify_solicit_node_on_iface_event(test_iface_down_up_delayed_carrier);
}

/* Verify that mcast solicited node is present after delayed carrier toggle */
ZTEST(net_mld_test_suite, test_solicit_node_after_carrier_toggle)
{
	verify_solicit_node_on_iface_event(test_iface_carrier_off_on);
}

ZTEST(net_mld_test_suite, test_join_leave)
{
	test_join_group();
	test_leave_group();
}

/* Store the record type the expected multicast group was reported with. */
static void record_mcast_report(struct net_pkt *pkt, void *user_data)
{
	struct mld_report_mcast_record record;
	uint8_t *record_type = user_data;
	uint16_t records_count;
	uint16_t res_bytes;

	zassert_not_null(exp_mcast_group, "Expected mcast group not sent");

	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, sizeof(struct net_icmp_hdr));

	zassert_ok(net_pkt_read_be16(pkt, &res_bytes), "Failed to read reserved bytes");
	zassert_equal(0, res_bytes, "Reserved bytes must be zeroed");

	zassert_ok(net_pkt_read_be16(pkt, &records_count), "Failed to read addr count");
	zexpect_equal(records_count, 1, "Incorrect record count");

	zassert_ok(net_pkt_read(pkt, &record, sizeof(struct mld_report_mcast_record)),
		   "Failed to read mcast record");

	if (net_ipv6_addr_cmp_raw((const uint8_t *)exp_mcast_group,
				  (const uint8_t *)&record.mcast_addr)) {
		*record_type = record.record_type;

		k_sem_give(&wait_report);
	}
}

/* Verify that a group joined by the application is kept on the interface while
 * the interface is down. The membership is given up on the wire, but the
 * address stays registered so that it is rejoined - without the application
 * having to join it again - once the interface comes back up.
 */
ZTEST(net_mld_test_suite, test_group_preserved_over_iface_down_up)
{
	struct net_if_mcast_addr *ifmaddr;
	struct net_if *iface = NULL;
	uint8_t record_type = 0;
	struct mld_report_handler handler = {
		.fn = record_mcast_report,
		.user_data = &record_type
	};

	test_join_group();

	ifmaddr = net_if_ipv6_maddr_lookup(&mcast_addr, &iface);
	zassert_not_null(ifmaddr, "Interface does not contain the multicast address");
	zassert_true(net_if_ipv6_maddr_is_joined(ifmaddr),
		     "Multicast address is not marked as joined");

	exp_mcast_group_storage = mcast_addr;
	exp_mcast_group = &exp_mcast_group_storage;
	report_handler = &handler;

	/* Interface down - the group is left on the wire only. */
	k_sem_reset(&wait_report);

	zassert_ok(net_if_down(net_iface), "Failed to bring iface down");

	zassert_ok(k_sem_take(&wait_report, K_MSEC(WAIT_TIME)),
		   "Timeout while waiting for the MLD leave report");
	zassert_equal(record_type, NET_IPV6_MLDv2_CHANGE_TO_INCLUDE_MODE,
		      "Interface down did not report leaving the group");

	iface = NULL;
	ifmaddr = net_if_ipv6_maddr_lookup(&mcast_addr, &iface);
	zassert_not_null(ifmaddr, "Multicast address was removed on iface down");
	zassert_false(net_if_ipv6_maddr_is_joined(ifmaddr),
		      "Multicast address is still marked as joined while down");

	/* Interface up - the preserved group is rejoined. */
	k_sem_reset(&wait_report);
	record_type = 0;

	zassert_ok(net_if_up(net_iface), "Failed to bring iface up");

	zassert_ok(k_sem_take(&wait_report, K_MSEC(WAIT_TIME)),
		   "Timeout while waiting for the MLD report");
	cancel_retransmits(net_iface);
	zassert_equal(record_type, NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE,
		      "Interface up did not rejoin the group");

	iface = NULL;
	ifmaddr = net_if_ipv6_maddr_lookup(&mcast_addr, &iface);
	zassert_not_null(ifmaddr, "Interface does not contain the multicast address");
	zassert_true(net_if_ipv6_maddr_is_joined(ifmaddr),
		     "Multicast address was not rejoined");

	report_handler = NULL;

	test_leave_group();
}

ZTEST(net_mld_test_suite, test_catch_join_leave)
{
	test_catch_join_group();
	test_catch_leave_group();
}

ZTEST(net_mld_test_suite, test_verify_join_leave)
{
	test_verify_join_group();
	test_verify_leave_group();
	test_catch_query();
	test_verify_send_report();
}

/* Join a group, inject a query that must not be answered, leave again */
static void verify_query_unanswered(const struct mld_query_opts *opts)
{
	test_join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;

	send_mld_query(net_iface, opts);

	zassert_equal(-EAGAIN, k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Unexpected report");
	zassert_false(is_report_sent, "Query was answered");

	test_leave_group();
}

/* A Multicast Address Specific Query is answered by a listener of that
 * address with its current state (RFC 3810 ch 6.3).
 */
ZTEST(net_mld_test_suite, test_address_specific_query)
{
	uint8_t record_type = 0;
	struct mld_report_handler handler = {
		.fn = record_mcast_report,
		.user_data = &record_type
	};
	struct mld_query_opts opts = { .max_resp = 3 };

	test_join_group();

	exp_mcast_group_storage = mcast_addr;
	exp_mcast_group = &exp_mcast_group_storage;
	report_handler = &handler;
	k_sem_reset(&wait_report);

	opts.dst = &mcast_addr;
	opts.group = &mcast_addr;
	send_mld_query(net_iface, &opts);

	zassert_ok(k_sem_take(&wait_report, K_MSEC(WAIT_TIME)),
		   "Timeout while waiting for the report");
	zassert_equal(record_type, NET_IPV6_MLDv2_MODE_IS_EXCLUDE,
		      "Query not answered with the current state");

	report_handler = NULL;
	test_leave_group();
}

/* A query for a group that is not listened to gets no answer */
ZTEST(net_mld_test_suite, test_address_specific_query_not_listener)
{
	struct net_in6_addr other;
	struct mld_query_opts opts = { .max_resp = 3 };

	net_ipv6_addr_create(&other, 0xff15, 0, 0, 0, 0, 0, 0, 0x0002);
	opts.dst = &other;
	opts.group = &other;

	verify_query_unanswered(&opts);
}

/* After an MLDv1 query the host operates in MLDv1 mode (RFC 3810 ch
 * 8.2.1): queries are answered with MLDv1 Reports, a join is announced with
 * one and a leave with a Done message.
 */
ZTEST(net_mld_test_suite, test_mldv1_querier_present)
{
	const struct mld_query_opts opts = { .v1 = true, .max_resp = 3 };

	k_sem_reset(&wait_data);
	is_v1_report_sent = false;

	/* The solicited-node group of the address is answered for */
	send_mld_query(net_iface, &opts);
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting for a report");
	zassert_true(is_v1_report_sent, "Query not answered with MLDv1");

	k_sem_reset(&wait_data);
	is_v1_report_sent = false;
	is_v1_done_sent = false;

	test_join_group();
	zassert_true(is_v1_report_sent, "Join not reported with MLDv1");

	test_leave_group();
	zassert_true(is_v1_done_sent, "Leave not reported with a Done message");
}

/* Maximum Response Code decoding: MLDv1 is linear in milliseconds (RFC 2710
 * ch 3.4), MLDv2 switches to a floating point form at 32768 (RFC 3810 ch
 * 5.1.3).
 */
ZTEST(net_mld_test_suite, test_max_resp_delay)
{
	zassert_equal(net_ipv6_mld_max_resp_delay(0, false), 0);
	zassert_equal(net_ipv6_mld_max_resp_delay(3, false), 3);
	zassert_equal(net_ipv6_mld_max_resp_delay(0x8000, false), 32768);
	zassert_equal(net_ipv6_mld_max_resp_delay(32767, true), 32767);
	/* mant 0, exp 0: 0x1000 << 3 */
	zassert_equal(net_ipv6_mld_max_resp_delay(0x8000, true), 32768);
	/* mant 0xfff, exp 7: 0x1fff << 10 */
	zassert_equal(net_ipv6_mld_max_resp_delay(0xffff, true), 8387584);
}

/* A query is not answered right away but after a random delay bounded by
 * its Maximum Response Delay (RFC 3810 ch 6.2).
 */
ZTEST(net_mld_test_suite, test_query_delayed)
{
	const struct mld_query_opts opts = { .max_resp = 1000 };
	int64_t start;

	test_join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;

	start = k_uptime_get();
	send_mld_query(net_iface, &opts);
	/* A report is only immediate when no time passed while the query was
	 * handled; the host may have been stalled for longer than the delay.
	 */
	zassert_true(!is_report_sent || k_uptime_get() > start, "Query answered without delay");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(1500)), "Timeout while waiting for the report");
	zassert_true(is_report_sent, "Query not answered");

	test_leave_group();
}

/* A second General Query while a response is pending does not schedule
 * another response (RFC 3810 ch 6.2 rule 1).
 */
ZTEST(net_mld_test_suite, test_query_merged)
{
	const struct mld_query_opts opts = { .max_resp = 500 };

	test_join_group();

	k_sem_reset(&wait_data);
	report_count = 0;

	send_mld_query(net_iface, &opts);
	send_mld_query(net_iface, &opts);
	k_msleep(THREAD_SLEEP);

	if (report_count != 0) {
		/* Stalled so long that the first response was already due */
		test_leave_group();
		ztest_test_skip();
	}

	k_msleep(1000);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);

	test_leave_group();
}

/* A second query for an address is answered at the earliest of the pending
 * and the newly selected delay (RFC 3810 ch 6.2 rule 4).
 */
ZTEST(net_mld_test_suite, test_query_address_earliest)
{
	struct mld_query_opts opts = { .max_resp = 32767 };

	test_join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	opts.dst = &mcast_addr;
	opts.group = &mcast_addr;

	/* Up to 32.767 seconds, then up to 100 milliseconds */
	send_mld_query(net_iface, &opts);
	opts.max_resp = 100;
	send_mld_query(net_iface, &opts);

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)),
		   "Timeout while waiting for the report");

	k_msleep(1000);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);

	test_leave_group();
}

/* Switching compatibility mode cancels the pending responses (RFC 3810 ch
 * 8.2.1), here when an MLDv2 query arrives after the MLDv1 querier present
 * timer ran out.
 */
ZTEST(net_mld_test_suite, test_mode_change_cancels_response)
{
	struct net_if_ipv6 *ipv6 = net_iface->config.ip.ipv6;
	struct mld_query_opts opts = { .v1 = true, .max_resp = 25000 };

	test_join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	is_v1_report_sent = false;

	/* An MLDv1 querier: responses are pending for up to 25 seconds */
	send_mld_query(net_iface, &opts);
	k_msleep(THREAD_SLEEP);

	if (report_count != 0) {
		/* Stalled so long that a response was already due */
		test_leave_group();
		ztest_test_skip();
	}

	/* The querier went away and an MLDv2 one shows up */
	ipv6->mld_v1_querier_timeout = sys_timepoint_calc(K_NO_WAIT);

	opts.v1 = false;
	opts.max_resp = 100;
	send_mld_query(net_iface, &opts);

	k_msleep(1500);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);
	zassert_false(is_v1_report_sent, "Cancelled MLDv1 response was sent");

	test_leave_group();
}

/* The compatibility mode also changes when the querier present timer runs
 * out on its own, which cancels the pending responses as well.
 */
ZTEST(net_mld_test_suite, test_mode_expiry_cancels_response)
{
	struct net_if_ipv6 *ipv6 = net_iface->config.ip.ipv6;
	struct mld_query_opts opts = { .v1 = true, .max_resp = 200 };
	struct net_if *iface = net_iface;
	struct net_if_mcast_addr *maddr;

	test_join_group();

	maddr = net_if_ipv6_maddr_lookup(&mcast_addr, &iface);
	zassert_not_null(maddr, "Group not registered");

	k_sem_reset(&wait_data);
	report_count = 0;
	opts.dst = &mcast_addr;
	opts.group = &mcast_addr;

	/* An MLDv1 querier: a response is pending for up to 200 milliseconds */
	send_mld_query(net_iface, &opts);
	k_msleep(THREAD_SLEEP);

	/* The querier present timer runs out before the response is due */
	ipv6->mld_v1_querier_timeout = sys_timepoint_calc(K_NO_WAIT);

	if (report_count != 0 || sys_timepoint_expired(maddr->mld_resp_timeout)) {
		/* Stalled so long that the response was already due */
		test_leave_group();
		ztest_test_skip();
	}

	k_msleep(1000);
	zassert_equal(report_count, 0, "Cancelled response was sent");

	test_leave_group();
}

/* The unsolicited report of a join is sent Robustness Variable times within
 * the Unsolicited Report Interval (RFC 3810 ch 6.1).
 */
ZTEST(net_mld_test_suite, test_join_retransmit)
{
	int ret;

	k_sem_reset(&wait_data);
	report_count = 0;

	ret = net_ipv6_mld_join(net_iface, &mcast_addr);
	zassert_equal(ret, 0, "Cannot join IPv6 multicast group");

	k_msleep(1000 + WAIT_TIME);
	zassert_equal(report_count, CONFIG_NET_IPV6_MLD_ROBUSTNESS, "Expected %d reports, got %d",
		      CONFIG_NET_IPV6_MLD_ROBUSTNESS, report_count);

	test_leave_group();
}

/* Leaving a group ends the retransmission of its join report */
ZTEST(net_mld_test_suite, test_leave_stops_retransmit)
{
	int ret;

	k_sem_reset(&wait_data);
	report_count = 0;

	/* Leave right away, before the first retransmission can be due; the
	 * leave is a report as well.
	 */
	ret = net_ipv6_mld_join(net_iface, &mcast_addr);
	zassert_equal(ret, 0, "Cannot join IPv6 multicast group");
	test_leave_group();

	if (report_count != 2) {
		/* Stalled so long that a retransmission was due before the leave */
		ztest_test_skip();
	}

	k_msleep(1000 + WAIT_TIME);
	zassert_equal(report_count, 2, "Expected two reports, got %d", report_count);
}

/* A query must come from a link-local address (RFC 3810 ch 5.1.14) */
ZTEST(net_mld_test_suite, test_query_global_source_ignored)
{
	const struct mld_query_opts opts = { .src = &peer_addr, .max_resp = 3 };

	verify_query_unanswered(&opts);
}

/* A query must carry the Router Alert option (RFC 3810 ch 6.2) */
ZTEST(net_mld_test_suite, test_query_without_router_alert_ignored)
{
	const struct mld_query_opts opts = { .no_router_alert = true, .max_resp = 3 };

	verify_query_unanswered(&opts);
}

/* The Router Alert option is found wherever it sits among the options */
ZTEST(net_mld_test_suite, test_query_router_alert_after_padding)
{
	const struct mld_query_opts opts = { .pad_first = true, .max_resp = 3 };

	test_join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;

	send_mld_query(net_iface, &opts);

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)),
		   "Timeout while waiting for the report");
	zassert_true(is_report_sent, "Query not answered");

	test_leave_group();
}

/* The Router Alert option has to be in the Hop-by-Hop header, not in a
 * Destination Options header (RFC 3810 ch 6.2).
 */
ZTEST(net_mld_test_suite, test_query_router_alert_in_dest_opts_ignored)
{
	const struct mld_query_opts opts = { .dest_opts = true, .max_resp = 3 };

	verify_query_unanswered(&opts);
}

/* A query must arrive with a hop limit of 1 (RFC 3810 ch 6.2) */
ZTEST(net_mld_test_suite, test_query_hop_limit_ignored)
{
	const struct mld_query_opts opts = { .hop_limit = 64, .max_resp = 3 };

	verify_query_unanswered(&opts);
}

ZTEST(net_mld_test_suite, test_no_mld_flag)
{
	int ret;

	is_join_msg_ok = false;
	is_leave_msg_ok = false;

	net_if_flag_set(net_iface, NET_IF_IPV6_NO_MLD);

	ret = net_ipv6_mld_join(net_iface, &mcast_addr);
	zassert_equal(ret, 0, "Cannot add multicast address");

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	zassert_false(is_join_msg_ok, "Received join message when not expected");

	ret = net_ipv6_mld_leave(net_iface, &mcast_addr);
	zassert_equal(ret, 0, "Cannot remove multicast address");

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	zassert_false(is_leave_msg_ok, "Received leave message when not expected");

	net_if_flag_clear(net_iface, NET_IF_IPV6_NO_MLD);
}

static void handle_mld_report(struct net_pkt *pkt, void *user_data)
{
	struct mld_report_info *info = (struct mld_report_info *)user_data;
	uint16_t res_bytes;

	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, sizeof(struct net_icmp_hdr));

	zassert_ok(net_pkt_read_be16(pkt, &res_bytes), "Failed to read reserved bytes");
	zassert_equal(0, res_bytes, "Reserved bytes must be zeroed");

	zassert_ok(net_pkt_read_be16(pkt, &info->records_count), "Failed to read addr count");
	zexpect_between_inclusive(info->records_count, 0, MLD_REPORT_ADDR_COUNT,
				  "Cannot decode all addresses");

	for (size_t i = 0; i < info->records_count; ++i) {
		net_pkt_read(pkt, &info->records[i], sizeof(struct mld_report_mcast_record));
	}
}

/* Number of groups a report is sent for: all-nodes is never reported */
static size_t get_mcast_addr_count(struct net_if *iface)
{
	struct net_in6_addr all_nodes;
	size_t ret = 0;

	net_ipv6_addr_create_ll_allnodes_mcast(&all_nodes);

	ARRAY_FOR_EACH_PTR(iface->config.ip.ipv6->mcast, mcast_addr) {
		if (mcast_addr->is_used &&
		    !net_ipv6_addr_cmp(&mcast_addr->address.in6_addr, &all_nodes)) {
			ret++;
		}
	}

	return ret;
}

static void add_mcast_route_and_verify(struct net_if *iface, struct net_in6_addr *addr,
				       struct mld_report_info *info)
{
	k_sem_reset(&wait_data);

	zassert_not_null(net_route_ipv6_mcast_add(iface, addr, 128),
			 "Failed to add multicast route");

	k_msleep(THREAD_SLEEP);

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting for a report");

	zassert_equal(info->records_count, 1, "Invalid number of reported addresses");
	zassert_equal(info->records[0].record_type, NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE,
		      "Invalid MLDv2 record type");
	zassert_mem_equal(&info->records[0].mcast_addr, addr,
			  sizeof(struct net_in6_addr), "Invalid reported address");
}

static void del_mcast_route_and_verify(struct net_if *iface, struct net_in6_addr *addr,
				       struct mld_report_info *info)
{
	struct net_route_ipv6_entry_mcast *entry;

	k_sem_reset(&wait_data);

	entry = net_route_ipv6_mcast_lookup(addr);

	zassert_not_null(entry, "Could not find the multicast route entry");
	zassert_true(net_route_ipv6_mcast_del(entry), "Failed to delete a route");

	k_msleep(THREAD_SLEEP);

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting for a report");

	zassert_equal(info->records_count, 1, "Invalid number of reported addresses");
	zassert_equal(info->records[0].record_type, NET_IPV6_MLDv2_CHANGE_TO_INCLUDE_MODE,
		      "Invalid MLDv2 record type");
	zassert_mem_equal(&info->records[0].mcast_addr, addr,
			  sizeof(struct net_in6_addr), "Invalid reported address");
}

static void verify_mcast_routes_in_mld(struct mld_report_info *info)
{
	struct net_if *dummy_iface = net_if_get_by_index(net_if_get_by_name("dummy0"));
	struct net_if *null_iface = net_if_get_by_index(net_if_get_by_name("dummy1"));
	struct net_in6_addr site_local_mcast_addr_abcd;
	struct net_in6_addr site_local_mcast_addr_beef;
	struct net_in6_addr site_local_mcast_addr_cafe;
	struct net_in6_addr reserved_scope_mcast_addr;

	zassert_not_null(dummy_iface, "Invalid dummy iface");
	zassert_not_null(null_iface, "Invalid null iface");

	net_if_flag_set(null_iface, NET_IF_FORWARD_MULTICASTS);

	net_ipv6_addr_create(&site_local_mcast_addr_abcd, 0xff05, 0, 0, 0, 0, 0, 0, 0xabcd);
	net_ipv6_addr_create(&site_local_mcast_addr_beef, 0xff05, 0, 0, 0, 0, 0, 0, 0xbeef);
	net_ipv6_addr_create(&site_local_mcast_addr_cafe, 0xff05, 0, 0, 0, 0, 0, 0, 0xcafe);

	/* Next steps: verify that adding a multicast routes to a complete IPv6 address emits
	 * MLDv2 reports with a single entries.
	 */
	add_mcast_route_and_verify(null_iface, &site_local_mcast_addr_abcd, info);
	add_mcast_route_and_verify(null_iface, &site_local_mcast_addr_beef, info);

	/* Next steps: verify that report is not sent to an iface if it has already joined
	 * the group.
	 */
	zassert_ok(net_ipv6_mld_join(dummy_iface, &site_local_mcast_addr_cafe),
		   "Failed to join a group");
	cancel_retransmits(dummy_iface);

	k_msleep(THREAD_SLEEP);

	k_sem_reset(&wait_data);

	zassert_not_null(net_route_ipv6_mcast_add(null_iface, &site_local_mcast_addr_cafe, 128),
			 "Failed to add multicast route");

	k_msleep(THREAD_SLEEP);

	zassert_equal(-EAGAIN, k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Expected a timeout");

	k_sem_reset(&wait_data);

	/* A route to an address of the reserved scope 0 can be added, but is
	 * never reported (RFC 3810 ch 6).
	 */
	net_ipv6_addr_create(&reserved_scope_mcast_addr, 0xff00, 0, 0, 0, 0, 0, 0, 0x1);

	zassert_not_null(net_route_ipv6_mcast_add(null_iface, &reserved_scope_mcast_addr, 128),
			 "Failed to add multicast route");

	k_msleep(THREAD_SLEEP);

	zassert_equal(-EAGAIN, k_sem_take(&wait_data, K_MSEC(WAIT_TIME)),
		      "Report for a reserved scope address");

	k_sem_reset(&wait_data);

	/* Verify that multicast routes can be found in MLDv2 report and that there are
	 * no duplicates.
	 */
	send_query(dummy_iface);

	k_msleep(THREAD_SLEEP);

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Expected a report");

	/* Expect 2 additional addresses as 3rd is a duplicate of iface's multicast address */
	zassert_equal(info->records_count, get_mcast_addr_count(dummy_iface) + 2,
		      "Different number of reported addresses");

	/* Next steps: Remove routes and expect MLDv2 reports as these addresses are not
	 * used by the interface.
	 */
	del_mcast_route_and_verify(dummy_iface, &site_local_mcast_addr_abcd, info);
	del_mcast_route_and_verify(dummy_iface, &site_local_mcast_addr_beef, info);

	/* Next steps: Remove the last route and verify that report is NOT sent as this address
	 * is joined by the interface itself.
	 */
	k_sem_reset(&wait_data);

	zassert_true(net_route_ipv6_mcast_del(
			net_route_ipv6_mcast_lookup(&site_local_mcast_addr_cafe)),
		     "Failed to cleanup route to ff05::cafe");

	k_msleep(THREAD_SLEEP);

	zassert_equal(-EAGAIN, k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Expected a timeout");

	/* Finalize cleanup */
	zassert_true(net_route_ipv6_mcast_del(
			net_route_ipv6_mcast_lookup(&reserved_scope_mcast_addr)),
		     "Failed to cleanup route to ff00::1");
	net_ipv6_mld_leave(dummy_iface, &site_local_mcast_addr_cafe);
}

ZTEST(net_mld_test_suite, test_mcast_routes_in_mld)
{
	struct mld_report_info info;
	struct mld_report_handler handler = { .fn = handle_mld_report, .user_data = &info};
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	char str[NET_INET6_ADDRSTRLEN], *addr_str;

	memset(&info, 0, sizeof(info));

	join_mldv2_capable_routers_group();

	/* Enable report handler */
	report_handler = &handler;

	k_msleep(THREAD_SLEEP);

	k_sem_reset(&wait_data);

	send_query(iface);

	k_msleep(THREAD_SLEEP);

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting for a report");

	for (int i = 0; i < info.records_count; ++i) {
		addr_str = zsock_inet_ntop(NET_AF_INET6, &info.records[i].mcast_addr,
					   str, sizeof(str));
	}

	/* 1. Expect that report contains all iface's multicast addresses and no route */
	zassert_equal(info.records_count, get_mcast_addr_count(iface),
		      "Different number of reported addresses");

	/* 2. If CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS is enabled check that
	 * functionality works
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS)) {
		verify_mcast_routes_in_mld(&info);
	}

	leave_mldv2_capable_routers_group();
}

static void socket_group_with_index(const struct net_in6_addr *local_addr, bool do_join)
{
	struct net_ipv6_mreq mreq = { 0 };
	int option;
	int ret, fd;

	if (do_join) {
		option = ZSOCK_IPV6_ADD_MEMBERSHIP;
	} else {
		option = ZSOCK_IPV6_DROP_MEMBERSHIP;
	}

	fd = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, 0);
	zassert_true(fd >= 0, "Cannot get socket (%d)", -errno);

	ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, option,
			       NULL, sizeof(mreq));
	zassert_true(ret == -1 && errno == EINVAL,
		     "Incorrect return value (%d)", -errno);

	ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, option,
			       (void *)&mreq, 1);
	zassert_true(ret == -1 && errno == EINVAL,
		     "Incorrect return value (%d)", -errno);

	/* First try with empty mreq */
	ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, option,
			       (void *)&mreq, sizeof(mreq));
	zassert_true(ret == -1 && errno == EINVAL,
		     "Incorrect return value (%d)", -errno);

	mreq.ipv6mr_ifindex = net_if_ipv6_addr_lookup_by_index(local_addr);
	memcpy(&mreq.ipv6mr_multiaddr, &mcast_addr,
	       sizeof(mreq.ipv6mr_multiaddr));

	ret = zsock_setsockopt(fd, NET_IPPROTO_IPV6, option,
			       (void *)&mreq, sizeof(mreq));

	if (do_join) {
		zassert_equal(ret, 0,
			      "Cannot join IPv6 multicast group (%d)",
			      -errno);
	} else {
		zassert_equal(ret, 0, "Cannot leave IPv6 multicast group (%d)",
			      -errno);

		if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
			/* Let the network stack to proceed */
			k_msleep(THREAD_SLEEP);
		} else {
			k_yield();
		}
	}

	zsock_close(fd);

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void socket_join_group_with_index(const struct net_in6_addr *addr)
{
	socket_group_with_index(addr, true);
}

static void socket_leave_group_with_index(const struct net_in6_addr *addr)
{
	socket_group_with_index(addr, false);
}

ZTEST_USER(net_mld_test_suite, test_socket_catch_join_with_index)
{
	socket_join_group_with_index(net_ipv6_unspecified_address());
	socket_leave_group_with_index(net_ipv6_unspecified_address());
	socket_join_group_with_index(&my_addr);
	socket_leave_group_with_index(&my_addr);
}

ZTEST(net_mld_test_suite, test_mld_multi_join)
{
	is_join_msg_ok = false;
	test_join_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting join event");
	zassert_true(is_join_msg_ok, "Join msg invalid");

	is_join_msg_ok = false;
	test_join_group();
	k_msleep(THREAD_SLEEP);
	zassert_false(is_join_msg_ok, "Unexpected join msg");

	/* First leave should not send report due to two refs on the address */
	is_leave_msg_ok = false;
	test_leave_group();
	k_msleep(THREAD_SLEEP);
	zassert_false(is_leave_msg_ok, "Unexpected leave msg");

	is_leave_msg_ok = false;
	test_leave_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting leave event");
	zassert_true(is_leave_msg_ok, "Leave msg invalid");
}

ZTEST_SUITE(net_mld_test_suite, NULL, test_mld_setup, test_mld_before, NULL, NULL);
