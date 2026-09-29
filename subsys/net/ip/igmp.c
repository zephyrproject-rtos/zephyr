/** @file
 * @brief IPv4 IGMP related functions
 */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_ipv4, CONFIG_NET_IPV4_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/igmp.h>
#include "net_private.h"
#include "connection.h"
#include "ipv4.h"
#include "net_stats.h"
#include "igmp.h"

/* Timeout for various buffer allocations in this file. */
#define PKT_WAIT_TIME K_MSEC(50)

#define IPV4_OPT_HDR_ROUTER_ALERT_LEN 4

#define IGMPV2_PAYLOAD_MIN_LEN 8
#define IGMPV3_PAYLOAD_MIN_LEN 12

static const struct net_in_addr all_systems = { { { 224, 0, 0, 1 } } };
#if defined(CONFIG_NET_IPV4_IGMPV3)
static const struct net_in_addr igmp_multicast_addr = { { { 224, 0, 0, 22 } } };
#else
static const struct net_in_addr all_routers = { { { 224, 0, 0, 2 } } };
#endif

#define dbg_addr(action, pkt_str, src, dst)				\
	NET_DBG("%s %s from %s to %s", action, pkt_str,			\
		net_sprint_ipv4_addr(src),			\
		net_sprint_ipv4_addr(dst));

#define dbg_addr_recv(pkt_str, src, dst) \
	dbg_addr("Received", pkt_str, src, dst)

enum igmp_version {
	IGMPV1,
	IGMPV2,
	IGMPV3,
};

/* Groups the interface reports membership of. The all systems group
 * 224.0.0.1 is joined on every interface at init time and is never reported.
 */
static bool igmp_is_reported(const struct net_if_mcast_addr *mcast)
{
	return mcast->is_used && mcast->is_joined &&
	       !net_ipv4_addr_cmp(&mcast->address.in_addr, &all_systems);
}

static int igmp_v2_create(struct net_pkt *pkt, const struct net_in_addr *addr,
			  uint8_t type)
{
	NET_PKT_DATA_ACCESS_DEFINE(igmp_access,
				   struct net_ipv4_igmp_v2_report);
	struct net_ipv4_igmp_v2_report *igmp;
	int ret;
	uint16_t chksum = 0;

	igmp = (struct net_ipv4_igmp_v2_report *)
				net_pkt_get_data(pkt, &igmp_access);
	if (!igmp) {
		return -ENOBUFS;
	}

	igmp->type = type;
	igmp->max_rsp = 0U;
	net_ipaddr_copy(UNALIGNED_MEMBER_ADDR(igmp, address), addr);
	igmp->chksum = 0;

	if (net_pkt_set_data(pkt, &igmp_access)) {
		return -ENOBUFS;
	}

	ret = net_calc_chksum_igmp(pkt, &chksum);
	if (ret < 0) {
		return ret;
	}

	igmp->chksum = chksum;

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, offsetof(struct net_ipv4_igmp_v2_report, chksum)) < 0) {
		return -ENOBUFS;
	}
	if (net_pkt_write(pkt, &igmp->chksum, sizeof(igmp->chksum))) {
		return -ENOBUFS;
	}

	return 0;
}

#if defined(CONFIG_NET_IPV4_IGMPV3)
/* Record type of a group in a report. A state-change report carries a
 * filter-mode-change record derived from the current filter mode (RFC 3376
 * ch 4.2.12), a query response carries the current filter mode itself.
 */
static uint8_t igmp_v3_record_type(const struct net_if_mcast_addr *mcast, bool state_change)
{
	if (!state_change) {
		return mcast->record_type;
	}

	return mcast->record_type == IGMPV3_MODE_IS_INCLUDE ? IGMPV3_CHANGE_TO_INCLUDE_MODE
							    : IGMPV3_CHANGE_TO_EXCLUDE_MODE;
}

