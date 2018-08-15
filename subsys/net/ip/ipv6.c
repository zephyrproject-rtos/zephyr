/** @file
 * @brief IPv6 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_IPV6)
#define SYS_LOG_DOMAIN "net/ipv6"
#define NET_LOG_ENABLED 1

/* By default this prints too much data, set the value to 1 to see
 * neighbor cache contents.
 */
#define NET_DEBUG_NBR 0
#endif

#include <errno.h>
#include <stdlib.h>
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

/* Timeout value to be used when allocating net buffer during various
 * neighbor discovery procedures.
 */
#define ND_NET_BUF_TIMEOUT K_MSEC(100)

/* Timeout for various buffer allocations in this file. */
#define NET_BUF_TIMEOUT K_MSEC(50)

/* Maximum reachable time value specified in RFC 4861 section
 * 6.2.1. Router Configuration Variables, AdvReachableTime
 */
#define MAX_REACHABLE_TIME 3600000

/* IPv6 wildcard and loopback address defined by RFC2553 */
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

int net_ipv6_find_last_ext_hdr(struct net_pkt *pkt, u16_t *next_hdr_idx,
			       u16_t *last_hdr_idx)
{
	struct net_buf *next_hdr_frag;
	struct net_buf *last_hdr_frag;
	struct net_buf *frag;
	u16_t pkt_offset;
	u16_t offset;
	u16_t length;
	u8_t next_hdr;
	u8_t next;

	if (!pkt || !pkt->frags || !next_hdr_idx || !last_hdr_idx) {
		return -EINVAL;
	}

	next = NET_IPV6_HDR(pkt)->nexthdr;

	/* Initial value if no extension fragments are found */
	*next_hdr_idx = 6;
	*last_hdr_idx = sizeof(struct net_ipv6_hdr);

	/* First check the simplest case where there is no extension headers
	 * in the packet. There cannot be any extensions after the normal or
	 * typical IP protocols
	 */
	if (next == IPPROTO_ICMPV6 || next == IPPROTO_UDP ||
	    next == IPPROTO_TCP || next == NET_IPV6_NEXTHDR_NONE) {
		return 0;
	}

	frag = pkt->frags;
	offset = *last_hdr_idx;
	*next_hdr_idx = *last_hdr_idx;
	next_hdr_frag = last_hdr_frag = frag;

	while (frag) {
		frag = net_frag_read_u8(frag, offset, &offset, &next_hdr);
		if (!frag) {
			goto fail;
		}

		switch (next) {
		case NET_IPV6_NEXTHDR_FRAG:
			frag = net_frag_skip(frag, offset, &offset, 7);
			if (!frag) {
				goto fail;
			}

			break;

		case NET_IPV6_NEXTHDR_HBHO:
			length = 0;
			frag = net_frag_read_u8(frag, offset, &offset,
						(u8_t *)&length);
			if (!frag) {
				goto fail;
			}

			length = length * 8 + 8;

			frag = net_frag_skip(frag, offset, &offset, length - 2);
			if (!frag) {
				goto fail;
			}

			break;

		case NET_IPV6_NEXTHDR_NONE:
		case IPPROTO_ICMPV6:
		case IPPROTO_UDP:
		case IPPROTO_TCP:
			goto out;

		default:
			/* TODO: Add more IPv6 extension headers to check */
			goto fail;
		}

		*next_hdr_idx = *last_hdr_idx;
		next_hdr_frag = last_hdr_frag;

		*last_hdr_idx = offset;
		last_hdr_frag = frag;

		next = next_hdr;
	}

fail:
	return -EINVAL;

out:
	/* Current next_hdr_idx offset is based on respective fragment, but we
	 * need to calculate next_hdr_idx offset based on whole packet.
	 */
	pkt_offset = 0;
	frag = pkt->frags;
	while (frag) {
		if (next_hdr_frag == frag) {
			*next_hdr_idx += pkt_offset;
			break;
		}

		pkt_offset += frag->len;
		frag = frag->frags;
	}

	/* Current last_hdr_idx offset is based on respective fragment, but we
	 * need to calculate last_hdr_idx offset based on whole packet.
	 */
	pkt_offset = 0;
	frag = pkt->frags;
	while (frag) {
		if (last_hdr_frag == frag) {
			*last_hdr_idx += pkt_offset;
			break;
		}

		pkt_offset += frag->len;
		frag = frag->frags;
	}

	return 0;
}

const struct in6_addr *net_ipv6_unspecified_address(void)
{
	return &in6addr_any;
}

struct net_pkt *net_ipv6_create(struct net_pkt *pkt,
				const struct in6_addr *src,
				const struct in6_addr *dst,
				struct net_if *iface,
				u8_t next_header_proto)
{
	struct net_buf *header;

	header = net_pkt_get_frag(pkt, NET_BUF_TIMEOUT);
	if (!header) {
		return NULL;
	}

	net_pkt_frag_insert(pkt, header);

	NET_IPV6_HDR(pkt)->vtc = 0x60;
	NET_IPV6_HDR(pkt)->tcflow = 0;
	NET_IPV6_HDR(pkt)->flow = 0;

	NET_IPV6_HDR(pkt)->nexthdr = 0;

	/* User can tweak the default hop limit if needed */
	NET_IPV6_HDR(pkt)->hop_limit = net_pkt_ipv6_hop_limit(pkt);
	if (NET_IPV6_HDR(pkt)->hop_limit == 0) {
		NET_IPV6_HDR(pkt)->hop_limit =
					net_if_ipv6_get_hop_limit(iface);
	}

	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, dst);
	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, src);

	net_pkt_set_ipv6_ext_len(pkt, 0);
	NET_IPV6_HDR(pkt)->nexthdr = next_header_proto;

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);

	net_buf_add(header, sizeof(struct net_ipv6_hdr));

	return pkt;
}

