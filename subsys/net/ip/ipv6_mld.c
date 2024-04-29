/** @file
 * @brief IPv6 MLD related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_ipv6, CONFIG_NET_IPV6_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/icmp.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
#include "net_stats.h"

/* Timeout for various buffer allocations in this file. */
#define PKT_WAIT_TIME K_MSEC(50)

#define MLDv2_MCAST_RECORD_LEN sizeof(struct net_icmpv6_mld_mcast_record)
#define IPV6_OPT_HDR_ROUTER_ALERT_LEN 8

#define MLDv2_LEN (MLDv2_MCAST_RECORD_LEN + sizeof(struct in6_addr))

static int mld_create(struct net_pkt *pkt,
		      const struct in6_addr *addr,
		      uint8_t record_type,
		      uint16_t num_sources)
{
	NET_PKT_DATA_ACCESS_DEFINE(mld_access,
				   struct net_icmpv6_mld_mcast_record);
	struct net_icmpv6_mld_mcast_record *mld;

	mld = (struct net_icmpv6_mld_mcast_record *)
				net_pkt_get_data(pkt, &mld_access);
	if (!mld) {
		return -ENOBUFS;
	}

	mld->record_type = record_type;
	mld->aux_data_len = 0U;
	mld->num_sources = htons(num_sources);

	net_ipv6_addr_copy_raw(mld->mcast_address, (uint8_t *)addr);

	if (net_pkt_set_data(pkt, &mld_access)) {
		return -ENOBUFS;
	}

	if (num_sources > 0) {
		/* All source addresses, RFC 3810 ch 3 */
		if (net_pkt_write(pkt,
				  net_ipv6_unspecified_address()->s6_addr,
				  sizeof(struct in6_addr))) {
			return -ENOBUFS;
		}
	}

	return 0;
}

static int mld_create_packet(struct net_pkt *pkt, uint16_t count)
{
	struct in6_addr dst;

	/* Sent to all MLDv2-capable routers */
	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);

	net_pkt_set_ipv6_hop_limit(pkt, 1); /* RFC 3810 ch 7.4 */

	if (net_ipv6_create(pkt, net_if_ipv6_select_src_addr(
				    net_pkt_iface(pkt), &dst),
			    &dst)) {
		return -ENOBUFS;
	}

	/* Add hop-by-hop option and router alert option, RFC 3810 ch 5. */
	if (net_pkt_write_u8(pkt, IPPROTO_ICMPV6) ||
	    net_pkt_write_u8(pkt, 0)) {
		return -ENOBUFS;
	}

	/* IPv6 router alert option is described in RFC 2711.
	 * - 0x0502 RFC 2711 ch 2.1
	 * - MLD (value 0)
	 * - 2 bytes of padding
	 */
	if (net_pkt_write_be16(pkt, 0x0502) ||
	    net_pkt_write_be16(pkt, 0) ||
	    net_pkt_write_be16(pkt, 0)) {
		return -ENOBUFS;
	}

	net_pkt_set_ipv6_ext_len(pkt, IPV6_OPT_HDR_ROUTER_ALERT_LEN);

	/* ICMPv6 header + reserved space + count.
	 * MLDv6 stuff will come right after
	 */
	if (net_icmpv6_create(pkt, NET_ICMPV6_MLDv2, 0) ||
	    net_pkt_write_be16(pkt, 0) ||
	    net_pkt_write_be16(pkt, count)) {
		return -ENOBUFS;
	}

	net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);

	return 0;
}

static int mld_send(struct net_pkt *pkt)
{
	int ret;

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_stats_update_icmp_drop(net_pkt_iface(pkt));
		net_stats_update_ipv6_mld_drop(net_pkt_iface(pkt));

		net_pkt_unref(pkt);

		return ret;
	}

	net_stats_update_icmp_sent(net_pkt_iface(pkt));
	net_stats_update_ipv6_mld_sent(net_pkt_iface(pkt));

	return 0;
}

static int mld_send_generic(struct net_if *iface,
			    const struct in6_addr *addr,
			    uint8_t mode)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, IPV6_OPT_HDR_ROUTER_ALERT_LEN +
					NET_ICMPV6_UNUSED_LEN +
					MLDv2_MCAST_RECORD_LEN +
					sizeof(struct in6_addr),
					AF_INET6, IPPROTO_ICMPV6,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	if (mld_create_packet(pkt, 1) ||
	    mld_create(pkt, addr, mode, 1)) {
		ret = -ENOBUFS;
		goto drop;
	}

	ret = mld_send(pkt);
	if (ret) {
		goto drop;
	}

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

