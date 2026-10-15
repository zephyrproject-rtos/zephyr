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
#include <zephyr/net/net_log.h>
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
#include <zephyr/sys/byteorder.h>

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

static struct net_in_addr my_addr = { { { 192, 0, 2, 1 } } };
static struct net_in_addr mcast_addr = { { { 224, 0, 2, 63 } } };
static struct net_in_addr other_mcast_addr = { { { 224, 0, 2, 64 } } };
static struct net_in_addr any_addr = NET_INADDR_ANY_INIT;

static struct net_if *net_iface;
static bool is_group_joined;
static bool is_group_left;
static bool is_join_msg_ok;
static bool is_leave_msg_ok;
static bool is_query_received;
static bool is_report_sent;
static bool is_query_resp_ok;
static bool is_v1_report_sent;
static bool is_v2_report_sent;
static bool is_v2_leave_sent;
static int report_count;
static bool expect_v2_report;
static int expected_groups = 1;
K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME 500
/* Max Resp Code of the queries sent by the tests, in tenths of a second */
#define QUERY_MAX_RSP  1
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

/* Length of the IPv4 header with the Router Alert option */
#define IGMP_IP_HDR_LEN  24
#define IGMP_MSG_MAX_LEN 256

static const struct net_in_addr all_systems_addr = { { { 224, 0, 0, 1 } } };
static const struct net_in_addr all_routers_addr = { { { 224, 0, 0, 2 } } };
static const struct net_in_addr igmpv3_report_addr = { { { 224, 0, 0, 22 } } };

static uint16_t test_chksum(const uint8_t *data, size_t len)
{
	uint32_t sum = 0;

	for (size_t i = 0; i + 1 < len; i += 2) {
		sum += sys_get_be16(&data[i]);
	}

	if ((len & 1) != 0) {
		sum += data[len - 1] << 8;
	}

	while ((sum >> 16) != 0) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return ~sum;
}

/* Every IGMP message is sent with the Router Alert option and a TTL of 1
 * (RFC 3376 ch 2). Reports go to the group, an IGMPv2 leave to the all
 * routers group (RFC 2236 ch 9) and an IGMPv3 report to 224.0.0.22 (RFC 3376
 * ch 4.2.14).
 */
static void check_sent_msg(const uint8_t *buf, size_t len)
{
	static const uint8_t router_alert[] = { 0x94, 0x04, 0x00, 0x00 };
	const uint8_t *igmp = &buf[IGMP_IP_HDR_LEN];
	const uint8_t *dst = NULL;

	zassert_true(len >= IGMP_IP_HDR_LEN + 8, "Message too short (%zu)", len);
	zassert_equal(buf[0], 0x46, "Not an IPv4 header with one option word (0x%02x)", buf[0]);
	zassert_equal(sys_get_be16(&buf[2]), len, "Wrong total length");
	zassert_equal(buf[8], 1, "TTL is not 1");
	zassert_equal(buf[9], NET_IPPROTO_IGMP, "Not an IGMP message");
	zassert_equal(test_chksum(buf, IGMP_IP_HDR_LEN), 0, "Bad IPv4 header checksum");
	zassert_mem_equal(&buf[20], router_alert, sizeof(router_alert), "No Router Alert option");
	zassert_equal(test_chksum(igmp, len - IGMP_IP_HDR_LEN), 0, "Bad IGMP checksum");

	switch (igmp[0]) {
	case NET_IPV4_IGMP_REPORT_V1:
	case NET_IPV4_IGMP_REPORT_V2:
		dst = &igmp[4];
		break;
	case NET_IPV4_IGMP_LEAVE:
		dst = all_routers_addr.s4_addr;
		break;
	case NET_IPV4_IGMP_REPORT_V3:
		dst = igmpv3_report_addr.s4_addr;
		break;
	default:
		break;
	}

	if (dst != NULL) {
		zassert_mem_equal(&buf[16], dst, 4, "Wrong destination address");
	}

	if (dst != NULL && igmp[0] != NET_IPV4_IGMP_REPORT_V3) {
		/* The all-systems group never changes state, RFC 2236 ch 6 */
		zassert_true(memcmp(&igmp[4], all_systems_addr.s4_addr, 4) != 0,
			     "Message for the all-systems group");
	}
}

