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
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/igmp.h>
#include "net_private.h"
#include "connection.h"
#include "ipv4.h"
#include "net_stats.h"

/* Timeout for various buffer allocations in this file. */
#define PKT_WAIT_TIME K_MSEC(50)

#define IPV4_OPT_HDR_ROUTER_ALERT_LEN 4

#define IGMPV3_MODE_IS_INCLUDE        0x01
#define IGMPV3_MODE_IS_EXCLUDE        0x02
#define IGMPV3_CHANGE_TO_INCLUDE_MODE 0x03
#define IGMPV3_CHANGE_TO_EXCLUDE_MODE 0x04
#define IGMPV3_ALLOW_NEW_SOURCES      0x05
#define IGMPV3_BLOCK_OLD_SOURCES      0x06

static const struct in_addr all_systems = { { { 224, 0, 0, 1 } } };
#if defined(CONFIG_NET_IPV4_IGMPV3)
static const struct in_addr igmp_multicast_addr = { { { 224, 0, 0, 22 } } };
#else
static const struct in_addr all_routers = { { { 224, 0, 0, 2 } } };
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

static int igmp_v2_create(struct net_pkt *pkt, const struct in_addr *addr,
			  uint8_t type)
{
	NET_PKT_DATA_ACCESS_DEFINE(igmp_access,
				   struct net_ipv4_igmp_v2_report);
	struct net_ipv4_igmp_v2_report *igmp;

	igmp = (struct net_ipv4_igmp_v2_report *)
				net_pkt_get_data(pkt, &igmp_access);
	if (!igmp) {
		return -ENOBUFS;
	}

	igmp->type = type;
	igmp->max_rsp = 0U;
	net_ipaddr_copy(&igmp->address, addr);
	igmp->chksum = 0;

	if (net_pkt_set_data(pkt, &igmp_access)) {
		return -ENOBUFS;
	}

	igmp->chksum = net_calc_chksum_igmp(pkt);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	net_pkt_skip(pkt, offsetof(struct net_ipv4_igmp_v2_report, chksum));
	if (net_pkt_write(pkt, &igmp->chksum, sizeof(igmp->chksum))) {
		return -ENOBUFS;
	}

	return 0;
}

#if defined(CONFIG_NET_IPV4_IGMPV3)
static int igmp_v3_create(struct net_pkt *pkt, uint8_t type, struct net_if_mcast_addr mcast[],
			  size_t mcast_len)
{
	NET_PKT_DATA_ACCESS_DEFINE(igmp_access, struct net_ipv4_igmp_v3_report);
	NET_PKT_DATA_ACCESS_DEFINE(group_record_access, struct net_ipv4_igmp_v3_group_record);
	struct net_ipv4_igmp_v3_report *igmp;
	struct net_ipv4_igmp_v3_group_record *group_record;

	uint16_t group_count = 0;

	igmp = (struct net_ipv4_igmp_v3_report *)net_pkt_get_data(pkt, &igmp_access);
	if (!igmp) {
		return -ENOBUFS;
	}

	for (int i = 0; i < mcast_len; i++) {
		/* We don't need to send an IGMP membership report to the IGMP
		 * all systems multicast address of 224.0.0.1 so skip over it.
		 * Since the IGMP all systems multicast address is marked as
		 * used and joined during init time, we have to check this
		 * address separately to skip over it.
		 */
		if (!mcast[i].is_used || !mcast[i].is_joined ||
		    net_ipv4_addr_cmp_raw((uint8_t *)&mcast[i].address.in_addr,
					  (uint8_t *)&all_systems)) {
			continue;
		}

		group_count++;
	}

	igmp->type = type;
	igmp->reserved_1 = 0U;
	igmp->reserved_2 = 0U;
	igmp->groups_len = htons(group_count);
	/* Setting initial value of chksum to 0 to calculate chksum as described in RFC 3376
	 * ch 4.1.2
	 */
	igmp->chksum = 0;

	if (net_pkt_set_data(pkt, &igmp_access)) {
		return -ENOBUFS;
	}

	for (int i = 0; i < mcast_len; i++) {
		/* We don't need to send an IGMP membership report to the IGMP
		 * all systems multicast address of 224.0.0.1 so skip over it.
		 * Since the IGMP all systems multicast address is marked as
		 * used and joined during init time, we have to check this
		 * address separately to skip over it.
		 */
		if (!mcast[i].is_used || !mcast[i].is_joined ||
		    net_ipv4_addr_cmp_raw((uint8_t *)&mcast[i].address.in_addr,
					  (uint8_t *)&all_systems)) {
			continue;
		}

		group_record = (struct net_ipv4_igmp_v3_group_record *)net_pkt_get_data(
			pkt, &group_record_access);
		if (!group_record) {
			return -ENOBUFS;
		}

		group_record->type = mcast[i].record_type;
		group_record->aux_len = 0U;
		net_ipaddr_copy(&group_record->address, &mcast[i].address.in_addr);
		group_record->sources_len = htons(mcast[i].sources_len);

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

	igmp->chksum = net_calc_chksum_igmp(pkt);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	net_pkt_skip(pkt, offsetof(struct net_ipv4_igmp_v3_report, chksum));
	if (net_pkt_write(pkt, &igmp->chksum, sizeof(igmp->chksum))) {
		return -ENOBUFS;
	}

	return 0;
}
#endif

static int igmp_v2_create_packet(struct net_pkt *pkt, const struct in_addr *dst,
				 const struct in_addr *group, uint8_t type)
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
static int igmp_v3_create_packet(struct net_pkt *pkt, const struct in_addr *dst,
				 struct net_if_mcast_addr mcast[], size_t mcast_len, uint8_t type)
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

	return igmp_v3_create(pkt, type, mcast, mcast_len);
}
#endif

