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
#include <net/nbuf.h>
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

static inline bool net_is_solicited(struct net_buf *buf)
{
	return NET_ICMPV6_NA_BUF(buf)->flags & NET_ICMPV6_NA_FLAG_SOLICITED;
}

static inline bool net_is_router(struct net_buf *buf)
{
	return NET_ICMPV6_NA_BUF(buf)->flags & NET_ICMPV6_NA_FLAG_ROUTER;
}

static inline bool net_is_override(struct net_buf *buf)
{
	return NET_ICMPV6_NA_BUF(buf)->flags & NET_ICMPV6_NA_FLAG_OVERRIDE;
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

		if (nbr->data == (uint8_t *)data) {
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

struct net_ipv6_nbr_data *net_ipv6_get_nbr_by_index(uint8_t idx)
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
		net_nbuf_unref(data->pending);
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
		net_sprint_ipv6_addr(&NET_IPV6_BUF(data->pending)->dst));

	/* To unref when pending variable was set */
	net_nbuf_unref(data->pending);

	/* To unref the original buf allocation */
	net_nbuf_unref(data->pending);

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

struct net_nbr *net_ipv6_nbr_add(struct net_if *iface,
				 struct in6_addr *addr,
				 struct net_linkaddr *lladdr,
				 bool is_router,
				 enum net_ipv6_nbr_state state)
{
	struct net_nbr *nbr = net_nbr_get(&net_neighbor.table);

	if (!nbr) {
		return NULL;
	}

	nbr_init(nbr, iface, addr, is_router, state);

	if (net_nbr_link(nbr, iface, lladdr)) {
		nbr_free(nbr);
		return NULL;
	}

	NET_DBG("[%d] nbr %p state %d router %d IPv6 %s ll %s",
		nbr->idx, nbr, state, is_router,
		net_sprint_ipv6_addr(addr),
		net_sprint_ll_addr(lladdr->addr, lladdr->len));

	return nbr;
}

static inline struct net_nbr *nbr_add(struct net_buf *buf,
				      struct in6_addr *addr,
				      struct net_linkaddr *lladdr,
				      bool is_router,
				      enum net_ipv6_nbr_state state)
{
	return net_ipv6_nbr_add(net_nbuf_iface(buf), addr, lladdr,
				is_router, state);
}

static struct net_nbr *nbr_new(struct net_if *iface,
			       struct in6_addr *addr,
			       enum net_ipv6_nbr_state state)
{
	struct net_nbr *nbr = net_nbr_get(&net_neighbor.table);

	if (!nbr) {
		return NULL;
	}

	nbr_init(nbr, iface, addr, false, state);

	NET_DBG("nbr %p iface %p state %d IPv6 %s",
		nbr, iface, state, net_sprint_ipv6_addr(addr));

	return nbr;
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
					      uint8_t idx)
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

int net_ipv6_find_last_ext_hdr(struct net_buf *buf)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_BUF(buf);
	struct net_buf *frag = buf->frags;
	int pos = 6; /* Initial value if no extension fragments were found */
	uint16_t offset;
	uint8_t next_hdr;
	uint8_t length;
	uint8_t next;

	offset = sizeof(struct net_ipv6_hdr);
	next = hdr->nexthdr;

	while (frag) {
		frag = net_nbuf_read_u8(frag, offset, &offset, &next_hdr);
		if (frag != buf->frags) {
			break;
		}

		frag = net_nbuf_read_u8(frag, offset, &offset, &length);
		if (!frag && offset == 0xffff) {
			pos = -EINVAL;
			goto fail;
		}

		length = length * 8 + 8;

		switch (next) {
		case NET_IPV6_NEXTHDR_NONE:
			pos = offset;
			goto out;

		case NET_IPV6_NEXTHDR_HBHO:
			pos = offset;
			offset += length;
			break;

		case NET_IPV6_NEXTHDR_FRAG:
			pos = offset;
			offset += sizeof(struct net_ipv6_frag_hdr);
			goto out;

		case IPPROTO_ICMPV6:
		case IPPROTO_UDP:
		case IPPROTO_TCP:
			pos = offset;
			goto out;

		default:
			pos = -EINVAL;
			goto fail;
		}
	}

out:
	if (pos > frag->len) {
		pos = -EINVAL;
	}

fail:
	return pos;
}

const struct in6_addr *net_ipv6_unspecified_address(void)
{
	static const struct in6_addr addr = IN6ADDR_ANY_INIT;

	return &addr;
}

struct net_buf *net_ipv6_create_raw(struct net_buf *buf,
				    const struct in6_addr *src,
				    const struct in6_addr *dst,
				    struct net_if *iface,
				    uint8_t next_header)
{
	struct net_buf *header;

	header = net_nbuf_get_frag(buf, K_FOREVER);

	net_buf_frag_insert(buf, header);

	NET_IPV6_BUF(buf)->vtc = 0x60;
	NET_IPV6_BUF(buf)->tcflow = 0;
	NET_IPV6_BUF(buf)->flow = 0;

	NET_IPV6_BUF(buf)->nexthdr = 0;

	/* User can tweak the default hop limit if needed */
	NET_IPV6_BUF(buf)->hop_limit = net_nbuf_ipv6_hop_limit(buf);
	if (NET_IPV6_BUF(buf)->hop_limit == 0) {
		NET_IPV6_BUF(buf)->hop_limit =
					net_if_ipv6_get_hop_limit(iface);
	}

	net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, dst);
	net_ipaddr_copy(&NET_IPV6_BUF(buf)->src, src);

	net_nbuf_set_ext_len(buf, 0);
	NET_IPV6_BUF(buf)->nexthdr = next_header;

	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));
	net_nbuf_set_family(buf, AF_INET6);

	net_buf_add(header, sizeof(struct net_ipv6_hdr));

	return buf;
}

struct net_buf *net_ipv6_create(struct net_context *context,
				struct net_buf *buf,
				const struct in6_addr *src,
				const struct in6_addr *dst)
{
	NET_ASSERT(((struct sockaddr_in6_ptr *)&context->local)->sin6_addr);

	if (!src) {
		src = ((struct sockaddr_in6_ptr *)&context->local)->sin6_addr;
	}

	if (net_is_ipv6_addr_unspecified(src)
	    || net_is_ipv6_addr_mcast(src)) {
		src = net_if_ipv6_select_src_addr(net_nbuf_iface(buf),
						  (struct in6_addr *)dst);
	}

	return net_ipv6_create_raw(buf,
				   src,
				   dst,
				   net_context_get_iface(context),
				   net_context_get_ip_proto(context));
}

int net_ipv6_finalize_raw(struct net_buf *buf, uint8_t next_header)
{
	/* Set the length of the IPv6 header */
	size_t total_len;

#if defined(CONFIG_NET_UDP) && defined(CONFIG_NET_RPL_INSERT_HBH_OPTION)
	if (next_header != IPPROTO_TCP && next_header != IPPROTO_ICMPV6) {
		/* Check if we need to add RPL header to sent UDP packet. */
		if (net_rpl_insert_header(buf) < 0) {
			NET_DBG("RPL HBHO insert failed");
			return -EINVAL;
		}
	}
#endif

	net_nbuf_compact(buf);

	total_len = net_buf_frags_len(buf->frags);

	total_len -= sizeof(struct net_ipv6_hdr);

	NET_IPV6_BUF(buf)->len[0] = total_len / 256;
	NET_IPV6_BUF(buf)->len[1] = total_len - NET_IPV6_BUF(buf)->len[0] * 256;

#if defined(CONFIG_NET_UDP)
	if (next_header == IPPROTO_UDP) {
		NET_UDP_BUF(buf)->chksum = 0;
		NET_UDP_BUF(buf)->chksum = ~net_calc_chksum_udp(buf);
	} else
#endif

#if defined(CONFIG_NET_TCP)
	if (next_header == IPPROTO_TCP) {
		NET_TCP_BUF(buf)->chksum = 0;
		NET_TCP_BUF(buf)->chksum = ~net_calc_chksum_tcp(buf);
	} else
#endif

	if (next_header == IPPROTO_ICMPV6) {
		NET_ICMP_BUF(buf)->chksum = 0;
		NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum(buf,
							     IPPROTO_ICMPV6);
	}

	return 0;
}

int net_ipv6_finalize(struct net_context *context, struct net_buf *buf)
{
	return net_ipv6_finalize_raw(buf, net_context_get_ip_proto(context));
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

	net_if_ipv6_addr_rm(iface, addr);

	return true;
}
#endif /* CONFIG_NET_IPV6_DAD */

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

