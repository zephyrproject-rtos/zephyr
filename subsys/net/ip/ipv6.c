/** @file
 * @brief ICMPv6 related functions
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
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
#include "rpl.h"
#include "net_stats.h"

#if defined(CONFIG_NET_IPV6_ND)
static void nd_reachable_timeout(struct k_work *work);
#endif

#if defined(CONFIG_NET_IPV6_NBR_CACHE)

#define MAX_MULTICAST_SOLICIT 3
#define MAX_UNICAST_SOLICIT   3
#define DELAY_FIRST_PROBE_TIME (5 * MSEC_PER_SEC) /* RFC 4861 ch 10 */
#define RETRANS_TIMER 1000 /* in ms, RFC 4861 ch 10 */

extern void net_neighbor_data_remove(struct net_nbr *nbr);
extern void net_neighbor_table_clear(struct net_nbr_table *table);

NET_NBR_POOL_INIT(net_neighbor_pool,
		  CONFIG_NET_IPV6_MAX_NEIGHBORS,
		  sizeof(struct net_ipv6_nbr_data),
		  net_neighbor_data_remove,
		  0);

NET_NBR_TABLE_INIT(NET_NBR_GLOBAL,
		   neighbor,
		   net_neighbor_pool,
		   net_neighbor_table_clear);

const char *net_ipv6_nbr_state2str(enum net_ipv6_nbr_state state)
{
	switch (state) {
	case NET_IPV6_NBR_STATE_INCOMPLETE:
		return "incomplete";
	case NET_IPV6_NBR_STATE_REACHABLE:
		return "reachable";
	case NET_IPV6_NBR_STATE_STALE:
		return "stale";
	case NET_IPV6_NBR_STATE_DELAY:
		return "delay";
	case NET_IPV6_NBR_STATE_PROBE:
		return "probe";
	}

	return "<invalid state>";
}

static void ipv6_nbr_set_state(struct net_nbr *nbr,
			       enum net_ipv6_nbr_state new_state)
{
	if (new_state == net_ipv6_nbr_data(nbr)->state) {
		return;
	}

	NET_DBG("nbr %p %s -> %s", nbr,
		net_ipv6_nbr_state2str(net_ipv6_nbr_data(nbr)->state),
		net_ipv6_nbr_state2str(new_state));

	net_ipv6_nbr_data(nbr)->state = new_state;
}

static inline bool net_is_solicited(struct net_pkt *pkt)
{
	return NET_ICMPV6_NA_HDR(pkt)->flags & NET_ICMPV6_NA_FLAG_SOLICITED;
}

static inline bool net_is_router(struct net_pkt *pkt)
{
	return NET_ICMPV6_NA_HDR(pkt)->flags & NET_ICMPV6_NA_FLAG_ROUTER;
}

static inline bool net_is_override(struct net_pkt *pkt)
{
	return NET_ICMPV6_NA_HDR(pkt)->flags & NET_ICMPV6_NA_FLAG_OVERRIDE;
}

static inline struct net_nbr *get_nbr(int idx)
{
	return &net_neighbor_pool[idx].nbr;
}

static inline struct net_nbr *get_nbr_from_data(struct net_ipv6_nbr_data *data)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (nbr->data == (u8_t *)data) {
			return nbr;
		}
	}

	return NULL;
}

void net_ipv6_nbr_foreach(net_nbr_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		cb(nbr, user_data);
	}
}

#if NET_DEBUG_NBR
void nbr_print(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		NET_DBG("[%d] %p %d/%d/%d/%d/%d pending %p iface %p idx %d "
			"ll %s addr %s",
			i, nbr, nbr->ref, net_ipv6_nbr_data(nbr)->ns_count,
			net_ipv6_nbr_data(nbr)->is_router,
			net_ipv6_nbr_data(nbr)->state,
			net_ipv6_nbr_data(nbr)->link_metric,
			net_ipv6_nbr_data(nbr)->pending,
			nbr->iface, nbr->idx,
			nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
			net_sprint_ll_addr(
				net_nbr_get_lladdr(nbr->idx)->addr,
				net_nbr_get_lladdr(nbr->idx)->len),
			net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));
	}
}
#else
#define nbr_print(...)
#endif

static struct net_nbr *nbr_lookup(struct net_nbr_table *table,
				  struct net_if *iface,
				  struct in6_addr *addr)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (nbr->iface == iface &&
		    net_ipv6_addr_cmp(&net_ipv6_nbr_data(nbr)->addr, addr)) {
			return nbr;
		}
	}

	return NULL;
}

struct net_ipv6_nbr_data *net_ipv6_get_nbr_by_index(u8_t idx)
{
	struct net_nbr *nbr = get_nbr(idx);

	NET_ASSERT_INFO(nbr, "Invalid ll index %d", idx);

	return net_ipv6_nbr_data(nbr);
}

static inline void nbr_clear_ns_pending(struct net_ipv6_nbr_data *data)
{
	int ret;

	ret = k_delayed_work_cancel(&data->send_ns);
	if (ret < 0) {
		NET_DBG("Cannot cancel NS work (%d)", ret);
	}

	if (data->pending) {
		net_pkt_unref(data->pending);
		data->pending = NULL;
	}
}

static inline void nbr_free(struct net_nbr *nbr)
{
	NET_DBG("nbr %p", nbr);

	nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));

	k_delayed_work_cancel(&net_ipv6_nbr_data(nbr)->reachable);

	net_nbr_unref(nbr);
}

bool net_ipv6_nbr_rm(struct net_if *iface, struct in6_addr *addr)
{
	struct net_nbr *nbr;

	nbr = nbr_lookup(&net_neighbor.table, iface, addr);
	if (!nbr) {
		return false;
	}

	nbr_free(nbr);

	return true;
}

#define NS_REPLY_TIMEOUT MSEC_PER_SEC

static void ns_reply_timeout(struct k_work *work)
{
	/* We did not receive reply to a sent NS */
	struct net_ipv6_nbr_data *data = CONTAINER_OF(work,
						      struct net_ipv6_nbr_data,
						      send_ns);

	struct net_nbr *nbr = get_nbr_from_data(data);

	if (!nbr) {
		NET_DBG("NS timeout but no nbr data");
		return;
	}

	if (!data->pending) {
		/* Silently return, this is not an error as the work
		 * cannot be cancelled in certain cases.
		 */
		return;
	}

	NET_DBG("NS nbr %p pending %p timeout to %s", nbr, data->pending,
		net_sprint_ipv6_addr(&NET_IPV6_HDR(data->pending)->dst));

	/* To unref when pending variable was set */
	net_pkt_unref(data->pending);

	/* To unref the original pkt allocation */
	net_pkt_unref(data->pending);

	data->pending = NULL;

	net_nbr_unref(nbr);
}

static void nbr_init(struct net_nbr *nbr, struct net_if *iface,
		     struct in6_addr *addr, bool is_router,
		     enum net_ipv6_nbr_state state)
{
	nbr->idx = NET_NBR_LLADDR_UNKNOWN;
	nbr->iface = iface;

	net_ipaddr_copy(&net_ipv6_nbr_data(nbr)->addr, addr);
	ipv6_nbr_set_state(nbr, state);
	net_ipv6_nbr_data(nbr)->is_router = is_router;
	net_ipv6_nbr_data(nbr)->pending = NULL;

#if defined(CONFIG_NET_IPV6_ND)
	k_delayed_work_init(&net_ipv6_nbr_data(nbr)->reachable,
			    nd_reachable_timeout);
#endif
	k_delayed_work_init(&net_ipv6_nbr_data(nbr)->send_ns,
			    ns_reply_timeout);
}

static struct net_nbr *nbr_new(struct net_if *iface,
			       struct in6_addr *addr, bool is_router,
			       enum net_ipv6_nbr_state state)
{
	struct net_nbr *nbr = net_nbr_get(&net_neighbor.table);

	if (!nbr) {
		return NULL;
	}

	nbr_init(nbr, iface, addr, true, state);

	NET_DBG("nbr %p iface %p state %d IPv6 %s",
		nbr, iface, state, net_sprint_ipv6_addr(addr));

	return nbr;
}

#if defined(CONFIG_NET_DEBUG_IPV6)
static inline void dbg_update_neighbor_lladdr(struct net_linkaddr *new_lladdr,
				struct net_linkaddr_storage *old_lladdr,
				struct in6_addr *addr)
{
	char out[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ll_addr(old_lladdr->addr, old_lladdr->len));

	NET_DBG("Updating neighbor %s lladdr %s (was %s)",
		net_sprint_ipv6_addr(addr),
		net_sprint_ll_addr(new_lladdr->addr, new_lladdr->len),
		out);
}

static inline void dbg_update_neighbor_lladdr_raw(u8_t *new_lladdr,
				struct net_linkaddr_storage *old_lladdr,
				struct in6_addr *addr)
{
	struct net_linkaddr lladdr = {
		.len = old_lladdr->len,
		.addr = new_lladdr,
	};

	dbg_update_neighbor_lladdr(&lladdr, old_lladdr, addr);
}

#define dbg_addr(action, pkt_str, src, dst)				\
	do {								\
		char out[NET_IPV6_ADDR_LEN];				\
									\
		snprintk(out, sizeof(out), "%s",			\
			 net_sprint_ipv6_addr(dst));			\
									\
		NET_DBG("%s %s from %s to %s", action,			\
			pkt_str, net_sprint_ipv6_addr(src), out);	\
									\
	} while (0)

#define dbg_addr_recv(pkt_str, src, dst)	\
	dbg_addr("Received", pkt_str, src, dst)

#define dbg_addr_sent(pkt_str, src, dst)	\
	dbg_addr("Sent", pkt_str, src, dst)

#define dbg_addr_with_tgt(action, pkt_str, src, dst, target)		\
	do {								\
		char out[NET_IPV6_ADDR_LEN];				\
		char tgt[NET_IPV6_ADDR_LEN];				\
									\
		snprintk(out, sizeof(out), "%s",			\
			 net_sprint_ipv6_addr(dst));			\
		snprintk(tgt, sizeof(tgt), "%s",			\
			 net_sprint_ipv6_addr(target));			\
									\
		NET_DBG("%s %s from %s to %s, target %s", action,	\
			pkt_str, net_sprint_ipv6_addr(src), out, tgt);	\
									\
	} while (0)

#define dbg_addr_recv_tgt(pkt_str, src, dst, tgt)		\
	dbg_addr_with_tgt("Received", pkt_str, src, dst, tgt)

#define dbg_addr_sent_tgt(pkt_str, src, dst, tgt)		\
	dbg_addr_with_tgt("Sent", pkt_str, src, dst, tgt)
#else
#define dbg_update_neighbor_lladdr(...)
#define dbg_update_neighbor_lladdr_raw(...)
#define dbg_addr(...)
#define dbg_addr_recv(...)
#define dbg_addr_sent(...)

#define dbg_addr_with_tgt(...)
#define dbg_addr_recv_tgt(...)
#define dbg_addr_sent_tgt(...)
#endif /* CONFIG_NET_DEBUG_IPV6 */

struct net_nbr *net_ipv6_nbr_add(struct net_if *iface,
				 struct in6_addr *addr,
				 struct net_linkaddr *lladdr,
				 bool is_router,
				 enum net_ipv6_nbr_state state)
{
	struct net_nbr *nbr;

	nbr = nbr_lookup(&net_neighbor.table, iface, addr);
	if (!nbr) {
		nbr = nbr_new(iface, addr, is_router, state);
		if (!nbr) {
			NET_ERR("Could not add router neighbor %s [%s]",
				net_sprint_ipv6_addr(addr),
				net_sprint_ll_addr(lladdr->addr, lladdr->len));
			return NULL;
		}
	}

	if (net_nbr_link(nbr, iface, lladdr) == -EALREADY) {
		/* Update the lladdr if the node was already known */
		struct net_linkaddr_storage *cached_lladdr;

		cached_lladdr = net_nbr_get_lladdr(nbr->idx);

		if (memcmp(cached_lladdr->addr, lladdr->addr, lladdr->len)) {
			dbg_update_neighbor_lladdr(lladdr, cached_lladdr, addr);

			net_linkaddr_set(cached_lladdr, lladdr->addr,
					 lladdr->len);

			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		} else if (net_ipv6_nbr_data(nbr)->state ==
			   NET_IPV6_NBR_STATE_INCOMPLETE) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		}
	}

	if (net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_INCOMPLETE) {
		/* Send NS so that we can verify that the neighbor is
		 * reachable.
		 */
		net_ipv6_send_ns(iface, NULL, NULL, NULL, addr, false);
	}

	NET_DBG("[%d] nbr %p state %d router %d IPv6 %s ll %s",
		nbr->idx, nbr, state, is_router,
		net_sprint_ipv6_addr(addr),
		net_sprint_ll_addr(lladdr->addr, lladdr->len));

	return nbr;
}