static int igmp_send(struct net_pkt *pkt)
{
	int ret;

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, IPPROTO_IGMP);

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_stats_update_ipv4_igmp_drop(net_pkt_iface(pkt));
		return ret;
	}

	net_stats_update_ipv4_igmp_sent(net_pkt_iface(pkt));

	return 0;
}

static int send_igmp_report(struct net_if *iface,
			    struct net_ipv4_igmp_v2_query *igmp_v2_hdr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	struct net_pkt *pkt = NULL;
	int i, count = 0;
	int ret = 0;

	if (!ipv4) {
		return -ENOENT;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		/* We don't need to send an IGMP membership report to the IGMP
		 * all systems multicast address of 224.0.0.1 so skip over it.
		 * Since the IGMP all systems multicast address is marked as
		 * used and joined during init time, we have to check this
		 * address separately to skip over it.
		 */
		if (!ipv4->mcast[i].is_used || !ipv4->mcast[i].is_joined ||
			net_ipv4_addr_cmp_raw((uint8_t *)&ipv4->mcast[i].address.in_addr,
					(uint8_t *)&all_systems)) {
			continue;
		}

		count++;
	}

	if (count == 0) {
		return -ESRCH;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		/* We don't need to send an IGMP membership report to the IGMP
		 * all systems multicast address of 224.0.0.1 so skip over it.
		 * Since the IGMP all systems multicast address is marked as
		 * used and joined during init time, we have to check this
		 * address separately to skip over it.
		 */
		if (!ipv4->mcast[i].is_used || !ipv4->mcast[i].is_joined ||
			net_ipv4_addr_cmp_raw((uint8_t *)&ipv4->mcast[i].address.in_addr,
					(uint8_t *)&all_systems)) {
			continue;
		}

		pkt = net_pkt_alloc_with_buffer(iface,
					IPV4_OPT_HDR_ROUTER_ALERT_LEN +
					sizeof(struct net_ipv4_igmp_v2_report),
					AF_INET, IPPROTO_IGMP,
					PKT_WAIT_TIME);
		if (!pkt) {
			return -ENOMEM;
		}

		/* Send the IGMP V2 membership report to the group multicast
		 * address, as per RFC 2236 Section 9.
		 */
		ret = igmp_v2_create_packet(pkt, &ipv4->mcast[i].address.in_addr,
					    &ipv4->mcast[i].address.in_addr,
					    NET_IPV4_IGMP_REPORT_V2);
		if (ret < 0) {
			goto drop;
		}

		ret = igmp_send(pkt);
		if (ret < 0) {
			goto drop;
		}

		/* So that we do not free the data while it is being sent */
		pkt = NULL;
	}

drop:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	return ret;
}