static inline void dbg_update_neighbor_lladdr_raw(uint8_t *new_lladdr,
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

#if defined(CONFIG_NET_IPV6_NBR_CACHE)

/* If the reserve has changed, we need to adjust it accordingly in the
 * fragment chain. This can only happen in IEEE 802.15.4 where the link
 * layer header size can change if the destination address changes.
 * Thus we need to check it here. Note that this cannot happen for IPv4
 * as 802.15.4 supports IPv6 only.
 */
static struct net_buf *update_ll_reserve(struct net_buf *buf,
					 struct in6_addr *addr)
{
	/* We need to go through all the fragments and adjust the
	 * fragment data size.
	 */
	uint16_t reserve, room_len, copy_len, pos;
	struct net_buf *orig_frag, *frag;

	/* No need to do anything if we are forwarding the packet
	 * as we already know everything about the destination of
	 * the packet.
	 */
	if (net_nbuf_forwarding(buf)) {
		return buf;
	}

	reserve = net_if_get_ll_reserve(net_nbuf_iface(buf), addr);
	if (reserve == net_nbuf_ll_reserve(buf)) {
		return buf;
	}

	NET_DBG("Adjust reserve old %d new %d",
		net_nbuf_ll_reserve(buf), reserve);

	net_nbuf_set_ll_reserve(buf, reserve);

	orig_frag = buf->frags;
	copy_len = orig_frag->len;
	pos = 0;

	buf->frags = NULL;
	room_len = 0;
	frag = NULL;

	while (orig_frag) {
		if (!room_len) {
			frag = net_nbuf_get_frag(buf, K_FOREVER);

			net_buf_frag_add(buf, frag);

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
			net_nbuf_unref(tmp);

			if (!orig_frag) {
				break;
			}

			copy_len = orig_frag->len;
			pos = 0;
		}
	}

	return buf;
}

struct net_buf *net_ipv6_prepare_for_send(struct net_buf *buf)
{
	struct in6_addr *nexthop = NULL;
	struct net_if *iface = NULL;
	struct net_nbr *nbr;

	NET_ASSERT(buf && buf->frags);

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	/* If we have already fragmented the packet, the fragment id will
	 * contain a proper value and we can skip other checks.
	 */
	if (net_nbuf_ipv6_fragment_id(buf) == 0) {
		size_t pkt_len = net_buf_frags_len(buf);

		if (pkt_len > NET_IPV6_MTU) {
			int ret;

			ret = net_ipv6_send_fragmented_pkt(net_nbuf_iface(buf),
							   buf, pkt_len);
			if (ret < 0) {
				net_nbuf_unref(buf);
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
	if (atomic_test_bit(net_nbuf_iface(buf)->flags, NET_IF_POINTOPOINT)) {
		return buf;
	}

	if (net_nbuf_ll_dst(buf)->addr ||
	    net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
		return update_ll_reserve(buf, &NET_IPV6_BUF(buf)->dst);
	}

	if (net_if_ipv6_addr_onlink(&iface,
				    &NET_IPV6_BUF(buf)->dst)) {
		nexthop = &NET_IPV6_BUF(buf)->dst;
		net_nbuf_set_iface(buf, iface);
	} else {
		/* We need to figure out where the destination
		 * host is located.
		 */
		struct net_route_entry *route;
		struct net_if_router *router;

		route = net_route_lookup(NULL, &NET_IPV6_BUF(buf)->dst);
		if (route) {
			nexthop = net_route_get_nexthop(route);
			if (!nexthop) {
				net_route_del(route);

				net_rpl_global_repair(route);

				NET_DBG("No route to host %s",
					net_sprint_ipv6_addr(
						&NET_IPV6_BUF(buf)->dst));

				net_nbuf_unref(buf);
				return NULL;
			}
		} else {
			/* No specific route to this host, use the default
			 * route instead.
			 */
			router = net_if_ipv6_router_find_default(NULL,
						&NET_IPV6_BUF(buf)->dst);
			if (!router) {
				NET_DBG("No default route to %s",
					net_sprint_ipv6_addr(
						&NET_IPV6_BUF(buf)->dst));

				/* Try to send the packet anyway */
				nexthop = &NET_IPV6_BUF(buf)->dst;
				goto try_send;
			}

			nexthop = &router->address.in6_addr;
		}
	}

	if (net_rpl_update_header(buf, nexthop) < 0) {
		net_nbuf_unref(buf);
		return NULL;
	}

	if (!iface) {
		/* This means that the dst was not onlink, so try to
		 * figure out the interface using nexthop instead.
		 */
		if (net_if_ipv6_addr_onlink(&iface, nexthop)) {
			net_nbuf_set_iface(buf, iface);
		}

		/* If the above check returns null, we try to send
		 * the packet and hope for the best.
		 */
	}

try_send:
	nbr = nbr_lookup(&net_neighbor.table, net_nbuf_iface(buf), nexthop);

	NET_DBG("Neighbor lookup %p (%d) iface %p addr %s state %s", nbr,
		nbr ? nbr->idx : NET_NBR_LLADDR_UNKNOWN,
		net_nbuf_iface(buf),
		net_sprint_ipv6_addr(nexthop),
		nbr ? net_ipv6_nbr_state2str(net_ipv6_nbr_data(nbr)->state) :
		"-");

	if (nbr && nbr->idx != NET_NBR_LLADDR_UNKNOWN) {
		struct net_linkaddr_storage *lladdr;

		lladdr = net_nbr_get_lladdr(nbr->idx);

		net_nbuf_ll_dst(buf)->addr = lladdr->addr;
		net_nbuf_ll_dst(buf)->len = lladdr->len;

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

		return update_ll_reserve(buf, nexthop);
	}

#if defined(CONFIG_NET_IPV6_ND)
	/* We need to send NS and wait for NA before sending the packet. */
	if (net_ipv6_send_ns(net_nbuf_iface(buf),
			     buf,
			     &NET_IPV6_BUF(buf)->src,
			     NULL,
			     nexthop,
			     false) < 0) {
		/* In case of an error, the NS send function will unref
		 * the buf.
		 */
		return NULL;
	}

	NET_DBG("Buf %p (frag %p) will be sent later", buf, buf->frags);
#else
	NET_DBG("Buf %p (frag %p) cannot be sent, dropping it.", buf,
		buf->frags);

	net_nbuf_unref(buf);
#endif /* CONFIG_NET_IPV6_ND */

	return NULL;
}

struct net_nbr *net_ipv6_nbr_lookup(struct net_if *iface,
				    struct in6_addr *addr)
{
	return nbr_lookup(&net_neighbor.table, iface, addr);
}

struct net_nbr *net_ipv6_get_nbr(struct net_if *iface, uint8_t idx)
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

static inline uint8_t get_llao_len(struct net_if *iface)
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
			    uint8_t *llao, uint8_t llao_len, uint8_t type)
{
	llao[NET_ICMPV6_OPT_TYPE_OFFSET] = type;
	llao[NET_ICMPV6_OPT_LEN_OFFSET] = llao_len >> 3;

	memcpy(&llao[NET_ICMPV6_OPT_DATA_OFFSET], lladdr->addr, lladdr->len);

	memset(&llao[NET_ICMPV6_OPT_DATA_OFFSET + lladdr->len], 0,
	       llao_len - lladdr->len - 2);
}

static void setup_headers(struct net_buf *buf, uint8_t nd6_len,
			  uint8_t icmp_type)
{
	NET_IPV6_BUF(buf)->vtc = 0x60;
	NET_IPV6_BUF(buf)->tcflow = 0;
	NET_IPV6_BUF(buf)->flow = 0;
	NET_IPV6_BUF(buf)->len[0] = 0;
	NET_IPV6_BUF(buf)->len[1] = NET_ICMPH_LEN + nd6_len;

	NET_IPV6_BUF(buf)->nexthdr = IPPROTO_ICMPV6;
	NET_IPV6_BUF(buf)->hop_limit = NET_IPV6_ND_HOP_LIMIT;

	NET_ICMP_BUF(buf)->type = icmp_type;
	NET_ICMP_BUF(buf)->code = 0;
}

static inline void handle_ns_neighbor(struct net_buf *buf,
				      struct net_icmpv6_nd_opt_hdr *hdr)
{
	struct net_nbr *nbr;
	struct net_linkaddr lladdr = {
		.len = 8 * hdr->len - 2,
		.addr = (uint8_t *)hdr + 2,
	};

	/**
	 * IEEE802154 lladdress is 8 bytes long, so it requires
	 * 2 * 8 bytes - 2 - padding.
	 * The formula above needs to be adjusted.
	 */
	if (net_nbuf_ll_src(buf)->len < lladdr.len) {
		lladdr.len = net_nbuf_ll_src(buf)->len;
	}

	nbr = nbr_lookup(&net_neighbor.table, net_nbuf_iface(buf),
			 &NET_IPV6_BUF(buf)->src);

	NET_DBG("Neighbor lookup %p iface %p addr %s", nbr,
		net_nbuf_iface(buf),
		net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src));

	if (!nbr) {
		nbr_print();

		nbr = nbr_new(net_nbuf_iface(buf),
			      &NET_IPV6_BUF(buf)->src,
			      NET_IPV6_NBR_STATE_INCOMPLETE);
		if (nbr) {
			NET_DBG("Added %s to nbr cache",
				net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src));
		} else {
			NET_ERR("Could not add neighbor %s",
				net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src));

			return;
		}

		/* Send NS so that we can verify that the neighbor is
		 * reachable.
		 */
		net_ipv6_send_ns(net_nbuf_iface(buf), NULL, NULL, NULL,
				 &net_ipv6_nbr_data(nbr)->addr, false);
	}

	if (net_nbr_link(nbr, net_nbuf_iface(buf), &lladdr) == -EALREADY) {
		/* Update the lladdr if the node was already known */
		struct net_linkaddr_storage *cached_lladdr;

		cached_lladdr = net_nbr_get_lladdr(nbr->idx);

		if (memcmp(cached_lladdr->addr, lladdr.addr, lladdr.len)) {

			dbg_update_neighbor_lladdr(&lladdr,
						   cached_lladdr,
						   &NET_IPV6_BUF(buf)->src);

			net_linkaddr_set(cached_lladdr, lladdr.addr,
					 lladdr.len);

			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		} else {
			if (net_ipv6_nbr_data(nbr)->state ==
			    NET_IPV6_NBR_STATE_INCOMPLETE) {
				ipv6_nbr_set_state(nbr,
						   NET_IPV6_NBR_STATE_STALE);
			}
		}
	}
}

int net_ipv6_send_na(struct net_if *iface, struct in6_addr *src,
		     struct in6_addr *dst, struct in6_addr *tgt,
		     uint8_t flags)
{
	struct net_buf *buf, *frag;
	uint8_t llao_len;

	buf = net_nbuf_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				      K_FOREVER);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	frag = net_nbuf_get_frag(buf, K_FOREVER);

	NET_ASSERT_INFO(frag, "Out of DATA buffers");

	net_buf_frag_add(buf, frag);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	llao_len = get_llao_len(iface);

	net_nbuf_set_ext_len(buf, 0);

	setup_headers(buf, sizeof(struct net_icmpv6_na_hdr) + llao_len,
		      NET_ICMPV6_NA);

	net_ipaddr_copy(&NET_IPV6_BUF(buf)->src, src);
	net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, dst);
	net_ipaddr_copy(&NET_ICMPV6_NA_BUF(buf)->tgt, tgt);

	set_llao(&net_nbuf_iface(buf)->link_addr,
		 net_nbuf_icmp_data(buf) + sizeof(struct net_icmp_hdr) +
					sizeof(struct net_icmpv6_na_hdr),
		 llao_len, NET_ICMPV6_ND_OPT_TLLAO);

	NET_ICMPV6_NA_BUF(buf)->flags = flags;

	net_nbuf_set_len(buf->frags, NET_IPV6ICMPH_LEN +
			 sizeof(struct net_icmpv6_na_hdr) +
			 llao_len);

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv6(buf);

	dbg_addr_sent_tgt("Neighbor Advertisement",
			  &NET_IPV6_BUF(buf)->src,
			  &NET_IPV6_BUF(buf)->dst,
			  &NET_ICMPV6_NS_BUF(buf)->tgt);

	if (net_send_data(buf) < 0) {
		goto drop;
	}

	net_stats_update_ipv6_nd_sent();

	return 0;

drop:
	net_nbuf_unref(buf);
	net_stats_update_ipv6_nd_drop();

	return -EINVAL;
}