int net_ipv6_finalize(struct net_pkt *pkt, u8_t next_header_proto)
{
	/* Set the length of the IPv6 header */
	size_t total_len;
	int ret;

#if defined(CONFIG_NET_UDP) && defined(CONFIG_NET_RPL_INSERT_HBH_OPTION)
	if (next_header_proto != IPPROTO_TCP &&
	    next_header_proto != IPPROTO_ICMPV6) {
		/* Check if we need to add RPL header to sent UDP packet. */
		if (net_rpl_insert_header(pkt) < 0) {
			NET_DBG("RPL HBHO insert failed");
			return -EINVAL;
		}
	}
#endif
	net_pkt_compact(pkt);

	total_len = net_pkt_get_len(pkt) - sizeof(struct net_ipv6_hdr);

	NET_IPV6_HDR(pkt)->len = htons(total_len);

#if defined(CONFIG_NET_UDP)
	if (next_header_proto == IPPROTO_UDP &&
	    net_if_need_calc_tx_checksum(net_pkt_iface(pkt))) {
		net_udp_set_chksum(pkt, pkt->frags);
	} else
#endif

#if defined(CONFIG_NET_TCP)
	if (next_header_proto == IPPROTO_TCP &&
	    net_if_need_calc_tx_checksum(net_pkt_iface(pkt))) {
		net_tcp_set_chksum(pkt, pkt->frags);
	} else
#endif

	if (next_header_proto == IPPROTO_ICMPV6) {
		ret = net_icmpv6_set_chksum(pkt);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

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

#if defined(CONFIG_NET_IPV6_MLD)
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
#endif /* CONFIG_NET_IPV6_MLD */

#if defined(CONFIG_NET_IPV6_FRAGMENT)
#if defined(CONFIG_NET_IPV6_FRAGMENT_TIMEOUT)
#define IPV6_REASSEMBLY_TIMEOUT K_SECONDS(CONFIG_NET_IPV6_FRAGMENT_TIMEOUT)
#else
#define IPV6_REASSEMBLY_TIMEOUT K_SECONDS(5)
#endif /* CONFIG_NET_IPV6_FRAGMENT_TIMEOUT */

#define FRAG_BUF_WAIT K_MSEC(10) /* how long to max wait for a buffer */

static void reassembly_timeout(struct k_work *work);
static bool reassembly_init_done;

static struct net_ipv6_reassembly
reassembly[CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT];

static struct net_ipv6_reassembly *reassembly_get(u32_t id,
						  struct in6_addr *src,
						  struct in6_addr *dst)
{
	int i, avail = -1;

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {

		if (k_delayed_work_remaining_get(&reassembly[i].timer) &&
		    reassembly[i].id == id &&
		    net_ipv6_addr_cmp(src, &reassembly[i].src) &&
		    net_ipv6_addr_cmp(dst, &reassembly[i].dst)) {
			return &reassembly[i];
		}

		if (k_delayed_work_remaining_get(&reassembly[i].timer)) {
			continue;
		}

		if (avail < 0) {
			avail = i;
		}
	}

	if (avail < 0) {
		return NULL;
	}

	k_delayed_work_submit(&reassembly[avail].timer,
			      IPV6_REASSEMBLY_TIMEOUT);

	net_ipaddr_copy(&reassembly[avail].src, src);
	net_ipaddr_copy(&reassembly[avail].dst, dst);

	reassembly[avail].id = id;

	return &reassembly[avail];
}

static bool reassembly_cancel(u32_t id,
			      struct in6_addr *src,
			      struct in6_addr *dst)
{
	int i, j;

	NET_DBG("Cancel 0x%x", id);

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		s32_t remaining;

		if (reassembly[i].id != id ||
		    !net_ipv6_addr_cmp(src, &reassembly[i].src) ||
		    !net_ipv6_addr_cmp(dst, &reassembly[i].dst)) {
			continue;
		}

		remaining = k_delayed_work_remaining_get(&reassembly[i].timer);
		if (remaining) {
			k_delayed_work_cancel(&reassembly[i].timer);
		}

		NET_DBG("IPv6 reassembly id 0x%x remaining %d ms",
			reassembly[i].id, remaining);

		reassembly[i].id = 0;

		for (j = 0; j < NET_IPV6_FRAGMENTS_MAX_PKT; j++) {
			if (!reassembly[i].pkt[j]) {
				continue;
			}

			NET_DBG("[%d] IPv6 reassembly pkt %p %zd bytes data",
				j, reassembly[i].pkt[j],
				net_pkt_get_len(reassembly[i].pkt[j]));

			net_pkt_unref(reassembly[i].pkt[j]);
			reassembly[i].pkt[j] = NULL;
		}

		return true;
	}

	return false;
}

static void reassembly_info(char *str, struct net_ipv6_reassembly *reass)
{
	int i, len;

	for (i = 0, len = 0; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		if (!reass->pkt[i]) {
			continue;
		}

		len += net_pkt_get_len(reass->pkt[i]);
	}

	NET_DBG("%s id 0x%x src %s dst %s remain %d ms len %d", str, reass->id,
		net_sprint_ipv6_addr(&reass->src),
		net_sprint_ipv6_addr(&reass->dst),
		k_delayed_work_remaining_get(&reass->timer), len);
}

static void reassembly_timeout(struct k_work *work)
{
	struct net_ipv6_reassembly *reass =
		CONTAINER_OF(work, struct net_ipv6_reassembly, timer);

	reassembly_info("Reassembly cancelled", reass);

	reassembly_cancel(reass->id, &reass->src, &reass->dst);
}

static void reassemble_packet(struct net_ipv6_reassembly *reass)
{
	struct net_pkt *pkt;
	struct net_buf *last;
	struct net_buf *frag;
	u8_t next_hdr;
	int i, len, ret;
	u16_t pos;

	k_delayed_work_cancel(&reass->timer);

	NET_ASSERT(reass->pkt[0]);

	last = net_buf_frag_last(reass->pkt[0]->frags);

	/* We start from 2nd packet which is then appended to
	 * the first one.
	 */
	for (i = 1; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		int removed_len;
		int ret;

		pkt = reass->pkt[i];

		/* Get rid of IPv6 and fragment header which are at
		 * the beginning of the fragment.
		 */
		removed_len = net_pkt_ipv6_fragment_start(pkt) +
			      sizeof(struct net_ipv6_frag_hdr);

		NET_DBG("Removing %d bytes from start of pkt %p",
			removed_len, pkt->frags);

		ret = net_pkt_pull(pkt, 0, removed_len);
		if (ret) {
			NET_ERR("Failed to pull headers");
			NET_ASSERT(ret != 0);
		}

		/* Attach the data to previous pkt */
		last->frags = pkt->frags;
		last = net_buf_frag_last(pkt->frags);

		pkt->frags = NULL;
		reass->pkt[i] = NULL;

		net_pkt_unref(pkt);
	}

	pkt = reass->pkt[0];
	reass->pkt[0] = NULL;

	/* Next we need to strip away the fragment header from the first packet
	 * and set the various pointers and values in packet.
	 */

	frag = net_frag_read_u8(pkt->frags, net_pkt_ipv6_fragment_start(pkt),
				&pos, &next_hdr);
	if (!frag && pos == 0xFFFF) {
		NET_ERR("Failed to read next header");
		NET_ASSERT(frag);
	}

	ret = net_pkt_pull(pkt, net_pkt_ipv6_fragment_start(pkt),
			   sizeof(struct net_ipv6_frag_hdr));
	if (ret) {
		NET_ERR("Failed to pull fragmentation header");
		NET_ASSERT(ret);
	}

	/* This one updates the previous header's nexthdr value */
	if (!net_pkt_write_u8_timeout(pkt, pkt->frags,
				      net_pkt_ipv6_hdr_prev(pkt),
				      &pos, next_hdr, NET_BUF_TIMEOUT)) {
		net_pkt_unref(pkt);
		return;
	}

	if (!net_pkt_compact(pkt)) {
		NET_ERR("Cannot compact reassembly packet %p", pkt);
		net_pkt_unref(pkt);
		return;
	}

	/* Fix the total length of the IPv6 packet. */
	len = net_pkt_ipv6_ext_len(pkt);
	if (len > 0) {
		NET_DBG("Old pkt %p IPv6 ext len is %d bytes", pkt, len);
		net_pkt_set_ipv6_ext_len(pkt,
				len - sizeof(struct net_ipv6_frag_hdr));
	}

	len = net_pkt_get_len(pkt) - sizeof(struct net_ipv6_hdr);

	NET_IPV6_HDR(pkt)->len = htons(len);

	NET_DBG("New pkt %p IPv6 len is %d bytes", pkt, len);

	/* We need to use the queue when feeding the packet back into the
	 * IP stack as we might run out of stack if we call processing_data()
	 * directly. As the packet does not contain link layer header, we
	 * MUST NOT pass it to L2 so there will be a special check for that
	 * in process_data() when handling the packet.
	 */
	ret = net_recv_data(net_pkt_iface(pkt), pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}
}

void net_ipv6_frag_foreach(net_ipv6_frag_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; reassembly_init_done &&
		     i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		if (!k_delayed_work_remaining_get(&reassembly[i].timer)) {
			continue;
		}

		cb(&reassembly[i], user_data);
	}
}