static inline struct net_nbr *nbr_add(struct net_pkt *pkt,
				      struct net_linkaddr *lladdr,
				      bool is_router,
				      enum net_ipv6_nbr_state state)
{
	return net_ipv6_nbr_add(net_pkt_iface(pkt), &NET_IPV6_HDR(pkt)->src,
				lladdr, is_router, state);
}

void net_neighbor_data_remove(struct net_nbr *nbr)
{
	NET_DBG("Neighbor %p removed", nbr);

	return;
}

void net_neighbor_table_clear(struct net_nbr_table *table)
{
	NET_DBG("Neighbor table %p cleared", table);
}

struct in6_addr *net_ipv6_nbr_lookup_by_index(struct net_if *iface,
					      u8_t idx)
{
	int i;

	if (idx == NET_NBR_LLADDR_UNKNOWN) {
		return NULL;
	}

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (iface && nbr->iface != iface) {
			continue;
		}

		if (nbr->idx == idx) {
			return &net_ipv6_nbr_data(nbr)->addr;
		}
	}

	return NULL;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

int net_ipv6_find_last_ext_hdr(struct net_pkt *pkt, u16_t *next_hdr_idx,
			       u16_t *last_hdr_idx)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_HDR(pkt);
	struct net_buf *frag = pkt->frags;
	int pos = 0;
	u16_t offset, prev, tmp;
	u8_t next_hdr;
	u8_t length;
	u8_t next;

	next = hdr->nexthdr;

	/* Initial value if no extension fragments are found */
	*next_hdr_idx = 6;

	offset = *last_hdr_idx = sizeof(struct net_ipv6_hdr);

	/* First check the simplest case where there is no extension headers
	 * in the packet. There cannot be any extensions after the normal or
	 * typical IP protocols
	 */
	if (next == IPPROTO_ICMPV6 || next == IPPROTO_UDP ||
	    next == IPPROTO_TCP) {
		return 0;
	}

	prev = pos;

	while (frag) {
		frag = net_frag_read_u8(frag, offset, &offset, &next_hdr);
		if (!frag && offset == 0xffff) {
			goto fail;
		}

		frag = net_frag_read_u8(frag, offset, &offset, &length);
		if (!frag && offset == 0xffff) {
			goto fail;
		}

		length = length * 8 + 8;

		/* TODO: Add here more IPv6 extension headers to check */
		switch (next) {
		case NET_IPV6_NEXTHDR_NONE:
			*next_hdr_idx = prev;
			*last_hdr_idx = offset - 2;
			goto out;

		case NET_IPV6_NEXTHDR_FRAG:
			prev = pos;
			pos = offset - 2;
			offset += 2 + 4;
			break;

		case NET_IPV6_NEXTHDR_HBHO:
			prev = pos;
			pos = offset - 2;
			offset += length - 2;
			break;

		case IPPROTO_ICMPV6:
		case IPPROTO_UDP:
		case IPPROTO_TCP:
			prev = pos;
			pos = *next_hdr_idx = offset - 2;
			goto out;

		default:
			goto fail;
		}

		/* Get the next header value */
		frag = net_frag_read_u8(frag, pos, &tmp, &next);
		if (!frag && pos == 0xffff) {
			goto fail;
		}
	}

out:
	*next_hdr_idx = prev;
	*last_hdr_idx = offset - 2;

	return 0;

fail:
	return -EINVAL;
}

const struct in6_addr *net_ipv6_unspecified_address(void)
{
	static const struct in6_addr addr = IN6ADDR_ANY_INIT;

	return &addr;
}

struct net_pkt *net_ipv6_create_raw(struct net_pkt *pkt,
				    const struct in6_addr *src,
				    const struct in6_addr *dst,
				    struct net_if *iface,
				    u8_t next_header)
{
	struct net_buf *header;

	header = net_pkt_get_frag(pkt, K_FOREVER);

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
	NET_IPV6_HDR(pkt)->nexthdr = next_header;

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);

	net_buf_add(header, sizeof(struct net_ipv6_hdr));

	return pkt;
}

struct net_pkt *net_ipv6_create(struct net_context *context,
				struct net_pkt *pkt,
				const struct in6_addr *src,
				const struct in6_addr *dst)
{
	NET_ASSERT(((struct sockaddr_in6_ptr *)&context->local)->sin6_addr);

	if (!src) {
		src = ((struct sockaddr_in6_ptr *)&context->local)->sin6_addr;
	}

	if (net_is_ipv6_addr_unspecified(src)
	    || net_is_ipv6_addr_mcast(src)) {
		src = net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						  (struct in6_addr *)dst);
	}

	return net_ipv6_create_raw(pkt,
				   src,
				   dst,
				   net_context_get_iface(context),
				   net_context_get_ip_proto(context));
}

int net_ipv6_finalize_raw(struct net_pkt *pkt, u8_t next_header)
{
	/* Set the length of the IPv6 header */
	size_t total_len;

#if defined(CONFIG_NET_UDP) && defined(CONFIG_NET_RPL_INSERT_HBH_OPTION)
	if (next_header != IPPROTO_TCP && next_header != IPPROTO_ICMPV6) {
		/* Check if we need to add RPL header to sent UDP packet. */
		if (net_rpl_insert_header(pkt) < 0) {
			NET_DBG("RPL HBHO insert failed");
			return -EINVAL;
		}
	}
#endif

	net_pkt_compact(pkt);

	total_len = net_pkt_get_len(pkt);

	total_len -= sizeof(struct net_ipv6_hdr);

	NET_IPV6_HDR(pkt)->len[0] = total_len / 256;
	NET_IPV6_HDR(pkt)->len[1] = total_len - NET_IPV6_HDR(pkt)->len[0] * 256;

#if defined(CONFIG_NET_UDP)
	if (next_header == IPPROTO_UDP) {
		NET_UDP_HDR(pkt)->chksum = 0;
		NET_UDP_HDR(pkt)->chksum = ~net_calc_chksum_udp(pkt);
	} else
#endif

#if defined(CONFIG_NET_TCP)
	if (next_header == IPPROTO_TCP) {
		NET_TCP_HDR(pkt)->chksum = 0;
		NET_TCP_HDR(pkt)->chksum = ~net_calc_chksum_tcp(pkt);
	} else
#endif

	if (next_header == IPPROTO_ICMPV6) {
		NET_ICMP_HDR(pkt)->chksum = 0;
		NET_ICMP_HDR(pkt)->chksum = ~net_calc_chksum(pkt,
							     IPPROTO_ICMPV6);
	}

	return 0;
}

int net_ipv6_finalize(struct net_context *context, struct net_pkt *pkt)
{
	return net_ipv6_finalize_raw(pkt, net_context_get_ip_proto(context));
}

#if defined(CONFIG_NET_IPV6_DAD)
int net_ipv6_start_dad(struct net_if *iface, struct net_if_addr *ifaddr)
{
	return net_ipv6_send_ns(iface, NULL, NULL, NULL,
				&ifaddr->address.in6_addr, true);
}

static inline bool dad_failed(struct net_if *iface, struct in6_addr *addr)
{
	if (net_is_ipv6_ll_addr(addr)) {
		NET_ERR("DAD failed, no ll IPv6 address!");
		return false;
	}

	net_if_ipv6_dad_failed(iface, addr);

	return true;
}
#endif /* CONFIG_NET_IPV6_DAD */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)

/* If the reserve has changed, we need to adjust it accordingly in the
 * fragment chain. This can only happen in IEEE 802.15.4 where the link
 * layer header size can change if the destination address changes.
 * Thus we need to check it here. Note that this cannot happen for IPv4
 * as 802.15.4 supports IPv6 only.
 */
static struct net_pkt *update_ll_reserve(struct net_pkt *pkt,
					 struct in6_addr *addr)
{
	/* We need to go through all the fragments and adjust the
	 * fragment data size.
	 */
	u16_t reserve, room_len, copy_len, pos;
	struct net_buf *orig_frag, *frag;

	/* No need to do anything if we are forwarding the packet
	 * as we already know everything about the destination of
	 * the packet.
	 */
	if (net_pkt_forwarding(pkt)) {
		return pkt;
	}

	reserve = net_if_get_ll_reserve(net_pkt_iface(pkt), addr);
	if (reserve == net_pkt_ll_reserve(pkt)) {
		return pkt;
	}

	NET_DBG("Adjust reserve old %d new %d",
		net_pkt_ll_reserve(pkt), reserve);

	net_pkt_set_ll_reserve(pkt, reserve);

	orig_frag = pkt->frags;
	copy_len = orig_frag->len;
	pos = 0;

	pkt->frags = NULL;
	room_len = 0;
	frag = NULL;

	while (orig_frag) {
		if (!room_len) {
			frag = net_pkt_get_frag(pkt, K_FOREVER);

			net_pkt_frag_add(pkt, frag);

			room_len = net_buf_tailroom(frag);
		}

		if (room_len >= copy_len) {
			memcpy(net_buf_add(frag, copy_len),
			       orig_frag->data + pos, copy_len);

			room_len -= copy_len;
			copy_len = 0;
		} else {
			memcpy(net_buf_add(frag, room_len),
			       orig_frag->data + pos, room_len);

			copy_len -= room_len;
			pos += room_len;
			room_len = 0;
		}

		if (!copy_len) {
			struct net_buf *tmp = orig_frag;

			orig_frag = orig_frag->frags;

			tmp->frags = NULL;
			net_pkt_frag_unref(tmp);

			if (!orig_frag) {
				break;
			}

			copy_len = orig_frag->len;
			pos = 0;
		}
	}

	return pkt;
}

struct net_pkt *net_ipv6_prepare_for_send(struct net_pkt *pkt)
{
	struct in6_addr *nexthop = NULL;
	struct net_if *iface = NULL;
	struct net_nbr *nbr;

	NET_ASSERT(pkt && pkt->frags);

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	/* If we have already fragmented the packet, the fragment id will
	 * contain a proper value and we can skip other checks.
	 */
	if (net_pkt_ipv6_fragment_id(pkt) == 0) {
		size_t pkt_len = net_pkt_get_len(pkt);

		if (pkt_len > NET_IPV6_MTU) {
			int ret;

			ret = net_ipv6_send_fragmented_pkt(net_pkt_iface(pkt),
							   pkt, pkt_len);
			if (ret < 0) {
				NET_DBG("Cannot fragment IPv6 pkt (%d)", ret);
				net_pkt_unref(pkt);
			}

			/* No need to continue with the sending as the packet
			 * is now split and its fragments will be sent
			 * separately to network.
			 */
			return NULL;
		}
	}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

	/* Workaround Linux bug, see:
	 * https://jira.zephyrproject.org/browse/ZEP-1656
	 */
	if (atomic_test_bit(net_pkt_iface(pkt)->flags, NET_IF_POINTOPOINT)) {
		return pkt;
	}

	if (net_pkt_ll_dst(pkt)->addr ||
	    net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
		return update_ll_reserve(pkt, &NET_IPV6_HDR(pkt)->dst);
	}

	if (net_if_ipv6_addr_onlink(&iface,
				    &NET_IPV6_HDR(pkt)->dst)) {
		nexthop = &NET_IPV6_HDR(pkt)->dst;
		net_pkt_set_iface(pkt, iface);
	} else {
		/* We need to figure out where the destination
		 * host is located.
		 */
		struct net_route_entry *route;
		struct net_if_router *router;

		route = net_route_lookup(NULL, &NET_IPV6_HDR(pkt)->dst);
		if (route) {
			nexthop = net_route_get_nexthop(route);
			if (!nexthop) {
				net_route_del(route);

				net_rpl_global_repair(route);

				NET_DBG("No route to host %s",
					net_sprint_ipv6_addr(
						&NET_IPV6_HDR(pkt)->dst));

				net_pkt_unref(pkt);
				return NULL;
			}
		} else {
			/* No specific route to this host, use the default
			 * route instead.
			 */
			router = net_if_ipv6_router_find_default(NULL,
						&NET_IPV6_HDR(pkt)->dst);
			if (!router) {
				NET_DBG("No default route to %s",
					net_sprint_ipv6_addr(
						&NET_IPV6_HDR(pkt)->dst));

				/* Try to send the packet anyway */
				nexthop = &NET_IPV6_HDR(pkt)->dst;
				goto try_send;
			}

			nexthop = &router->address.in6_addr;
		}
	}