static enum net_verdict handle_ns_input(struct net_buf *buf)
{
	uint16_t total_len = net_buf_frags_len(buf);
	struct net_icmpv6_nd_opt_hdr *hdr;
	struct net_if_addr *ifaddr;
	uint8_t flags = 0, prev_opt_len = 0;
	int ret;
	size_t left_len;

	dbg_addr_recv_tgt("Neighbor Solicitation",
			  &NET_IPV6_BUF(buf)->src,
			  &NET_IPV6_BUF(buf)->dst,
			  &NET_ICMPV6_NS_BUF(buf)->tgt);

	net_stats_update_ipv6_nd_recv();

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_ns_hdr))) ||
	    (NET_ICMP_BUF(buf)->code != 0) ||
	    (NET_IPV6_BUF(buf)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    net_is_ipv6_addr_mcast(&NET_ICMPV6_NS_BUF(buf)->tgt)) {
		NET_DBG("Preliminary check failed %u/%zu, code %u, hop %u",
			total_len, (sizeof(struct net_ipv6_hdr) +
				    sizeof(struct net_icmp_hdr) +
				    sizeof(struct net_icmpv6_ns_hdr)),
			NET_ICMP_BUF(buf)->code, NET_IPV6_BUF(buf)->hop_limit);
		goto drop;
	}

	net_nbuf_set_ext_opt_len(buf, sizeof(struct net_icmpv6_ns_hdr));
	hdr = NET_ICMPV6_ND_OPT_HDR_BUF(buf);

	/* The parsing gets tricky if the ND struct is split
	 * between two fragments. FIXME later.
	 */
	if (buf->frags->len < ((uint8_t *)hdr - buf->frags->data)) {
		NET_DBG("NS struct split between fragments");
		goto drop;
	}

	left_len = buf->frags->len - (sizeof(struct net_ipv6_hdr) +
				      sizeof(struct net_icmp_hdr));

	while (net_nbuf_ext_opt_len(buf) < left_len &&
	       left_len < buf->frags->len) {

		if (!hdr->len) {
			break;
		}

		switch (hdr->type) {
		case NET_ICMPV6_ND_OPT_SLLAO:
			if (net_is_ipv6_addr_unspecified(
				    &NET_IPV6_BUF(buf)->src)) {
				goto drop;
			}

			handle_ns_neighbor(buf, hdr);
			break;

		default:
			NET_DBG("Unknown ND option 0x%x", hdr->type);
			break;
		}

		prev_opt_len = net_nbuf_ext_opt_len(buf);

		net_nbuf_set_ext_opt_len(buf, net_nbuf_ext_opt_len(buf) +
					 (hdr->len << 3));

		if (prev_opt_len == net_nbuf_ext_opt_len(buf)) {
			NET_ERR("Corrupted NS message");
			goto drop;
		}

		hdr = NET_ICMPV6_ND_OPT_HDR_BUF(buf);
	}

	ifaddr = net_if_ipv6_addr_lookup_by_iface(net_nbuf_iface(buf),
						  &NET_ICMPV6_NS_BUF(buf)->tgt);
	if (!ifaddr) {
		NET_DBG("No such interface address %s",
			net_sprint_ipv6_addr(&NET_ICMPV6_NS_BUF(buf)->tgt));
		goto drop;
	}

#if !defined(CONFIG_NET_IPV6_DAD)
	if (net_is_ipv6_addr_unspecified(&NET_IPV6_BUF(buf)->src)) {
		goto drop;
	}

#else /* CONFIG_NET_IPV6_DAD */

	/* Do DAD */
	if (net_is_ipv6_addr_unspecified(&NET_IPV6_BUF(buf)->src)) {

		if (!net_is_ipv6_addr_solicited_node(&NET_IPV6_BUF(buf)->dst)) {
			NET_DBG("Not solicited node addr %s",
				net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
			goto drop;
		}

		if (ifaddr->addr_state == NET_ADDR_TENTATIVE) {
			NET_DBG("DAD failed for %s iface %p",
				net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
				net_nbuf_iface(buf));

			dad_failed(net_nbuf_iface(buf),
				   &ifaddr->address.in6_addr);
			goto drop;
		}

		/* We reuse the received buffer to send the NA */
		net_ipv6_addr_create_ll_allnodes_mcast(&NET_IPV6_BUF(buf)->dst);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				net_if_ipv6_select_src_addr(net_nbuf_iface(buf),
						&NET_IPV6_BUF(buf)->dst));
		flags = NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	}
#endif /* CONFIG_NET_IPV6_DAD */

	if (net_is_my_ipv6_addr(&NET_IPV6_BUF(buf)->src)) {
		NET_DBG("Duplicate IPv6 %s address",
			net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src));
		goto drop;
	}

	/* Address resolution */
	if (net_is_ipv6_addr_solicited_node(&NET_IPV6_BUF(buf)->dst)) {
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst,
				&NET_IPV6_BUF(buf)->src);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				&NET_ICMPV6_NS_BUF(buf)->tgt);
		flags = NET_ICMPV6_NA_FLAG_SOLICITED |
			NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	}

	/* Neighbor Unreachability Detection (NUD) */
	if (net_if_ipv6_addr_lookup_by_iface(net_nbuf_iface(buf),
					     &NET_IPV6_BUF(buf)->dst)) {
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst,
				&NET_IPV6_BUF(buf)->src);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				&NET_ICMPV6_NS_BUF(buf)->tgt);
		flags = NET_ICMPV6_NA_FLAG_SOLICITED |
			NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	} else {
		NET_DBG("NUD failed");
		goto drop;
	}

send_na:
	ret = net_ipv6_send_na(net_nbuf_iface(buf),
			       &NET_IPV6_BUF(buf)->src,
			       &NET_IPV6_BUF(buf)->dst,
			       &ifaddr->address.in6_addr,
			       flags);
	if (!ret) {
		net_nbuf_unref(buf);
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

				net_if_router_rm(router);
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
	uint32_t time;

	time = net_if_ipv6_get_reachable_time(iface);

	NET_ASSERT_INFO(time, "Zero reachable timeout!");

	NET_DBG("Starting reachable timer nbr %p data %p time %d ms",
		nbr, net_ipv6_nbr_data(nbr), time);

	k_delayed_work_submit(&net_ipv6_nbr_data(nbr)->reachable, time);
}
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static inline bool handle_na_neighbor(struct net_buf *buf,
				      struct net_icmpv6_nd_opt_hdr *hdr,
				      uint8_t *tllao)
{
	bool lladdr_changed = false;
	struct net_nbr *nbr;
	struct net_linkaddr_storage *cached_lladdr;
	struct net_buf *pending;

	ARG_UNUSED(hdr);

	nbr = nbr_lookup(&net_neighbor.table, net_nbuf_iface(buf),
			 &NET_ICMPV6_NS_BUF(buf)->tgt);

	NET_DBG("Neighbor lookup %p iface %p addr %s", nbr,
		net_nbuf_iface(buf),
		net_sprint_ipv6_addr(&NET_ICMPV6_NS_BUF(buf)->tgt));

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

		lladdr.len = net_nbuf_iface(buf)->link_addr.len;
		lladdr.addr = &tllao[NET_ICMPV6_OPT_DATA_OFFSET];

		if (net_nbr_link(nbr, net_nbuf_iface(buf), &lladdr)) {
			nbr_free(nbr);
			return false;
		}

		NET_DBG("[%d] nbr %p state %d IPv6 %s ll %s",
			nbr->idx, nbr, net_ipv6_nbr_data(nbr)->state,
			net_sprint_ipv6_addr(&NET_ICMPV6_NS_BUF(buf)->tgt),
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
				&NET_ICMPV6_NS_BUF(buf)->tgt);

			net_linkaddr_set(cached_lladdr,
					 &tllao[NET_ICMPV6_OPT_DATA_OFFSET],
					 cached_lladdr->len);
		}

		if (net_is_solicited(buf)) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);
			net_ipv6_nbr_data(nbr)->ns_count = 0;

			/* We might have active timer from PROBE */
			k_delayed_work_cancel(
				&net_ipv6_nbr_data(nbr)->reachable);

			net_ipv6_nbr_set_reachable_timer(net_nbuf_iface(buf),
							 nbr);
		} else {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		}

		net_ipv6_nbr_data(nbr)->is_router = net_is_router(buf);

		goto send_pending;
	}

	/* We do not update the address if override bit is not set
	 * and we have a valid address in the cache.
	 */
	if (!net_is_override(buf) && lladdr_changed) {
		if (net_ipv6_nbr_data(nbr)->state ==
		    NET_IPV6_NBR_STATE_REACHABLE) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		}

		return false;
	}

	if (net_is_override(buf) ||
	    (!net_is_override(buf) && tllao && !lladdr_changed)) {

		if (lladdr_changed) {
			dbg_update_neighbor_lladdr_raw(
				&tllao[NET_ICMPV6_OPT_DATA_OFFSET],
				cached_lladdr,
				&NET_ICMPV6_NS_BUF(buf)->tgt);

			net_linkaddr_set(cached_lladdr,
					 &tllao[NET_ICMPV6_OPT_DATA_OFFSET],
					 cached_lladdr->len);
		}

		if (net_is_solicited(buf)) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);

			/* We might have active timer from PROBE */
			k_delayed_work_cancel(
				&net_ipv6_nbr_data(nbr)->reachable);

			net_ipv6_nbr_set_reachable_timer(net_nbuf_iface(buf),
							 nbr);
		} else {
			if (lladdr_changed) {
				ipv6_nbr_set_state(nbr,
						   NET_IPV6_NBR_STATE_STALE);
			}
		}
	}

	if (net_ipv6_nbr_data(nbr)->is_router && !net_is_router(buf)) {
		/* Update the routing if the peer is no longer
		 * a router.
		 */
		/* FIXME */
	}

	net_ipv6_nbr_data(nbr)->is_router = net_is_router(buf);

send_pending:
	/* Next send any pending messages to the peer. */
	pending = net_ipv6_nbr_data(nbr)->pending;

	if (pending) {
		NET_DBG("Sending pending %p to %s lladdr %s", pending,
			net_sprint_ipv6_addr(&NET_IPV6_BUF(pending)->dst),
			net_sprint_ll_addr(cached_lladdr->addr,
					   cached_lladdr->len));

		if (net_send_data(pending) < 0) {
			nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
		} else {
			net_ipv6_nbr_data(nbr)->pending = NULL;
		}

		net_nbuf_unref(pending);
	}

	return true;
}

static enum net_verdict handle_na_input(struct net_buf *buf)
{
	uint16_t total_len = net_buf_frags_len(buf);
	struct net_icmpv6_nd_opt_hdr *hdr;
	struct net_if_addr *ifaddr;
	uint8_t *tllao = NULL;
	uint8_t prev_opt_len = 0;
	size_t left_len;

