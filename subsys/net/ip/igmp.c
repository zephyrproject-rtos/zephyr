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

static const struct in_addr all_systems = { { { 224, 0, 0, 1 } } };
static const struct in_addr all_routers = { { { 224, 0, 0, 2 } } };

#define dbg_addr(action, pkt_str, src, dst)				\
	NET_DBG("%s %s from %s to %s", action, pkt_str,			\
		log_strdup(net_sprint_ipv4_addr(src)),			\
		log_strdup(net_sprint_ipv4_addr(dst)));

#define dbg_addr_recv(pkt_str, src, dst) \
	dbg_addr("Received", pkt_str, src, dst)

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
	igmp->chksum = net_calc_chksum_igmp((uint8_t *)igmp, sizeof(*igmp));

	if (net_pkt_set_data(pkt, &igmp_access)) {
		return -ENOBUFS;
	}

	return 0;
}

static int igmp_v2_create_packet(struct net_pkt *pkt, const struct in_addr *dst,
				 const struct in_addr *group, uint8_t type)
{
	const uint32_t router_alert = 0x94040000; /* RFC 2213 ch 2.1 */
	int ret;

	ret = net_ipv4_create_full(pkt,
				   net_if_ipv4_select_src_addr(
							net_pkt_iface(pkt),
							dst),
				   dst,
				   0U,
				   0U,
				   0U,
				   0U,
				   1U);  /* TTL set to 1, RFC 3376 ch 2 */
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

static int igmp_send(struct net_pkt *pkt)
{
	int ret;

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, IPPROTO_IGMP);

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_stats_update_ipv4_igmp_drop(net_pkt_iface(pkt));
		net_pkt_unref(pkt);
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
		if (!ipv4->mcast[i].is_used || !ipv4->mcast[i].is_joined) {
			continue;
		}

		count++;
	}

	if (count == 0) {
		return -ESRCH;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		if (!ipv4->mcast[i].is_used || !ipv4->mcast[i].is_joined) {
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

		/* TODO: send to arbitrary group address instead of
		 * all_routers
		 */
		ret = igmp_v2_create_packet(pkt, &all_routers,
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

enum net_verdict net_ipv4_igmp_input(struct net_pkt *pkt,
				     struct net_ipv4_hdr *ip_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(igmp_access,
					      struct net_ipv4_igmp_v2_query);
	struct net_ipv4_igmp_v2_query *igmp_hdr;

	/* TODO: receive from arbitrary group address instead of
	 * all_systems
	 */
	if (!net_ipv4_addr_cmp_raw(ip_hdr->dst, (uint8_t *)&all_systems)) {
		NET_DBG("DROP: Invalid dst address");
		return NET_DROP;
	}

	igmp_hdr = (struct net_ipv4_igmp_v2_query *)net_pkt_get_data(pkt,
								&igmp_access);
	if (!igmp_hdr) {
		NET_DBG("DROP: NULL IGMP header");
		return NET_DROP;
	}

	if (net_calc_chksum_igmp((uint8_t *)igmp_hdr,
				 sizeof(*igmp_hdr)) != 0U) {
		NET_DBG("DROP: Invalid checksum");
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &igmp_access);

	dbg_addr_recv("Internet Group Management Protocol", &ip_hdr->src,
		      &ip_hdr->dst);

	net_stats_update_ipv4_igmp_recv(net_pkt_iface(pkt));

	(void)send_igmp_report(net_pkt_iface(pkt), igmp_hdr);

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_ipv4_igmp_drop(net_pkt_iface(pkt));

	return NET_DROP;
}

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

	ret = igmp_v2_create_packet(pkt, &all_routers, addr,
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

int net_ipv4_igmp_join(struct net_if *iface, const struct in_addr *addr)
{
	struct net_if_mcast_addr *maddr;
	int ret;

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

	ret = igmp_send_generic(iface, addr, true);
	if (ret < 0) {
		return ret;
	}

	net_if_ipv4_maddr_join(maddr);

	net_if_mcast_monitor(iface, &maddr->address, true);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_JOIN, iface,
					&maddr->address.in_addr,
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

	if (!net_if_ipv4_maddr_rm(iface, addr)) {
		return -EINVAL;
	}

	ret = igmp_send_generic(iface, addr, false);
	if (ret < 0) {
		return ret;
	}

	net_if_ipv4_maddr_leave(maddr);

	net_if_mcast_monitor(iface, &maddr->address, false);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_MCAST_LEAVE, iface,
					&maddr->address.in_addr,
					sizeof(struct in_addr));
	return ret;
}