static void check_v3_report(const uint8_t *igmp, size_t len)
{
	uint16_t groups = sys_get_be16(&igmp[6]);
	const uint8_t *record = &igmp[8];

	zassert_true(IS_ENABLED(CONFIG_NET_IPV4_IGMPV3), "Wrong IGMP report received (IGMPv3)");
	zassert_false(expect_v2_report, "IGMPv3 response to IGMPv2 request");
	zassert_equal(groups, expected_groups, "Expected %d group records, got %d", expected_groups,
		      groups);

	for (uint16_t i = 0; i < groups; i++) {
		uint16_t sources;

		zassert_true(record + 8 <= igmp + len, "Truncated group record");
		sources = sys_get_be16(&record[2]);
		zassert_equal(sources, 0, "Invalid sources length of IGMPv3 group record");
		zassert_true(memcmp(&record[4], all_systems_addr.s4_addr, 4) != 0,
			     "Record for the all-systems group");

		switch (record[0]) {
		case IGMPV3_CHANGE_TO_EXCLUDE_MODE:
			is_join_msg_ok = true;
			break;
		case IGMPV3_CHANGE_TO_INCLUDE_MODE:
			is_leave_msg_ok = true;
			break;
		case IGMPV3_MODE_IS_EXCLUDE:
			/* Current-state record, only sent in response to a query */
			is_query_resp_ok = true;
			break;
		default:
			zassert_unreachable("Unexpected group record type %d", record[0]);
		}

		record += 8 + 4 * sources;
	}
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	uint8_t buf[IGMP_MSG_MAX_LEN];
	size_t len = net_pkt_get_len(pkt);
	const uint8_t *igmp = &buf[IGMP_IP_HDR_LEN];

	ARG_UNUSED(dev);

	if (pkt->buffer == NULL) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	zassert_true(len <= sizeof(buf), "Message too long (%zu)", len);
	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, buf, len), "Cannot read the message");

	check_sent_msg(buf, len);

	switch (igmp[0]) {
	case NET_IPV4_IGMP_QUERY:
		NET_DBG("Received query....");
		is_query_received = true;
		k_sem_give(&wait_data);
		break;
	case NET_IPV4_IGMP_REPORT_V1:
		NET_DBG("Received v1 report....");
		is_v1_report_sent = true;
		is_join_msg_ok = true;
		is_query_resp_ok = true;
		is_report_sent = true;
		report_count++;
		k_sem_give(&wait_data);
		break;
	case NET_IPV4_IGMP_REPORT_V2:
		NET_DBG("Received v2 report....");
		zassert_true(!IS_ENABLED(CONFIG_NET_IPV4_IGMPV3) || expect_v2_report,
			     "Wrong IGMP report received (IGMPv2)");
		is_v2_report_sent = true;
		is_join_msg_ok = true;
		is_query_resp_ok = true;
		is_report_sent = true;
		report_count++;
		k_sem_give(&wait_data);
		break;
	case NET_IPV4_IGMP_REPORT_V3:
		NET_DBG("Received v3 report....");
		check_v3_report(igmp, len - IGMP_IP_HDR_LEN);
		is_report_sent = true;
		report_count++;
		k_sem_give(&wait_data);
		break;
	case NET_IPV4_IGMP_LEAVE:
		NET_DBG("Received leave....");
		is_v2_leave_sent = true;
		is_leave_msg_ok = true;
		k_sem_give(&wait_data);
		break;
	default:
		zassert_unreachable("Unexpected IGMP message type %d", igmp[0]);
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

/* Forget about older version queriers heard and responses scheduled by a
 * previous test.
 */
static void igmp_before(void *fixture)
{
	struct net_if_ipv4 *ipv4 = net_iface->config.ip.ipv4;

	ARG_UNUSED(fixture);

	ipv4->igmp_v1_querier_timeout = sys_timepoint_calc(K_NO_WAIT);
	ipv4->igmp_v2_querier_timeout = sys_timepoint_calc(K_NO_WAIT);
	ipv4->igmp_version = 0;
	ipv4->igmp_general_timeout = sys_timepoint_calc(K_FOREVER);
	ARRAY_FOR_EACH(ipv4->mcast, i) {
		ipv4->mcast[i].igmp_resp_timeout = sys_timepoint_calc(K_FOREVER);
		ipv4->mcast[i].igmp_retx_timeout = sys_timepoint_calc(K_FOREVER);
		ipv4->mcast[i].igmp_retx_left = 0;
	}

	expect_v2_report = false;
	expected_groups = 1;
	k_sem_reset(&wait_data);
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

struct igmp_msg {
	/* IGMPv3 query with the 12 byte header, IGMPv1 or IGMPv2 otherwise */
	bool igmpv3;
	/* IGMP message type */
	uint8_t type;
	/* Max Resp Code */
	uint8_t max_rsp;
	/* IP destination, the all systems group when NULL */
	const struct net_in_addr *dst;
	/* Group Address field, unspecified when NULL */
	const struct net_in_addr *group;
	/* Leave out the Router Alert option */
	bool no_router_alert;
	/* Number of (unspecified) source addresses appended to an IGMPv3 query */
	uint16_t sources;
	/* Value of the Number of Sources field when it should differ from sources */
	uint16_t claimed_sources;
	/* Extra zero bytes appended to the IGMP message */
	uint8_t extra_len;
};

/* Build an IGMP message inside an IPv4 header with the Router Alert option */
static struct net_pkt *prepare_igmp_msg(struct net_if *iface, const struct igmp_msg *msg)
{
	static const struct net_in_addr src_addr = { { { 192, 0, 2, 69 } } };
	const struct net_in_addr *dst = msg->dst != NULL ? msg->dst : &all_systems_addr;
	size_t ip_hdr_len = msg->no_router_alert ? 20 : IGMP_IP_HDR_LEN;
	size_t igmp_len = (msg->igmpv3 ? 12 : 8) + 4 * msg->sources + msg->extra_len;
	size_t len = ip_hdr_len + igmp_len;
	uint8_t buf[IGMP_MSG_MAX_LEN];
	uint8_t *igmp = &buf[ip_hdr_len];
	struct net_pkt *pkt;

	zassert_true(len <= sizeof(buf), "Message too long (%zu)", len);
	memset(buf, 0, len);

	buf[0] = 0x40 | (ip_hdr_len / 4);
	buf[1] = 0xc0;
	sys_put_be16(len, &buf[2]);
	buf[8] = 1;
	buf[9] = NET_IPPROTO_IGMP;
	memcpy(&buf[12], src_addr.s4_addr, sizeof(src_addr.s4_addr));
	memcpy(&buf[16], dst->s4_addr, sizeof(dst->s4_addr));
	if (!msg->no_router_alert) {
		buf[20] = 0x94;
		buf[21] = 0x04;
	}
	sys_put_be16(test_chksum(buf, ip_hdr_len), &buf[10]);

	igmp[0] = msg->type;
	igmp[1] = msg->max_rsp;
	if (msg->group != NULL) {
		memcpy(&igmp[4], msg->group->s4_addr, sizeof(msg->group->s4_addr));
	}
	if (msg->igmpv3) {
		igmp[8] = 0x02; /* QRV 2 */
		igmp[9] = 0x7d; /* QQIC 125 seconds */
		sys_put_be16(msg->claimed_sources != 0 ? msg->claimed_sources : msg->sources,
			     &igmp[10]);
	}
	sys_put_be16(test_chksum(igmp, igmp_len), &igmp[2]);

	pkt = net_pkt_alloc_with_buffer(iface, len, NET_AF_INET, NET_IPPROTO_IGMP, K_FOREVER);
	zassert_not_null(pkt, "Failed to allocate buffer");

	zassert_ok(net_pkt_write(pkt, buf, len));

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	return pkt;
}

/* A General Query, or a Group-Specific Query when group is given */
static struct net_pkt *prepare_igmp_query(struct net_if *iface, bool is_igmpv3,
					  const struct net_in_addr *group, uint8_t max_rsp)
{
	const struct igmp_msg msg = {
		.igmpv3 = is_igmpv3,
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = max_rsp,
		.dst = group,
		.group = group,
	};

	return prepare_igmp_msg(iface, &msg);
}

/* The unsolicited report of a join is retransmitted after a random delay,
 * which would disturb the tests that count reports. Cancel it right after
 * the join and verify the retransmission in its own test.
 */
static void cancel_retransmit(const struct net_in_addr *group)
{
	struct net_if *iface = net_iface;
	struct net_if_mcast_addr *maddr;

	maddr = net_if_ipv4_maddr_lookup(group, &iface);
	zassert_not_null(maddr, "Group not registered");

	maddr->igmp_retx_left = 0;
	maddr->igmp_retx_timeout = sys_timepoint_calc(K_FOREVER);
}

static void join_group_retransmit(bool retransmit)
{
	int ret;

	ret = net_ipv4_igmp_join(net_iface, &mcast_addr, NULL);
	zassert_ok(ret, "Cannot join IPv4 multicast group");

	if (!retransmit) {
		cancel_retransmit(&mcast_addr);
	}

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void join_group(void)
{
	join_group_retransmit(false);
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

static void socket_group_with_address(struct net_in_addr *local_addr, bool do_join)
{
	struct net_ip_mreqn mreqn = { 0 };
	int option;
	int ret, fd;

	if (do_join) {
		option = ZSOCK_IP_ADD_MEMBERSHIP;
	} else {
		option = ZSOCK_IP_DROP_MEMBERSHIP;
	}

	fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, 0);
	zassert_true(fd >= 0, "Cannot get socket (%d)", -errno);

	ret = zsock_setsockopt(fd, NET_IPPROTO_IP, option,
			       NULL, sizeof(mreqn));
	zassert_equal(ret, -1, "Incorrect return value (%d)", ret);
	zassert_equal(errno, EINVAL, "Incorrect errno value (%d)", -errno);

	ret = zsock_setsockopt(fd, NET_IPPROTO_IP, option,
			       (void *)&mreqn, 1);
	zassert_equal(ret, -1, "Incorrect return value (%d)", ret);
	zassert_equal(errno, EINVAL, "Incorrect errno value (%d)", -errno);

	/* First try with empty mreqn */
	ret = zsock_setsockopt(fd, NET_IPPROTO_IP, option,
			       (void *)&mreqn, sizeof(mreqn));
	zassert_equal(ret, -1, "Incorrect return value (%d)", ret);
	zassert_equal(errno, EINVAL, "Incorrect errno value (%d)", -errno);

	memcpy(&mreqn.imr_address, local_addr, sizeof(mreqn.imr_address));
	memcpy(&mreqn.imr_multiaddr, &mcast_addr, sizeof(mreqn.imr_multiaddr));

	ret = zsock_setsockopt(fd, NET_IPPROTO_IP, option,
			       (void *)&mreqn, sizeof(mreqn));

	if (do_join) {
		zassert_ok(ret,
			   "Cannot join IPv4 multicast group (%d) "
			   "with local addr %s",
			   -errno, net_sprint_ipv4_addr(local_addr));
		cancel_retransmit(&mcast_addr);
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

static void socket_group_with_index(struct net_in_addr *local_addr, bool do_join)
{
	struct net_ip_mreqn mreqn = { 0 };
	int option;
	int ret, fd;

	if (do_join) {
		option = ZSOCK_IP_ADD_MEMBERSHIP;
	} else {
		option = ZSOCK_IP_DROP_MEMBERSHIP;
	}

	fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, 0);
	zassert_true(fd >= 0, "Cannot get socket (%d)", -errno);

	mreqn.imr_ifindex = net_if_ipv4_addr_lookup_by_index(local_addr);
	memcpy(&mreqn.imr_multiaddr, &mcast_addr, sizeof(mreqn.imr_multiaddr));

	ret = zsock_setsockopt(fd, NET_IPPROTO_IP, option,
			       (void *)&mreqn, sizeof(mreqn));

	if (do_join) {
		zassert_ok(ret, "Cannot join IPv4 multicast group (%d)", -errno);
		cancel_retransmit(&mcast_addr);
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

static void socket_join_group_with_address(struct net_in_addr *addr)
{
	socket_group_with_address(addr, true);
}

static void socket_leave_group_with_address(struct net_in_addr *addr)
{
	socket_group_with_address(addr, false);
}

static void socket_join_group_with_index(struct net_in_addr *addr)
{
	socket_group_with_index(addr, true);
}

static void socket_leave_group_with_index(struct net_in_addr *addr)
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

static void igmp_send_query(bool is_imgpv3, const struct net_in_addr *group)
{
	struct net_pkt *pkt;

	expect_v2_report = false;

	/* Joining group first to get reply on query*/
	join_group();

	/* Discard the events of the join itself */
	k_sem_reset(&wait_data);
	is_report_sent = false;
	is_query_resp_ok = false;

	/* Only a General Query from an IGMPv2 querier switches an IGMPv3 host
	 * to IGMPv2, a Group-Specific one is answered in IGMPv3.
	 */
	expect_v2_report = !IS_ENABLED(CONFIG_NET_IPV4_IGMPV3) || (!is_imgpv3 && group == NULL);

	pkt = prepare_igmp_query(net_iface, is_imgpv3, group, QUERY_MAX_RSP);
	zassert_not_null(pkt, "IGMP query packet prep failed");

	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting query event");

	zassert_true(is_report_sent, "Did not catch query event");

	zassert_true(is_query_resp_ok, "Query response invalid");

	expect_v2_report = false;

	leave_group();
}

ZTEST_USER(net_igmp, test_igmpv3_query)
{
	igmp_send_query(true, NULL);
}

ZTEST_USER(net_igmp, test_igmpv2_query)
{
	igmp_send_query(false, NULL);
}

ZTEST_USER(net_igmp, test_igmpv3_group_query)
{
	igmp_send_query(true, &mcast_addr);
}

ZTEST_USER(net_igmp, test_igmpv2_group_query)
{
	igmp_send_query(false, &mcast_addr);
}

/* Inject a message that must not trigger any report */
static void igmp_send_unanswered(const struct igmp_msg *msg, enum net_verdict verdict)
{
	struct net_pkt *pkt;

	join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;

	pkt = prepare_igmp_msg(net_iface, msg);
	zassert_not_null(pkt, "IGMP packet prep failed");

	zassert_equal(net_ipv4_input(pkt), verdict, "Unexpected verdict");
	if (verdict == NET_DROP) {
		net_pkt_unref(pkt);
	}

	zassert_equal(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), -EAGAIN, "Unexpected event");
	zassert_false(is_report_sent, "Unexpected report");

	leave_group();
}

/* The IPv4 layer drops multicast packets for groups that are not joined */
ZTEST_USER(net_igmp, test_igmp_group_query_not_member)
{
	const struct igmp_msg msg = {
		.igmpv3 = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3),
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
		.dst = &other_mcast_addr,
		.group = &other_mcast_addr,
	};

	igmp_send_unanswered(&msg, NET_DROP);
}

/* A message that is not a query is consumed without an answer */
ZTEST_USER(net_igmp, test_igmp_report_not_answered)
{
	const struct igmp_msg msg = {
		.type = NET_IPV4_IGMP_REPORT_V2,
		.dst = &mcast_addr,
		.group = &mcast_addr,
	};

	igmp_send_unanswered(&msg, NET_OK);
}

/* A General Query has to be sent to the all systems group (RFC 3376 ch 9) */
ZTEST_USER(net_igmp, test_igmp_general_query_wrong_dst)
{
	const struct igmp_msg msg = {
		.igmpv3 = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3),
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
		.dst = &mcast_addr,
	};

	igmp_send_unanswered(&msg, NET_DROP);
}

/* A query whose length matches no version is ignored (RFC 3376 ch 7.1) */
ZTEST_USER(net_igmp, test_igmp_query_odd_length)
{
	const struct igmp_msg msg = {
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
		.extra_len = 2,
	};

	igmp_send_unanswered(&msg, NET_DROP);
}

/* An IGMPv3 query shorter than its Number of Sources requires is dropped */
ZTEST_USER(net_igmp, test_igmp_v3_query_truncated)
{
	const struct igmp_msg msg = {
		.igmpv3 = true,
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
		.claimed_sources = 1,
	};

	igmp_send_unanswered(&msg, NET_DROP);
}

/* Octets beyond the source list of an IGMPv3 query are ignored (RFC 3376 ch
 * 4.1.10), the query is answered.
 */
ZTEST_USER(net_igmp, test_igmp_v3_query_extra_octets)
{
	const struct igmp_msg msg = {
		.igmpv3 = true,
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
		.extra_len = 2,
	};
	struct net_pkt *pkt;

	join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;
	expect_v2_report = !IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);

	pkt = prepare_igmp_msg(net_iface, &msg);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting query event");
	zassert_true(is_report_sent, "Query not answered");

	leave_group();
}

/* A query longer than one network buffer is still recognized and answered */
ZTEST_USER(net_igmp, test_igmp_long_query)
{
	const struct igmp_msg msg = {
		.igmpv3 = true,
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
		.sources = (CONFIG_NET_BUF_DATA_SIZE - IGMP_IP_HDR_LEN) / 4,
	};
	struct net_pkt *pkt;

	join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;
	expect_v2_report = !IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);

	pkt = prepare_igmp_msg(net_iface, &msg);
	zassert_true(pkt->buffer->frags != NULL, "Query fits in one buffer");
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting query event");
	zassert_true(is_report_sent, "Long query not answered");

	leave_group();
}

/* A query without the Router Alert option is ignored (RFC 3376 ch 9.1) when
 * the option is required, and answered like any other query otherwise.
 */
ZTEST_USER(net_igmp, test_igmp_query_without_router_alert)
{
	const struct igmp_msg msg = {
		.igmpv3 = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3),
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
		.no_router_alert = true,
	};
	struct net_pkt *pkt;

	if (IS_ENABLED(CONFIG_NET_IPV4_IGMP_REQUIRE_ROUTER_ALERT)) {
		igmp_send_unanswered(&msg, NET_OK);
		return;
	}

	join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;
	expect_v2_report = !IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);

	pkt = prepare_igmp_msg(net_iface, &msg);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting query event");
	zassert_true(is_report_sent, "Query without Router Alert not answered");

	leave_group();
}