	if (net_rpl_update_header(pkt, nexthop) < 0) {
		net_pkt_unref(pkt);
		return NULL;
	}

	if (!iface) {
		/* This means that the dst was not onlink, so try to
		 * figure out the interface using nexthop instead.
		 */
		if (net_if_ipv6_addr_onlink(&iface, nexthop)) {
			net_pkt_set_iface(pkt, iface);
		}

		/* If the above check returns null, we try to send
		 * the packet and hope for the best.
		 */
	}

try_send:
	nbr = nbr_lookup(&net_neighbor.table, net_pkt_iface(pkt), nexthop);

	NET_DBG("Neighbor lookup %p (%d) iface %p addr %s state %s", nbr,
		nbr ? nbr->idx : NET_NBR_LLADDR_UNKNOWN,
		net_pkt_iface(pkt),
		net_sprint_ipv6_addr(nexthop),
		nbr ? net_ipv6_nbr_state2str(net_ipv6_nbr_data(nbr)->state) :
		"-");

	if (nbr && nbr->idx != NET_NBR_LLADDR_UNKNOWN) {
		struct net_linkaddr_storage *lladdr;

		lladdr = net_nbr_get_lladdr(nbr->idx);

		net_pkt_ll_dst(pkt)->addr = lladdr->addr;
		net_pkt_ll_dst(pkt)->len = lladdr->len;

		NET_DBG("Neighbor %p addr %s", nbr,
			net_sprint_ll_addr(lladdr->addr, lladdr->len));

		/* Start the NUD if we are in STALE state.
		 * See RFC 4861 ch 7.3.3 for details.
		 */
#if defined(CONFIG_NET_IPV6_ND)
		if (net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_STALE) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_DELAY);

			k_delayed_work_submit(
				&net_ipv6_nbr_data(nbr)->reachable,
				DELAY_FIRST_PROBE_TIME);
		}
#endif

		return update_ll_reserve(pkt, nexthop);
	}

#if defined(CONFIG_NET_IPV6_ND)
	/* We need to send NS and wait for NA before sending the packet. */
	if (net_ipv6_send_ns(net_pkt_iface(pkt),
			     pkt,
			     &NET_IPV6_HDR(pkt)->src,
			     NULL,
			     nexthop,
			     false) < 0) {
		/* In case of an error, the NS send function will unref
		 * the pkt.
		 */
		return NULL;
	}

	NET_DBG("pkt %p (frag %p) will be sent later", pkt, pkt->frags);
#else
	NET_DBG("pkt %p (frag %p) cannot be sent, dropping it.", pkt,
		pkt->frags);

	net_pkt_unref(pkt);
#endif /* CONFIG_NET_IPV6_ND */

	return NULL;
}

struct net_nbr *net_ipv6_nbr_lookup(struct net_if *iface,
				    struct in6_addr *addr)
{
	return nbr_lookup(&net_neighbor.table, iface, addr);
}

struct net_nbr *net_ipv6_get_nbr(struct net_if *iface, u8_t idx)
{
	int i;

	if (idx == NET_NBR_LLADDR_UNKNOWN) {
		return NULL;
	}

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (nbr->ref) {
			if (iface && nbr->iface != iface) {
				continue;
			}

			if (nbr->idx == idx) {
				return nbr;
			}
		}
	}

	return NULL;
}

static inline u8_t get_llao_len(struct net_if *iface)
{
	if (iface->link_addr.len == 6) {
		return 8;
	} else if (iface->link_addr.len == 8) {
		return 16;
	}

	/* What else could it be? */
	NET_ASSERT_INFO(0, "Invalid link address length %d",
			iface->link_addr.len);

	return 0;
}

static inline void set_llao(struct net_linkaddr *lladdr,
			    u8_t *llao, u8_t llao_len, u8_t type)
{
	llao[NET_ICMPV6_OPT_TYPE_OFFSET] = type;
	llao[NET_ICMPV6_OPT_LEN_OFFSET] = llao_len >> 3;

	memcpy(&llao[NET_ICMPV6_OPT_DATA_OFFSET], lladdr->addr, lladdr->len);

	memset(&llao[NET_ICMPV6_OPT_DATA_OFFSET + lladdr->len], 0,
	       llao_len - lladdr->len - 2);
}

static void setup_headers(struct net_pkt *pkt, u8_t nd6_len,
			  u8_t icmp_type)
{
	NET_IPV6_HDR(pkt)->vtc = 0x60;
	NET_IPV6_HDR(pkt)->tcflow = 0;
	NET_IPV6_HDR(pkt)->flow = 0;
	NET_IPV6_HDR(pkt)->len[0] = 0;
	NET_IPV6_HDR(pkt)->len[1] = NET_ICMPH_LEN + nd6_len;

	NET_IPV6_HDR(pkt)->nexthdr = IPPROTO_ICMPV6;
	NET_IPV6_HDR(pkt)->hop_limit = NET_IPV6_ND_HOP_LIMIT;

	NET_ICMP_HDR(pkt)->type = icmp_type;
	NET_ICMP_HDR(pkt)->code = 0;
}

static inline void handle_ns_neighbor(struct net_pkt *pkt,
				      struct net_icmpv6_nd_opt_hdr *hdr)
{
	struct net_linkaddr lladdr = {
		.len = 8 * hdr->len - 2,
		.addr = (u8_t *)hdr + 2,
	};

	/**
	 * IEEE802154 lladdress is 8 bytes long, so it requires
	 * 2 * 8 bytes - 2 - padding.
	 * The formula above needs to be adjusted.
	 */
	if (net_pkt_ll_src(pkt)->len < lladdr.len) {
		lladdr.len = net_pkt_ll_src(pkt)->len;
	}

	nbr_add(pkt, &lladdr, false, NET_IPV6_NBR_STATE_INCOMPLETE);
}

int net_ipv6_send_na(struct net_if *iface, struct in6_addr *src,
		     struct in6_addr *dst, struct in6_addr *tgt,
		     u8_t flags)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	u8_t llao_len;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				      K_FOREVER);

	NET_ASSERT_INFO(pkt, "Out of TX packets");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	NET_ASSERT_INFO(frag, "Out of DATA buffers");

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	llao_len = get_llao_len(iface);

	net_pkt_set_ipv6_ext_len(pkt, 0);

	setup_headers(pkt, sizeof(struct net_icmpv6_na_hdr) + llao_len,
		      NET_ICMPV6_NA);

	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, src);
	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, dst);
	net_ipaddr_copy(&NET_ICMPV6_NA_HDR(pkt)->tgt, tgt);

	set_llao(&net_pkt_iface(pkt)->link_addr,
		 net_pkt_icmp_data(pkt) + sizeof(struct net_icmp_hdr) +
					sizeof(struct net_icmpv6_na_hdr),
		 llao_len, NET_ICMPV6_ND_OPT_TLLAO);

	NET_ICMPV6_NA_HDR(pkt)->flags = flags;

	pkt->frags->len = NET_IPV6ICMPH_LEN +
		sizeof(struct net_icmpv6_na_hdr) + llao_len;

	NET_ICMP_HDR(pkt)->chksum = 0;
	NET_ICMP_HDR(pkt)->chksum = ~net_calc_chksum_icmpv6(pkt);

	dbg_addr_sent_tgt("Neighbor Advertisement",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &NET_ICMPV6_NS_HDR(pkt)->tgt);

	if (net_send_data(pkt) < 0) {
		goto drop;
	}

	net_stats_update_ipv6_nd_sent();

	return 0;

drop:
	net_pkt_unref(pkt);
	net_stats_update_ipv6_nd_drop();

	return -EINVAL;
}

static enum net_verdict handle_ns_input(struct net_pkt *pkt)
{
	u16_t total_len = net_pkt_get_len(pkt);
	struct net_icmpv6_nd_opt_hdr *hdr;
	struct net_if_addr *ifaddr;
	u8_t flags = 0, prev_opt_len = 0;
	int ret;
	size_t left_len;

	dbg_addr_recv_tgt("Neighbor Solicitation",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &NET_ICMPV6_NS_HDR(pkt)->tgt);

	net_stats_update_ipv6_nd_recv();

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_ns_hdr))) ||
	    (NET_ICMP_HDR(pkt)->code != 0) ||
	    (NET_IPV6_HDR(pkt)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    net_is_ipv6_addr_mcast(&NET_ICMPV6_NS_HDR(pkt)->tgt)) {
		NET_DBG("Preliminary check failed %u/%zu, code %u, hop %u",
			total_len, (sizeof(struct net_ipv6_hdr) +
				    sizeof(struct net_icmp_hdr) +
				    sizeof(struct net_icmpv6_ns_hdr)),
			NET_ICMP_HDR(pkt)->code, NET_IPV6_HDR(pkt)->hop_limit);
		goto drop;
	}

	net_pkt_set_ipv6_ext_opt_len(pkt, sizeof(struct net_icmpv6_ns_hdr));
	hdr = NET_ICMPV6_ND_OPT_HDR_HDR(pkt);

	/* The parsing gets tricky if the ND struct is split
	 * between two fragments. FIXME later.
	 */
	if (pkt->frags->len < ((u8_t *)hdr - pkt->frags->data)) {
		NET_DBG("NS struct split between fragments");
		goto drop;
	}

	left_len = pkt->frags->len - (sizeof(struct net_ipv6_hdr) +
				      sizeof(struct net_icmp_hdr));

	while (net_pkt_ipv6_ext_opt_len(pkt) < left_len &&
	       left_len < pkt->frags->len) {

		if (!hdr->len) {
			break;
		}

		switch (hdr->type) {
		case NET_ICMPV6_ND_OPT_SLLAO:
			if (net_is_ipv6_addr_unspecified(
				    &NET_IPV6_HDR(pkt)->src)) {
				goto drop;
			}

			handle_ns_neighbor(pkt, hdr);
			break;

		default:
			NET_DBG("Unknown ND option 0x%x", hdr->type);
			break;
		}

		prev_opt_len = net_pkt_ipv6_ext_opt_len(pkt);

		net_pkt_set_ipv6_ext_opt_len(pkt,
					     net_pkt_ipv6_ext_opt_len(pkt) +
					     (hdr->len << 3));

		if (prev_opt_len >= net_pkt_ipv6_ext_opt_len(pkt)) {
			NET_ERR("Corrupted NS message");
			goto drop;
		}

		hdr = NET_ICMPV6_ND_OPT_HDR_HDR(pkt);
	}

	ifaddr = net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
						  &NET_ICMPV6_NS_HDR(pkt)->tgt);
	if (!ifaddr) {
		NET_DBG("No such interface address %s",
			net_sprint_ipv6_addr(&NET_ICMPV6_NS_HDR(pkt)->tgt));
		goto drop;
	}

#if !defined(CONFIG_NET_IPV6_DAD)
	if (net_is_ipv6_addr_unspecified(&NET_IPV6_HDR(pkt)->src)) {
		goto drop;
	}

#else /* CONFIG_NET_IPV6_DAD */

	/* Do DAD */
	if (net_is_ipv6_addr_unspecified(&NET_IPV6_HDR(pkt)->src)) {

		if (!net_is_ipv6_addr_solicited_node(&NET_IPV6_HDR(pkt)->dst)) {
			NET_DBG("Not solicited node addr %s",
				net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
			goto drop;
		}

		if (ifaddr->addr_state == NET_ADDR_TENTATIVE) {
			NET_DBG("DAD failed for %s iface %p",
				net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
				net_pkt_iface(pkt));

			dad_failed(net_pkt_iface(pkt),
				   &ifaddr->address.in6_addr);
			goto drop;
		}

		/* We reuse the received packet to send the NA */
		net_ipv6_addr_create_ll_allnodes_mcast(&NET_IPV6_HDR(pkt)->dst);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						&NET_IPV6_HDR(pkt)->dst));
		flags = NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	}