/* Verify that we have all the fragments received and in correct order.
 */
static bool fragment_verify(struct net_ipv6_reassembly *reass)
{
	u16_t offset;
	int i, prev_len;

	prev_len = net_pkt_get_len(reass->pkt[0]);
	offset = net_pkt_ipv6_fragment_offset(reass->pkt[0]);

	NET_DBG("pkt %p offset %u", reass->pkt[0], offset);

	if (offset != 0) {
		return false;
	}

	for (i = 1; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		offset = net_pkt_ipv6_fragment_offset(reass->pkt[i]);

		NET_DBG("pkt %p offset %u prev_len %d", reass->pkt[i],
			offset, prev_len);

		if (prev_len < offset) {
			/* Something wrong with the offset value */
			return false;
		}

		prev_len = net_pkt_get_len(reass->pkt[i]);
	}

	return true;
}

static int shift_packets(struct net_ipv6_reassembly *reass, int pos)
{
	int i;

	for (i = pos + 1; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		if (!reass->pkt[i]) {
			NET_DBG("Moving [%d] %p (offset 0x%x) to [%d]",
				pos, reass->pkt[pos],
				net_pkt_ipv6_fragment_offset(reass->pkt[pos]),
				i);

			/* Do we have enough space in packet array to make
			 * the move?
			 */
			if (((i - pos) + 1) >
			    (NET_IPV6_FRAGMENTS_MAX_PKT - i)) {
				return -ENOMEM;
			}

			memmove(&reass->pkt[i], &reass->pkt[pos],
				sizeof(void *) * (i - pos));

			return 0;
		}
	}

	return -EINVAL;
}