static int igmp_v3_create(struct net_pkt *pkt, uint8_t type, struct net_if_mcast_addr mcast[],
			  size_t mcast_len, bool state_change)
{
	NET_PKT_DATA_ACCESS_DEFINE(igmp_access, struct net_ipv4_igmp_v3_report);
	NET_PKT_DATA_ACCESS_DEFINE(group_record_access, struct net_ipv4_igmp_v3_group_record);
	struct net_ipv4_igmp_v3_report *igmp;
	struct net_ipv4_igmp_v3_group_record *group_record;
	int ret;
	uint16_t chksum = 0;
	uint16_t group_count = 0;

	igmp = (struct net_ipv4_igmp_v3_report *)net_pkt_get_data(pkt, &igmp_access);
	if (!igmp) {
		return -ENOBUFS;
	}

	for (int i = 0; i < mcast_len; i++) {
		if (!igmp_is_reported(&mcast[i])) {
			continue;
		}

		group_count++;
	}

	igmp->type = type;
	igmp->reserved_1 = 0U;
	igmp->reserved_2 = 0U;
	igmp->groups_len = net_htons(group_count);
	/* Setting initial value of chksum to 0 to calculate chksum as described in RFC 3376
	 * ch 4.1.2
	 */
	igmp->chksum = 0;

	if (net_pkt_set_data(pkt, &igmp_access)) {
		return -ENOBUFS;
	}

	for (int i = 0; i < mcast_len; i++) {
		if (!igmp_is_reported(&mcast[i])) {
			continue;
		}

		group_record = (struct net_ipv4_igmp_v3_group_record *)net_pkt_get_data(
			pkt, &group_record_access);
		if (!group_record) {
			return -ENOBUFS;
		}

		group_record->type = igmp_v3_record_type(&mcast[i], state_change);
		group_record->aux_len = 0U;
		net_ipaddr_copy(&group_record->address, &mcast[i].address.in_addr);
		group_record->sources_len = net_htons(mcast[i].sources_len);

		if (net_pkt_set_data(pkt, &group_record_access)) {
			return -ENOBUFS;
		}

		for (int j = 0; j < mcast[i].sources_len; j++) {
			if (net_pkt_write(pkt, &mcast[i].sources[j].in_addr.s_addr,
					  sizeof(mcast[i].sources[j].in_addr.s_addr))) {
				return -ENOBUFS;
			}
		}
	}

	ret = net_calc_chksum_igmp(pkt, &chksum);
	if (ret < 0) {
		return ret;
	}

	igmp->chksum = chksum;

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, offsetof(struct net_ipv4_igmp_v3_report, chksum)) < 0) {
		return -ENOBUFS;
	}

	if (net_pkt_write(pkt, &igmp->chksum, sizeof(igmp->chksum))) {
		return -ENOBUFS;
	}

	return 0;
}
#endif

static int igmp_v2_create_packet(struct net_pkt *pkt, const struct net_in_addr *dst,
				 const struct net_in_addr *group, uint8_t type)
{
	const uint32_t router_alert = 0x94040000; /* RFC 2213 ch 2.1 */
	int ret;

	/* TTL set to 1, RFC 3376 ch 2 */
	net_pkt_set_ipv4_ttl(pkt, 1U);

	ret = net_ipv4_create_full(pkt,
				   net_if_ipv4_select_src_addr(
							net_pkt_iface(pkt),
							dst),
				   dst,
				   0U,
				   0U,
				   0U,
				   0U);
	if (ret) {
		return -ENOBUFS;
	}

	/* Add router alert option, RFC 3376 ch 2 */
	if (net_pkt_write_be32(pkt, router_alert)) {
		return -ENOBUFS;
	}

	net_pkt_set_ipv4_opts_len(pkt, IPV4_OPT_HDR_ROUTER_ALERT_LEN);

	return igmp_v2_create(pkt, group, type);
}