#endif /* CONFIG_NET_IPV6_DAD */

	if (net_is_my_ipv6_addr(&NET_IPV6_HDR(pkt)->src)) {
		NET_DBG("Duplicate IPv6 %s address",
			net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src));
		goto drop;
	}

	/* Address resolution */
	if (net_is_ipv6_addr_solicited_node(&NET_IPV6_HDR(pkt)->dst)) {
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst,
				&NET_IPV6_HDR(pkt)->src);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				&NET_ICMPV6_NS_HDR(pkt)->tgt);
		flags = NET_ICMPV6_NA_FLAG_SOLICITED |
			NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	}

	/* Neighbor Unreachability Detection (NUD) */
	if (net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
					     &NET_IPV6_HDR(pkt)->dst)) {
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst,
				&NET_IPV6_HDR(pkt)->src);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				&NET_ICMPV6_NS_HDR(pkt)->tgt);
		flags = NET_ICMPV6_NA_FLAG_SOLICITED |
			NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	} else {
		NET_DBG("NUD failed");
		goto drop;
	}

send_na:
	ret = net_ipv6_send_na(net_pkt_iface(pkt),
			       &NET_IPV6_HDR(pkt)->src,
			       &NET_IPV6_HDR(pkt)->dst,
			       &ifaddr->address.in6_addr,
			       flags);
	if (!ret) {
		net_pkt_unref(pkt);
		return NET_OK;
	}

	return NET_DROP;

drop:
	net_stats_update_ipv6_nd_drop();

	return NET_DROP;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
static void nd_reachable_timeout(struct k_work *work)
{
	struct net_ipv6_nbr_data *data = CONTAINER_OF(work,
						      struct net_ipv6_nbr_data,
						      reachable);

	struct net_nbr *nbr = get_nbr_from_data(data);

	if (!data || !nbr) {
		NET_DBG("ND reachable timeout but no nbr data "
			"(nbr %p data %p)", nbr, data);
		return;
	}

	switch (data->state) {

	case NET_IPV6_NBR_STATE_INCOMPLETE:
		if (data->ns_count >= MAX_MULTICAST_SOLICIT) {
			nbr_free(nbr);
		} else {
			data->ns_count++;

			NET_DBG("nbr %p incomplete count %u", nbr,
				data->ns_count);

			net_ipv6_send_ns(nbr->iface, NULL, NULL, NULL,
					 &data->addr, false);
		}
		break;

	case NET_IPV6_NBR_STATE_REACHABLE:
		data->state = NET_IPV6_NBR_STATE_STALE;

		NET_DBG("nbr %p moving %s state to STALE (%d)",
			nbr, net_sprint_ipv6_addr(&data->addr), data->state);
		break;

	case NET_IPV6_NBR_STATE_STALE:
		NET_DBG("nbr %p removing stale address %s",
			nbr, net_sprint_ipv6_addr(&data->addr));
		nbr_free(nbr);
		break;

	case NET_IPV6_NBR_STATE_DELAY:
		data->state = NET_IPV6_NBR_STATE_PROBE;
		data->ns_count = 0;

		NET_DBG("nbr %p moving %s state to PROBE (%d)",
			nbr, net_sprint_ipv6_addr(&data->addr), data->state);

		/* Intentionally continuing to probe state */

	case NET_IPV6_NBR_STATE_PROBE:
		if (data->ns_count >= MAX_UNICAST_SOLICIT) {
			struct net_if_router *router;

			router = net_if_ipv6_router_lookup(nbr->iface,
							   &data->addr);
			if (router && !router->is_infinite) {
				NET_DBG("nbr %p address %s PROBE ended (%d)",
					nbr, net_sprint_ipv6_addr(&data->addr),
					data->state);

				net_if_ipv6_router_rm(router);
				nbr_free(nbr);
			}
		} else {
			data->ns_count++;

			NET_DBG("nbr %p probe count %u", nbr,
				data->ns_count);

			net_ipv6_send_ns(nbr->iface, NULL, NULL, NULL,
					 &data->addr, false);

			k_delayed_work_submit(
				&net_ipv6_nbr_data(nbr)->reachable,
				RETRANS_TIMER);
		}
		break;
	}
}

void net_ipv6_nbr_set_reachable_timer(struct net_if *iface, struct net_nbr *nbr)
{
	u32_t time;

	time = net_if_ipv6_get_reachable_time(iface);

	NET_ASSERT_INFO(time, "Zero reachable timeout!");

	NET_DBG("Starting reachable timer nbr %p data %p time %d ms",
		nbr, net_ipv6_nbr_data(nbr), time);

	k_delayed_work_submit(&net_ipv6_nbr_data(nbr)->reachable, time);
}
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static inline bool handle_na_neighbor(struct net_pkt *pkt,
				      struct net_icmpv6_nd_opt_hdr *hdr,
				      u8_t *tllao)
{
	bool lladdr_changed = false;
	struct net_nbr *nbr;
	struct net_linkaddr_storage *cached_lladdr;
	struct net_pkt *pending;

	ARG_UNUSED(hdr);

	nbr = nbr_lookup(&net_neighbor.table, net_pkt_iface(pkt),
			 &NET_ICMPV6_NS_HDR(pkt)->tgt);

	NET_DBG("Neighbor lookup %p iface %p addr %s", nbr,
		net_pkt_iface(pkt),
		net_sprint_ipv6_addr(&NET_ICMPV6_NS_HDR(pkt)->tgt));

	if (!nbr) {
		nbr_print();

		NET_DBG("No such neighbor found, msg discarded");
		return false;
	}

	if (nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
		struct net_linkaddr lladdr;

		if (!tllao) {
			NET_DBG("No target link layer address.");
			return false;
		}

		lladdr.len = net_pkt_iface(pkt)->link_addr.len;
		lladdr.addr = &tllao[NET_ICMPV6_OPT_DATA_OFFSET];

		if (net_nbr_link(nbr, net_pkt_iface(pkt), &lladdr)) {
			nbr_free(nbr);
			return false;
		}

		NET_DBG("[%d] nbr %p state %d IPv6 %s ll %s",
			nbr->idx, nbr, net_ipv6_nbr_data(nbr)->state,
			net_sprint_ipv6_addr(&NET_ICMPV6_NS_HDR(pkt)->tgt),
			net_sprint_ll_addr(lladdr.addr, lladdr.len));
	}

	cached_lladdr = net_nbr_get_lladdr(nbr->idx);
	if (!cached_lladdr) {
		NET_DBG("No lladdr but index defined");
		return false;
	}

	if (tllao) {
		lladdr_changed = memcmp(&tllao[NET_ICMPV6_OPT_DATA_OFFSET],
					cached_lladdr->addr,
					cached_lladdr->len);
	}

	/* Update the cached address if we do not yet known it */
	if (net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_INCOMPLETE) {
		if (!tllao) {
			return false;
		}

		if (lladdr_changed) {
			dbg_update_neighbor_lladdr_raw(
				&tllao[NET_ICMPV6_OPT_DATA_OFFSET],
				cached_lladdr,
				&NET_ICMPV6_NS_HDR(pkt)->tgt);

			net_linkaddr_set(cached_lladdr,
					 &tllao[NET_ICMPV6_OPT_DATA_OFFSET],
					 cached_lladdr->len);
		}

		if (net_is_solicited(pkt)) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);
			net_ipv6_nbr_data(nbr)->ns_count = 0;

			/* We might have active timer from PROBE */
			k_delayed_work_cancel(
				&net_ipv6_nbr_data(nbr)->reachable);

			net_ipv6_nbr_set_reachable_timer(net_pkt_iface(pkt),
							 nbr);
		} else {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		}

		net_ipv6_nbr_data(nbr)->is_router = net_is_router(pkt);

		goto send_pending;
	}

	/* We do not update the address if override bit is not set
	 * and we have a valid address in the cache.
	 */
	if (!net_is_override(pkt) && lladdr_changed) {
		if (net_ipv6_nbr_data(nbr)->state ==
		    NET_IPV6_NBR_STATE_REACHABLE) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		}

		return false;
	}

	if (net_is_override(pkt) ||
	    (!net_is_override(pkt) && tllao && !lladdr_changed)) {

		if (lladdr_changed) {
			dbg_update_neighbor_lladdr_raw(
				&tllao[NET_ICMPV6_OPT_DATA_OFFSET],
				cached_lladdr,
				&NET_ICMPV6_NS_HDR(pkt)->tgt);

			net_linkaddr_set(cached_lladdr,
					 &tllao[NET_ICMPV6_OPT_DATA_OFFSET],
					 cached_lladdr->len);
		}

		if (net_is_solicited(pkt)) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);

			/* We might have active timer from PROBE */
			k_delayed_work_cancel(
				&net_ipv6_nbr_data(nbr)->reachable);

			net_ipv6_nbr_set_reachable_timer(net_pkt_iface(pkt),
							 nbr);
		} else {
			if (lladdr_changed) {
				ipv6_nbr_set_state(nbr,
						   NET_IPV6_NBR_STATE_STALE);
			}
		}
	}

	if (net_ipv6_nbr_data(nbr)->is_router && !net_is_router(pkt)) {
		/* Update the routing if the peer is no longer
		 * a router.
		 */
		/* FIXME */
	}

	net_ipv6_nbr_data(nbr)->is_router = net_is_router(pkt);

send_pending:
	/* Next send any pending messages to the peer. */
	pending = net_ipv6_nbr_data(nbr)->pending;

	if (pending) {
		NET_DBG("Sending pending %p to %s lladdr %s", pending,
			net_sprint_ipv6_addr(&NET_IPV6_HDR(pending)->dst),
			net_sprint_ll_addr(cached_lladdr->addr,
					   cached_lladdr->len));

		if (net_send_data(pending) < 0) {
			nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
		} else {
			net_ipv6_nbr_data(nbr)->pending = NULL;
		}

		net_pkt_unref(pending);
	}

	return true;
}

static enum net_verdict handle_na_input(struct net_pkt *pkt)
{
	u16_t total_len = net_pkt_get_len(pkt);
	struct net_icmpv6_nd_opt_hdr *hdr;
	struct net_if_addr *ifaddr;
	u8_t *tllao = NULL;
	u8_t prev_opt_len = 0;
	size_t left_len;

	dbg_addr_recv_tgt("Neighbor Advertisement",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &NET_ICMPV6_NS_HDR(pkt)->tgt);

	net_stats_update_ipv6_nd_recv();

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_na_hdr) +
			  sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	    (NET_ICMP_HDR(pkt)->code != 0) ||
	    (NET_IPV6_HDR(pkt)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    net_is_ipv6_addr_mcast(&NET_ICMPV6_NS_HDR(pkt)->tgt) ||
	    (net_is_solicited(pkt) &&
	     net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst))) {
		goto drop;
	}

	net_pkt_set_ipv6_ext_opt_len(pkt, sizeof(struct net_icmpv6_na_hdr));
	hdr = NET_ICMPV6_ND_OPT_HDR_HDR(pkt);

	/* The parsing gets tricky if the ND struct is split
	 * between two fragments. FIXME later.
	 */
	if (pkt->frags->len < ((u8_t *)hdr - pkt->frags->data)) {
		NET_DBG("NA struct split between fragments");
		goto drop;
	}

	left_len = pkt->frags->len - (sizeof(struct net_ipv6_hdr) +
				      sizeof(struct net_icmp_hdr));

	while (net_pkt_ipv6_ext_opt_len(pkt) < left_len &&
	       left_len < pkt->frags->len) {

		if (!hdr->len) {
			break;
		}

		switch (hdr->type) {
		case NET_ICMPV6_ND_OPT_TLLAO:
			tllao = (u8_t *)hdr;
			break;

		default:
			NET_DBG("Unknown ND option 0x%x", hdr->type);
			break;
		}

		prev_opt_len = net_pkt_ipv6_ext_opt_len(pkt);

		net_pkt_set_ipv6_ext_opt_len(pkt,
					     net_pkt_ipv6_ext_opt_len(pkt) +
					     (hdr->len << 3));

		if (prev_opt_len >= net_pkt_ipv6_ext_opt_len(pkt)) {
			NET_ERR("Corrupted NA message");
			goto drop;
		}

		hdr = NET_ICMPV6_ND_OPT_HDR_HDR(pkt);
	}

	ifaddr = net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
						  &NET_ICMPV6_NA_HDR(pkt)->tgt);
	if (ifaddr) {
		NET_DBG("Interface %p already has address %s",
			net_pkt_iface(pkt),
			net_sprint_ipv6_addr(&NET_ICMPV6_NA_HDR(pkt)->tgt));

#if defined(CONFIG_NET_IPV6_DAD)
		if (ifaddr->addr_state == NET_ADDR_TENTATIVE) {
			dad_failed(net_pkt_iface(pkt),
				   &NET_ICMPV6_NA_HDR(pkt)->tgt);
		}
#endif /* CONFIG_NET_IPV6_DAD */

		goto drop;
	}

	if (!handle_na_neighbor(pkt, hdr, tllao)) {
		goto drop;
	}

	net_pkt_unref(pkt);

	net_stats_update_ipv6_nd_sent();

	return NET_OK;