/* An IGMPv1 query predates the Router Alert option and is accepted without it,
 * here seen from the host switching to IGMPv1 reports.
 */
ZTEST_USER(net_igmp, test_igmp_v1_query_without_router_alert)
{
	const struct igmp_msg msg = {
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = 0,
		.no_router_alert = true,
	};
	struct net_pkt *pkt;

	pkt = prepare_igmp_msg(net_iface, &msg);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	is_v1_report_sent = false;

	join_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting join event");
	zassert_true(is_v1_report_sent, "Join not reported with IGMPv1");

	leave_group();
}

/* A General Query is answered for every joined group: with one record each
 * in a single IGMPv3 report, or with one IGMPv2 report per group.
 */
ZTEST_USER(net_igmp, test_igmp_general_query_all_groups)
{
	bool is_igmpv3 = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);
	int expected = is_igmpv3 ? 1 : 2;
	struct net_pkt *pkt;

	join_group();
	zassert_ok(net_ipv4_igmp_join(net_iface, &other_mcast_addr, NULL), "Cannot join");
	cancel_retransmit(&other_mcast_addr);
	k_msleep(THREAD_SLEEP);

	k_sem_reset(&wait_data);
	report_count = 0;
	is_query_resp_ok = false;
	expect_v2_report = !is_igmpv3;
	expected_groups = 2;

	pkt = prepare_igmp_query(net_iface, is_igmpv3, NULL, QUERY_MAX_RSP);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	k_msleep(WAIT_TIME);
	zassert_equal(report_count, expected, "Expected %d reports, got %d", expected,
		      report_count);
	zassert_true(is_query_resp_ok, "Query not answered");

	expected_groups = 1;
	zassert_ok(net_ipv4_igmp_leave(net_iface, &other_mcast_addr), "Cannot leave");
	k_msleep(THREAD_SLEEP);
	leave_group();
}

