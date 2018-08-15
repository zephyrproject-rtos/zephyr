/** @file
 * @brief IPv6 MLD related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_IPV6)
#define SYS_LOG_DOMAIN "net/ipv6-mld"
#define NET_LOG_ENABLED 1

/* By default this prints too much data, set the value to 1 to see
 * neighbor cache contents.
 */
#define NET_DEBUG_NBR 0
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/tcp.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
#include "rpl.h"
#include "net_stats.h"

/* Timeout for various buffer allocations in this file. */
#define NET_BUF_TIMEOUT K_MSEC(50)

#define append(pkt, type, value)					\
	do {								\
		if (!net_pkt_append_##type##_timeout(pkt, value,	\
						     NET_BUF_TIMEOUT)) { \
			ret = -ENOMEM;					\
			goto drop;					\
		}							\
	} while (0)

#define append_all(pkt, size, value)					\
	do {								\
		if (!net_pkt_append_all(pkt, size, value,		\
					NET_BUF_TIMEOUT)) {		\
			ret = -ENOMEM;					\
			goto drop;					\
		}							\
	} while (0)

#define MLDv2_LEN (2 + 1 + 1 + 2 + sizeof(struct in6_addr) * 2)

static struct net_pkt *create_mldv2(struct net_pkt *pkt,
				    const struct in6_addr *addr,
				    u16_t record_type,
				    u8_t num_sources)
{
	int ret;

	append(pkt, u8, record_type);
	append(pkt, u8, 0);             /* aux data len */
	append(pkt, be16, num_sources); /* number of addresses */

	append_all(pkt, sizeof(struct in6_addr), addr->s6_addr);

	if (num_sources > 0) {
		/* All source addresses, RFC 3810 ch 3 */
		append_all(pkt, sizeof(struct in6_addr),
			   net_ipv6_unspecified_address()->s6_addr);
	}

	return pkt;

drop:
	return NULL;
}

static int send_mldv2_raw(struct net_if *iface, struct net_buf *frags)
{
	struct net_pkt *pkt;
	struct in6_addr dst;
	u16_t pos;
	int ret;

	/* Sent to all MLDv2-capable routers */
	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, &dst),
				     NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	if (!net_ipv6_create(pkt,
			     net_if_ipv6_select_src_addr(iface, &dst),
			     &dst,
			     iface,
			     NET_IPV6_NEXTHDR_HBHO)) {
		ret = -ENOMEM;
		goto drop;
	}

	NET_IPV6_HDR(pkt)->hop_limit = 1; /* RFC 3810 ch 7.4 */

	net_pkt_set_ipv6_hdr_prev(pkt, pkt->frags->len);

	/* Add hop-by-hop option and router alert option, RFC 3810 ch 5. */
	append(pkt, u8, IPPROTO_ICMPV6);
	append(pkt, u8, 0);        /* length (0 means 8 bytes) */

	/* IPv6 router alert option is described in RFC 2711. */
	append(pkt, be16, 0x0502); /* RFC 2711 ch 2.1 */
	append(pkt, be16, 0);      /* pkt contains MLD msg */
	append(pkt, u8, 0);        /* padding */
	append(pkt, u8, 0);        /* padding */

	/* ICMPv6 header */
	append(pkt, u8, NET_ICMPV6_MLDv2); /* type */
	append(pkt, u8, 0);                /* code */
	append(pkt, be16, 0);              /* chksum */
	append(pkt, be16, 0);              /* reserved field */

#define ROUTER_ALERT_LEN 8

	pkt->frags->len = NET_IPV6ICMPH_LEN + ROUTER_ALERT_LEN;
	net_pkt_set_iface(pkt, iface);

	/* Insert the actual multicast record(s) here */
	net_pkt_frag_add(pkt, frags);

	ret = net_ipv6_finalize(pkt, NET_IPV6_NEXTHDR_HBHO);
	if (ret < 0) {
		goto drop;
	}

	net_pkt_set_ipv6_ext_len(pkt, ROUTER_ALERT_LEN);

	if (!net_pkt_write_be16_timeout(pkt, pkt->frags,
					NET_IPV6H_LEN + ROUTER_ALERT_LEN + 2,
					&pos,
					ntohs(~net_calc_chksum_icmpv6(pkt)),
					NET_BUF_TIMEOUT)) {
		ret = -ENOMEM;
		goto drop;
	}

	ret = net_send_data(pkt);
	if (ret < 0) {
		goto drop;
	}

	net_stats_update_icmp_sent(net_pkt_iface(pkt));
	net_stats_update_ipv6_mld_sent(net_pkt_iface(pkt));

	return 0;

drop:
	net_stats_update_icmp_drop(net_pkt_iface(pkt));
	net_stats_update_ipv6_mld_drop(net_pkt_iface(pkt));

	net_pkt_unref(pkt);

	return ret;
}

static int send_mldv2(struct net_if *iface, const struct in6_addr *addr,
		      u8_t mode)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				     NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	append(pkt, be16, 1); /* number of records */

	if (!create_mldv2(pkt, addr, mode, 1)) {
		ret = -ENOMEM;
		goto drop;
	}

	ret = send_mldv2_raw(iface, pkt->frags);

	pkt->frags = NULL;

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

	ret = send_mldv2(iface, addr, NET_IPV6_MLDv2_MODE_IS_EXCLUDE);
	if (ret < 0) {
		return ret;
	}

	net_if_ipv6_maddr_join(maddr);

	net_if_mcast_monitor(iface, addr, true);

	net_mgmt_event_notify(NET_EVENT_IPV6_MCAST_JOIN, iface);

	return ret;
}

