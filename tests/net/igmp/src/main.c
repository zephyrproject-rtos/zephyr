/* main.c - Application main entry point */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV4_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/igmp.h>
#include <zephyr/net/socket.h>

#include <zephyr/random/random.h>

#include "ipv4.h"
#include "igmp.h"

#define THREAD_SLEEP 50 /* ms */

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IPV4_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static const unsigned char igmp_v2_query[] = {
	/* IPv4 header */
	0x46, 0xc0, 0x00, 0x20, 0x1b, 0x58, 0x00, 0x00, 0x01, 0x02, 0x66, 0x79,
	0xc0, 0x00, 0x02, 0x45, 0xe0, 0x00, 0x00, 0x01, 0x94, 0x04, 0x00, 0x00,

	/* IGMP header */
	0x11, 0xff, 0xee, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char igmp_v3_query[] = {
	/* IPv4 header */
	0x46, 0xc0, 0x00, 0x24, 0xac, 0x72, 0x00, 0x00, 0x01, 0x02, 0xd5, 0x5a,
	0xc0, 0x00, 0x02, 0x45, 0xe0, 0x00, 0x00, 0x01, 0x94, 0x04, 0x00, 0x00,

	/* IGMP header */
	0x11, 0x64, 0xec, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x02, 0x7d, 0x00, 0x00,
};

static struct in_addr my_addr = { { { 192, 0, 2, 1 } } };
static struct in_addr mcast_addr = { { { 224, 0, 2, 63 } } };
static struct in_addr any_addr = INADDR_ANY_INIT;

static struct net_if *net_iface;
static bool is_group_joined;
static bool is_group_left;
static bool is_join_msg_ok;
static bool is_leave_msg_ok;
static bool is_query_received;
static bool is_report_sent;
static bool is_igmpv2_query_sent;
static bool is_igmpv3_query_sent;
static bool ignore_already;
K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME 500
#define WAIT_TIME_LONG MSEC_PER_SEC
#define MY_PORT 1969
#define PEER_PORT 13856

struct net_test_igmp {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_igmp *context = dev->data;

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

#if defined(CONFIG_NET_IPV4_IGMPV3)
static struct net_ipv4_igmp_v3_report *get_igmp_hdr(struct net_pkt *pkt)
#else
static struct net_ipv4_igmp_v2_query *get_igmp_hdr(struct net_pkt *pkt)
#endif
{
	net_pkt_cursor_init(pkt);

	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
		     net_pkt_ipv4_opts_len(pkt));

#if defined(CONFIG_NET_IPV4_IGMPV3)
	return (struct net_ipv4_igmp_v3_report *)net_pkt_cursor_get_pos(pkt);
#else
	return (struct net_ipv4_igmp_v2_query *)net_pkt_cursor_get_pos(pkt);
#endif
}

#if defined(CONFIG_NET_IPV4_IGMPV3)
static struct net_ipv4_igmp_v3_group_record *get_igmp_group_record(struct net_pkt *pkt)
{
	net_pkt_cursor_init(pkt);

	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) + net_pkt_ipv4_opts_len(pkt));
	net_pkt_skip(pkt, sizeof(struct net_ipv4_igmp_v3_report));

	return (struct net_ipv4_igmp_v3_group_record *)net_pkt_cursor_get_pos(pkt);
}
#endif

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
#if defined(CONFIG_NET_IPV4_IGMPV3)
	struct net_ipv4_igmp_v3_report *igmp_header;
	struct net_ipv4_igmp_v3_group_record *igmp_group_record;
#else
	struct net_ipv4_igmp_v2_query *igmp_header;