ZTEST_USER(net_igmp, test_group_rejoin)
{
	socket_join_group_with_index(&my_addr);

	is_report_sent = false;
	is_join_msg_ok = false;

	net_if_carrier_off(net_iface);
	net_if_carrier_on(net_iface);

	zassert_true(is_report_sent, "Did not catch query event");
	zassert_true(is_join_msg_ok, "Group not reported on rejoin");

	socket_leave_group_with_index(&my_addr);
}

/* Taking the interface down sends a Leave for every group but the
 * all-systems group, which never changes state (RFC 2236 ch 6), here in
 * IGMPv2 mode where the Leave is a message of its own.
 */
ZTEST_USER(net_igmp, test_v2_leave_on_iface_down)
{
	const struct igmp_msg msg = {
		.type = NET_IPV4_IGMP_QUERY,
		.max_rsp = QUERY_MAX_RSP,
	};
	struct net_pkt *pkt;

	/* An IGMPv2 querier puts the host in IGMPv2 mode */
	pkt = prepare_igmp_msg(net_iface, &msg);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");
	expect_v2_report = true;

	join_group();

	is_leave_msg_ok = false;
	zassert_ok(net_if_down(net_iface), "Cannot take the interface down");
	zassert_true(is_leave_msg_ok, "Group not left on interface down");

	zassert_ok(net_if_up(net_iface), "Cannot bring the interface up");
	leave_group();
}