int net_ipv6_mld_leave(struct net_if *iface, const struct in6_addr *addr)
{
	int ret;

	if (!net_if_ipv6_maddr_rm(iface, addr)) {
		return -EINVAL;
	}

	ret = send_mldv2(iface, addr, NET_IPV6_MLDv2_MODE_IS_INCLUDE);
	if (ret < 0) {
		return ret;
	}

	net_if_mcast_monitor(iface, addr, false);

	net_mgmt_event_notify(NET_EVENT_IPV6_MCAST_LEAVE, iface);

	return ret;
}

static void send_mld_report(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	struct net_pkt *pkt;
	int i, ret, count = 0;

	NET_ASSERT(ipv6);

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				     NET_BUF_TIMEOUT);
	if (!pkt) {
		return;
	}

	append(pkt, u8, 0); /* This will be the record count */

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used || !ipv6->mcast[i].is_joined) {
			continue;
		}

		if (!create_mldv2(pkt, &ipv6->mcast[i].address.in6_addr,
				  NET_IPV6_MLDv2_MODE_IS_EXCLUDE, 0)) {
			goto drop;
		}

		count++;
	}

	if (count > 0) {
		u16_t pos;

		/* Write back the record count */
		if (!net_pkt_write_u8_timeout(pkt, pkt->frags, 0, &pos,
					      count, NET_BUF_TIMEOUT)) {
			goto drop;
		}

		send_mldv2_raw(iface, pkt->frags);

		pkt->frags = NULL;
	}

drop:
	net_pkt_unref(pkt);
}

#if defined(CONFIG_NET_DEBUG_IPV6)
#define dbg_addr(action, pkt_str, src, dst)				\
	do {								\
		NET_DBG("%s %s from %s to %s", action, pkt_str,         \
			net_sprint_ipv6_addr(src),                      \
			net_sprint_ipv6_addr(dst));                     \
	} while (0)

#define dbg_addr_recv(pkt_str, src, dst)	\
	dbg_addr("Received", pkt_str, src, dst)
#else
#define dbg_addr(...)
#define dbg_addr_recv(...)
#endif /* CONFIG_NET_DEBUG_IPV6 */

static enum net_verdict handle_mld_query(struct net_pkt *pkt)
{
	u16_t total_len = net_pkt_get_len(pkt);
	struct in6_addr mcast;
	u16_t max_rsp_code, num_src, pkt_len;
	u16_t offset, pos;
	struct net_buf *frag;
	int ret;

	dbg_addr_recv("Multicast Listener Query",
		      &NET_IPV6_HDR(pkt)->src,
		      &NET_IPV6_HDR(pkt)->dst);

	net_stats_update_ipv6_mld_recv(net_pkt_iface(pkt));

	/* offset tells now where the ICMPv6 header is starting */
	frag = net_frag_get_pos(pkt,
				net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_icmp_hdr),
				&offset);

	frag = net_frag_read_be16(frag, offset, &pos, &max_rsp_code);
	frag = net_frag_skip(frag, pos, &pos, 2); /* two reserved bytes */
	frag = net_frag_read(frag, pos, &pos, sizeof(mcast), mcast.s6_addr);
	frag = net_frag_skip(frag, pos, &pos, 2); /* skip S, QRV & QQIC */
	frag = net_frag_read_be16(pkt->frags, pos, &pos, &num_src);
	if (!frag && pos == 0xffff) {
		goto drop;
	}

	pkt_len = sizeof(struct net_ipv6_hdr) +	net_pkt_ipv6_ext_len(pkt) +
		sizeof(struct net_icmp_hdr) + (2 + 2 + 16 + 2 + 2) +
		sizeof(struct in6_addr) * num_src;

	if ((total_len < pkt_len || pkt_len > NET_IPV6_MTU ||
	     (NET_IPV6_HDR(pkt)->hop_limit != 1))) {
		struct net_icmp_hdr icmp_hdr;

		ret = net_icmpv6_get_hdr(pkt, &icmp_hdr);
		if (ret < 0 || icmp_hdr.code != 0) {
			NET_DBG("Preliminary check failed %u/%u, code %u, "
				"hop %u", total_len, pkt_len,
				icmp_hdr.code, NET_IPV6_HDR(pkt)->hop_limit);
			goto drop;
		}
	}

	/* Currently we only support a unspecified address query. */
	if (!net_ipv6_addr_cmp(&mcast, net_ipv6_unspecified_address())) {
		NET_DBG("Only supporting unspecified address query (%s)",
			net_sprint_ipv6_addr(&mcast));
		goto drop;
	}

	send_mld_report(net_pkt_iface(pkt));

drop:
	net_stats_update_ipv6_mld_drop(net_pkt_iface(pkt));

	return NET_DROP;
}

static struct net_icmpv6_handler mld_query_input_handler = {
	.type = NET_ICMPV6_MLD_QUERY,
	.code = 0,
	.handler = handle_mld_query,
};

void net_ipv6_mld_init(void)
{
	net_icmpv6_register_handler(&mld_query_input_handler);
}