#endif

	if (!pkt->buffer) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	igmp_header = get_igmp_hdr(pkt);

	if (igmp_header->type == NET_IPV4_IGMP_QUERY) {
		NET_DBG("Received query....");
		is_query_received = true;
		k_sem_give(&wait_data);
	} else if (igmp_header->type == NET_IPV4_IGMP_REPORT_V2) {
		NET_DBG("Received v2 report....");
		zassert_true(!IS_ENABLED(CONFIG_NET_IPV4_IGMPV3) || is_igmpv2_query_sent,
			     "Wrong IGMP report received (IGMPv2)");
		is_join_msg_ok = true;
		is_report_sent = true;
		k_sem_give(&wait_data);
	} else if (igmp_header->type == NET_IPV4_IGMP_REPORT_V3) {
		NET_DBG("Received v3 report....");
		zassert_true(IS_ENABLED(CONFIG_NET_IPV4_IGMPV3),
			     "Wrong IGMP report received (IGMPv3)");
		zassert_false(is_igmpv2_query_sent, "IGMPv3 response to IGMPv2 request");

#if defined(CONFIG_NET_IPV4_IGMPV3)
		zassert_equal(ntohs(igmp_header->groups_len), 1,
			      "Invalid group length of IGMPv3 report (%d)",
			      igmp_header->groups_len);

		igmp_group_record = get_igmp_group_record(pkt);
		zassert_equal(igmp_group_record->sources_len, 0,
			      "Invalid sources length of IGMPv3 group record");

		if (igmp_group_record->type == IGMPV3_CHANGE_TO_EXCLUDE_MODE) {
			is_join_msg_ok = true;
		} else if (igmp_group_record->type == IGMPV3_CHANGE_TO_INCLUDE_MODE) {
			is_leave_msg_ok = true;
		}
#else
		is_join_msg_ok = true;
#endif
		is_report_sent = true;
		k_sem_give(&wait_data);
	} else if (igmp_header->type == NET_IPV4_IGMP_LEAVE) {
		NET_DBG("Received leave....");
		is_leave_msg_ok = true;
		k_sem_give(&wait_data);
	}

	return 0;
}

struct net_test_igmp net_test_data;

static struct dummy_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test_igmp, "net_test_igmp",
		net_test_dev_init, NULL, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

static void group_joined(struct net_mgmt_event_callback *cb,
			 uint64_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV4_MCAST_JOIN) {
		/* Spurious callback. */
		return;
	}

	is_group_joined = true;

	k_sem_give(&wait_data);
}

static void group_left(struct net_mgmt_event_callback *cb,
			 uint64_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV4_MCAST_LEAVE) {
		/* Spurious callback. */
		return;
	}

	is_group_left = true;

	k_sem_give(&wait_data);
}