drop:
	net_stats_update_ipv6_nd_drop();

	return NET_DROP;
}

int net_ipv6_send_ns(struct net_if *iface,
		     struct net_pkt *pending,
		     struct in6_addr *src,
		     struct in6_addr *dst,
		     struct in6_addr *tgt,
		     bool is_my_address)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_nbr *nbr;
	u8_t llao_len;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				      K_FOREVER);

	NET_ASSERT_INFO(pkt, "Out of TX packets");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	NET_ASSERT_INFO(frag, "Out of DATA buffers");

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	llao_len = get_llao_len(net_pkt_iface(pkt));

	setup_headers(pkt, sizeof(struct net_icmpv6_ns_hdr) + llao_len,
		      NET_ICMPV6_NS);

	if (!dst) {
		net_ipv6_addr_create_solicited_node(tgt,
						    &NET_IPV6_HDR(pkt)->dst);
	} else {
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, dst);
	}

	NET_ICMPV6_NS_HDR(pkt)->reserved = 0;

	net_ipaddr_copy(&NET_ICMPV6_NS_HDR(pkt)->tgt, tgt);

	if (is_my_address) {
		/* DAD */
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				net_ipv6_unspecified_address());

		NET_IPV6_HDR(pkt)->len[1] -= llao_len;

		net_buf_add(frag,
			    sizeof(struct net_ipv6_hdr) +
			    sizeof(struct net_icmp_hdr) +
			    sizeof(struct net_icmpv6_ns_hdr));
	} else {
		if (src) {
			net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, src);
		} else {
			net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
					net_if_ipv6_select_src_addr(
						net_pkt_iface(pkt),
						&NET_IPV6_HDR(pkt)->dst));
		}

		if (net_is_ipv6_addr_unspecified(&NET_IPV6_HDR(pkt)->src)) {
			NET_DBG("No source address for NS");
			goto drop;
		}

		set_llao(&net_pkt_iface(pkt)->link_addr,
			 net_pkt_icmp_data(pkt) +
			 sizeof(struct net_icmp_hdr) +
			 sizeof(struct net_icmpv6_ns_hdr),
			 llao_len, NET_ICMPV6_ND_OPT_SLLAO);

		net_buf_add(frag,
			    sizeof(struct net_ipv6_hdr) +
			    sizeof(struct net_icmp_hdr) +
			    sizeof(struct net_icmpv6_ns_hdr) + llao_len);
	}

	NET_ICMP_HDR(pkt)->chksum = 0;
	NET_ICMP_HDR(pkt)->chksum = ~net_calc_chksum_icmpv6(pkt);

	nbr = nbr_lookup(&net_neighbor.table, net_pkt_iface(pkt),
			 &NET_ICMPV6_NS_HDR(pkt)->tgt);
	if (!nbr) {
		nbr_print();

		nbr = nbr_new(net_pkt_iface(pkt),
			      &NET_ICMPV6_NS_HDR(pkt)->tgt, false,
			      NET_IPV6_NBR_STATE_INCOMPLETE);
		if (!nbr) {
			NET_DBG("Could not create new neighbor %s",
				net_sprint_ipv6_addr(
						&NET_ICMPV6_NS_HDR(pkt)->tgt));
			goto drop;
		}
	}

	if (pending) {
		if (!net_ipv6_nbr_data(nbr)->pending) {
			net_ipv6_nbr_data(nbr)->pending = net_pkt_ref(pending);
		} else {
			NET_DBG("Packet %p already pending for "
				"operation. Discarding pending %p and pkt %p",
				net_ipv6_nbr_data(nbr)->pending, pending, pkt);
			net_pkt_unref(pending);
			goto drop;
		}

		NET_DBG("Setting timeout %d for NS", NS_REPLY_TIMEOUT);

		k_delayed_work_submit(&net_ipv6_nbr_data(nbr)->send_ns,
				      NS_REPLY_TIMEOUT);
	}

	dbg_addr_sent_tgt("Neighbor Solicitation",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &NET_ICMPV6_NS_HDR(pkt)->tgt);

	if (net_send_data(pkt) < 0) {
		NET_DBG("Cannot send NS %p (pending %p)", pkt, pending);

		if (pending) {
			nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
		}

		goto drop;
	}

	net_stats_update_ipv6_nd_sent();

	return 0;

drop:
	net_pkt_unref(pkt);
	net_stats_update_ipv6_nd_drop();

	return -EINVAL;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
int net_ipv6_send_rs(struct net_if *iface)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	bool unspec_src;
	u8_t llao_len = 0;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	net_ipv6_addr_create_ll_allnodes_mcast(&NET_IPV6_HDR(pkt)->dst);

	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
			net_if_ipv6_select_src_addr(iface,
						    &NET_IPV6_HDR(pkt)->dst));

	unspec_src = net_is_ipv6_addr_unspecified(&NET_IPV6_HDR(pkt)->src);
	if (!unspec_src) {
		llao_len = get_llao_len(net_pkt_iface(pkt));
	}

	setup_headers(pkt, sizeof(struct net_icmpv6_rs_hdr) + llao_len,
		      NET_ICMPV6_RS);

	if (!unspec_src) {
		set_llao(&net_pkt_iface(pkt)->link_addr,
			 net_pkt_icmp_data(pkt) +
			 sizeof(struct net_icmp_hdr) +
			 sizeof(struct net_icmpv6_rs_hdr),
			 llao_len, NET_ICMPV6_ND_OPT_SLLAO);

		net_buf_add(frag, sizeof(struct net_ipv6_hdr) +
			    sizeof(struct net_icmp_hdr) +
			    sizeof(struct net_icmpv6_rs_hdr) +
			    llao_len);
	} else {
		net_buf_add(frag, sizeof(struct net_ipv6_hdr) +
			    sizeof(struct net_icmp_hdr) +
			    sizeof(struct net_icmpv6_rs_hdr));
	}

	NET_ICMP_HDR(pkt)->chksum = 0;
	NET_ICMP_HDR(pkt)->chksum = ~net_calc_chksum_icmpv6(pkt);

	dbg_addr_sent("Router Solicitation",
		      &NET_IPV6_HDR(pkt)->src,
		      &NET_IPV6_HDR(pkt)->dst);

	if (net_send_data(pkt) < 0) {
		goto drop;
	}

	net_stats_update_ipv6_nd_sent();

	return 0;

drop:
	net_pkt_unref(pkt);
	net_stats_update_ipv6_nd_drop();

	return -EINVAL;
}

int net_ipv6_start_rs(struct net_if *iface)
{
	return net_ipv6_send_rs(iface);
}

static inline struct net_buf *handle_ra_neighbor(struct net_pkt *pkt,
						 struct net_buf *frag,
						 u8_t len,
						 u16_t offset, u16_t *pos,
						 struct net_nbr **nbr)

{
	struct net_linkaddr lladdr;
	struct net_linkaddr_storage llstorage;
	u8_t padding;

	if (!nbr) {
		return NULL;
	}

	llstorage.len = NET_LINK_ADDR_MAX_LENGTH;
	lladdr.len = NET_LINK_ADDR_MAX_LENGTH;
	lladdr.addr = llstorage.addr;
	if (net_pkt_ll_src(pkt)->len < lladdr.len) {
		lladdr.len = net_pkt_ll_src(pkt)->len;
	}

	frag = net_frag_read(frag, offset, pos, lladdr.len, lladdr.addr);
	if (!frag && offset) {
		return NULL;
	}

	padding = len * 8 - 2 - lladdr.len;
	if (padding) {
		frag = net_frag_read(frag, *pos, pos, padding, NULL);
		if (!frag && *pos) {
			return NULL;
		}
	}

	*nbr = nbr_add(pkt, &lladdr, true, NET_IPV6_NBR_STATE_STALE);

	return frag;
}

static inline void handle_prefix_onlink(struct net_pkt *pkt,
			struct net_icmpv6_nd_opt_prefix_info *prefix_info)
{
	struct net_if_ipv6_prefix *prefix;

	prefix = net_if_ipv6_prefix_lookup(net_pkt_iface(pkt),
					   &prefix_info->prefix,
					   prefix_info->prefix_len);
	if (!prefix) {
		if (!prefix_info->valid_lifetime) {
			return;
		}

		prefix = net_if_ipv6_prefix_add(net_pkt_iface(pkt),
						&prefix_info->prefix,
						prefix_info->prefix_len,
						prefix_info->valid_lifetime);
		if (prefix) {
			NET_DBG("Interface %p add prefix %s/%d lifetime %u",
				net_pkt_iface(pkt),
				net_sprint_ipv6_addr(&prefix_info->prefix),
				prefix_info->prefix_len,
				prefix_info->valid_lifetime);
		} else {
			NET_ERR("Prefix %s/%d could not be added to iface %p",
				net_sprint_ipv6_addr(&prefix_info->prefix),
				prefix_info->prefix_len,
				net_pkt_iface(pkt));

			return;
		}
	}

	switch (prefix_info->valid_lifetime) {
	case 0:
		NET_DBG("Interface %p delete prefix %s/%d",
			net_pkt_iface(pkt),
			net_sprint_ipv6_addr(&prefix_info->prefix),
			prefix_info->prefix_len);

		net_if_ipv6_prefix_rm(net_pkt_iface(pkt),
				      &prefix->prefix,
				      prefix->len);
		break;

	case NET_IPV6_ND_INFINITE_LIFETIME:
		NET_DBG("Interface %p prefix %s/%d infinite",
			net_pkt_iface(pkt),
			net_sprint_ipv6_addr(&prefix->prefix),
			prefix->len);

		net_if_ipv6_prefix_set_lf(prefix, true);
		break;

	default:
		NET_DBG("Interface %p update prefix %s/%u lifetime %u",
			net_pkt_iface(pkt),
			net_sprint_ipv6_addr(&prefix_info->prefix),
			prefix_info->prefix_len,
			prefix_info->valid_lifetime);

		net_if_ipv6_prefix_set_lf(prefix, false);
		net_if_ipv6_prefix_set_timer(prefix,
					prefix_info->valid_lifetime);
		break;
	}
}

#define TWO_HOURS (2 * 60 * 60)

static inline u32_t remaining(struct k_delayed_work *work)
{
	return k_delayed_work_remaining_get(work) / MSEC_PER_SEC;
}

static inline void handle_prefix_autonomous(struct net_pkt *pkt,
			struct net_icmpv6_nd_opt_prefix_info *prefix_info)
{
	struct in6_addr addr = { };
	struct net_if_addr *ifaddr;

	/* Create IPv6 address using the given prefix and iid. We first
	 * setup link local address, and then copy prefix over first 8
	 * bytes of that address.
	 */
	net_ipv6_addr_create_iid(&addr,
				 net_if_get_link_addr(net_pkt_iface(pkt)));
	memcpy(&addr, &prefix_info->prefix, sizeof(struct in6_addr) / 2);

	ifaddr = net_if_ipv6_addr_lookup(&addr, NULL);
	if (ifaddr && ifaddr->addr_type == NET_ADDR_AUTOCONF) {
		if (prefix_info->valid_lifetime ==
		    NET_IPV6_ND_INFINITE_LIFETIME) {
			net_if_addr_set_lf(ifaddr, true);
			return;
		}

		/* RFC 4862 ch 5.5.3 */
		if ((prefix_info->valid_lifetime > TWO_HOURS) ||
		    (prefix_info->valid_lifetime >
		     remaining(&ifaddr->lifetime))) {
			NET_DBG("Timer updating for address %s "
				"long lifetime %u secs",
				net_sprint_ipv6_addr(&addr),
				prefix_info->valid_lifetime);

			net_if_ipv6_addr_update_lifetime(ifaddr,
						  prefix_info->valid_lifetime);
		} else {
			NET_DBG("Timer updating for address %s "
				"lifetime %u secs",
				net_sprint_ipv6_addr(&addr), TWO_HOURS);

			net_if_ipv6_addr_update_lifetime(ifaddr, TWO_HOURS);
		}

		net_if_addr_set_lf(ifaddr, false);
	} else {
		if (prefix_info->valid_lifetime ==
		    NET_IPV6_ND_INFINITE_LIFETIME) {
			net_if_ipv6_addr_add(net_pkt_iface(pkt),
					     &addr, NET_ADDR_AUTOCONF, 0);
		} else {
			net_if_ipv6_addr_add(net_pkt_iface(pkt),
					     &addr, NET_ADDR_AUTOCONF,
					     prefix_info->valid_lifetime);
		}
	}
}