static enum net_verdict handle_fragment_hdr(struct net_pkt *pkt,
					    struct net_buf *frag,
					    int total_len,
					    u16_t buf_offset,
					    u16_t *loc,
					    u8_t nexthdr)
{
	struct net_ipv6_reassembly *reass = NULL;
	u32_t id;
	u16_t offset;
	u16_t flag;
	u8_t more;
	bool found;
	int i;

	if (!reassembly_init_done) {
		/* Static initializing does not work here because of the array
		 * so we must do it at runtime.
		 */
		for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
			k_delayed_work_init(&reassembly[i].timer,
					    reassembly_timeout);
		}

		reassembly_init_done = true;
	}

	/* Each fragment has a fragment header. */
	frag = net_frag_skip(frag, buf_offset, loc, 1); /* reserved */
	frag = net_frag_read_be16(frag, *loc, loc, &flag);
	frag = net_frag_read_be32(frag, *loc, loc, &id);
	if (!frag && *loc == 0xffff) {
		goto drop;
	}

	reass = reassembly_get(id, &NET_IPV6_HDR(pkt)->src,
			       &NET_IPV6_HDR(pkt)->dst);
	if (!reass) {
		NET_DBG("Cannot get reassembly slot, dropping pkt %p", pkt);
		goto drop;
	}

	offset = flag & 0xfff8;
	more = flag & 0x01;

	net_pkt_set_ipv6_fragment_offset(pkt, offset);

	if (!reass->pkt[0]) {
		NET_DBG("Storing pkt %p to slot %d offset 0x%x", pkt, 0,
			offset);
		reass->pkt[0] = pkt;

		reassembly_info("Reassembly 1st pkt", reass);

		/* Wait for more fragments to receive. */
		goto accept;
	}

	/* The fragments might come in wrong order so place them
	 * in reassembly chain in correct order.
	 */
	for (i = 0, found = false; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		if (!reass->pkt[i]) {
			NET_DBG("Storing pkt %p to slot %d offset 0x%x", pkt,
				i, offset);
			reass->pkt[i] = pkt;
			found = true;
			break;
		}

		if (net_pkt_ipv6_fragment_offset(reass->pkt[i]) < offset) {
			continue;
		}

		/* Make room for this fragment, if there is no room, then
		 * discard the whole reassembly.
		 */
		if (shift_packets(reass, i)) {
			break;
		}

		NET_DBG("Storing %p (offset 0x%x) to [%d]", pkt, offset, i);
		reass->pkt[i] = pkt;
		found = true;
		break;
	}

	if (!found) {
		/* We could not add this fragment into our saved fragment
		 * list. We must discard the whole packet at this point.
		 */
		NET_DBG("No slots available for 0x%x", reass->id);
		net_pkt_unref(pkt);
		goto drop;
	}

	if (more) {
		if (net_pkt_get_len(pkt) % 8) {
			/* Fragment length is not multiple of 8, discard
			 * the packet and send parameter problem error.
			 */
			net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
					      NET_ICMPV6_PARAM_PROB_OPTION, 0);
			goto drop;
		}

		reassembly_info("Reassembly nth pkt", reass);

		NET_DBG("More fragments to be received");
		goto accept;
	}

	reassembly_info("Reassembly last pkt", reass);

	if (!fragment_verify(reass)) {
		NET_DBG("Reassembled IPv6 verify failed, dropping id %u",
			reass->id);

		/* Let the caller release the already inserted pkt */
		if (i < NET_IPV6_FRAGMENTS_MAX_PKT) {
			reass->pkt[i] = NULL;
		}

		net_pkt_unref(pkt);
		goto drop;
	}

	/* The last fragment received, reassemble the packet */
	reassemble_packet(reass);

accept:
	return NET_OK;

drop:
	if (reass) {
		if (reassembly_cancel(reass->id, &reass->src, &reass->dst)) {
			return NET_OK;
		}
	}

	return NET_DROP;
}

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