/* A query is not answered right away but after a random delay bounded by
 * its Max Resp Time (RFC 3376 ch 5.2, RFC 2236 ch 3).
 */
ZTEST_USER(net_igmp, test_igmp_query_delayed)
{
	struct net_pkt *pkt;
	int64_t start;

	join_group();

	k_sem_reset(&wait_data);
	is_report_sent = false;
	expect_v2_report = !IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);

	/* Max Resp Code 10 is one second */
	pkt = prepare_igmp_query(net_iface, IS_ENABLED(CONFIG_NET_IPV4_IGMPV3), NULL, 10);
	start = k_uptime_get();
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");
	/* A report is only immediate when no time passed while the query was
	 * handled; the host may have been stalled for longer than the delay.
	 */
	zassert_true(!is_report_sent || k_uptime_get() > start, "Query answered without delay");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(1500)), "Timeout while waiting query event");
	zassert_true(is_report_sent, "Query not answered");

	leave_group();
}

/* A second General Query while a response is pending does not schedule
 * another response (RFC 3376 ch 5.2 rule 1).
 */
ZTEST_USER(net_igmp, test_igmp_query_merged)
{
	struct net_pkt *pkt;

	join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	expect_v2_report = !IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);

	for (int i = 0; i < 2; i++) {
		pkt = prepare_igmp_query(net_iface, IS_ENABLED(CONFIG_NET_IPV4_IGMPV3), NULL, 5);
		zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");
	}

	if (report_count != 0) {
		/* Stalled so long that the first response was already due */
		leave_group();
		ztest_test_skip();
	}

	k_msleep(1000);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);

	leave_group();
}