static inline struct net_buf *handle_ra_prefix(struct net_pkt *pkt,
					       struct net_buf *frag,
					       u8_t len,
					       u16_t offset, u16_t *pos)
{
	struct net_icmpv6_nd_opt_prefix_info prefix_info;

	prefix_info.type = NET_ICMPV6_ND_OPT_PREFIX_INFO;
	prefix_info.len = len * 8 - 2;

	frag = net_frag_read(frag, offset, pos, 1, &prefix_info.prefix_len);
	frag = net_frag_read(frag, *pos, pos, 1, &prefix_info.flags);
	frag = net_frag_read_be32(frag, *pos, pos, &prefix_info.valid_lifetime);
	frag = net_frag_read_be32(frag, *pos, pos,
				  &prefix_info.preferred_lifetime);
	/* Skip reserved bytes */
	frag = net_frag_skip(frag, *pos, pos, 4);
	frag = net_frag_read(frag, *pos, pos, sizeof(struct in6_addr),
			     prefix_info.prefix.s6_addr);
	if (!frag && *pos) {
		return NULL;
	}

	if (prefix_info.valid_lifetime >= prefix_info.preferred_lifetime &&
	    !net_is_ipv6_ll_addr(&prefix_info.prefix)) {

		if (prefix_info.flags & NET_ICMPV6_RA_FLAG_ONLINK) {
			handle_prefix_onlink(pkt, &prefix_info);
		}

		if ((prefix_info.flags & NET_ICMPV6_RA_FLAG_AUTONOMOUS) &&
		    prefix_info.valid_lifetime &&
		    (prefix_info.prefix_len == NET_IPV6_DEFAULT_PREFIX_LEN)) {
			handle_prefix_autonomous(pkt, &prefix_info);
		}
	}

	return frag;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
/* 6lowpan Context Option RFC 6775, 4.2 */
static inline struct net_buf *handle_ra_6co(struct net_pkt *pkt,
					    struct net_buf *frag,
					    u8_t len,
					    u16_t offset, u16_t *pos)
{
	struct net_icmpv6_nd_opt_6co context;

	context.type = NET_ICMPV6_ND_OPT_6CO;
	context.len = len * 8 - 2;

	frag = net_frag_read_u8(frag, offset, pos, &context.context_len);

	/* RFC 6775, 4.2
	 * Context Length: 8-bit unsigned integer.  The number of leading
	 * bits in the Context Prefix field that are valid.  The value ranges
	 * from 0 to 128.  If it is more than 64, then the Length MUST be 3.
	 */
	if (context.context_len > 64 && len != 3) {
		return NULL;
	}

	if (context.context_len <= 64 && len != 2) {
		return NULL;
	}

	context.context_len = context.context_len / 8;
	frag = net_frag_read_u8(frag, *pos, pos, &context.flag);

	/* Skip reserved bytes */
	frag = net_frag_skip(frag, *pos, pos, 2);
	frag = net_frag_read_be16(frag, *pos, pos, &context.lifetime);

	/* RFC 6775, 4.2 (Length field). Length can be 2 or 3 depending
	 * on the length of context prefix field.
	 */
	if (len == 3) {
		frag = net_frag_read(frag, *pos, pos, sizeof(struct in6_addr),
				     context.prefix.s6_addr);
	} else if (len == 2) {
		/* If length is 2 means only 64 bits of context prefix
		 * is available, rest set to zeros.
		 */
		frag = net_frag_read(frag, *pos, pos, 8,
				     context.prefix.s6_addr);
	}

	if (!frag && *pos) {
		return NULL;
	}

	/* context_len: The number of leading bits in the Context Prefix
	 * field that are valid. So set remaining data to zero.
	 */
	if (context.context_len != sizeof(struct in6_addr)) {
		memset(context.prefix.s6_addr + context.context_len, 0,
		       sizeof(struct in6_addr) - context.context_len);
	}

	net_6lo_set_context(net_pkt_iface(pkt), &context);

	return frag;
}
#endif

static enum net_verdict handle_ra_input(struct net_pkt *pkt)
{
	u16_t total_len = net_pkt_get_len(pkt);
	struct net_nbr *nbr = NULL;
	struct net_if_router *router;
	struct net_buf *frag;
	u16_t router_lifetime;
	u32_t reachable_time;
	u32_t retrans_timer;
	u8_t hop_limit;
	u16_t offset;
	u8_t length;
	u8_t type;
	u32_t mtu;

	dbg_addr_recv("Router Advertisement",
		      &NET_IPV6_HDR(pkt)->src,
		      &NET_IPV6_HDR(pkt)->dst);

	net_stats_update_ipv6_nd_recv();

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_ra_hdr) +
			  sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	    (NET_ICMP_HDR(pkt)->code != 0) ||
	    (NET_IPV6_HDR(pkt)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    !net_is_ipv6_ll_addr(&NET_IPV6_HDR(pkt)->src)) {
		goto drop;
	}

	frag = pkt->frags;
	offset = sizeof(struct net_ipv6_hdr) + net_pkt_ipv6_ext_len(pkt) +
		sizeof(struct net_icmp_hdr);

	frag = net_frag_read_u8(frag, offset, &offset, &hop_limit);
	frag = net_frag_skip(frag, offset, &offset, 1); /* flags */
	if (!frag) {
		goto drop;
	}

	if (hop_limit) {
		net_ipv6_set_hop_limit(net_pkt_iface(pkt), hop_limit);
		NET_DBG("New hop limit %d",
			net_if_ipv6_get_hop_limit(net_pkt_iface(pkt)));
	}

	frag = net_frag_read_be16(frag, offset, &offset, &router_lifetime);
	frag = net_frag_read_be32(frag, offset, &offset, &reachable_time);
	frag = net_frag_read_be32(frag, offset, &offset, &retrans_timer);
	if (!frag) {
		goto drop;
	}

	if (reachable_time &&
	    (net_if_ipv6_get_reachable_time(net_pkt_iface(pkt)) !=
	     NET_ICMPV6_RA_HDR(pkt)->reachable_time)) {
		net_if_ipv6_set_base_reachable_time(net_pkt_iface(pkt),
						    reachable_time);

		net_if_ipv6_set_reachable_time(net_pkt_iface(pkt));
	}

	if (retrans_timer) {
		net_if_ipv6_set_retrans_timer(net_pkt_iface(pkt),
					      retrans_timer);
	}

	while (frag) {
		frag = net_frag_read(frag, offset, &offset, 1, &type);
		frag = net_frag_read(frag, offset, &offset, 1, &length);
		if (!frag) {
			goto drop;
		}

		switch (type) {
		case NET_ICMPV6_ND_OPT_SLLAO:
			frag = handle_ra_neighbor(pkt, frag, length, offset,
						  &offset, &nbr);
			if (!frag && offset) {
				goto drop;
			}

			break;
		case NET_ICMPV6_ND_OPT_MTU:
			/* MTU has reserved 2 bytes, so skip it. */
			frag = net_frag_skip(frag, offset, &offset, 2);
			frag = net_frag_read_be32(frag, offset, &offset, &mtu);
			if (!frag && offset) {
				goto drop;
			}

			net_if_set_mtu(net_pkt_iface(pkt), mtu);

			if (mtu > 0xffff) {
				/* TODO: discard packet? */
				NET_ERR("MTU %u, max is %u", mtu, 0xffff);
			}

			break;
		case NET_ICMPV6_ND_OPT_PREFIX_INFO:
			frag = handle_ra_prefix(pkt, frag, length, offset,
						&offset);
			if (!frag && offset) {
				goto drop;
			}

			break;
#if defined(CONFIG_NET_6LO_CONTEXT)
		case NET_ICMPV6_ND_OPT_6CO:
			/* RFC 6775, 4.2 (Length)*/
			if (!(length == 2 || length == 3)) {
				NET_ERR("Invalid 6CO length %d", length);
				goto drop;
			}

			frag = handle_ra_6co(pkt, frag, length, offset,
					     &offset);
			if (!frag && offset) {
				goto drop;
			}

			break;
#endif
		case NET_ICMPV6_ND_OPT_ROUTE:
			NET_DBG("Route option (0x%x) skipped", type);
			goto skip;

#if defined(CONFIG_NET_IPV6_RA_RDNSS)
		case NET_ICMPV6_ND_OPT_RDNSS:
			NET_DBG("RDNSS option (0x%x) skipped", type);
			goto skip;
#endif

		case NET_ICMPV6_ND_OPT_DNSSL:
			NET_DBG("DNSSL option (0x%x) skipped", type);
			goto skip;

		default:
			NET_DBG("Unknown ND option 0x%x", type);
		skip:
			frag = net_frag_skip(frag, offset, &offset,
					     length * 8 - 2);
			if (!frag && offset) {
				goto drop;
			}

			break;
		}
	}

	router = net_if_ipv6_router_lookup(net_pkt_iface(pkt),
					   &NET_IPV6_HDR(pkt)->src);
	if (router) {
		if (!router_lifetime) {
			/* TODO: Start rs_timer on iface if no routers
			 * at all available on iface.
			 */
			net_if_ipv6_router_rm(router);
		} else {
			if (nbr) {
				net_ipv6_nbr_data(nbr)->is_router = true;
			}

			net_if_ipv6_router_update_lifetime(router,
							   router_lifetime);
		}
	} else {
		net_if_ipv6_router_add(net_pkt_iface(pkt),
				       &NET_IPV6_HDR(pkt)->src,
				       router_lifetime);
	}

	if (nbr && net_ipv6_nbr_data(nbr)->pending) {
		NET_DBG("Sending pending pkt %p to %s",
			net_ipv6_nbr_data(nbr)->pending,
			net_sprint_ipv6_addr(&NET_IPV6_HDR(
					net_ipv6_nbr_data(nbr)->pending)->dst));

		if (net_send_data(net_ipv6_nbr_data(nbr)->pending) < 0) {
			net_pkt_unref(net_ipv6_nbr_data(nbr)->pending);
		}

		nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
	}

	/* Cancel the RS timer on iface */
	k_delayed_work_cancel(&net_pkt_iface(pkt)->ipv6.rs_timer);

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_ipv6_nd_drop();

	return NET_DROP;
}
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_MLD)
#define MLDv2_LEN (2 + 1 + 1 + 2 + sizeof(struct in6_addr) * 2)

static struct net_pkt *create_mldv2(struct net_pkt *pkt,
				    const struct in6_addr *addr,
				    u16_t record_type,
				    u8_t num_sources)
{
	net_pkt_append_u8(pkt, record_type);
	net_pkt_append_u8(pkt, 0); /* aux data len */
	net_pkt_append_be16(pkt, num_sources); /* number of addresses */
	net_pkt_append_all(pkt, sizeof(struct in6_addr), addr->s6_addr,
			K_FOREVER);

	if (num_sources > 0) {
		/* All source addresses, RFC 3810 ch 3 */
		net_pkt_append_all(pkt, sizeof(struct in6_addr),
				net_ipv6_unspecified_address()->s6_addr,
				K_FOREVER);
	}

	return pkt;
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
				     K_FOREVER);

	pkt = net_ipv6_create_raw(pkt,
				  net_if_ipv6_select_src_addr(iface, &dst),
				  &dst,
				  iface,
				  NET_IPV6_NEXTHDR_HBHO);

	NET_IPV6_HDR(pkt)->hop_limit = 1; /* RFC 3810 ch 7.4 */

	net_pkt_set_ipv6_hdr_prev(pkt, pkt->frags->len);

	/* Add hop-by-hop option and router alert option, RFC 3810 ch 5. */
	net_pkt_append_u8(pkt, IPPROTO_ICMPV6);
	net_pkt_append_u8(pkt, 0); /* length (0 means 8 bytes) */