static struct mgmt_events {
	uint64_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
} mgmt_events[] = {
	{ .event = NET_EVENT_IPV4_MCAST_JOIN, .handler = group_joined },
	{ .event = NET_EVENT_IPV4_MCAST_LEAVE, .handler = group_left },
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

static void *igmp_setup(void)
{
	struct net_if_addr *ifaddr;

	setup_mgmt_events();

	net_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	zassert_not_null(net_iface, "Interface is NULL");

	ifaddr = net_if_ipv4_addr_add(net_iface, &my_addr, NET_ADDR_MANUAL, 0);

	zassert_not_null(ifaddr, "Cannot add IPv4 address");

	return NULL;
}

static void igmp_teardown(void *dummy)
{
	ARG_UNUSED(dummy);

	int i;

	for (i = 0; mgmt_events[i].event; i++) {
		net_mgmt_del_event_callback(&mgmt_events[i].cb);
	}

	net_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	net_if_ipv4_addr_rm(net_iface, &my_addr);
}

static struct net_pkt *prepare_igmp_query(struct net_if *iface, bool is_igmpv3)
{
	struct net_pkt *pkt;

	const unsigned char *igmp_query = is_igmpv3 ? igmp_v3_query : igmp_v2_query;
	size_t igmp_query_size = is_igmpv3 ? sizeof(igmp_v3_query) : sizeof(igmp_v2_query);

	pkt = net_pkt_alloc_with_buffer(iface, igmp_query_size, AF_INET, IPPROTO_IGMP, K_FOREVER);
	zassert_not_null(pkt, "Failed to allocate buffer");

	zassert_ok(net_pkt_write(pkt, igmp_query, igmp_query_size));

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	return pkt;
}

static void join_group(void)
{
	int ret;

	ret = net_ipv4_igmp_join(net_iface, &mcast_addr, NULL);

	if (ignore_already) {
		zassert_true(ret == 0 || ret == -EALREADY,
			     "Cannot join IPv4 multicast group");
	} else {
		zassert_ok(ret, "Cannot join IPv4 multicast group");
	}

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void leave_group(void)
{
	int ret;

	ret = net_ipv4_igmp_leave(net_iface, &mcast_addr);

	zassert_ok(ret, "Cannot leave IPv4 multicast group");

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
		/* Let the network stack to proceed */
		k_msleep(THREAD_SLEEP);
	} else {
		k_yield();
	}
}

static void catch_join_group(void)
{
	is_group_joined = false;

	ignore_already = false;

	join_group();

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting join event");

	zassert_true(is_group_joined, "Did not catch join event");

	is_group_joined = false;
}

static void catch_leave_group(void)
{
	is_group_joined = false;

	leave_group();

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting leave event");

	zassert_true(is_group_left, "Did not catch leave event");

	is_group_left = false;
}

static void verify_join_group(void)
{
	is_join_msg_ok = false;

	ignore_already = false;

	join_group();

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting join event");

	zassert_true(is_join_msg_ok, "Join msg invalid");

	is_join_msg_ok = false;
}

static void verify_leave_group(void)
{
	is_leave_msg_ok = false;

	leave_group();

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting leave event");

	zassert_true(is_leave_msg_ok, "Leave msg invalid");

	is_leave_msg_ok = false;
}

ZTEST(net_igmp, test_igmp_catch_join)
{
	join_group();
	leave_group();
}

ZTEST(net_igmp, test_igmp_catch_catch_join)
{
	catch_join_group();
	catch_leave_group();
}

ZTEST(net_igmp, test_igmp_verify_catch_join)
{
	verify_join_group();
	verify_leave_group();
}

static void socket_group_with_address(struct in_addr *local_addr, bool do_join)
{
	struct ip_mreqn mreqn = { 0 };
	int option;
	int ret, fd;

	if (do_join) {
		option = IP_ADD_MEMBERSHIP;
	} else {
		option = IP_DROP_MEMBERSHIP;
	}

	fd = zsock_socket(AF_INET, SOCK_DGRAM, 0);
	zassert_true(fd >= 0, "Cannot get socket (%d)", -errno);

	ret = zsock_setsockopt(fd, IPPROTO_IP, option,
			       NULL, sizeof(mreqn));
	zassert_equal(ret, -1, "Incorrect return value (%d)", ret);
	zassert_equal(errno, EINVAL, "Incorrect errno value (%d)", -errno);

	ret = zsock_setsockopt(fd, IPPROTO_IP, option,
			       (void *)&mreqn, 1);
	zassert_equal(ret, -1, "Incorrect return value (%d)", ret);
	zassert_equal(errno, EINVAL, "Incorrect errno value (%d)", -errno);

	/* First try with empty mreqn */
	ret = zsock_setsockopt(fd, IPPROTO_IP, option,
			       (void *)&mreqn, sizeof(mreqn));
	zassert_equal(ret, -1, "Incorrect return value (%d)", ret);
	zassert_equal(errno, EINVAL, "Incorrect errno value (%d)", -errno);

	memcpy(&mreqn.imr_address, local_addr, sizeof(mreqn.imr_address));
	memcpy(&mreqn.imr_multiaddr, &mcast_addr, sizeof(mreqn.imr_multiaddr));

	ret = zsock_setsockopt(fd, IPPROTO_IP, option,
			       (void *)&mreqn, sizeof(mreqn));

	if (do_join) {
		if (ignore_already) {
			zassert_true(ret == 0 || ret == -EALREADY,
				     "Cannot join IPv4 multicast group (%d)", -errno);
		} else {
			zassert_ok(ret,
				   "Cannot join IPv4 multicast group (%d) "
				   "with local addr %s",
				   -errno, net_sprint_ipv4_addr(local_addr));
		}
	} else {
		zassert_ok(ret, "Cannot leave IPv4 multicast group (%d)", -errno);

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

static void socket_group_with_index(struct in_addr *local_addr, bool do_join)
{
	struct ip_mreqn mreqn = { 0 };
	int option;
	int ret, fd;

	if (do_join) {
		option = IP_ADD_MEMBERSHIP;
	} else {
		option = IP_DROP_MEMBERSHIP;
	}

	fd = zsock_socket(AF_INET, SOCK_DGRAM, 0);
	zassert_true(fd >= 0, "Cannot get socket (%d)", -errno);

	mreqn.imr_ifindex = net_if_ipv4_addr_lookup_by_index(local_addr);
	memcpy(&mreqn.imr_multiaddr, &mcast_addr, sizeof(mreqn.imr_multiaddr));

	ret = zsock_setsockopt(fd, IPPROTO_IP, option,
			       (void *)&mreqn, sizeof(mreqn));

	if (do_join) {
		if (ignore_already) {
			zassert_true(ret == 0 || ret == -EALREADY,
				     "Cannot join IPv4 multicast group (%d)", -errno);
		} else {
			zassert_ok(ret, "Cannot join IPv4 multicast group (%d)", -errno);
		}
	} else {
		zassert_ok(ret, "Cannot leave IPv4 multicast group (%d)", -errno);

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

static void socket_join_group_with_address(struct in_addr *addr)
{
	socket_group_with_address(addr, true);
}

static void socket_leave_group_with_address(struct in_addr *addr)
{
	socket_group_with_address(addr, false);
}

static void socket_join_group_with_index(struct in_addr *addr)
{
	socket_group_with_index(addr, true);
}

static void socket_leave_group_with_index(struct in_addr *addr)
{
	socket_group_with_index(addr, false);
}

ZTEST_USER(net_igmp, test_socket_catch_join_with_address)
{
	socket_join_group_with_address(&any_addr);
	socket_leave_group_with_address(&any_addr);
	socket_join_group_with_address(&my_addr);
	socket_leave_group_with_address(&my_addr);
}

ZTEST_USER(net_igmp, test_socket_catch_join_with_index)
{
	socket_join_group_with_index(&any_addr);
	socket_leave_group_with_index(&any_addr);
	socket_join_group_with_index(&my_addr);
	socket_leave_group_with_index(&my_addr);
}

static void igmp_send_query(bool is_imgpv3)
{
	struct net_pkt *pkt;

	is_report_sent = false;
	is_join_msg_ok = false;

	is_igmpv2_query_sent = false;
	is_igmpv3_query_sent = false;

	/* Joining group first to get reply on query*/
	join_group();

	is_igmpv2_query_sent = !is_imgpv3;
	is_igmpv3_query_sent = is_imgpv3;

	pkt = prepare_igmp_query(net_iface, is_imgpv3);
	zassert_not_null(pkt, "IGMPv2 query packet prep failed");

	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting query event");

	zassert_true(is_report_sent, "Did not catch query event");

	zassert_true(is_join_msg_ok, "Join msg invalid");

	is_igmpv2_query_sent = false;
	is_igmpv3_query_sent = false;

	leave_group();
}

ZTEST_USER(net_igmp, test_igmpv3_query)
{
	igmp_send_query(true);
}

ZTEST_USER(net_igmp, test_igmpv2_query)
{
	igmp_send_query(false);
}

ZTEST_USER(net_igmp, test_group_rejoin)
{
	/* It is enough if this is tested with IGMPv2 only because we do not
	 * really care about specific IGMP version here.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4_IGMPV3)) {
		ztest_test_skip();
	}

	socket_join_group_with_index(&my_addr);

	is_report_sent = false;

	net_if_carrier_off(net_iface);
	net_if_carrier_on(net_iface);

	zassert_true(is_report_sent, "Did not catch query event");

	socket_leave_group_with_index(&my_addr);
}

ZTEST_SUITE(net_igmp, NULL, igmp_setup, NULL, NULL, igmp_teardown);