/* Max Resp Code decoding: IGMPv2 is linear in tenths of a second, IGMPv3
 * switches to a floating point form at 128 (RFC 3376 ch 4.1.1), a code of 0
 * comes from an IGMPv1 querier and means 10 seconds (RFC 3376 ch 7.2.1).
 */
ZTEST(net_igmp, test_igmp_max_resp_time)
{
	zassert_equal(net_ipv4_igmp_max_resp_time(0, false), 10000);
	zassert_equal(net_ipv4_igmp_max_resp_time(0, true), 10000);
	zassert_equal(net_ipv4_igmp_max_resp_time(1, false), 100);
	zassert_equal(net_ipv4_igmp_max_resp_time(100, false), 10000);
	zassert_equal(net_ipv4_igmp_max_resp_time(255, false), 25500);
	zassert_equal(net_ipv4_igmp_max_resp_time(127, true), 12700);
	/* mant 0, exp 0: (0x10 << 3) tenths */
	zassert_equal(net_ipv4_igmp_max_resp_time(0x80, true), 12800);
	/* mant 15, exp 7: (0x1f << 10) tenths */
	zassert_equal(net_ipv4_igmp_max_resp_time(0xff, true), 3174400);
}

/* A pending response to a General Query that is due sooner covers a later
 * Group-Specific Query (RFC 3376 ch 5.2 rule 1). With per-group timers the
 * same single report results.
 */
ZTEST_USER(net_igmp, test_igmp_query_general_covers_group)
{
	bool is_igmpv3 = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);
	struct net_pkt *pkt;

	join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	expect_v2_report = !is_igmpv3;

	pkt = prepare_igmp_query(net_iface, is_igmpv3, NULL, 1);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	pkt = prepare_igmp_query(net_iface, is_igmpv3, &mcast_addr, 0xff);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	if (report_count != 0) {
		/* Stalled so long that the first response was already due */
		leave_group();
		ztest_test_skip();
	}

	k_msleep(1500);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);

	leave_group();
}

/* A second query for a group is answered at the earliest of the pending and
 * the newly selected delay (RFC 3376 ch 5.2 rule 4, RFC 2236 ch 3).
 */
ZTEST_USER(net_igmp, test_igmp_query_group_earliest)
{
	bool is_igmpv3 = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3);
	struct net_pkt *pkt;

	join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	expect_v2_report = !is_igmpv3;

	/* Up to 25.5 seconds, then up to 100 milliseconds */
	pkt = prepare_igmp_query(net_iface, is_igmpv3, &mcast_addr, 0xff);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	pkt = prepare_igmp_query(net_iface, is_igmpv3, &mcast_addr, 1);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting query event");

	k_msleep(1000);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);

	leave_group();
}

/* Switching compatibility mode cancels the pending responses (RFC 3376 ch
 * 7.2.1), here when an IGMPv3 query arrives after the IGMPv2 querier present
 * timer ran out.
 */