#define ROUTER_ALERT_LEN 8

	/* IPv6 router alert option is described in RFC 2711. */
	net_pkt_append_be16(pkt, 0x0502); /* RFC 2711 ch 2.1 */
	net_pkt_append_be16(pkt, 0); /* pkt contains MLD msg */

	net_pkt_append_u8(pkt, 0); /* padding */
	net_pkt_append_u8(pkt, 0); /* padding */

	/* ICMPv6 header */
	net_pkt_append_u8(pkt, NET_ICMPV6_MLDv2); /* type */
	net_pkt_append_u8(pkt, 0); /* code */
	net_pkt_append_be16(pkt, 0); /* chksum */

	pkt->frags->len = NET_IPV6ICMPH_LEN + ROUTER_ALERT_LEN;
	net_pkt_set_iface(pkt, iface);

	net_pkt_append_be16(pkt, 0); /* reserved field */

	/* Insert the actual multicast record(s) here */
	net_pkt_frag_add(pkt, frags);

	ret = net_ipv6_finalize_raw(pkt, NET_IPV6_NEXTHDR_HBHO);
	if (ret < 0) {
		goto drop;
	}

	net_pkt_set_ipv6_ext_len(pkt, ROUTER_ALERT_LEN);

	net_pkt_write_be16(pkt, pkt->frags,
			   NET_IPV6H_LEN + ROUTER_ALERT_LEN + 2,
			   &pos, ntohs(~net_calc_chksum_icmpv6(pkt)));

	ret = net_send_data(pkt);
	if (ret < 0) {
		goto drop;
	}

	net_stats_update_icmp_sent();
	net_stats_update_ipv6_mld_sent();

	return 0;

drop:
	net_pkt_unref(pkt);
	net_stats_update_icmp_drop();
	net_stats_update_ipv6_mld_drop();

	return ret;
}

static int send_mldv2(struct net_if *iface, const struct in6_addr *addr,
		      u8_t mode)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	net_pkt_append_be16(pkt, 1); /* number of records */

	pkt = create_mldv2(pkt, addr, mode, 1);

	ret = send_mldv2_raw(iface, pkt->frags);

	pkt->frags = NULL;

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

	net_mgmt_event_notify(NET_EVENT_IPV6_MCAST_LEAVE, iface);

	return ret;
}

static void send_mld_report(struct net_if *iface)
{
	struct net_pkt *pkt;
	int i, count = 0;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	net_pkt_append_u8(pkt, 0); /* This will be the record count */

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!iface->ipv6.mcast[i].is_used ||
		    !iface->ipv6.mcast[i].is_joined) {
			continue;
		}

		pkt = create_mldv2(pkt, &iface->ipv6.mcast[i].address.in6_addr,
				   NET_IPV6_MLDv2_MODE_IS_EXCLUDE, 0);
		count++;
	}

	if (count > 0) {
		u16_t pos;

		/* Write back the record count */
		net_pkt_write_u8(pkt, pkt->frags, 0, &pos, count);

		send_mldv2_raw(iface, pkt->frags);

		pkt->frags = NULL;
	}

	net_pkt_unref(pkt);
}

static enum net_verdict handle_mld_query(struct net_pkt *pkt)
{
	u16_t total_len = net_pkt_get_len(pkt);
	struct in6_addr mcast;
	u16_t max_rsp_code, num_src, pkt_len;
	u16_t offset, pos;
	struct net_buf *frag;

	dbg_addr_recv("Multicast Listener Query",
		      &NET_IPV6_HDR(pkt)->src,
		      &NET_IPV6_HDR(pkt)->dst);

	net_stats_update_ipv6_mld_recv();

	/* offset tells now where the ICMPv6 header is starting */
	offset = net_pkt_icmp_data(pkt) - net_pkt_ip_data(pkt);
	offset += sizeof(struct net_icmp_hdr);

	frag = net_frag_read_be16(pkt->frags, offset, &pos, &max_rsp_code);
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
	     (NET_ICMP_HDR(pkt)->code != 0) ||
	     (NET_IPV6_HDR(pkt)->hop_limit != 1))) {
		NET_DBG("Preliminary check failed %u/%u, code %u, hop %u",
			total_len, pkt_len,
			NET_ICMP_HDR(pkt)->code, NET_IPV6_HDR(pkt)->hop_limit);
		goto drop;
	}

	/* Currently we only support a unspecified address query. */
	if (!net_ipv6_addr_cmp(&mcast, net_ipv6_unspecified_address())) {
		NET_DBG("Only supporting unspecified address query (%s)",
			net_sprint_ipv6_addr(&mcast));
		goto drop;
	}

	send_mld_report(net_pkt_iface(pkt));

drop:
	net_stats_update_ipv6_mld_drop();

	return NET_DROP;
}

static struct net_icmpv6_handler mld_query_input_handler = {
	.type = NET_ICMPV6_MLD_QUERY,
	.code = 0,
	.handler = handle_mld_query,
};
#endif /* CONFIG_NET_IPV6_MLD */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static struct net_icmpv6_handler ns_input_handler = {
	.type = NET_ICMPV6_NS,
	.code = 0,
	.handler = handle_ns_input,
};

static struct net_icmpv6_handler na_input_handler = {
	.type = NET_ICMPV6_NA,
	.code = 0,
	.handler = handle_na_input,
};
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
static struct net_icmpv6_handler ra_input_handler = {
	.type = NET_ICMPV6_RA,
	.code = 0,
	.handler = handle_ra_input,
};
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_FRAGMENT)
#if defined(CONFIG_NET_IPV6_FRAGMENT_TIMEOUT)
#define IPV6_REASSEMBLY_TIMEOUT K_SECONDS(CONFIG_NET_IPV6_FRAGMENT_TIMEOUT)
#else
#define IPV6_REASSEMBLY_TIMEOUT K_SECONDS(5)
#endif /* CONFIG_NET_IPV6_FRAGMENT_TIMEOUT */

#define FRAG_BUF_WAIT 10 /* how long to max wait for a buffer */

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
	char out[NET_IPV6_ADDR_LEN];
	int i, len;

	snprintk(out, sizeof(out), "%s", net_sprint_ipv6_addr(&reass->dst));

	for (i = 0, len = 0; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		len += net_pkt_get_len(reass->pkt[i]);
	}

	NET_DBG("%s id 0x%x src %s dst %s remain %d ms len %d",
		str, reass->id, net_sprint_ipv6_addr(&reass->src), out,
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

		pkt = reass->pkt[i];

		/* Get rid of IPv6 and fragment header which are at
		 * the beginning of the fragment.
		 */
		removed_len = net_pkt_ipv6_fragment_start(pkt) +
			sizeof(struct net_ipv6_frag_hdr) -
			pkt->frags->data;

		NET_DBG("Removing %d bytes from start of pkt %p",
			removed_len, pkt->frags);

		NET_ASSERT(removed_len >= (sizeof(struct net_ipv6_hdr) +
					   sizeof(struct net_ipv6_frag_hdr)));

		net_buf_pull(pkt->frags, removed_len);

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

	next_hdr = net_pkt_ipv6_fragment_start(pkt)[0];

	/* How much data we need to move in order to get rid of the
	 * fragmentation header.
	 */
	len = pkt->frags->len - sizeof(struct net_ipv6_frag_hdr) -
		(net_pkt_ipv6_fragment_start(pkt) - pkt->frags->data);

	memmove(net_pkt_ipv6_fragment_start(pkt),
		net_pkt_ipv6_fragment_start(pkt) +
		sizeof(struct net_ipv6_frag_hdr), len);

	/* This one updates the previous header's nexthdr value */
	net_pkt_write_u8(pkt, pkt->frags, net_pkt_ipv6_hdr_prev(pkt),
			  &pos, next_hdr);

	pkt->frags->len -= sizeof(struct net_ipv6_frag_hdr);

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

	NET_IPV6_HDR(pkt)->len[0] = len / 256;
	NET_IPV6_HDR(pkt)->len[1] = len - NET_IPV6_HDR(pkt)->len[0] * 256;

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
					    u16_t buf_offset)
{
	struct net_ipv6_reassembly *reass = NULL;
	u32_t id;
	u16_t loc;
	u16_t offset;
	u16_t flag;
	u8_t nexthdr;
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

	net_pkt_set_ipv6_fragment_start(pkt, frag->data + buf_offset);

	/* Each fragment has a fragment header. */
	frag = net_frag_read_u8(frag, buf_offset, &loc, &nexthdr);
	frag = net_frag_skip(frag, loc, &loc, 1); /* reserved */
	frag = net_frag_read_be16(frag, loc, &loc, &flag);
	frag = net_frag_read_be32(frag, loc, &loc, &id);
	if (!frag && loc == 0xffff) {
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

		goto drop;
	}

	/* The last fragment received, reassemble the packet */
	reassemble_packet(reass);

accept:
	return NET_OK;

drop:
	if (reass) {
		reassembly_cancel(reass->id, &reass->src, &reass->dst);
	}

	return NET_DROP;
}

static int get_next_hdr(struct net_pkt *pkt, u16_t *next_hdr_idx,
			u16_t *last_hdr_idx, u8_t *next_hdr)
{
	struct net_buf *buf;
	u16_t pos;
	int ret;

	/* We need to fix the next header value so find out where
	 * is the last IPv6 extension header. The next_hdr_idx value is
	 * offset from the start of the 1st fragment, it is not the
	 * actual next header value.
	 */
	ret = net_ipv6_find_last_ext_hdr(pkt, next_hdr_idx, last_hdr_idx);
	if (ret < 0) {
		NET_DBG("Cannot find the last IPv6 ext header");
		return ret;
	}

	/* The IPv6 must fit into first fragment, otherwise the next read
	 * will fail.
	 */
	if (*next_hdr_idx > pkt->frags->len) {
		NET_DBG("IPv6 header too short (%d vs %d)", *next_hdr_idx,
			pkt->frags->len);
		return -EINVAL;
	}

	if (*last_hdr_idx > pkt->frags->len) {
		NET_DBG("IPv6 header too short (%d vs %d)", *last_hdr_idx,
			pkt->frags->len);
		return -EINVAL;
	}

	buf = net_frag_read_u8(pkt->frags, *next_hdr_idx, &pos, next_hdr);
	if (!buf && pos == 0xffff) {
		NET_DBG("Next header too far (%d vs %d)", *next_hdr_idx,
			pkt->frags->len);
		return -EINVAL;
	}

	return 0;
}