	dbg_addr_recv_tgt("Neighbor Advertisement",
			  &NET_IPV6_BUF(buf)->src,
			  &NET_IPV6_BUF(buf)->dst,
			  &NET_ICMPV6_NS_BUF(buf)->tgt);

	net_stats_update_ipv6_nd_recv();

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_na_hdr) +
			  sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	    (NET_ICMP_BUF(buf)->code != 0) ||
	    (NET_IPV6_BUF(buf)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    net_is_ipv6_addr_mcast(&NET_ICMPV6_NS_BUF(buf)->tgt) ||
	    (net_is_solicited(buf) &&
	     net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst))) {
		goto drop;
	}

	net_nbuf_set_ext_opt_len(buf, sizeof(struct net_icmpv6_na_hdr));
	hdr = NET_ICMPV6_ND_OPT_HDR_BUF(buf);

	/* The parsing gets tricky if the ND struct is split
	 * between two fragments. FIXME later.
	 */
	if (buf->frags->len < ((uint8_t *)hdr - buf->frags->data)) {
		NET_DBG("NA struct split between fragments");
		goto drop;
	}

	left_len = buf->frags->len - (sizeof(struct net_ipv6_hdr) +
				      sizeof(struct net_icmp_hdr));

	while (net_nbuf_ext_opt_len(buf) < left_len &&
	       left_len < buf->frags->len) {

		if (!hdr->len) {
			break;
		}

		switch (hdr->type) {
		case NET_ICMPV6_ND_OPT_TLLAO:
			tllao = (uint8_t *)hdr;
			break;

		default:
			NET_DBG("Unknown ND option 0x%x", hdr->type);
			break;
		}

		prev_opt_len = net_nbuf_ext_opt_len(buf);

		net_nbuf_set_ext_opt_len(buf, net_nbuf_ext_opt_len(buf) +
					 (hdr->len << 3));

		if (prev_opt_len == net_nbuf_ext_opt_len(buf)) {
			NET_ERR("Corrupted NA message");
			goto drop;
		}

		hdr = NET_ICMPV6_ND_OPT_HDR_BUF(buf);
	}

	ifaddr = net_if_ipv6_addr_lookup_by_iface(net_nbuf_iface(buf),
						  &NET_ICMPV6_NA_BUF(buf)->tgt);
	if (ifaddr) {
		NET_DBG("Interface %p already has address %s",
			net_nbuf_iface(buf),
			net_sprint_ipv6_addr(&NET_ICMPV6_NA_BUF(buf)->tgt));

#if defined(CONFIG_NET_IPV6_DAD)
		if (ifaddr->addr_state == NET_ADDR_TENTATIVE) {
			dad_failed(net_nbuf_iface(buf),
				   &NET_ICMPV6_NA_BUF(buf)->tgt);
		}
#endif /* CONFIG_NET_IPV6_DAD */

		goto drop;
	}

	if (!handle_na_neighbor(buf, hdr, tllao)) {
		goto drop;
	}

	net_nbuf_unref(buf);

	net_stats_update_ipv6_nd_sent();

	return NET_OK;

drop:
	net_stats_update_ipv6_nd_drop();

	return NET_DROP;
}

int net_ipv6_send_ns(struct net_if *iface,
		     struct net_buf *pending,
		     struct in6_addr *src,
		     struct in6_addr *dst,
		     struct in6_addr *tgt,
		     bool is_my_address)
{
	struct net_buf *buf, *frag;
	struct net_nbr *nbr;
	uint8_t llao_len;

	buf = net_nbuf_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				      K_FOREVER);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	frag = net_nbuf_get_frag(buf, K_FOREVER);

	NET_ASSERT_INFO(frag, "Out of DATA buffers");

	net_buf_frag_add(buf, frag);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	llao_len = get_llao_len(net_nbuf_iface(buf));

	setup_headers(buf, sizeof(struct net_icmpv6_ns_hdr) + llao_len,
		      NET_ICMPV6_NS);

	if (!dst) {
		net_ipv6_addr_create_solicited_node(tgt,
						    &NET_IPV6_BUF(buf)->dst);
	} else {
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, dst);
	}

	NET_ICMPV6_NS_BUF(buf)->reserved = 0;

	net_ipaddr_copy(&NET_ICMPV6_NS_BUF(buf)->tgt, tgt);

	if (is_my_address) {
		/* DAD */
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				net_ipv6_unspecified_address());

		NET_IPV6_BUF(buf)->len[1] -= llao_len;

		net_buf_add(frag,
			    sizeof(struct net_ipv6_hdr) +
			    sizeof(struct net_icmp_hdr) +
			    sizeof(struct net_icmpv6_ns_hdr));
	} else {
		if (src) {
			net_ipaddr_copy(&NET_IPV6_BUF(buf)->src, src);
		} else {
			net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
					net_if_ipv6_select_src_addr(
						net_nbuf_iface(buf),
						&NET_IPV6_BUF(buf)->dst));
		}

		if (net_is_ipv6_addr_unspecified(&NET_IPV6_BUF(buf)->src)) {
			NET_DBG("No source address for NS");
			goto drop;
		}

		set_llao(&net_nbuf_iface(buf)->link_addr,
			 net_nbuf_icmp_data(buf) +
			 sizeof(struct net_icmp_hdr) +
			 sizeof(struct net_icmpv6_ns_hdr),
			 llao_len, NET_ICMPV6_ND_OPT_SLLAO);

		net_buf_add(frag,
			    sizeof(struct net_ipv6_hdr) +
			    sizeof(struct net_icmp_hdr) +
			    sizeof(struct net_icmpv6_ns_hdr) + llao_len);
	}

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv6(buf);

	nbr = nbr_lookup(&net_neighbor.table, net_nbuf_iface(buf),
			 &NET_ICMPV6_NS_BUF(buf)->tgt);
	if (!nbr) {
		nbr_print();

		nbr = nbr_new(net_nbuf_iface(buf),
			      &NET_ICMPV6_NS_BUF(buf)->tgt,
			      NET_IPV6_NBR_STATE_INCOMPLETE);
		if (!nbr) {
			NET_DBG("Could not create new neighbor %s",
				net_sprint_ipv6_addr(
						&NET_ICMPV6_NS_BUF(buf)->tgt));
			goto drop;
		}
	}

	if (pending) {
		if (!net_ipv6_nbr_data(nbr)->pending) {
			net_ipv6_nbr_data(nbr)->pending = net_nbuf_ref(pending);
		} else {
			NET_DBG("Buffer %p already pending for "
				"operation. Discarding pending %p and buf %p",
				net_ipv6_nbr_data(nbr)->pending, pending, buf);
			net_nbuf_unref(pending);
			goto drop;
		}

		NET_DBG("Setting timeout %d for NS", NS_REPLY_TIMEOUT);

		k_delayed_work_submit(&net_ipv6_nbr_data(nbr)->send_ns,
				      NS_REPLY_TIMEOUT);
	}

	dbg_addr_sent_tgt("Neighbor Solicitation",
			  &NET_IPV6_BUF(buf)->src,
			  &NET_IPV6_BUF(buf)->dst,
			  &NET_ICMPV6_NS_BUF(buf)->tgt);

	if (net_send_data(buf) < 0) {
		NET_DBG("Cannot send NS %p (pending %p)", buf, pending);

		if (pending) {
			nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
		}

		goto drop;
	}

	net_stats_update_ipv6_nd_sent();

	return 0;

drop:
	net_nbuf_unref(buf);
	net_stats_update_ipv6_nd_drop();

	return -EINVAL;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
int net_ipv6_send_rs(struct net_if *iface)
{
	struct net_buf *buf, *frag;
	bool unspec_src;
	uint8_t llao_len = 0;

	buf = net_nbuf_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	frag = net_nbuf_get_frag(buf, K_FOREVER);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	net_ipv6_addr_create_ll_allnodes_mcast(&NET_IPV6_BUF(buf)->dst);

	net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
			net_if_ipv6_select_src_addr(iface,
						    &NET_IPV6_BUF(buf)->dst));

	unspec_src = net_is_ipv6_addr_unspecified(&NET_IPV6_BUF(buf)->src);
	if (!unspec_src) {
		llao_len = get_llao_len(net_nbuf_iface(buf));
	}

	setup_headers(buf, sizeof(struct net_icmpv6_rs_hdr) + llao_len,
		      NET_ICMPV6_RS);

	if (!unspec_src) {
		set_llao(&net_nbuf_iface(buf)->link_addr,
			 net_nbuf_icmp_data(buf) +
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

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv6(buf);

	dbg_addr_sent("Router Solicitation",
		      &NET_IPV6_BUF(buf)->src,
		      &NET_IPV6_BUF(buf)->dst);

	if (net_send_data(buf) < 0) {
		goto drop;
	}

	net_stats_update_ipv6_nd_sent();

	return 0;

drop:
	net_nbuf_unref(buf);
	net_stats_update_ipv6_nd_drop();

	return -EINVAL;
}

int net_ipv6_start_rs(struct net_if *iface)
{
	return net_ipv6_send_rs(iface);
}

static inline struct net_buf *handle_ra_neighbor(struct net_buf *buf,
						 struct net_buf *frag,
						 uint8_t len,
						 uint16_t offset, uint16_t *pos,
						 struct net_nbr **nbr)

{
	struct net_linkaddr lladdr;
	struct net_linkaddr_storage llstorage;
	uint8_t padding;

	if (!nbr) {
		return NULL;
	}

	llstorage.len = NET_LINK_ADDR_MAX_LENGTH;
	lladdr.len = NET_LINK_ADDR_MAX_LENGTH;
	lladdr.addr = llstorage.addr;
	if (net_nbuf_ll_src(buf)->len < lladdr.len) {
		lladdr.len = net_nbuf_ll_src(buf)->len;
	}

	frag = net_nbuf_read(frag, offset, pos, lladdr.len, lladdr.addr);
	if (!frag && offset) {
		return NULL;
	}

	padding = len * 8 - 2 - lladdr.len;
	if (padding) {
		frag = net_nbuf_read(frag, *pos, pos, padding, NULL);
		if (!frag && *pos) {
			return NULL;
		}
	}

	*nbr = nbr_lookup(&net_neighbor.table, net_nbuf_iface(buf),
			  &NET_IPV6_BUF(buf)->src);

	NET_DBG("Neighbor lookup %p iface %p addr %s", *nbr,
		net_nbuf_iface(buf),
		net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src));

	if (!*nbr) {
		nbr_print();

		*nbr = nbr_add(buf, &NET_IPV6_BUF(buf)->src, &lladdr,
			       true, NET_IPV6_NBR_STATE_STALE);
		if (!*nbr) {
			NET_ERR("Could not add router neighbor %s [%s]",
				net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src),
				net_sprint_ll_addr(lladdr.addr, lladdr.len));
			return NULL;
		}
	}

	if (net_nbr_link(*nbr, net_nbuf_iface(buf), &lladdr) == -EALREADY) {
		/* Update the lladdr if the node was already known */
		struct net_linkaddr_storage *cached_lladdr;

		cached_lladdr = net_nbr_get_lladdr((*nbr)->idx);

		if (memcmp(cached_lladdr->addr, lladdr.addr, lladdr.len)) {

			dbg_update_neighbor_lladdr(&lladdr,
						   cached_lladdr,
						   &NET_IPV6_BUF(buf)->src);

			net_linkaddr_set(cached_lladdr, lladdr.addr,
					 lladdr.len);

			ipv6_nbr_set_state(*nbr, NET_IPV6_NBR_STATE_STALE);
		} else {
			if (net_ipv6_nbr_data(*nbr)->state ==
			    NET_IPV6_NBR_STATE_INCOMPLETE) {
				ipv6_nbr_set_state(*nbr,
						   NET_IPV6_NBR_STATE_STALE);
			}
		}
	}

	net_ipv6_nbr_data(*nbr)->is_router = true;

	return frag;
}