static int send_ipv6_fragment(struct net_if *iface,
			      struct net_pkt *pkt,
			      struct net_buf **rest,
			      u16_t ipv6_hdrs_len,
			      u16_t fit_len,
			      u16_t frag_offset,
			      u8_t next_hdr,
			      u16_t next_hdr_idx,
			      u8_t last_hdr,
			      u16_t last_hdr_idx,
			      u16_t frag_count)
{
	struct net_pkt *ipv6 = NULL;
	bool final;
	struct net_ipv6_frag_hdr hdr;
	struct net_buf *frag;
	struct net_buf *temp;
	u16_t pos;
	bool res;
	int ret;

	ipv6 = net_pkt_clone(pkt, BUF_ALLOC_TIMEOUT);
	if (!ipv6) {
		NET_DBG("Cannot clone %p", ipv6);
		return -ENOMEM;
	}

	/* And we need to update the last header in the IPv6 packet to point to
	 * fragment header.
	 */
	temp = net_pkt_write_u8_timeout(ipv6, ipv6->frags, next_hdr_idx, &pos,
					NET_IPV6_NEXTHDR_FRAG,
					BUF_ALLOC_TIMEOUT);
	if (!temp) {
		if (pos == 0xffff) {
			ret = -EINVAL;
		} else {
			ret = -ENOMEM;
		}

		goto fail;
	}

	/* Update the extension length metadata so that upper layer checksum
	 * will be calculated properly by net_ipv6_finalize().
	 */
	net_pkt_set_ipv6_ext_len(ipv6,
				 net_pkt_ipv6_ext_len(pkt) +
				 sizeof(struct net_ipv6_frag_hdr));

	frag = *rest;
	if (fit_len < net_buf_frags_len(*rest)) {
		ret = net_pkt_split(pkt, frag, fit_len, rest, FRAG_BUF_WAIT);
		if (ret < 0) {
			net_buf_unref(frag);
			goto fail;
		}
	} else {
		*rest = NULL;
	}

	final = false;
	/* *rest == NULL means no more data to send */
	if (!*rest) {
		final = true;
	}

	/* Append the Fragmentation Header */
	hdr.nexthdr = next_hdr;
	hdr.reserved = 0;
	hdr.id = net_pkt_ipv6_fragment_id(pkt);
	hdr.offset = htons(((frag_offset / 8) << 3) | !final);

	res = net_pkt_append_all(ipv6, sizeof(struct net_ipv6_frag_hdr),
				 (u8_t *)&hdr, FRAG_BUF_WAIT);
	if (!res) {
		net_buf_unref(frag);
		ret = EINVAL;
		goto fail;
	}

	/* Attach the first part of split payload to end of the packet. And
	 * "rest" of the packet will be sent in next iteration.
	 */
	temp = ipv6->frags;
	while (1) {
		if (!temp->frags) {
			temp->frags = frag;
			break;
		}

		temp = temp->frags;
	}

	res = net_pkt_compact(ipv6);
	if (!res) {
		ret = -EINVAL;
		goto fail;
	}

	/* Note that we must not calculate possible UDP/TCP/ICMPv6 checksum
	 * as that is already calculated in the non-fragmented packet.
	 */
	ret = net_ipv6_finalize(ipv6, NET_IPV6_NEXTHDR_FRAG);
	if (ret < 0) {
		NET_DBG("Cannot create IPv6 packet (%d)", ret);
		goto fail;
	}

	/* If everything has been ok so far, we can send the packet.
	 * Note that we cannot send this re-constructed packet directly
	 * as the link layer headers will not be properly set (because
	 * we recreated the packet). So pass this packet back to TX
	 * so that the pkt is going back to L2 for setup.
	 */
	ret = net_send_data(ipv6);
	if (ret < 0) {
		NET_DBG("Cannot send fragment (%d)", ret);
		goto fail;
	}

	/* Let this packet to be sent and hopefully it will release
	 * the memory that can be utilized for next sent IPv6 fragment.
	 */
	k_yield();

	return 0;

fail:
	if (ipv6) {
		net_pkt_unref(ipv6);
	}

	return ret;
}

int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 u16_t pkt_len)
{
	struct net_pkt *clone;
	struct net_buf *rest;
	struct net_buf *temp;
	u16_t next_hdr_idx;
	u16_t last_hdr_idx;
	u16_t ipv6_hdrs_len;
	u16_t frag_offset;
	u16_t frag_count;
	u16_t pos;
	u8_t next_hdr;
	u8_t last_hdr;
	int fit_len;
	int ret = -EINVAL;

	/* We cannot touch original pkt because it might be used for
	 * some other purposes, like TCP resend etc. So we need to copy
	 * the large pkt here and do the fragmenting with the clone.
	 */
	clone = net_pkt_clone(pkt, BUF_ALLOC_TIMEOUT);
	if (!clone) {
		NET_DBG("Cannot clone %p", pkt);
		return -ENOMEM;
	}

	pkt = clone;
	net_pkt_set_ipv6_fragment_id(pkt, sys_rand32_get());

	ret = net_ipv6_find_last_ext_hdr(pkt, &next_hdr_idx, &last_hdr_idx);
	if (ret < 0) {
		goto fail;
	}

	temp = net_frag_read_u8(pkt->frags, next_hdr_idx, &pos, &next_hdr);
	if (!temp && pos == 0xffff) {
		ret = -EINVAL;
		goto fail;
	}

	temp = net_frag_read_u8(pkt->frags, last_hdr_idx, &pos, &last_hdr);
	if (!temp && pos == 0xffff) {
		ret = -EINVAL;
		goto fail;
	}

	ipv6_hdrs_len = net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt);

	ret = net_pkt_split(pkt, pkt->frags, ipv6_hdrs_len, &rest,
			    FRAG_BUF_WAIT);
	if (ret < 0 || ipv6_hdrs_len != net_pkt_get_len(pkt)) {
		NET_DBG("Cannot split packet (%d)", ret);
		goto fail;
	}

	frag_count = 0;
	frag_offset = 0;

	/* The Maximum payload can fit into each packet after IPv6 header,
	 * Extenstion headers and Fragmentation header.
	 */
	fit_len = NET_IPV6_MTU - NET_IPV6_FRAGH_LEN - ipv6_hdrs_len;
	if (fit_len <= 0) {
		/* Must be invalid extension headers length */
		NET_DBG("No room for IPv6 payload MTU %d hdrs_len %d",
			NET_IPV6_MTU, NET_IPV6_FRAGH_LEN + ipv6_hdrs_len);
		ret = -EINVAL;
		goto fail;
	}

	while (rest) {
		ret = send_ipv6_fragment(iface, pkt, &rest, ipv6_hdrs_len,
					 fit_len, frag_offset, next_hdr,
					 next_hdr_idx, last_hdr, last_hdr_idx,
					 frag_count);
		if (ret < 0) {
			goto fail;
		}

		frag_count++;
		frag_offset += fit_len;
	}

	net_pkt_unref(pkt);

	return 0;