int net_ipv6_mld_join(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_if_mcast_addr *maddr;
	int ret;

	maddr = net_if_ipv6_maddr_lookup(addr, &iface);
	if (maddr && net_if_ipv6_maddr_is_joined(maddr)) {
		return -EALREADY;
	}

	if (!maddr) {
		maddr = net_if_ipv6_maddr_add(iface, addr);
		if (!maddr) {
			return -ENOMEM;
		}
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		return 0;
	}

	if (!net_if_is_up(iface)) {
		return -ENETDOWN;
	}

	ret = mld_send_generic(iface, addr, NET_IPV6_MLDv2_MODE_IS_EXCLUDE);
	if (ret < 0) {
		return ret;
	}

	net_if_ipv6_maddr_join(iface, maddr);

	net_if_mcast_monitor(iface, &maddr->address, true);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_MCAST_JOIN, iface,
					&maddr->address.in6_addr,
					sizeof(struct in6_addr));

	return ret;
}

int net_ipv6_mld_leave(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_if_mcast_addr *maddr;
	int ret;

	maddr = net_if_ipv6_maddr_lookup(addr, &iface);
	if (!maddr) {
		return -ENOENT;
	}

	if (!net_if_ipv6_maddr_rm(iface, addr)) {
		return -EINVAL;
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		return 0;
	}

	ret = mld_send_generic(iface, addr, NET_IPV6_MLDv2_MODE_IS_INCLUDE);
	if (ret < 0) {
		return ret;
	}

	net_if_mcast_monitor(iface, &maddr->address, false);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_MCAST_LEAVE, iface,
					&maddr->address.in6_addr,
					sizeof(struct in6_addr));

	return ret;
}

static int send_mld_report(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	struct net_pkt *pkt;
	int i, count = 0;
	int ret;

	NET_ASSERT(ipv6);

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used || !ipv6->mcast[i].is_joined) {
			continue;
		}

		count++;
	}

	pkt = net_pkt_alloc_with_buffer(iface, IPV6_OPT_HDR_ROUTER_ALERT_LEN +
					NET_ICMPV6_UNUSED_LEN +
					count * MLDv2_MCAST_RECORD_LEN,
					AF_INET6, IPPROTO_ICMPV6,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOBUFS;
	}

	ret = mld_create_packet(pkt, count);
	if (ret < 0) {
		goto drop;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used || !ipv6->mcast[i].is_joined) {
			continue;
		}

		ret = mld_create(pkt, &ipv6->mcast[i].address.in6_addr,
				 NET_IPV6_MLDv2_MODE_IS_EXCLUDE, 0);
		if (ret < 0) {
			goto drop;
		}
	}

	ret = mld_send(pkt);
	if (ret < 0) {
		goto drop;
	}

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

#define dbg_addr(action, pkt_str, src, dst)				\
	do {								\
		NET_DBG("%s %s from %s to %s", action, pkt_str,         \
			net_sprint_ipv6_addr(src),		\
			net_sprint_ipv6_addr(dst));		\
	} while (0)

#define dbg_addr_recv(pkt_str, src, dst)	\
	dbg_addr("Received", pkt_str, src, dst)

static int handle_mld_query(struct net_icmp_ctx *ctx,
			    struct net_pkt *pkt,
			    struct net_icmp_ip_hdr *hdr,
			    struct net_icmp_hdr *icmp_hdr,
			    void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(mld_access,
					      struct net_icmpv6_mld_query);
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	uint16_t length = net_pkt_get_len(pkt);
	struct net_icmpv6_mld_query *mld_query;
	uint16_t pkt_len;
	int ret = -EIO;

	if (net_pkt_remaining_data(pkt) < sizeof(struct net_icmpv6_mld_query)) {
		/* MLDv1 query, drop. */
		ret = 0;
		goto drop;
	}

	mld_query = (struct net_icmpv6_mld_query *)
				net_pkt_get_data(pkt, &mld_access);
	if (!mld_query) {
		NET_DBG("DROP: NULL MLD query");
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &mld_access);

	dbg_addr_recv("Multicast Listener Query", &ip_hdr->src, &ip_hdr->dst);

	net_stats_update_ipv6_mld_recv(net_pkt_iface(pkt));

	mld_query->num_sources = ntohs(mld_query->num_sources);

	pkt_len = sizeof(struct net_ipv6_hdr) +	net_pkt_ipv6_ext_len(pkt) +
		sizeof(struct net_icmp_hdr) +
		sizeof(struct net_icmpv6_mld_query) +
		sizeof(struct in6_addr) * mld_query->num_sources;

	if (length < pkt_len || pkt_len > NET_IPV6_MTU ||
	    ip_hdr->hop_limit != 1U || icmp_hdr->code != 0U) {
		goto drop;
	}

	/* Currently we only support an unspecified address query. */
	if (!net_ipv6_addr_cmp_raw(mld_query->mcast_address,
				   (uint8_t *)net_ipv6_unspecified_address())) {
		NET_DBG("DROP: only supporting unspecified address query");
		goto drop;
	}

	return send_mld_report(net_pkt_iface(pkt));

drop:
	net_stats_update_ipv6_mld_drop(net_pkt_iface(pkt));

	return ret;
}

void net_ipv6_mld_init(void)
{
	static struct net_icmp_ctx ctx;
	int ret;

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV6_MLD_QUERY, 0, handle_mld_query);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV6_MLD_QUERY),
			ret);
	}
}