static inline void handle_prefix_onlink(struct net_buf *buf,
			struct net_icmpv6_nd_opt_prefix_info *prefix_info)
{
	struct net_if_ipv6_prefix *prefix;

	prefix = net_if_ipv6_prefix_lookup(net_nbuf_iface(buf),
					   &prefix_info->prefix,
					   prefix_info->prefix_len);
	if (!prefix) {
		if (!prefix_info->valid_lifetime) {
			return;
		}

		prefix = net_if_ipv6_prefix_add(net_nbuf_iface(buf),
						&prefix_info->prefix,
						prefix_info->prefix_len,
						prefix_info->valid_lifetime);
		if (prefix) {
			NET_DBG("Interface %p add prefix %s/%d lifetime %u",
				net_nbuf_iface(buf),
				net_sprint_ipv6_addr(&prefix_info->prefix),
				prefix_info->prefix_len,
				prefix_info->valid_lifetime);
		} else {
			NET_ERR("Prefix %s/%d could not be added to iface %p",
				net_sprint_ipv6_addr(&prefix_info->prefix),
				prefix_info->prefix_len,
				net_nbuf_iface(buf));

			return;
		}
	}

	switch (prefix_info->valid_lifetime) {
	case 0:
		NET_DBG("Interface %p delete prefix %s/%d",
			net_nbuf_iface(buf),
			net_sprint_ipv6_addr(&prefix_info->prefix),
			prefix_info->prefix_len);

		net_if_ipv6_prefix_rm(net_nbuf_iface(buf),
				      &prefix->prefix,
				      prefix->len);
		break;

	case NET_IPV6_ND_INFINITE_LIFETIME:
		NET_DBG("Interface %p prefix %s/%d infinite",
			net_nbuf_iface(buf),
			net_sprint_ipv6_addr(&prefix->prefix),
			prefix->len);

		net_if_ipv6_prefix_set_lf(prefix, true);
		break;

	default:
		NET_DBG("Interface %p update prefix %s/%u lifetime %u",
			net_nbuf_iface(buf),
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

static inline uint32_t remaining(struct k_delayed_work *work)
{
	return k_delayed_work_remaining_get(work) / MSEC_PER_SEC;
}

static inline void handle_prefix_autonomous(struct net_buf *buf,
			struct net_icmpv6_nd_opt_prefix_info *prefix_info)
{
	struct in6_addr addr = { };
	struct net_if_addr *ifaddr;

	/* Create IPv6 address using the given prefix and iid. We first
	 * setup link local address, and then copy prefix over first 8
	 * bytes of that address.
	 */
	net_ipv6_addr_create_iid(&addr,
				 net_if_get_link_addr(net_nbuf_iface(buf)));
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
			net_if_ipv6_addr_add(net_nbuf_iface(buf),
					     &addr, NET_ADDR_AUTOCONF, 0);
		} else {
			net_if_ipv6_addr_add(net_nbuf_iface(buf),
					     &addr, NET_ADDR_AUTOCONF,
					     prefix_info->valid_lifetime);
		}
	}
}

static inline struct net_buf *handle_ra_prefix(struct net_buf *buf,
					       struct net_buf *frag,
					       uint8_t len,
					       uint16_t offset, uint16_t *pos)
{
	struct net_icmpv6_nd_opt_prefix_info prefix_info;

	prefix_info.type = NET_ICMPV6_ND_OPT_PREFIX_INFO;
	prefix_info.len = len * 8 - 2;

	frag = net_nbuf_read(frag, offset, pos, 1, &prefix_info.prefix_len);
	frag = net_nbuf_read(frag, *pos, pos, 1, &prefix_info.flags);
	frag = net_nbuf_read_be32(frag, *pos, pos, &prefix_info.valid_lifetime);
	frag = net_nbuf_read_be32(frag, *pos, pos,
				  &prefix_info.preferred_lifetime);
	/* Skip reserved bytes */
	frag = net_nbuf_skip(frag, *pos, pos, 4);
	frag = net_nbuf_read(frag, *pos, pos, sizeof(struct in6_addr),
			     prefix_info.prefix.s6_addr);
	if (!frag && *pos) {
		return NULL;
	}

	if (prefix_info.valid_lifetime >= prefix_info.preferred_lifetime &&
	    !net_is_ipv6_ll_addr(&prefix_info.prefix)) {

		if (prefix_info.flags & NET_ICMPV6_RA_FLAG_ONLINK) {
			handle_prefix_onlink(buf, &prefix_info);
		}

		if ((prefix_info.flags & NET_ICMPV6_RA_FLAG_AUTONOMOUS) &&
		    prefix_info.valid_lifetime &&
		    (prefix_info.prefix_len == NET_IPV6_DEFAULT_PREFIX_LEN)) {
			handle_prefix_autonomous(buf, &prefix_info);
		}
	}

	return frag;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
/* 6lowpan Context Option RFC 6775, 4.2 */
static inline struct net_buf *handle_ra_6co(struct net_buf *buf,
					    struct net_buf *frag,
					    uint8_t len,
					    uint16_t offset, uint16_t *pos)
{
	struct net_icmpv6_nd_opt_6co context;

	context.type = NET_ICMPV6_ND_OPT_6CO;
	context.len = len * 8 - 2;

	frag = net_nbuf_read_u8(frag, offset, pos, &context.context_len);

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
	frag = net_nbuf_read_u8(frag, *pos, pos, &context.flag);

	/* Skip reserved bytes */
	frag = net_nbuf_skip(frag, *pos, pos, 2);
	frag = net_nbuf_read_be16(frag, *pos, pos, &context.lifetime);

	/* RFC 6775, 4.2 (Length field). Length can be 2 or 3 depending
	 * on the length of context prefix field.
	 */
	if (len == 3) {
		frag = net_nbuf_read(frag, *pos, pos, sizeof(struct in6_addr),
				     context.prefix.s6_addr);
	} else if (len == 2) {
		/* If length is 2 means only 64 bits of context prefix
		 * is available, rest set to zeros.
		 */
		frag = net_nbuf_read(frag, *pos, pos, 8,
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

	net_6lo_set_context(net_nbuf_iface(buf), &context);

	return frag;
}
#endif

static enum net_verdict handle_ra_input(struct net_buf *buf)
{
	uint16_t total_len = net_buf_frags_len(buf);
	struct net_nbr *nbr = NULL;
	struct net_if_router *router;
	struct net_buf *frag;
	uint16_t router_lifetime;
	uint32_t reachable_time;
	uint32_t retrans_timer;
	uint8_t hop_limit;
	uint16_t offset;
	uint8_t length;
	uint8_t type;
	uint32_t mtu;

	dbg_addr_recv("Router Advertisement",
		      &NET_IPV6_BUF(buf)->src,
		      &NET_IPV6_BUF(buf)->dst);

	net_stats_update_ipv6_nd_recv();

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_ra_hdr) +
			  sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	    (NET_ICMP_BUF(buf)->code != 0) ||
	    (NET_IPV6_BUF(buf)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    !net_is_ipv6_ll_addr(&NET_IPV6_BUF(buf)->src)) {
		goto drop;
	}

	frag = buf->frags;
	offset = sizeof(struct net_ipv6_hdr) + net_nbuf_ext_len(buf) +
		sizeof(struct net_icmp_hdr);

	frag = net_nbuf_read_u8(frag, offset, &offset, &hop_limit);
	frag = net_nbuf_skip(frag, offset, &offset, 1); /* flags */
	if (!frag) {
		goto drop;
	}

	if (hop_limit) {
		net_ipv6_set_hop_limit(net_nbuf_iface(buf), hop_limit);
		NET_DBG("New hop limit %d",
			net_if_ipv6_get_hop_limit(net_nbuf_iface(buf)));
	}

	frag = net_nbuf_read_be16(frag, offset, &offset, &router_lifetime);
	frag = net_nbuf_read_be32(frag, offset, &offset, &reachable_time);
	frag = net_nbuf_read_be32(frag, offset, &offset, &retrans_timer);
	if (!frag) {
		goto drop;
	}

	if (reachable_time &&
	    (net_if_ipv6_get_reachable_time(net_nbuf_iface(buf)) !=
	     NET_ICMPV6_RA_BUF(buf)->reachable_time)) {
		net_if_ipv6_set_base_reachable_time(net_nbuf_iface(buf),
						    reachable_time);

		net_if_ipv6_set_reachable_time(net_nbuf_iface(buf));
	}

	if (retrans_timer) {
		net_if_ipv6_set_retrans_timer(net_nbuf_iface(buf),
					      retrans_timer);
	}

	while (frag) {
		frag = net_nbuf_read(frag, offset, &offset, 1, &type);
		frag = net_nbuf_read(frag, offset, &offset, 1, &length);
		if (!frag) {
			goto drop;
		}

		switch (type) {
		case NET_ICMPV6_ND_OPT_SLLAO:
			frag = handle_ra_neighbor(buf, frag, length, offset,
						  &offset, &nbr);
			if (!frag && offset) {
				goto drop;
			}

			break;
		case NET_ICMPV6_ND_OPT_MTU:
			/* MTU has reserved 2 bytes, so skip it. */
			frag = net_nbuf_skip(frag, offset, &offset, 2);
			frag = net_nbuf_read_be32(frag, offset, &offset, &mtu);
			if (!frag && offset) {
				goto drop;
			}

			net_if_set_mtu(net_nbuf_iface(buf), mtu);

			if (mtu > 0xffff) {
				/* TODO: discard packet? */
				NET_ERR("MTU %u, max is %u", mtu, 0xffff);
			}

			break;
		case NET_ICMPV6_ND_OPT_PREFIX_INFO:
			frag = handle_ra_prefix(buf, frag, length, offset,
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

			frag = handle_ra_6co(buf, frag, length, offset,
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
			frag = net_nbuf_skip(frag, offset, &offset,
					     length * 8 - 2);
			if (!frag && offset) {
				goto drop;
			}

			break;
		}
	}

	router = net_if_ipv6_router_lookup(net_nbuf_iface(buf),
					   &NET_IPV6_BUF(buf)->src);
	if (router) {
		if (!router_lifetime) {
			/*TODO: Start rs_timer on iface if no routers
			 * at all available on iface.
			 */
			net_if_router_rm(router);
		} else {
			if (nbr) {
				net_ipv6_nbr_data(nbr)->is_router = true;
			}

			net_if_ipv6_router_update_lifetime(router,
							   router_lifetime);
		}
	} else {
		net_if_ipv6_router_add(net_nbuf_iface(buf),
				       &NET_IPV6_BUF(buf)->src,
				       router_lifetime);
	}

	if (nbr && net_ipv6_nbr_data(nbr)->pending) {
		NET_DBG("Sending pending buf %p to %s",
			net_ipv6_nbr_data(nbr)->pending,
			net_sprint_ipv6_addr(&NET_IPV6_BUF(
					net_ipv6_nbr_data(nbr)->pending)->dst));

		if (net_send_data(net_ipv6_nbr_data(nbr)->pending) < 0) {
			net_nbuf_unref(net_ipv6_nbr_data(nbr)->pending);
		}

		nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
	}

	/* Cancel the RS timer on iface */
	k_delayed_work_cancel(&net_nbuf_iface(buf)->rs_timer);

	net_nbuf_unref(buf);

	return NET_OK;

drop:
	net_stats_update_ipv6_nd_drop();

	return NET_DROP;
}
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_MLD)
#define MLDv2_LEN (2 + 1 + 1 + 2 + sizeof(struct in6_addr) * 2)

static struct net_buf *create_mldv2(struct net_buf *buf,
				    const struct in6_addr *addr,
				    uint16_t record_type,
				    uint8_t num_sources)
{
	net_nbuf_append_u8(buf, record_type);
	net_nbuf_append_u8(buf, 0); /* aux data len */
	net_nbuf_append_be16(buf, num_sources); /* number of addresses */
	net_nbuf_append(buf, sizeof(struct in6_addr), addr->s6_addr,
			K_FOREVER);

	if (num_sources > 0) {
		/* All source addresses, RFC 3810 ch 3 */
		net_nbuf_append(buf, sizeof(struct in6_addr),
				net_ipv6_unspecified_address()->s6_addr,
				K_FOREVER);
	}

	return buf;
}

static int send_mldv2_raw(struct net_if *iface, struct net_buf *frags)
{
	struct net_buf *buf;
	struct in6_addr dst;
	uint16_t pos;
	int ret;

	/* Sent to all MLDv2-capable routers */
	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);

	buf = net_nbuf_get_reserve_tx(net_if_get_ll_reserve(iface, &dst),
				      K_FOREVER);

	buf = net_ipv6_create_raw(buf,
				  net_if_ipv6_select_src_addr(iface, &dst),
				  &dst,
				  iface,
				  NET_IPV6_NEXTHDR_HBHO);

	NET_IPV6_BUF(buf)->hop_limit = 1; /* RFC 3810 ch 7.4 */

	net_nbuf_set_ipv6_hdr_prev(buf, buf->len);

	/* Add hop-by-hop option and router alert option, RFC 3810 ch 5. */
	net_nbuf_append_u8(buf, IPPROTO_ICMPV6);
	net_nbuf_append_u8(buf, 0); /* length (0 means 8 bytes) */

#define ROUTER_ALERT_LEN 8

	/* IPv6 router alert option is described in RFC 2711. */
	net_nbuf_append_be16(buf, 0x0502); /* RFC 2711 ch 2.1 */
	net_nbuf_append_be16(buf, 0); /* pkt contains MLD msg */

	net_nbuf_append_u8(buf, 0); /* padding */
	net_nbuf_append_u8(buf, 0); /* padding */

	/* ICMPv6 header */
	net_nbuf_append_u8(buf, NET_ICMPV6_MLDv2); /* type */
	net_nbuf_append_u8(buf, 0); /* code */
	net_nbuf_append_be16(buf, 0); /* chksum */

	net_nbuf_set_len(buf->frags, NET_IPV6ICMPH_LEN + ROUTER_ALERT_LEN);
	net_nbuf_set_iface(buf, iface);

	net_nbuf_append_be16(buf, 0); /* reserved field */

	/* Insert the actual multicast record(s) here */
	net_buf_frag_add(buf, frags);

	ret = net_ipv6_finalize_raw(buf, NET_IPV6_NEXTHDR_HBHO);
	if (ret < 0) {
		goto drop;
	}

	net_nbuf_set_ext_len(buf, ROUTER_ALERT_LEN);

	net_nbuf_write_be16(buf, buf->frags,
			    NET_IPV6H_LEN + ROUTER_ALERT_LEN + 2,
			    &pos, ntohs(~net_calc_chksum_icmpv6(buf)));

	ret = net_send_data(buf);
	if (ret < 0) {
		goto drop;
	}

	net_stats_update_icmp_sent();
	net_stats_update_ipv6_mld_sent();

	return 0;

drop:
	net_nbuf_unref(buf);
	net_stats_update_icmp_drop();
	net_stats_update_ipv6_mld_drop();

	return ret;
}

static int send_mldv2(struct net_if *iface, const struct in6_addr *addr,
		      uint8_t mode)
{
	struct net_buf *buf;
	int ret;

	buf = net_nbuf_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	net_nbuf_append_be16(buf, 1); /* number of records */

	buf = create_mldv2(buf, addr, mode, 1);

	ret = send_mldv2_raw(iface, buf->frags);

	buf->frags = NULL;

	net_nbuf_unref(buf);

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
	struct net_buf *buf;
	int i, count = 0;

	buf = net_nbuf_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	net_nbuf_append_u8(buf, 0); /* This will be the record count */

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!iface->ipv6.mcast[i].is_used ||
		    !iface->ipv6.mcast[i].is_joined) {
			continue;
		}

		buf = create_mldv2(buf, &iface->ipv6.mcast[i].address.in6_addr,
				   NET_IPV6_MLDv2_MODE_IS_EXCLUDE, 0);
		count++;
	}

	if (count > 0) {
		uint16_t pos;

		/* Write back the record count */
		net_nbuf_write_u8(buf, buf->frags, 0, &pos, count);

		send_mldv2_raw(iface, buf->frags);

		buf->frags = NULL;
	}

	net_nbuf_unref(buf);
}

static enum net_verdict handle_mld_query(struct net_buf *buf)
{
	uint16_t total_len = net_buf_frags_len(buf);
	struct in6_addr mcast;
	uint16_t max_rsp_code, num_src, pkt_len;
	uint16_t offset, pos;
	struct net_buf *frag;