fail:
	net_pkt_unref(pkt);

	if (rest) {
		net_buf_unref(rest);
	}

	return ret;
}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

static inline enum net_verdict process_icmpv6_pkt(struct net_pkt *pkt,
						  struct net_ipv6_hdr *ipv6)
{
	struct net_icmp_hdr icmp_hdr;
	int ret;

	ret = net_icmpv6_get_hdr(pkt, &icmp_hdr);
	if (ret < 0) {
		NET_DBG("NULL ICMPv6 header - dropping");
		return NET_DROP;
	}

	NET_DBG("ICMPv6 %s received type %d code %d",
		net_icmpv6_type2str(icmp_hdr.type), icmp_hdr.type,
		icmp_hdr.code);

	return net_icmpv6_input(pkt, icmp_hdr.type, icmp_hdr.code);
}

static inline struct net_pkt *check_unknown_option(struct net_pkt *pkt,
						   u8_t opt_type,
						   u16_t length)
{
	/* RFC 2460 chapter 4.2 tells how to handle the unknown
	 * options by the two highest order bits of the option:
	 *
	 * 00: Skip over this option and continue processing the header.
	 * 01: Discard the packet.
	 * 10: Discard the packet and, regardless of whether or not the
	 *     packet's Destination Address was a multicast address,
	 *     send an ICMP Parameter Problem, Code 2, message to the packet's
	 *     Source Address, pointing to the unrecognized Option Type.
	 * 11: Discard the packet and, only if the packet's Destination
	 *     Address was not a multicast address, send an ICMP Parameter
	 *     Problem, Code 2, message to the packet's Source Address,
	 *     pointing to the unrecognized Option Type.
	 */
	NET_DBG("Unknown option %d (0x%02x) MSB %d", opt_type, opt_type,
		opt_type >> 6);

	switch (opt_type & 0xc0) {
	case 0x00:
		break;
	case 0x40:
		return NULL;
	case 0xc0:
		if (net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
			return NULL;
		}
		/* passthrough */
	case 0x80:
		net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_OPTION,
				      (u32_t)length);
		return NULL;
	}

	return pkt;
}

static inline struct net_buf *handle_ext_hdr_options(struct net_pkt *pkt,
						     struct net_buf *frag,
						     int total_len,
						     u16_t len,
						     u16_t offset,
						     u16_t *pos,
						     enum net_verdict *verdict)
{
	u8_t opt_type, opt_len;
	u16_t length = 0, loc;
#if defined(CONFIG_NET_RPL)
	bool result;
#endif

	if (len > total_len) {
		NET_DBG("Corrupted packet, extension header %d too long "
			"(max %d bytes)", len, total_len);
		*verdict = NET_DROP;
		return NULL;
	}

	length += 2;

	/* Each extension option has type and length */
	frag = net_frag_read_u8(frag, offset, &loc, &opt_type);
	if (!frag && loc == 0xffff) {
		goto drop;
	}

	if (opt_type != NET_IPV6_EXT_HDR_OPT_PAD1) {
		frag = net_frag_read_u8(frag, loc, &loc, &opt_len);
		if (!frag && loc == 0xffff) {
			goto drop;
		}
	}

	while (frag && (length < len)) {
		switch (opt_type) {
		case NET_IPV6_EXT_HDR_OPT_PAD1:
			length++;
			break;
		case NET_IPV6_EXT_HDR_OPT_PADN:
			NET_DBG("PADN option");
			length += opt_len + 2;
			loc += opt_len + 2;
			break;
#if defined(CONFIG_NET_RPL)
		case NET_IPV6_EXT_HDR_OPT_RPL:
			NET_DBG("Processing RPL option");
			frag = net_rpl_verify_header(pkt, frag, loc, &loc,
						     &result);
			if (!result) {
				NET_DBG("RPL option error, packet dropped");
				goto drop;
			}

			if (!frag && *pos == 0xffff) {
				goto drop;
			}

			*verdict = NET_CONTINUE;
			return frag;
#endif
		default:
			if (!check_unknown_option(pkt, opt_type, length)) {
				goto drop;
			}

			length += opt_len + 2;

			/* No need to +2 here as loc already contains option
			 * header len.
			 */
			loc += opt_len;

			break;
		}

		if (length >= len) {
			break;
		}

		frag = net_frag_read_u8(frag, loc, &loc, &opt_type);
		if (!frag && loc == 0xffff) {
			goto drop;
		}

		if (opt_type != NET_IPV6_EXT_HDR_OPT_PAD1) {
			frag = net_frag_read_u8(frag, loc, &loc, &opt_len);
			if (!frag && loc == 0xffff) {
				goto drop;
			}
		}
	}