static int send_ipv6_fragment(struct net_if *iface,
			      struct net_pkt *pkt,
			      struct net_buf *orig,
			      struct net_buf *prev,
			      struct net_buf *frag,
			      u16_t ipv6_len,
			      u16_t offset,
			      int len,
			      u8_t next_hdr,
			      u16_t next_hdr_idx,
			      u16_t last_hdr_idx,
			      bool final,
			      int frag_count)
{
	struct net_buf *rest = NULL, *end = NULL, *orig_copy = NULL;
	struct net_ipv6_frag_hdr hdr;
	struct net_pkt *ipv6;
	u16_t pos;
	int ret;

	/* Prepare the pkt so that the IPv6 packet will be sent properly
	 * to the device driver.
	 */
	if (net_pkt_context(pkt)) {
		ipv6 = net_pkt_get_tx(net_pkt_context(pkt), FRAG_BUF_WAIT);
	} else {
		ipv6 = net_pkt_get_reserve_tx(
			net_if_get_ll_reserve(iface, &NET_IPV6_HDR(pkt)->dst),
			FRAG_BUF_WAIT);
	}

	if (!ipv6) {
		NET_DBG("Cannot get packet (%d)", -ENOMEM);
		return -ENOMEM;
	}

	/* How much stuff we can send from this fragment so that it will fit
	 * into IPv6 MTU (1280 bytes).
	 */
	if (len > 0) {
		NET_ASSERT_INFO(len <= (NET_IPV6_MTU -
					sizeof(struct net_ipv6_frag_hdr) -
					ipv6_len),
				"len %u, frag->len %d", len, frag->len);

		ret = net_pkt_split(pkt, frag, len, &end, &rest,
				    FRAG_BUF_WAIT);
		if (ret < 0) {
			NET_DBG("Cannot split fragment (%d)", ret);
			goto free_pkts;
		}
	}

	/* So now the frag is split into two pieces, first one is called "end"
	 * (as it is the end of the packet), and the second one is called
	 * "rest" (as that part is the rest we need to still send).
	 *
	 * Then take out the "frag" from the list as it is now split and not
	 * needed.
	 */

	if (rest) {
		rest->frags = frag->frags;
		frag->frags = NULL;
		net_pkt_frag_unref(frag);
	}

	if (prev) {
		prev->frags = end;
	} else {
		pkt->frags = end;
	}

	end->frags = NULL;

	memcpy(ipv6, pkt, sizeof(struct net_pkt));

	/* We must not take the fragments from the original packet (yet) */
	ipv6->frags = NULL;

	/* Update the extension length metadata so that upper layer checksum
	 * will be calculated properly by net_ipv6_finalize_raw().
	 */
	net_pkt_set_ipv6_ext_len(ipv6,
				 net_pkt_ipv6_ext_len(pkt) +
				 sizeof(struct net_ipv6_frag_hdr));

	orig_copy = net_buf_clone(orig, FRAG_BUF_WAIT);
	if (!orig_copy) {
		ret = -ENOMEM;
		NET_DBG("Cannot clone IPv6 header (%d)", ret);
		goto free_pkts;
	}

	/* Then add the IPv6 header into the packet. */
	net_pkt_frag_insert(ipv6, orig_copy);

	/* Avoid double free if there is an error later in this function. */
	orig_copy = NULL;

	/* And we need to update the last header in the IPv6 packet to point to
	 * fragment header.
	 */
	net_pkt_write_u8(ipv6, ipv6->frags, next_hdr_idx, &pos,
			 NET_IPV6_NEXTHDR_FRAG);

	NET_ASSERT(pos != next_hdr_idx);

	hdr.reserved = 0;
	hdr.id = net_pkt_ipv6_fragment_id(pkt);
	hdr.offset = htons(((offset / 8) << 3) | !final);
	hdr.nexthdr = next_hdr;

	/* Then add the fragmentation header. */
	ret = net_pkt_insert(ipv6, ipv6->frags, last_hdr_idx,
			     sizeof(hdr), (u8_t *)&hdr, FRAG_BUF_WAIT);
	if (!ret) {
		/* If we could not insert because we are already at the
		 * end of fragment, then just append data to the end of
		 * the IPv6 header.
		 */
		ret = net_pkt_append_all(ipv6, sizeof(hdr), (u8_t *)&hdr,
					 FRAG_BUF_WAIT);
		if (!ret) {
			ret = -ENOMEM;
			NET_DBG("Cannot add IPv6 frag header (%d)", ret);
			goto free_pkts;
		}
	}

	/* Tie all the fragments together to form an IPv6 packet. Then
	 * update the length of the packet and optionally the checksum.
	 */
	net_pkt_frag_add(ipv6, pkt->frags);

	/* Note that we must not calculate possible UDP/TCP/ICMPv6 checksum
	 * as that is already calculated in the non-fragmented packet.
	 */
	ret = net_ipv6_finalize_raw(ipv6, NET_IPV6_NEXTHDR_FRAG);
	if (ret < 0) {
		NET_DBG("Cannot create IPv6 packet (%d)", ret);
		goto free_pkts;
	}

	NET_DBG("Sending fragment len %zd", net_pkt_get_len(ipv6));

	/* If everything has been ok so far, we can send the packet.
	 * Note that we cannot send this re-constructed packet directly
	 * as the link layer headers will not be properly set (because
	 * we recreated the packet). So pass this packet back to TX
	 * so that the pkt is going back to L2 for setup.
	 */
	ret = net_send_data(ipv6);
	if (ret < 0) {
		NET_DBG("Cannot send fragment (%d)", ret);
		goto free_pkts;
	}

	/* Then process the rest of the fragments */
	pkt->frags = rest;

	/* Let this packet to be sent and hopefully it will release
	 * the memory that can be utilized for next sent IPv6 fragment.
	 */
	k_yield();

	return 0;

free_pkts:
	net_pkt_unref(ipv6);

	if (rest) {
		net_pkt_frag_unref(rest);
	}

	if (orig_copy) {
		net_pkt_frag_unref(orig_copy);
	}

	return ret;
}

int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 u16_t pkt_len)
{
	struct net_buf *frag = pkt->frags;
	struct net_buf *prev = NULL;
	struct net_buf *orig_ipv6 = NULL;
	struct net_buf *rest;
	int frag_count = 0;
	int curr_len = 0;
	int status = false;
	u16_t ipv6_len = 0, offset = 0;
	u32_t id = sys_rand32_get();
	u8_t next_hdr;
	u16_t next_hdr_idx, last_hdr_idx;
	int ret;

	ret = get_next_hdr(pkt, &next_hdr_idx, &last_hdr_idx, &next_hdr);
	if (ret < 0) {
		return -EINVAL;
	}

	/* Split the first fragment that contains the IPv6 header into
	 * two pieces. The "orig_ipv6" will only contain the original IPv6
	 * header which is copied into each fragment together with
	 * fragmentation header.
	 */
	ret = net_pkt_split(pkt, frag,
			    net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt),
			    &orig_ipv6, &rest, FRAG_BUF_WAIT);
	if (ret < 0) {
		NET_DBG("Cannot split packet (%d)", ret);
		return ret;
	}

	ipv6_len = net_buf_frags_len(orig_ipv6);

	/* We do not need the first fragment any more. The "rest" will not
	 * have IPv6 header but it will contain the rest of the original data.
	 */
	rest->frags = pkt->frags->frags;
	pkt->frags = rest;

	frag->frags = NULL;
	net_pkt_frag_unref(frag);

	frag = pkt->frags;

	net_pkt_set_ipv6_fragment_id(pkt, id);

	/* Go through the fragment list, and create suitable IPv6 packet
	 * from the data.
	 */
	while (frag) {
		curr_len += frag->len;
		if (curr_len > (NET_IPV6_MTU -
				sizeof(struct net_ipv6_frag_hdr) - ipv6_len)) {
			/* The fit_len var tells how much data we need send
			 * from frag in order to fill the IPv6 MTU.
			 */
			int fit_len = NET_IPV6_MTU -
				sizeof(struct net_ipv6_frag_hdr) - ipv6_len -
				(curr_len - frag->len);

			status = send_ipv6_fragment(iface, pkt, orig_ipv6,
						    prev, frag, ipv6_len,
						    offset, fit_len,
						    next_hdr, next_hdr_idx,
						    last_hdr_idx, false,
						    frag_count);
			if (status < 0) {
				goto out;
			}

			offset += curr_len - (frag->len - fit_len);
			prev = NULL;

			frag = pkt->frags;
			curr_len = 0;
			frag_count++;
		}

		prev = frag;
		frag = frag->frags;
	}

	status = send_ipv6_fragment(iface, pkt, orig_ipv6, prev, prev,
				    ipv6_len, offset, 0, next_hdr,
				    next_hdr_idx, last_hdr_idx, true,
				    frag_count);

	net_pkt_unref(pkt);

out:
	if (orig_ipv6) {
		net_pkt_frag_unref(orig_ipv6);
	}

	return status;
}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

static inline enum net_verdict process_icmpv6_pkt(struct net_pkt *pkt,
						  struct net_ipv6_hdr *ipv6)
{
	struct net_icmp_hdr *hdr = NET_ICMP_HDR(pkt);

	NET_DBG("ICMPv6 %s received type %d code %d",
		net_icmpv6_type2str(hdr->type), hdr->type, hdr->code);

	return net_icmpv6_input(pkt, hdr->type, hdr->code);
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
	NET_DBG("Unknown option %d MSB %d", opt_type, opt_type >> 6);

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
	frag = net_frag_read_u8(frag, loc, &loc, &opt_len);
	if (!frag && loc == 0xffff) {
		goto drop;
	}

	while (frag && (length < len)) {
		switch (opt_type) {
		case NET_IPV6_EXT_HDR_OPT_PAD1:
			NET_DBG("PAD1 option");
			length++;
			loc++;
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
		frag = net_frag_read_u8(frag, loc, &loc, &opt_len);
		if (!frag && loc == 0xffff) {
			goto drop;
		}
	}

	if (length != len) {
		goto drop;
	}

	*pos += length;

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

enum net_verdict net_ipv6_process_pkt(struct net_pkt *pkt)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_HDR(pkt);
	int real_len = net_pkt_get_len(pkt);
	int pkt_len = (hdr->len[0] << 8) + hdr->len[1] + sizeof(*hdr);
	struct net_buf *frag;
	u8_t start_of_ext, prev_hdr;
	u8_t next, next_hdr, length;
	u8_t first_option;
	u16_t offset, total_len = 0;
	u8_t ext_bitmap;

	if (real_len != pkt_len) {
		NET_DBG("IPv6 packet size %d pkt len %d", pkt_len, real_len);
		net_stats_update_ipv6_drop();
		goto drop;
	}

#if defined(CONFIG_NET_DEBUG_IPV6)
	do {
		char out[NET_IPV6_ADDR_LEN];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv6_addr(&hdr->dst));
		NET_DBG("IPv6 packet len %d received from %s to %s",
			real_len, net_sprint_ipv6_addr(&hdr->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_IPV6 */

	if (net_is_ipv6_addr_mcast(&hdr->src)) {
		NET_DBG("Dropping src multicast packet");
		net_stats_update_ipv6_drop();
		goto drop;
	}

	if (!net_is_my_ipv6_addr(&hdr->dst) &&
	    !net_is_my_ipv6_maddr(&hdr->dst) &&
	    !net_is_ipv6_addr_mcast(&hdr->dst) &&
	    !net_is_ipv6_addr_loopback(&hdr->dst)) {
#if defined(CONFIG_NET_ROUTE)
		struct net_route_entry *route;
		struct in6_addr *nexthop;

		/* Check if the packet can be routed */
		if (net_route_get_info(net_pkt_iface(pkt), &hdr->dst, &route,
				       &nexthop)) {
			int ret;

			if (route) {
				net_pkt_set_iface(pkt, route->iface);
			}

			ret = net_route_packet(pkt, nexthop);
			if (ret < 0) {
				NET_DBG("Cannot re-route pkt %p via %s (%d)",
					pkt, net_sprint_ipv6_addr(nexthop),
					ret);
			} else {
				return NET_OK;
			}
		} else
#endif /* CONFIG_NET_ROUTE */

		{
			NET_DBG("IPv6 packet in pkt %p not for me", pkt);
		}

		net_stats_update_ipv6_drop();
		goto drop;
	}

	/* Check extension headers */
	net_pkt_set_next_hdr(pkt, &hdr->nexthdr);
	net_pkt_set_ipv6_ext_len(pkt, 0);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

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
	ext_bitmap = 0;
	offset = sizeof(struct net_ipv6_hdr);
	prev_hdr = &NET_IPV6_HDR(pkt)->nexthdr - &NET_IPV6_HDR(pkt)->vtc;

	while (frag) {
		enum net_verdict verdict;

		if (is_upper_layer_protocol_header(next)) {
			NET_DBG("IPv6 next header %d", next);
			net_pkt_set_ipv6_ext_len(pkt, offset -
						 sizeof(struct net_ipv6_hdr));
			goto upper_proto;
		}

		start_of_ext = offset;

		frag = net_frag_read_u8(frag, offset, &offset, &next_hdr);
		frag = net_frag_read_u8(frag, offset, &offset, &length);
		if (!frag && offset == 0xffff) {
			goto drop;
		}

		length = length * 8 + 8;
		total_len += length;
		verdict = NET_OK;

		if (next == NET_IPV6_NEXTHDR_HBHO) {
			NET_DBG("IPv6 next header %d length %d bytes", next,
				length);
		} else {
			/* There is no separate length for other headers */
			NET_DBG("IPv6 next header %d", next);
		}

		switch (next) {
		case NET_IPV6_NEXTHDR_NONE:
			/* There is nothing after this header (see RFC 2460,
			 * ch 4.7), so we can drop the packet now.
			 * This is not an error case so do not update drop
			 * statistics.
			 */
			goto drop;

		case NET_IPV6_NEXTHDR_HBHO:
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

			/* The fragment header does not have length field so
			 * we need to step back two bytes and start from the
			 * beginning of the fragment header.
			 */
			return handle_fragment_hdr(pkt, frag, real_len,
						   offset - 2);
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
	if (total_len > 0) {
		NET_DBG("Extension len %d", total_len);
		net_pkt_set_ipv6_ext_len(pkt, total_len);
	}

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
	net_stats_update_ip_errors_protoerr();

	return NET_DROP;
}

void net_ipv6_init(void)
{
#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	net_icmpv6_register_handler(&ns_input_handler);
	net_icmpv6_register_handler(&na_input_handler);
#endif
#if defined(CONFIG_NET_IPV6_ND)
	net_icmpv6_register_handler(&ra_input_handler);
#endif
#if defined(CONFIG_NET_IPV6_MLD)
	net_icmpv6_register_handler(&mld_query_input_handler);
#endif
}