	dbg_addr_recv("Multicast Listener Query",
		      &NET_IPV6_BUF(buf)->src,
		      &NET_IPV6_BUF(buf)->dst);

	net_stats_update_ipv6_mld_recv();

	/* offset tells now where the ICMPv6 header is starting */
	offset = net_nbuf_icmp_data(buf) - net_nbuf_ip_data(buf);
	offset += sizeof(struct net_icmp_hdr);

	frag = net_nbuf_read_be16(buf->frags, offset, &pos, &max_rsp_code);
	frag = net_nbuf_skip(frag, pos, &pos, 2); /* two reserved bytes */
	frag = net_nbuf_read(frag, pos, &pos, sizeof(mcast), mcast.s6_addr);
	frag = net_nbuf_skip(frag, pos, &pos, 2); /* skip S, QRV & QQIC */
	frag = net_nbuf_read_be16(buf->frags, pos, &pos, &num_src);
	if (!frag && pos == 0xffff) {
		goto drop;
	}

	pkt_len = sizeof(struct net_ipv6_hdr) +	net_nbuf_ext_len(buf) +
		sizeof(struct net_icmp_hdr) + (2 + 2 + 16 + 2 + 2) +
		sizeof(struct in6_addr) * num_src;

	if ((total_len < pkt_len || pkt_len > NET_IPV6_MTU ||
	     (NET_ICMP_BUF(buf)->code != 0) ||
	     (NET_IPV6_BUF(buf)->hop_limit != 1))) {
		NET_DBG("Preliminary check failed %u/%u, code %u, hop %u",
			total_len, pkt_len,
			NET_ICMP_BUF(buf)->code, NET_IPV6_BUF(buf)->hop_limit);
		goto drop;
	}

	/* Currently we only support a unspecified address query. */
	if (!net_ipv6_addr_cmp(&mcast, net_ipv6_unspecified_address())) {
		NET_DBG("Only supporting unspecified address query (%s)",
			net_sprint_ipv6_addr(&mcast));
		goto drop;
	}