#if defined(CONFIG_NET_IPV4_IGMPV3)
static int igmp_v3_create_packet(struct net_pkt *pkt, const struct net_in_addr *dst,
				 struct net_if_mcast_addr mcast[], size_t mcast_len, uint8_t type,
				 bool state_change)
{
	const uint32_t router_alert = 0x94040000; /* RFC 2213 ch 2.1 */
	int ret;

	/* TTL set to 1, RFC 3376 ch 2 */
	net_pkt_set_ipv4_ttl(pkt, 1U);

	ret = net_ipv4_create_full(pkt, net_if_ipv4_select_src_addr(net_pkt_iface(pkt), dst), dst,
				   0U, 0U, 0U, 0U);
	if (ret) {
		return -ENOBUFS;
	}

	/* Add router alert option, RFC 3376 ch 2 */
	if (net_pkt_write_be32(pkt, router_alert)) {
		return -ENOBUFS;
	}

	net_pkt_set_ipv4_opts_len(pkt, IPV4_OPT_HDR_ROUTER_ALERT_LEN);

	return igmp_v3_create(pkt, type, mcast, mcast_len, state_change);
}
#endif

static int igmp_send(struct net_pkt *pkt)
{
	__maybe_unused struct net_if *iface = net_pkt_iface(pkt);
	int ret;

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, NET_IPPROTO_IGMP);

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_stats_update_ipv4_igmp_drop(iface);
		return ret;
	}

	net_stats_update_ipv4_igmp_sent(iface);

	return 0;
}

static int igmp_send_v2(struct net_if *iface, const struct net_in_addr *dst,
			const struct net_in_addr *group, uint8_t type)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(
		iface, IPV4_OPT_HDR_ROUTER_ALERT_LEN + sizeof(struct net_ipv4_igmp_v2_report),
		NET_AF_INET, NET_IPPROTO_IGMP, PKT_WAIT_TIME);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = igmp_v2_create_packet(pkt, dst, group, type);
	if (ret < 0) {
		goto drop;
	}

	ret = igmp_send(pkt);
	if (ret < 0) {
		goto drop;
	}

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

/* Send a v2 Membership Report for the given group, or for every joined
 * group if group is NULL. Reports are sent to the group address, RFC 2236
 * ch 9.
 */
static int igmp_send_v2_response(struct net_if *iface, const struct net_in_addr *group)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	int ret = 0;

	if (!ipv4) {
		return -ENOENT;
	}

	ARRAY_FOR_EACH(ipv4->mcast, i) {
		if (!igmp_is_reported(&ipv4->mcast[i])) {
			continue;
		}

		if (group != NULL && !net_ipv4_addr_cmp(&ipv4->mcast[i].address.in_addr, group)) {
			continue;
		}

		ret = igmp_send_v2(iface, &ipv4->mcast[i].address.in_addr,
				   &ipv4->mcast[i].address.in_addr, NET_IPV4_IGMP_REPORT_V2);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

#if defined(CONFIG_NET_IPV4_IGMPV3)
/* Send one v3 Membership Report with a group record for every reported
 * group in mcast. The report is sent to 224.0.0.22, RFC 3376 ch 4.2.14.
 */
static int igmp_send_v3(struct net_if *iface, struct net_if_mcast_addr mcast[], size_t mcast_len,
			bool state_change)
{
	struct net_pkt *pkt;
	size_t group_count = 0;
	size_t source_count = 0;
	int ret;

	for (size_t i = 0; i < mcast_len; i++) {
		if (!igmp_is_reported(&mcast[i])) {
			continue;
		}

		group_count++;
		source_count += mcast[i].sources_len;
	}

	if (group_count == 0) {
		return -ESRCH;
	}

	pkt = net_pkt_alloc_with_buffer(
		iface,
		IPV4_OPT_HDR_ROUTER_ALERT_LEN + sizeof(struct net_ipv4_igmp_v3_report) +
			sizeof(struct net_ipv4_igmp_v3_group_record) * group_count +
			sizeof(struct net_in_addr) * source_count,
		NET_AF_INET, NET_IPPROTO_IGMP, PKT_WAIT_TIME);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = igmp_v3_create_packet(pkt, &igmp_multicast_addr, mcast, mcast_len,
				    NET_IPV4_IGMP_REPORT_V3, state_change);
	if (ret < 0) {
		goto drop;
	}

	ret = igmp_send(pkt);
	if (ret < 0) {
		goto drop;
	}

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}
#endif

#if defined(CONFIG_NET_IPV4_IGMPV3)
static int igmp_send_v3_response(struct net_if *iface, struct net_if_mcast_addr *group)
{
	if (group != NULL) {
		return igmp_send_v3(iface, group, 1, false);
	}

	return igmp_send_v3(iface, iface->config.ip.ipv4->mcast, NET_IF_MAX_IPV4_MADDR, false);
}
#else
static int igmp_send_v3_response(struct net_if *iface, struct net_if_mcast_addr *group)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(group);

	return -ENOTSUP;
}
#endif