	if (length != len) {
		goto drop;
	}

	*pos = loc;

	*verdict = NET_CONTINUE;
	return frag;

drop:
	*verdict = NET_DROP;
	return NULL;
}

static inline bool is_upper_layer_protocol_header(u8_t proto)
{
	return (proto == IPPROTO_ICMPV6 || proto == IPPROTO_UDP ||
		proto == IPPROTO_TCP);
}

#if defined(CONFIG_NET_ROUTE)
static struct net_route_entry *add_route(struct net_if *iface,
					 struct in6_addr *addr,
					 u8_t prefix_len)
{
	struct net_route_entry *route;

	route = net_route_lookup(iface, addr);
	if (route) {
		return route;
	}

	route = net_route_add(iface, addr, prefix_len, addr);

	NET_DBG("%s route to %s/%d iface %p", route ? "Add" : "Cannot add",
		net_sprint_ipv6_addr(addr), prefix_len, iface);

	return route;
}
#endif /* CONFIG_NET_ROUTE */

static void no_route_info(struct net_pkt *pkt,
			  struct in6_addr *src,
			  struct in6_addr *dst)
{
	NET_DBG("Will not route pkt %p ll src %s to dst %s between interfaces",
		pkt, net_sprint_ipv6_addr(src), net_sprint_ipv6_addr(dst));
}

#if defined(CONFIG_NET_ROUTE)
static enum net_verdict route_ipv6_packet(struct net_pkt *pkt,
					  struct net_ipv6_hdr *hdr)
{
	struct net_route_entry *route;
	struct in6_addr *nexthop;
	bool found;

	/* Check if the packet can be routed */
	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		found = net_route_get_info(NULL, &hdr->dst, &route,
					   &nexthop);
	} else {
		found = net_route_get_info(net_pkt_iface(pkt),
					   &hdr->dst, &route, &nexthop);
	}

	if (found) {
		int ret;

		if (IS_ENABLED(CONFIG_NET_ROUTING) &&
		    (net_is_ipv6_ll_addr(&hdr->src) ||
		     net_is_ipv6_ll_addr(&hdr->dst))) {
			/* RFC 4291 ch 2.5.6 */
			no_route_info(pkt, &hdr->src, &hdr->dst);
			goto drop;
		}

		/* Used when detecting if the original link
		 * layer address length is changed or not.
		 */
		net_pkt_set_orig_iface(pkt, net_pkt_iface(pkt));

		if (route) {
			net_pkt_set_iface(pkt, route->iface);
		}

		if (IS_ENABLED(CONFIG_NET_ROUTING) &&
		    net_pkt_orig_iface(pkt) != net_pkt_iface(pkt)) {
			/* If the route interface to destination is
			 * different than the original route, then add
			 * route to original source.
			 */
			NET_DBG("Route pkt %p from %p to %p",
				pkt, net_pkt_orig_iface(pkt),
				net_pkt_iface(pkt));

			add_route(net_pkt_orig_iface(pkt),
				  &NET_IPV6_HDR(pkt)->src, 128);
		}

		ret = net_route_packet(pkt, nexthop);
		if (ret < 0) {
			NET_DBG("Cannot re-route pkt %p via %s "
				"at iface %p (%d)",
				pkt, net_sprint_ipv6_addr(nexthop),
				net_pkt_iface(pkt), ret);
		} else {
			return NET_OK;
		}
	} else {
		NET_DBG("No route to %s pkt %p dropped",
			net_sprint_ipv6_addr(&hdr->dst), pkt);
	}

drop:
	return NET_DROP;
}
#endif /* CONFIG_NET_ROUTE */