ZTEST_USER(net_igmp, test_igmp_mode_change_cancels_response)
{
	struct net_if_ipv4 *ipv4 = net_iface->config.ip.ipv4;
	struct net_pkt *pkt;

	if (!IS_ENABLED(CONFIG_NET_IPV4_IGMPV3)) {
		ztest_test_skip();
	}

	join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	is_v2_report_sent = false;

	/* An IGMPv2 querier: a response is pending for up to 25.5 seconds */
	expect_v2_report = true;
	pkt = prepare_igmp_query(net_iface, false, NULL, 0xff);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	/* The querier went away and an IGMPv3 one shows up */
	ipv4->igmp_v2_querier_timeout = sys_timepoint_calc(K_NO_WAIT);

	expect_v2_report = false;
	pkt = prepare_igmp_query(net_iface, true, NULL, 1);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	k_msleep(1500);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);
	zassert_false(is_v2_report_sent, "Cancelled IGMPv2 response was sent");

	leave_group();
}

/* The compatibility mode also changes when the querier present timer runs
 * out on its own, which cancels the pending responses as well.
 */
ZTEST_USER(net_igmp, test_igmp_mode_expiry_cancels_response)
{
	struct net_if_ipv4 *ipv4 = net_iface->config.ip.ipv4;
	struct net_if *iface = net_iface;
	struct net_if_mcast_addr *maddr;
	struct net_pkt *pkt;

	if (!IS_ENABLED(CONFIG_NET_IPV4_IGMPV3)) {
		ztest_test_skip();
	}

	join_group();
	maddr = net_if_ipv4_maddr_lookup(&mcast_addr, &iface);
	zassert_not_null(maddr, "Group not registered");

	k_sem_reset(&wait_data);
	report_count = 0;
	expect_v2_report = true;

	/* An IGMPv2 querier: a response is pending for up to 200 milliseconds */
	pkt = prepare_igmp_query(net_iface, false, NULL, 2);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	/* The querier present timer runs out before the response is due */
	ipv4->igmp_v2_querier_timeout = sys_timepoint_calc(K_NO_WAIT);

	if (report_count != 0 || sys_timepoint_expired(maddr->igmp_resp_timeout)) {
		/* Stalled so long that the response was already due */
		leave_group();
		ztest_test_skip();
	}

	k_msleep(1000);
	zassert_equal(report_count, 0, "Cancelled response was sent");

	/* Back in IGMPv3 mode, the leave is reported with IGMPv3 */
	expect_v2_report = false;
	leave_group();
}

/* A report from another member of the group cancels the pending response of
 * an IGMPv2 host (RFC 2236 ch 3).
 */
ZTEST_USER(net_igmp, test_igmp_report_suppressed)
{
	struct net_pkt *pkt;

	join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	expect_v2_report = true;

	/* An IGMPv2 General Query with a Max Resp Time of one second */
	pkt = prepare_igmp_query(net_iface, false, NULL, 10);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	pkt = prepare_igmp_msg(net_iface, &(const struct igmp_msg){
						  .type = NET_IPV4_IGMP_REPORT_V2,
						  .dst = &mcast_addr,
						  .group = &mcast_addr,
					  });
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	if (report_count != 0) {
		/* Stalled so long that the response was due before the report */
		leave_group();
		ztest_test_skip();
	}

	k_msleep(1500);
	zassert_equal(report_count, 0, "Report not suppressed, got %d", report_count);

	leave_group();
}

/* An IGMPv3 host does not suppress its response when another member reports
 * with IGMPv2 (RFC 3376 ch 7.2.2).
 */
ZTEST_USER(net_igmp, test_igmp_report_not_suppressed_v3)
{
	struct net_pkt *pkt;

	if (!IS_ENABLED(CONFIG_NET_IPV4_IGMPV3)) {
		ztest_test_skip();
	}

	join_group();

	k_sem_reset(&wait_data);
	report_count = 0;
	expect_v2_report = false;

	/* An IGMPv3 General Query with a Max Resp Time of 500 milliseconds */
	pkt = prepare_igmp_query(net_iface, true, NULL, 5);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	pkt = prepare_igmp_msg(net_iface, &(const struct igmp_msg){
						  .type = NET_IPV4_IGMP_REPORT_V2,
						  .dst = &mcast_addr,
						  .group = &mcast_addr,
					  });
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	k_msleep(1000);
	zassert_equal(report_count, 1, "Expected one report, got %d", report_count);
	zassert_true(is_query_resp_ok, "Query not answered");

	leave_group();
}

/* The unsolicited report of a join is sent Robustness Variable times within
 * the Unsolicited Report Interval (RFC 3376 ch 5.1, RFC 2236 ch 3).
 */
ZTEST_USER(net_igmp, test_igmp_join_retransmit)
{
	/* The Unsolicited Report Interval is 1 second for IGMPv3 and 10 seconds
	 * for IGMPv2.
	 */
	int interval_ms = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3) ? 1000 : 10000;

	report_count = 0;

	join_group_retransmit(true);

	k_msleep(interval_ms + WAIT_TIME);
	zassert_equal(report_count, CONFIG_NET_IPV4_IGMP_ROBUSTNESS, "Expected %d reports, got %d",
		      CONFIG_NET_IPV4_IGMP_ROBUSTNESS, report_count);

	leave_group();
}