/* Respond to a Membership Query with the current state of the given group,
 * or of every joined group for a General Query (group is NULL).
 */
static void igmp_query_respond(struct net_if *iface, struct net_if_mcast_addr *group,
			       enum igmp_version version)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4_IGMPV3) && version == IGMPV3) {
		ret = igmp_send_v3_response(iface, group);
	} else {
		ret = igmp_send_v2_response(iface, group != NULL ? &group->address.in_addr : NULL);
	}

	if (ret < 0 && ret != -ESRCH) {
		NET_DBG("Cannot send IGMP report (%d)", ret);
	}
}

enum net_verdict net_ipv4_igmp_input(struct net_pkt *pkt, struct net_ipv4_hdr *ip_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(igmp_access, struct net_ipv4_igmp_v2_query);
	struct net_if *iface = net_pkt_iface(pkt);
	struct net_if_mcast_addr *maddr = NULL;
	struct net_ipv4_igmp_v2_query *igmp_hdr;
	struct net_in_addr group;
	enum igmp_version version;
	uint16_t chksum = 0;
	int igmp_len;
	int ret;

	if (iface->config.ip.ipv4 == NULL) {
		NET_DBG("DROP: no IPv4 configuration");
		return NET_DROP;
	}

	igmp_len = pkt->buffer->len - (net_pkt_ip_hdr_len(pkt) + net_pkt_ipv4_opts_len(pkt));

	/* The version of a query follows from its length, RFC 3376 ch 7.1 */
	if (igmp_len == IGMPV2_PAYLOAD_MIN_LEN) {
		version = IGMPV2;
	} else if (igmp_len >= IGMPV3_PAYLOAD_MIN_LEN) {
		version = IGMPV3;
	} else {
		NET_DBG("DROP: unsupported payload length");
		return NET_DROP;
	}

	/* The first eight octets are common to all versions */
	igmp_hdr = (struct net_ipv4_igmp_v2_query *)net_pkt_get_data(pkt, &igmp_access);
	if (igmp_hdr == NULL) {
		NET_DBG("DROP: NULL %s header", "IGMP");
		return NET_DROP;
	}

	ret = net_calc_chksum_igmp(pkt, &chksum);
	if (ret < 0 || chksum != 0U) {
		NET_DBG("DROP: Invalid checksum");
		goto drop;
	}

	ret = net_pkt_acknowledge_data(pkt, &igmp_access);
	if (ret < 0) {
		NET_DBG("DROP: cannot acknowledge data");
		goto drop;
	}

	dbg_addr_recv("Internet Group Management Protocol", &ip_hdr->src, &ip_hdr->dst);

	net_stats_update_ipv4_igmp_recv(iface);

	if (igmp_hdr->type != NET_IPV4_IGMP_QUERY) {
		NET_DBG("Ignoring IGMP message type 0x%02x", igmp_hdr->type);
		goto out;
	}

	net_ipv4_addr_copy_raw(group.s4_addr, igmp_hdr->address.s4_addr);

	if (net_ipv4_is_addr_unspecified(&group)) {
		/* General Query, sent to the all systems group (RFC 3376 ch 4.1.12) */
		if (!net_ipv4_addr_cmp_raw(ip_hdr->dst, (uint8_t *)&all_systems)) {
			NET_DBG("DROP: Invalid dst address");
			goto drop;
		}
	} else {
		/* Group-Specific Query, only a member of the group answers */
		maddr = net_if_ipv4_maddr_lookup(&group, &iface);
		if (maddr == NULL || !net_if_ipv4_maddr_is_joined(maddr)) {
			NET_DBG("Ignoring query for group %s", net_sprint_ipv4_addr(&group));
			goto out;
		}
	}

	igmp_query_respond(iface, maddr, version);

out:
	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_ipv4_igmp_drop(iface);

	return NET_DROP;
}