enum net_verdict net_ipv6_process_pkt(struct net_pkt *pkt)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_HDR(pkt);
	int real_len = net_pkt_get_len(pkt);
	int pkt_len = ntohs(hdr->len) + sizeof(*hdr);
	struct net_buf *frag;
	u8_t start_of_ext, prev_hdr;
	u8_t next, next_hdr;
	u8_t first_option;
	u16_t offset;
	u16_t length;
	u16_t total_len = 0;
	u8_t ext_bitmap;

	if (real_len != pkt_len) {
		NET_DBG("IPv6 packet size %d pkt len %d", pkt_len, real_len);
		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	NET_DBG("IPv6 packet len %d received from %s to %s", real_len,
		net_sprint_ipv6_addr(&hdr->src),
		net_sprint_ipv6_addr(&hdr->dst));

	if (net_is_ipv6_addr_mcast(&hdr->src)) {
		NET_DBG("Dropping src multicast packet");
		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	if (!net_is_my_ipv6_addr(&hdr->dst) &&
	    !net_is_my_ipv6_maddr(&hdr->dst) &&
	    !net_is_ipv6_addr_mcast(&hdr->dst) &&
	    !net_is_ipv6_addr_loopback(&hdr->dst)) {
#if defined(CONFIG_NET_ROUTE)
		enum net_verdict verdict;

		verdict = route_ipv6_packet(pkt, hdr);
		if (verdict == NET_OK) {
			return NET_OK;
		}
#else /* CONFIG_NET_ROUTE */
		NET_DBG("IPv6 packet in pkt %p not for me", pkt);
#endif /* CONFIG_NET_ROUTE */

		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	/* If we receive a packet with ll source address fe80: and destination
	 * address is one of ours, and if the packet would cross interface
	 * boundary, then drop the packet. RFC 4291 ch 2.5.6
	 */
	if (IS_ENABLED(CONFIG_NET_ROUTING) &&
	    net_is_ipv6_ll_addr(&hdr->src) &&
	    !net_is_ipv6_addr_mcast(&hdr->dst) &&
	    !net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
					      &hdr->dst)) {
		no_route_info(pkt, &hdr->src, &hdr->dst);

		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	/* Check extension headers */
	net_pkt_set_next_hdr(pkt, &hdr->nexthdr);
	net_pkt_set_ipv6_ext_len(pkt, 0);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_HDR(pkt)->hop_limit);

	/* Fast path for main upper layer protocols. The handling of extension
	 * headers can be slow so do this checking here. There cannot
	 * be any extension headers after the upper layer protocol header.
	 */
	next = *(net_pkt_next_hdr(pkt));
	if (is_upper_layer_protocol_header(next)) {
		goto upper_proto;
	}

	/* Go through the extensions */
	frag = pkt->frags;
	next = hdr->nexthdr;
	first_option = next;
	length = 0;
	ext_bitmap = 0;
	start_of_ext = 0;
	offset = sizeof(struct net_ipv6_hdr);
	prev_hdr = &NET_IPV6_HDR(pkt)->nexthdr - &NET_IPV6_HDR(pkt)->vtc;

	while (frag) {
		enum net_verdict verdict;

		if (is_upper_layer_protocol_header(next)) {
			NET_DBG("IPv6 next header %d", next);
			goto upper_proto;
		}

		if (!start_of_ext) {
			start_of_ext = offset;
		}

		frag = net_frag_read_u8(frag, offset, &offset, &next_hdr);
		if (!frag) {
			goto drop;
		}

		verdict = NET_OK;

		NET_DBG("IPv6 next header %d", next);

		switch (next) {
		case NET_IPV6_NEXTHDR_NONE:
			/* There is nothing after this header (see RFC 2460,
			 * ch 4.7), so we can drop the packet now.
			 * This is not an error case so do not update drop
			 * statistics.
			 */
			goto drop;

		case NET_IPV6_NEXTHDR_HBHO:
			frag = net_frag_read_u8(frag, offset, &offset,
						(u8_t *)&length);
			if (!frag) {
				goto drop;
			}

			length = length * 8 + 8;
			total_len += length;

			/* HBH option needs to be the first one */
			if (first_option != NET_IPV6_NEXTHDR_HBHO) {
				goto bad_hdr;
			}

			/* Hop by hop option */
			if (ext_bitmap & NET_IPV6_EXT_HDR_BITMAP_HBHO) {
				goto bad_hdr;
			}

			ext_bitmap |= NET_IPV6_EXT_HDR_BITMAP_HBHO;

			frag = handle_ext_hdr_options(pkt, frag, real_len,
						      length, offset, &offset,
						      &verdict);
			break;

#if defined(CONFIG_NET_IPV6_FRAGMENT)
		case NET_IPV6_NEXTHDR_FRAG:
			net_pkt_set_ipv6_hdr_prev(pkt, prev_hdr);

			net_pkt_set_ipv6_fragment_start(pkt,
							sizeof(struct
							       net_ipv6_hdr) +
							total_len);

			total_len += 8;
			return handle_fragment_hdr(pkt, frag, real_len, offset,
						   &offset, next_hdr);
#endif
		default:
			goto bad_hdr;
		}

		if (verdict == NET_DROP) {
			goto drop;
		}

		prev_hdr = start_of_ext;
		next = next_hdr;
	}

upper_proto:

	net_pkt_set_ipv6_ext_len(pkt, total_len);

	switch (next) {
	case IPPROTO_ICMPV6:
		return process_icmpv6_pkt(pkt, hdr);
	case IPPROTO_UDP:
#if defined(CONFIG_NET_UDP)
		return net_conn_input(IPPROTO_UDP, pkt);
#else
		return NET_DROP;
#endif
	case IPPROTO_TCP:
#if defined(CONFIG_NET_TCP)
		return net_conn_input(IPPROTO_TCP, pkt);
#else
		return NET_DROP;
#endif
	}

drop:
	return NET_DROP;

bad_hdr:
	/* Send error message about parameter problem (RFC 2460)
	 */
	net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
			      NET_ICMPV6_PARAM_PROB_NEXTHEADER,
			      offset - 1);

	NET_DBG("Unknown next header type");
	net_stats_update_ip_errors_protoerr(net_pkt_iface(pkt));

	return NET_DROP;
}

void net_ipv6_init(void)
{
	net_ipv6_nbr_init();

#if defined(CONFIG_NET_IPV6_MLD)
	net_icmpv6_register_handler(&mld_query_input_handler);
#endif
}