	send_mld_report(net_nbuf_iface(buf));

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
#define IPV6_REASSEMBLY_TIMEOUT K_SECONDS(60)
#endif /* CONFIG_NET_IPV6_FRAGMENT_TIMEOUT */

#define FRAG_BUF_WAIT 10 /* how long to max wait for a buffer */

static void reassembly_timeout(struct k_work *work);

static struct net_ipv6_reassembly
reassembly[CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT] = {
	[0 ... (CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT - 1)] = {
		.timer = {
			.work = K_WORK_INITIALIZER(reassembly_timeout),
		},
	},
};

static struct net_ipv6_reassembly *reassembly_get(uint32_t id,
						  struct in6_addr *src,
						  struct in6_addr *dst)
{
	int i, avail = -1;

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {

		if (k_work_pending(&reassembly[i].timer.work) &&
		    reassembly[i].id == id &&
		    net_ipv6_addr_cmp(src, &reassembly[i].src) &&
		    net_ipv6_addr_cmp(dst, &reassembly[i].dst)) {
			return &reassembly[i];
		}

		if (k_work_pending(&reassembly[i].timer.work)) {
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

static bool reassembly_cancel(uint32_t id,
			      struct in6_addr *src,
			      struct in6_addr *dst)
{
	int i, j;

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		int32_t remaining;

		if (!k_work_pending(&reassembly[i].timer.work) ||
		    reassembly[i].id != id ||
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

		for (j = 0; j < NET_IPV6_FRAGMENTS_MAX_BUF; j++) {
			if (!reassembly[i].buf[j]) {
				continue;
			}

			NET_DBG("IPv6 reassembly buf %p %zd bytes data",
				reassembly[i].buf[j],
				net_buf_frags_len(reassembly[i].buf[j]));

			net_nbuf_unref(reassembly[i].buf[j]);
			reassembly[i].buf[j] = NULL;
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

	for (i = 0, len = 0; i < NET_IPV6_FRAGMENTS_MAX_BUF; i++) {
		len += net_buf_frags_len(reass->buf[i]);
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
	struct net_buf *buf, *last;
	uint8_t next_hdr;
	int i, len;
	uint16_t pos;

	k_delayed_work_cancel(&reass->timer);

	NET_ASSERT(reass->buf[0]);

	last = net_buf_frag_last(reass->buf[0]->frags);

	/* We start from 2nd packet which is then appended to
	 * the first one.
	 */
	for (i = 1; i < NET_IPV6_FRAGMENTS_MAX_BUF; i++) {
		int removed_len;

		buf = reass->buf[i];

		/* Get rid of IPv6 and fragment header which are at
		 * the beginning of the fragment.
		 */
		removed_len = net_nbuf_ipv6_fragment_start(buf) +
			sizeof(struct net_ipv6_frag_hdr) -
			buf->frags->data;

		NET_DBG("Removing %d bytes from start of buf %p",
			removed_len, buf->frags);

		NET_ASSERT(removed_len >= (sizeof(struct net_ipv6_hdr) +
					   sizeof(struct net_ipv6_frag_hdr)));

		net_buf_pull(buf->frags, removed_len);

		/* Attach the data to previous buf */
		last->frags = buf->frags;
		last = net_buf_frag_last(buf->frags);

		buf->frags = NULL;
		reass->buf[i] = NULL;

		net_nbuf_unref(buf);
	}

	buf = reass->buf[0];

	/* Next we need to strip away the fragment header from the first packet
	 * and set the various pointers and values in buffer metadata.
	 */

	next_hdr = net_nbuf_ipv6_fragment_start(buf)[0];

	/* How much data we need to move in order to get rid of the
	 * fragmentation header.
	 */
	len = buf->frags->len - sizeof(struct net_ipv6_frag_hdr) -
		(net_nbuf_ipv6_fragment_start(buf) - buf->frags->data);

	memmove(net_nbuf_ipv6_fragment_start(buf),
		net_nbuf_ipv6_fragment_start(buf) +
		sizeof(struct net_ipv6_frag_hdr), len);

	/* This one updates the previous header's nexthdr value */
	net_nbuf_write_u8(buf, buf->frags, net_nbuf_ipv6_hdr_prev(buf),
			  &pos, next_hdr);

	buf->frags->len -= sizeof(struct net_ipv6_frag_hdr);

	if (!net_nbuf_compact(buf)) {
		NET_ERR("Cannot compact reassembly buffer %p", buf);
		reassembly_cancel(reass->id, &reass->src, &reass->dst);
		return;
	}

	/* Fix the total length of the IPv6 packet. */
	len = net_nbuf_ext_len(buf);
	if (len > 0) {
		NET_DBG("Old buf %p IPv6 ext len is %d bytes", buf, len);
		net_nbuf_set_ext_len(buf,
				     len - sizeof(struct net_ipv6_frag_hdr));
	}

	len = net_buf_frags_len(buf) - sizeof(struct net_ipv6_hdr);

	NET_IPV6_BUF(buf)->len[0] = len / 256;
	NET_IPV6_BUF(buf)->len[1] = len - NET_IPV6_BUF(buf)->len[0] * 256;

	NET_DBG("New buf %p IPv6 len is %d bytes", buf, len);

	/* We need to use the queue when feeding the packet back into the
	 * IP stack as we might run out of stack if we call processing_data()
	 * directly. As the packet does not contain link layer header, we
	 * MUST NOT pass it to L2 so there will be a special check for that
	 * in process_data() when handling the packet.
	 */
	net_recv_data(net_nbuf_iface(buf), buf);

	/* Make room for new packet that can be reassembled */
	k_delayed_work_cancel(&reass->timer);

	/* We do not need to unref the net_buf as that will be handled
	 * by the receiving code in upper part of the IP stack.
	 */
	for (i = 0; i < NET_IPV6_FRAGMENTS_MAX_BUF; i++) {
		reass->buf[i] = NULL;
	}
}

void net_ipv6_frag_foreach(net_ipv6_frag_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_COUNT; i++) {
		if (!k_work_pending(&reassembly[i].timer.work)) {
			continue;
		}

		cb(&reassembly[i], user_data);
	}
}

/* Verify that we have all the fragments received and in correct order.
 */
static bool fragment_verify(struct net_ipv6_reassembly *reass)
{
	uint16_t offset;
	int i, prev_len;

	prev_len = net_buf_frags_len(reass->buf[0]);
	offset = net_nbuf_ipv6_fragment_offset(reass->buf[0]);

	NET_DBG("buf %p offset %u", reass->buf[0], offset);

	if (offset != 0) {
		return false;
	}

	for (i = 1; i < NET_IPV6_FRAGMENTS_MAX_BUF; i++) {
		offset = net_nbuf_ipv6_fragment_offset(reass->buf[i]);

		NET_DBG("buf %p offset %u prev_len %d", reass->buf[i],
			offset, prev_len);

		if (prev_len < offset) {
			/* Something wrong with the offset value */
			return false;
		}

		prev_len = net_buf_frags_len(reass->buf[i]);
	}

	return true;
}

static enum net_verdict handle_fragment_hdr(struct net_buf *buf,
					    struct net_buf *frag,
					    int total_len,
					    uint16_t buf_offset)
{
	struct net_ipv6_reassembly *reass;
	uint32_t id;
	uint16_t loc;
	uint16_t offset;
	uint16_t flag;
	uint8_t nexthdr;
	uint8_t more;
	bool found;
	int i;

	net_nbuf_set_ipv6_fragment_start(buf, frag->data + buf_offset);

	/* Each fragment has a fragment header. */
	frag = net_nbuf_read_u8(frag, buf_offset, &loc, &nexthdr);
	frag = net_nbuf_skip(frag, loc, &loc, 1); /* reserved */
	frag = net_nbuf_read_be16(frag, loc, &loc, &flag);
	frag = net_nbuf_read_be32(frag, loc, &loc, &id);
	if (!frag && loc == 0xffff) {
		goto drop;
	}

	reass = reassembly_get(id, &NET_IPV6_BUF(buf)->src,
			       &NET_IPV6_BUF(buf)->dst);
	if (!reass) {
		NET_DBG("Cannot get reassembly slot, dropping buf %p", buf);
		goto drop;
	}

	offset = flag & 0xfff8;
	more = flag & 0x01;

	net_nbuf_set_ipv6_fragment_offset(buf, offset);

	if (!reass->buf[0]) {
		NET_DBG("Storing buf %p to slot %d", buf, 0);
		reass->buf[0] = buf;

		reassembly_info("Reassembly 1st pkt", reass);

		/* Wait for more fragments to receive. */
		goto accept;
	}

	/* The fragments might come in wrong order so place them
	 * in reassembly chain in correct order.
	 */
	for (i = 0, found = false; i < NET_IPV6_FRAGMENTS_MAX_BUF; i++) {
		bool move_done = false;
		int j;

		if (!reass->buf[i]) {
			NET_DBG("Storing buf %p to slot %d", buf, i);
			reass->buf[i] = buf;
			found = true;
			break;
		}

		if (net_nbuf_ipv6_fragment_offset(reass->buf[i]) <
		    offset) {
			continue;
		}

		for (j = i + 1; j < NET_IPV6_FRAGMENTS_MAX_BUF; j++) {
			if (!reass->buf[j]) {
				memmove(reass->buf[j], reass->buf[i],
					sizeof(void *));
				move_done = true;
				break;
			}
		}

		/* If we do not have any free space in the buf array,
		 * then the fragment needs to be discarded.
		 */
		if (!move_done) {
			break;
		}

		reass->buf[i] = buf;
		found = true;
		break;
	}

	if (!found) {
		/* We could not add this fragment into our saved fragment
		 * list. We must discard the whole packet at this point.
		 */
		reassembly_cancel(reass->id, &reass->src, &reass->dst);
		goto drop;
	}

	if (more) {
		if (net_buf_frags_len(buf) % 8) {
			/* Fragment length is not multiple of 8, discard
			 * the packet and send parameter problem error.
			 */
			net_icmpv6_send_error(buf, NET_ICMPV6_PARAM_PROBLEM,
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
		reassembly_cancel(reass->id, &reass->src, &reass->dst);
		goto drop;
	}

	/* The last fragment received, reassemble the packet */
	reassemble_packet(reass);

accept:
	return NET_OK;

drop:
	return NET_DROP;
}

static int send_ipv6_fragment(struct net_if *iface,
			      struct net_buf *buf,
			      struct net_buf *orig,
			      struct net_buf *prev,
			      struct net_buf *frag,
			      uint16_t ipv6_len,
			      uint16_t offset,
			      int len,
			      bool final)
{
	struct net_buf *ipv6, *rest = NULL, *end = NULL, *orig_copy = NULL;
	struct net_ipv6_frag_hdr hdr;
	uint16_t pos, ext_len;
	uint8_t prev_hdr;
	int ret;

	/* Prepare the head buf so that the IPv6 packet will be sent properly
	 * to the device driver.
	 */
	if (net_nbuf_context(buf)) {
		ipv6 = net_nbuf_get_tx(net_nbuf_context(buf), FRAG_BUF_WAIT);
	} else {
		ipv6 = net_nbuf_get_reserve_tx(
			net_if_get_ll_reserve(iface, &NET_IPV6_BUF(buf)->dst),
			FRAG_BUF_WAIT);
	}

	if (!ipv6) {
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

		ret = net_nbuf_split(buf, frag, len, &end, &rest,
				     FRAG_BUF_WAIT);
		if (ret < 0) {
			goto free_bufs;
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
		net_nbuf_unref(frag);
	}

	if (prev) {
		prev->frags = end;
	} else {
		buf->frags = end;
	}

	end->frags = NULL;
	net_nbuf_copy_user_data(ipv6, buf);

	/* Update the extension length metadata so that upper layer checksum
	 * will be calculated properly by net_ipv6_finalize_raw().
	 */
	ext_len = net_nbuf_ext_len(ipv6) + sizeof(struct net_ipv6_frag_hdr);
	net_nbuf_set_ext_len(ipv6, ext_len);

	orig_copy = net_buf_clone(orig, FRAG_BUF_WAIT);
	if (!orig_copy) {
		ret = -ENOMEM;
		goto free_bufs;
	}

	/* Then add the IPv6 header into the packet. */
	net_buf_frag_insert(ipv6, orig_copy);

	/* We need to fix the next header value so find out where
	 * is the last IPv6 extension header. The returned value is offset
	 * from the start of the 1st fragment, it is not the actual next
	 * header value.
	 */
	prev_hdr = net_ipv6_find_last_ext_hdr(ipv6);
	if (prev_hdr < 0) {
		ret = -EINVAL;
		goto free_bufs;
	}

	/* We need to update the next header of the packet. */
	net_nbuf_read_u8(ipv6->frags, prev_hdr, &pos, &hdr.nexthdr);

	hdr.reserved = 0;
	hdr.id = net_nbuf_ipv6_fragment_id(buf);
	hdr.offset = htons((offset & 0xfff8) | final);

	/* And we need to update the last header in the IPv6 packet to point to
	 * fragment header.
	 */
	net_nbuf_write_u8(ipv6, ipv6->frags, prev_hdr, &pos,
			  NET_IPV6_NEXTHDR_FRAG);

	/* Then just add the fragmentation header. */
	ret = net_nbuf_append(ipv6, sizeof(hdr), (uint8_t *)&hdr,
			      FRAG_BUF_WAIT);
	if (!ret) {
		ret = -ENOMEM;
		goto free_bufs;
	}

	/* Tie all the fragments together to form an IPv6 packet. Then
	 * update the length of the packet and optionally the checksum.
	 */
	net_buf_frag_add(ipv6, buf->frags);

	ret = net_ipv6_finalize_raw(ipv6, hdr.nexthdr);
	if (ret < 0) {
		NET_DBG("Cannot create IPv6 packet (%d)", ret);
		goto free_bufs;
	}

	NET_DBG("Sending fragment len %zd", net_buf_frags_len(ipv6));

	/* If everything has been ok so far, we can send the packet.
	 * Note that we cannot send this re-constructed packet directly
	 * as the link layer headers will not be properly set (because
	 * we recreated the packet). So pass this packet back to TX
	 * so that the buf is going back to L2 for setup.
	 */
	ret = net_send_data(ipv6);
	if (ret < 0) {
		goto free_bufs;
	}

	/* Then process the rest of the fragments */
	buf->frags = rest;

	return 0;

free_bufs:
	net_nbuf_unref(ipv6);

	if (rest) {
		net_nbuf_unref(rest);
	}

	if (orig_copy) {
		net_nbuf_unref(orig_copy);
	}

	return ret;
}

int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_buf *buf,
				 uint16_t pkt_len)
{
	struct net_buf *frag = buf->frags;
	struct net_buf *prev = NULL;
	struct net_buf *orig_ipv6, *rest;
	int curr_len = 0;
	int status = false;
	uint16_t ipv6_len = 0, offset = 0;
	uint32_t id = sys_rand32_get();
	int ret;

	/* Split the first fragment that contains the IPv6 header into
	 * two pieces. The "orig_ipv6" will only contain the original IPv6
	 * header which is copied into each fragment together with
	 * fragmentation header.
	 */
	ret = net_nbuf_split(buf, frag,
			     net_nbuf_ip_hdr_len(buf) + net_nbuf_ext_len(buf),
			     &orig_ipv6, &rest, FRAG_BUF_WAIT);
	if (ret < 0) {
		return -ENOMEM;
	}

	ipv6_len = net_buf_frags_len(orig_ipv6);

	/* We do not need the first fragment any more. The "rest" will not
	 * have IPv6 header but it will contain the rest of the original data.
	 */
	rest->frags = buf->frags->frags;
	buf->frags = rest;

	frag->frags = NULL;
	net_nbuf_unref(frag);

	frag = buf->frags;

	net_nbuf_set_ipv6_fragment_id(buf, id);

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

			status = send_ipv6_fragment(iface, buf, orig_ipv6,
						    prev, frag, ipv6_len,
						    offset, fit_len, false);
			if (status < 0) {
				goto out;
			}

			offset += curr_len;
			prev = NULL;

			frag = buf;
			curr_len = 0;
		}

		prev = frag;
		frag = frag->frags;
	}

	status = send_ipv6_fragment(iface, buf, orig_ipv6, prev, prev,
				    ipv6_len, offset, 0, true);

	net_nbuf_unref(buf);

out:
	net_nbuf_unref(orig_ipv6);

	return status;
}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

static inline enum net_verdict process_icmpv6_pkt(struct net_buf *buf,
						  struct net_ipv6_hdr *ipv6)
{
	struct net_icmp_hdr *hdr = NET_ICMP_BUF(buf);

	NET_DBG("ICMPv6 %s received type %d code %d",
		net_icmpv6_type2str(hdr->type), hdr->type, hdr->code);

	return net_icmpv6_input(buf, hdr->type, hdr->code);
}

static inline struct net_buf *check_unknown_option(struct net_buf *buf,
						   uint8_t opt_type,
						   uint16_t length)
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
		if (net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
			return NULL;
		}
		/* passthrough */
	case 0x80:
		net_icmpv6_send_error(buf, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_OPTION,
				      (uint32_t)length);
		return NULL;
	}

	return buf;
}

static inline struct net_buf *handle_ext_hdr_options(struct net_buf *buf,
						     struct net_buf *frag,
						     int total_len,
						     uint16_t len,
						     uint16_t offset,
						     uint16_t *pos,
						     enum net_verdict *verdict)
{
	uint8_t opt_type, opt_len;
	uint16_t length = 0, loc;
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
	frag = net_nbuf_read_u8(frag, offset, &loc, &opt_type);
	frag = net_nbuf_read_u8(frag, loc, &loc, &opt_len);
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
			frag = net_rpl_verify_header(buf, frag, loc, &loc,
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
			if (!check_unknown_option(buf, opt_type, length)) {
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

		frag = net_nbuf_read_u8(frag, loc, &loc, &opt_type);
		frag = net_nbuf_read_u8(frag, loc, &loc, &opt_len);
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

static inline bool is_upper_layer_protocol_header(uint8_t proto)
{
	return (proto == IPPROTO_ICMPV6 || proto == IPPROTO_UDP ||
		proto == IPPROTO_TCP);
}

enum net_verdict net_ipv6_process_pkt(struct net_buf *buf)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_BUF(buf);
	int real_len = net_buf_frags_len(buf);
	int pkt_len = (hdr->len[0] << 8) + hdr->len[1] + sizeof(*hdr);
	struct net_buf *frag;
	uint8_t start_of_ext, prev_hdr;
	uint8_t next, next_hdr, length;
	uint8_t first_option;
	uint16_t offset, total_len = 0;

	if (real_len != pkt_len) {
		NET_DBG("IPv6 packet size %d buf len %d", pkt_len, real_len);
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
		if (net_route_get_info(net_nbuf_iface(buf), &hdr->dst, &route,
				       &nexthop)) {
			int ret;

			if (route) {
				net_nbuf_set_iface(buf, route->iface);
			}

			ret = net_route_packet(buf, nexthop);
			if (ret < 0) {
				NET_DBG("Cannot re-route buf %p via %s (%d)",
					buf, net_sprint_ipv6_addr(nexthop),
					ret);
			} else {
				return NET_OK;
			}
		} else
#endif /* CONFIG_NET_ROUTE */

		{
			NET_DBG("IPv6 packet in buf %p not for me", buf);
		}

		net_stats_update_ipv6_drop();
		goto drop;
	}

	/* Check extension headers */
	net_nbuf_set_next_hdr(buf, &hdr->nexthdr);
	net_nbuf_set_ext_len(buf, 0);
	net_nbuf_set_ext_bitmap(buf, 0);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	/* Fast path for main upper layer protocols. The handling of extension
	 * headers can be slow so do this checking here. There cannot
	 * be any extension headers after the upper layer protocol header.
	 */
	next = *(net_nbuf_next_hdr(buf));
	if (is_upper_layer_protocol_header(next)) {
		goto upper_proto;
	}

	/* Go through the extensions */
	frag = buf->frags;
	next = hdr->nexthdr;
	first_option = next;
	offset = sizeof(struct net_ipv6_hdr);
	prev_hdr = &NET_IPV6_BUF(buf)->nexthdr - &NET_IPV6_BUF(buf)->vtc;

	while (frag) {
		enum net_verdict verdict;

		if (is_upper_layer_protocol_header(next)) {
			NET_DBG("IPv6 next header %d", next);
			net_nbuf_set_ext_len(buf, offset -
					     sizeof(struct net_ipv6_hdr));
			goto upper_proto;
		}

		start_of_ext = offset;

		frag = net_nbuf_read_u8(frag, offset, &offset, &next_hdr);
		frag = net_nbuf_read_u8(frag, offset, &offset, &length);
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
			if (net_nbuf_ext_bitmap(buf) &
			    NET_IPV6_EXT_HDR_BITMAP_HBHO) {
				goto bad_hdr;
			}

			net_nbuf_add_ext_bitmap(buf,
						NET_IPV6_EXT_HDR_BITMAP_HBHO);

			frag = handle_ext_hdr_options(buf, frag, real_len,
						      length, offset, &offset,
						      &verdict);
			break;

#if defined(CONFIG_NET_IPV6_FRAGMENT)
		case NET_IPV6_NEXTHDR_FRAG:
			net_nbuf_set_ipv6_hdr_prev(buf, prev_hdr);

			/* The fragment header does not have length field so
			 * we need to step back two bytes and start from the
			 * beginning of the fragment header.
			 */
			return handle_fragment_hdr(buf, frag, real_len,
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
		net_nbuf_set_ext_len(buf, total_len);
	}

	switch (next) {
	case IPPROTO_ICMPV6:
		return process_icmpv6_pkt(buf, hdr);
	case IPPROTO_UDP:
#if defined(CONFIG_NET_UDP)
		return net_conn_input(IPPROTO_UDP, buf);
#else
		return NET_DROP;
#endif
	case IPPROTO_TCP:
#if defined(CONFIG_NET_TCP)
		return net_conn_input(IPPROTO_TCP, buf);
#else
		return NET_DROP;
#endif
	}

drop:
	return NET_DROP;

bad_hdr:
	/* Send error message about parameter problem (RFC 2460)
	 */
	net_icmpv6_send_error(buf, NET_ICMPV6_PARAM_PROBLEM,
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