int net_ipv4_igmp_rejoin(struct net_if *iface, struct net_if_mcast_addr *addr)
{
	int ret;

	/* Only joined groups are reported, so join before sending the report.
	 * The all systems group is never reported, see net_ipv4_igmp_init().
	 */
	net_if_ipv4_maddr_join(iface, addr);

	if (net_if_is_offloaded(iface) || net_ipv4_addr_cmp(&addr->address.in_addr, &all_systems)) {
		goto out;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	ret = igmp_send_v3(iface, addr, 1, true);
#else
	ret = igmp_send_v2(iface, &addr->address.in_addr, &addr->address.in_addr,
			   NET_IPV4_IGMP_REPORT_V2);
#endif
	if (ret < 0) {
		net_if_ipv4_maddr_leave(iface, addr);
		return ret;
	}

out:
	net_if_mcast_monitor(iface, &addr->address, true);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_JOIN, iface, &addr->address.in_addr,
					sizeof(struct net_in_addr));

	return 0;
}

int net_ipv4_igmp_join(struct net_if *iface, const struct net_in_addr *addr,
		       const struct igmp_param *param)
{
	struct net_if_mcast_addr *maddr;
	int ret = 0;

#if defined(CONFIG_NET_IPV4_IGMPV3)
	maddr = net_if_ipv4_maddr_lookup(addr, &iface);
	if (maddr != NULL && maddr->sources_len > 0) {
		/* For IGMPv3 specifically, if the address with non-empty
		 * include/exclude list was registered, return an error as
		 * registering lists from different sources is currently
		 * not supported.
		 */
		return -EEXIST;
	}

	if (param != NULL) {
		if (param->sources_len > CONFIG_NET_IF_MCAST_IPV4_SOURCE_COUNT) {
			return -ENOMEM;
		}
	}
#endif

	maddr = net_if_ipv4_maddr_add(iface, addr);
	if (maddr == NULL) {
		return -ENOMEM;
	}

	if (net_if_ipv4_maddr_is_joined(maddr)) {
#if defined(CONFIG_NET_IPV4_IGMPV3)
		if (param == NULL) {
			return 0;
		}
#else
		return 0;
#endif
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	if (param != NULL) {
		maddr->record_type =
			param->include ? IGMPV3_MODE_IS_INCLUDE : IGMPV3_MODE_IS_EXCLUDE;
		maddr->sources_len = param->sources_len;
		for (int i = 0; i < param->sources_len; i++) {
			net_ipaddr_copy(&maddr->sources[i].in_addr.s_addr,
					&param->source_list[i].s_addr);
		}
	} else {
		maddr->record_type = IGMPV3_MODE_IS_EXCLUDE;
	}
#endif

	net_if_ipv4_maddr_join(iface, maddr);

	if (net_if_is_offloaded(iface)) {
		goto out;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	ret = igmp_send_v3(iface, maddr, 1, true);
#else
	ret = igmp_send_v2(iface, addr, addr, NET_IPV4_IGMP_REPORT_V2);
#endif
	if (ret < 0) {
		net_if_ipv4_maddr_leave(iface, maddr);

		/* -ENETDOWN Indicate that network interface is down - this may
		 * happen and should not be considered fatal, address group will
		 * be joined when the interface goes up. Any other error should
		 * be considered fatal though and address should be cleaned up.
		 */
		if (ret != -ENETDOWN) {
			net_if_ipv4_maddr_rm(iface, addr);
		}

		return ret;
	}

out:
	net_if_mcast_monitor(iface, &maddr->address, true);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_JOIN, iface, &maddr->address.in_addr,
					sizeof(struct net_in_addr));

	return ret;
}

int net_ipv4_igmp_leave(struct net_if *iface, const struct net_in_addr *addr)
{
	struct net_if_mcast_addr removed_addr;
	struct net_if_mcast_addr *maddr;
	int ret = 0;

	maddr = net_if_ipv4_maddr_lookup(addr, &iface);
	if (maddr == NULL) {
		return -ENOENT;
	}

	removed_addr = *maddr;
	if (!net_if_ipv4_maddr_rm(iface, addr)) {
		/* Address still in use */
		return 0;
	}

	if (net_if_is_offloaded(iface)) {
		goto out;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	/* Leaving is a change to INCLUDE mode with an empty source list,
	 * RFC 3376 ch 5.1.
	 */
	removed_addr.record_type = IGMPV3_MODE_IS_INCLUDE;
	removed_addr.sources_len = 0;

	ret = igmp_send_v3(iface, &removed_addr, 1, true);
#else
	/* A Leave Group message is sent to the all routers group, RFC 2236 ch 3 */
	ret = igmp_send_v2(iface, &all_routers, addr, NET_IPV4_IGMP_LEAVE);
#endif
	if (ret < 0) {
		return ret;
	}

out:
	net_if_mcast_monitor(iface, &removed_addr.address, false);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_LEAVE, iface,
					&removed_addr.address.in_addr,
					sizeof(struct net_in_addr));
	return ret;
}

void net_ipv4_igmp_send_leave(struct net_if *iface, const struct net_if_mcast_addr *addr)
{
	if (net_if_is_offloaded(iface)) {
		goto out;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	struct net_if_mcast_addr removed_addr = *addr;

	removed_addr.record_type = IGMPV3_MODE_IS_INCLUDE;
	removed_addr.sources_len = 0;

	igmp_send_v3(iface, &removed_addr, 1, true);
#else
	igmp_send_v2(iface, &all_routers, &addr->address.in_addr, NET_IPV4_IGMP_LEAVE);
#endif
out:
	net_if_mcast_monitor(iface, &addr->address, false);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_LEAVE, iface,
					&addr->address.in_addr,
					sizeof(struct net_in_addr));
}

void net_ipv4_igmp_init(struct net_if *iface)
{
	struct net_if_mcast_addr *maddr;

	/* Ensure multicast addresses are available */
	if (CONFIG_NET_IF_MCAST_IPV4_ADDR_COUNT < 1) {
		return;
	}

	/* This code adds the IGMP all systems 224.0.0.1 multicast address
	 * to the list of multicast addresses of the given interface.
	 * The address is marked as joined. However, an IGMP membership
	 * report is not generated for this address. Populating this
	 * address in the list of multicast addresses of the interface
	 * and marking it as joined is helpful for multicast hash filter
	 * implementations that need a list of multicast addresses it needs
	 * to add to the multicast hash filter after a multicast address
	 * has been removed from the membership list.
	 */
	maddr = net_if_ipv4_maddr_lookup(&all_systems, &iface);
	if (maddr && net_if_ipv4_maddr_is_joined(maddr)) {
		return;
	}

	if (!maddr) {
		maddr = net_if_ipv4_maddr_add(iface, &all_systems);
		if (!maddr) {
			return;
		}
	}

	net_if_ipv4_maddr_join(iface, maddr);

	net_if_mcast_monitor(iface, &maddr->address, true);
}