#if defined(CONFIG_NET_IPV4_IGMPV3)
static int send_igmp_v3_report(struct net_if *iface, struct net_ipv4_igmp_v3_query *igmp_v3_hdr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	struct net_pkt *pkt = NULL;
	int i, group_count = 0, source_count = 0;
	int ret = 0;

	if (!ipv4) {
		return -ENOENT;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		/* We don't need to send an IGMP membership report to the IGMP
		 * all systems multicast address of 224.0.0.1 so skip over it.
		 * Since the IGMP all systems multicast address is marked as
		 * used and joined during init time, we have to check this
		 * address separately to skip over it.
		 */
		if (!ipv4->mcast[i].is_used || !ipv4->mcast[i].is_joined ||
		    net_ipv4_addr_cmp_raw((uint8_t *)&ipv4->mcast[i].address.in_addr,
					  (uint8_t *)&all_systems)) {
			continue;
		}

		group_count++;
		source_count += ipv4->mcast[i].sources_len;
	}

	if (group_count == 0) {
		return -ESRCH;
	}

	pkt = net_pkt_alloc_with_buffer(
		iface,
		IPV4_OPT_HDR_ROUTER_ALERT_LEN + sizeof(struct net_ipv4_igmp_v3_report) +
			sizeof(struct net_ipv4_igmp_v3_group_record) * group_count +
			sizeof(struct in_addr) * source_count,
		AF_INET, IPPROTO_IGMP, PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	/* Send the IGMP V3 membership report to the igmp multicast
	 * address, as per RFC 3376 Section 4.2.14.
	 */

	ret = igmp_v3_create_packet(pkt, &igmp_multicast_addr, ipv4->mcast, NET_IF_MAX_IPV4_MADDR,
				    NET_IPV4_IGMP_REPORT_V3);
	if (ret < 0) {
		goto drop;
	}

	ret = igmp_send(pkt);
	if (ret < 0) {
		goto drop;
	}

	/* So that we do not free the data while it is being sent */
	pkt = NULL;

drop:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	return ret;
}
#endif

enum net_verdict net_ipv4_igmp_input(struct net_pkt *pkt, struct net_ipv4_hdr *ip_hdr)
{
#if defined(CONFIG_NET_IPV4_IGMPV3)
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(igmpv3_access, struct net_ipv4_igmp_v3_query);
	struct net_ipv4_igmp_v3_query *igmpv3_hdr;
#endif
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(igmpv2_access, struct net_ipv4_igmp_v2_query);
	struct net_ipv4_igmp_v2_query *igmpv2_hdr;
	enum igmp_version version;
	int ret;
	int igmp_buf_len = pkt->buffer->len - net_pkt_ip_hdr_len(pkt);

	/* Detect IGMP type (RFC 3376 ch 7.1) */
	if (igmp_buf_len == 8) {
		/* IGMPv1/2 detected */
		version = IGMPV2;
	} else if (igmp_buf_len >= 12) {
		/* IGMPv3 detected */
		version = IGMPV3;
#if !defined(CONFIG_NET_IPV4_IGMPV3)
		NET_DBG("DROP: %sv3 msg received but %s support is disabled", "IGMP", "IGMP");
		return NET_DROP;
#endif
	} else {
		NET_DBG("DROP: unsupported payload length");
		return NET_DROP;
	}

	if (!net_ipv4_addr_cmp_raw(ip_hdr->dst, (uint8_t *)&all_systems)) {
		NET_DBG("DROP: Invalid dst address");
		return NET_DROP;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	if (version == IGMPV3) {
		igmpv3_hdr = (struct net_ipv4_igmp_v3_query *)net_pkt_get_data(pkt, &igmpv3_access);
		if (!igmpv3_hdr) {
			NET_DBG("DROP: NULL %sv3 header", "IGMP");
			return NET_DROP;
		}
	} else {
#endif
		igmpv2_hdr = (struct net_ipv4_igmp_v2_query *)net_pkt_get_data(pkt, &igmpv2_access);
		if (!igmpv2_hdr) {
			NET_DBG("DROP: NULL %s header", "IGMP");
			return NET_DROP;
		}
#if defined(CONFIG_NET_IPV4_IGMPV3)
	}
#endif

	ret = net_calc_chksum_igmp(pkt);
	if (ret != 0u) {
		NET_DBG("DROP: Invalid checksum");
		goto drop;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	if (version == IGMPV3) {
		net_pkt_acknowledge_data(pkt, &igmpv3_access);
	} else {
#endif
		net_pkt_acknowledge_data(pkt, &igmpv2_access);
#if defined(CONFIG_NET_IPV4_IGMPV3)
	}
#endif

	dbg_addr_recv("Internet Group Management Protocol", &ip_hdr->src, &ip_hdr->dst);

	net_stats_update_ipv4_igmp_recv(net_pkt_iface(pkt));

#if defined(CONFIG_NET_IPV4_IGMPV3)
	if (version == IGMPV3) {
		(void)send_igmp_v3_report(net_pkt_iface(pkt), igmpv3_hdr);
	} else {
#endif
		(void)send_igmp_report(net_pkt_iface(pkt), igmpv2_hdr);
#if defined(CONFIG_NET_IPV4_IGMPV3)
	}
#endif

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_ipv4_igmp_drop(net_pkt_iface(pkt));

	return NET_DROP;
}

#if !defined(CONFIG_NET_IPV4_IGMPV3)
static int igmp_send_generic(struct net_if *iface,
			     const struct in_addr *addr,
			     bool join)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface,
					IPV4_OPT_HDR_ROUTER_ALERT_LEN +
					sizeof(struct net_ipv4_igmp_v2_report),
					AF_INET, IPPROTO_IGMP,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	/* Send the IGMP V2 membership report to the group multicast
	 * address, as per RFC 2236 Section 9. The leave report
	 * should be sent to the ALL ROUTERS multicast address (224.0.0.2)
	 */
	ret = igmp_v2_create_packet(pkt,
				join ? addr : &all_routers, addr,
				join ? NET_IPV4_IGMP_REPORT_V2 : NET_IPV4_IGMP_LEAVE);
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
static int igmpv3_send_generic(struct net_if *iface, struct net_if_mcast_addr *mcast)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface,
					IPV4_OPT_HDR_ROUTER_ALERT_LEN +
						sizeof(struct net_ipv4_igmp_v3_report) +
						sizeof(struct net_ipv4_igmp_v3_group_record) +
						sizeof(struct in_addr) * mcast->sources_len,
					AF_INET, IPPROTO_IGMP, PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	ret = igmp_v3_create_packet(pkt, &igmp_multicast_addr, mcast, 1, NET_IPV4_IGMP_REPORT_V3);
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

int net_ipv4_igmp_join(struct net_if *iface, const struct in_addr *addr,
		       const struct igmp_param *param)
{
	struct net_if_mcast_addr *maddr;
	int ret;

#if defined(CONFIG_NET_IPV4_IGMPV3)
	if (param != NULL) {
		if (param->sources_len > CONFIG_NET_IF_MCAST_IPV4_SOURCE_COUNT) {
			return -ENOMEM;
		}
	}
#endif

	maddr = net_if_ipv4_maddr_lookup(addr, &iface);
	if (maddr && net_if_ipv4_maddr_is_joined(maddr)) {
		return -EALREADY;
	}

	if (!maddr) {
		maddr = net_if_ipv4_maddr_add(iface, addr);
		if (!maddr) {
			return -ENOMEM;
		}
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	if (param != NULL) {
		maddr->record_type = param->include ? IGMPV3_CHANGE_TO_INCLUDE_MODE
						    : IGMPV3_CHANGE_TO_EXCLUDE_MODE;
		maddr->sources_len = param->sources_len;
		for (int i = 0; i < param->sources_len; i++) {
			net_ipaddr_copy(&maddr->sources[i].in_addr.s_addr,
					&param->source_list[i].s_addr);
		}
	} else {
		maddr->record_type = IGMPV3_CHANGE_TO_EXCLUDE_MODE;
	}
#endif

	net_if_ipv4_maddr_join(iface, maddr);

#if defined(CONFIG_NET_IPV4_IGMPV3)
	ret = igmpv3_send_generic(iface, maddr);
#else
	ret = igmp_send_generic(iface, addr, true);
#endif
	if (ret < 0) {
		net_if_ipv4_maddr_leave(iface, maddr);
		return ret;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	if (param != NULL) {
		/* Updating the record type for further use after sending the join report */
		maddr->record_type =
			param->include ? IGMPV3_MODE_IS_INCLUDE : IGMPV3_MODE_IS_EXCLUDE;
	}
#endif

	net_if_mcast_monitor(iface, &maddr->address, true);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_JOIN, iface, &maddr->address.in_addr,
					sizeof(struct in_addr));

	return ret;
}

int net_ipv4_igmp_leave(struct net_if *iface, const struct in_addr *addr)
{
	struct net_if_mcast_addr *maddr;
	int ret;

	maddr = net_if_ipv4_maddr_lookup(addr, &iface);
	if (!maddr) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_IPV4_IGMPV3)
	maddr->record_type = IGMPV3_CHANGE_TO_INCLUDE_MODE;
	maddr->sources_len = 0;

	ret = igmpv3_send_generic(iface, maddr);
#else
	ret = igmp_send_generic(iface, addr, false);
#endif
	if (ret < 0) {
		return ret;
	}

	if (!net_if_ipv4_maddr_rm(iface, addr)) {
		return -EINVAL;
	}

	net_if_ipv4_maddr_leave(iface, maddr);

	net_if_mcast_monitor(iface, &maddr->address, false);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_LEAVE, iface, &maddr->address.in_addr,
					sizeof(struct in_addr));
	return ret;
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