/* Leaving a group ends the retransmission of its join report */
ZTEST_USER(net_igmp, test_igmp_leave_stops_retransmit)
{
	int interval_ms = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3) ? 1000 : 10000;
	/* An IGMPv3 leave is a report as well, an IGMPv2 leave is not */
	int expected = IS_ENABLED(CONFIG_NET_IPV4_IGMPV3) ? 2 : 1;

	report_count = 0;

	/* Leave right away, before the first retransmission can be due */
	zassert_ok(net_ipv4_igmp_join(net_iface, &mcast_addr, NULL), "Cannot join");
	leave_group();

	if (report_count != expected) {
		/* Stalled so long that a retransmission was due before the leave */
		ztest_test_skip();
	}

	k_msleep(interval_ms + WAIT_TIME);
	zassert_equal(report_count, expected, "Expected %d reports, got %d", expected,
		      report_count);
}

/* The first query from an older querier switches the mode and cancels the
 * pending retransmission of a join report as well (RFC 3376 ch 7.2.1). The
 * cancellation is checked in the retransmission state, since the response to
 * the query looks the same on the wire as a retransmission would.
 */
ZTEST_USER(net_igmp, test_igmp_first_query_cancels_retransmit)
{
	struct net_if *iface = net_iface;
	struct net_if_mcast_addr *maddr;
	struct net_pkt *pkt;

	if (!IS_ENABLED(CONFIG_NET_IPV4_IGMPV3)) {
		ztest_test_skip();
	}

	zassert_ok(net_ipv4_igmp_join(net_iface, &mcast_addr, NULL), "Cannot join");

	maddr = net_if_ipv4_maddr_lookup(&mcast_addr, &iface);
	zassert_not_null(maddr, "Group not registered");
	if (maddr->igmp_retx_left == 0) {
		/* Stalled so long that the retransmission was already due */
		leave_group();
		ztest_test_skip();
	}

	/* An IGMPv2 querier */
	expect_v2_report = true;
	pkt = prepare_igmp_query(net_iface, false, NULL, 0xff);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	zassert_equal(maddr->igmp_retx_left, 0, "Retransmission not cancelled");
	zassert_true(K_TIMEOUT_EQ(sys_timepoint_timeout(maddr->igmp_retx_timeout), K_FOREVER),
		     "Retransmission still scheduled");

	leave_group();
}

/* After a General Query from an IGMPv2 querier, the host joins and leaves
 * with IGMPv2 messages (RFC 3376 ch 7.2.1).
 */
ZTEST_USER(net_igmp, test_igmp_v2_querier_present)
{
	struct net_pkt *pkt;

	if (!IS_ENABLED(CONFIG_NET_IPV4_IGMPV3)) {
		ztest_test_skip();
	}

	pkt = prepare_igmp_query(net_iface, false, NULL, QUERY_MAX_RSP);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	expect_v2_report = true;
	is_v2_report_sent = false;

	join_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting join event");
	zassert_true(is_v2_report_sent, "Join not reported with IGMPv2");

	k_sem_reset(&wait_data);
	is_v2_leave_sent = false;

	leave_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting leave event");
	zassert_true(is_v2_leave_sent, "Leave not reported with IGMPv2");
}

/* After a query from an IGMPv1 querier, the host joins with IGMPv1 reports
 * and does not send a leave message (RFC 2236 ch 4).
 */
ZTEST_USER(net_igmp, test_igmp_v1_querier_present)
{
	struct net_pkt *pkt;

	pkt = prepare_igmp_query(net_iface, false, NULL, 0);
	zassert_equal(net_ipv4_input(pkt), NET_OK, "Failed to send");

	is_v1_report_sent = false;

	join_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting join event");
	zassert_true(is_v1_report_sent, "Join not reported with IGMPv1");

	is_leave_msg_ok = false;
	is_v2_leave_sent = false;

	leave_group();
	zassert_false(is_leave_msg_ok, "Unexpected leave msg");
	zassert_false(is_v2_leave_sent, "Unexpected leave msg");
}

ZTEST(net_igmp, test_igmp_multi_join)
{
	is_join_msg_ok = false;
	join_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting join event");
	zassert_true(is_join_msg_ok, "Join msg invalid");

	is_join_msg_ok = false;
	join_group();
	k_msleep(THREAD_SLEEP);
	zassert_false(is_join_msg_ok, "Unexpected join msg");

	/* First leave should not send report due to two refs on the address */
	is_leave_msg_ok = false;
	leave_group();
	k_msleep(THREAD_SLEEP);
	zassert_false(is_leave_msg_ok, "Unexpected leave msg");

	is_leave_msg_ok = false;
	leave_group();
	zassert_ok(k_sem_take(&wait_data, K_MSEC(WAIT_TIME)), "Timeout while waiting leave event");
	zassert_true(is_leave_msg_ok, "Leave msg invalid");
}

ZTEST_SUITE(net_igmp, NULL, igmp_setup, igmp_before, NULL, igmp_teardown);
