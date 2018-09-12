/** @file
 * @brief IPv6 Neighbor related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_IPV6)
#define SYS_LOG_DOMAIN "net/ipv6-nbr"
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

/* IPv6 minimum link MTU specified in RFC 8200 section 5
 * Packet Size Issues
 */
#define MIN_IPV6_MTU NET_IPV6_MTU
#define MAX_IPV6_MTU 0xffff

#if defined(CONFIG_NET_IPV6_ND)
static struct k_delayed_work ipv6_nd_reachable_timer;
static void ipv6_nd_reachable_timeout(struct k_work *work);
#endif

#if defined(CONFIG_NET_IPV6_NBR_CACHE)

#define MAX_MULTICAST_SOLICIT 3
#define MAX_UNICAST_SOLICIT   3
#define DELAY_FIRST_PROBE_TIME K_SECONDS(5) /* RFC 4861 ch 10 */
#define RETRANS_TIMER K_MSEC(1000) /* in ms, RFC 4861 ch 10 */

extern void net_neighbor_data_remove(struct net_nbr *nbr);
extern void net_neighbor_table_clear(struct net_nbr_table *table);

/** Neighbor Solicitation reply timer */
static struct k_delayed_work ipv6_ns_reply_timer;

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
	case NET_IPV6_NBR_STATE_STATIC:
		return "static";
	}

	return "<invalid state>";
}

static void ipv6_nbr_set_state(struct net_nbr *nbr,
			       enum net_ipv6_nbr_state new_state)
{
	if (new_state == net_ipv6_nbr_data(nbr)->state ||
	    net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_STATIC) {
		return;
	}

	NET_DBG("nbr %p %s -> %s", nbr,
		net_ipv6_nbr_state2str(net_ipv6_nbr_data(nbr)->state),
		net_ipv6_nbr_state2str(new_state));

	net_ipv6_nbr_data(nbr)->state = new_state;
}

static inline bool net_is_solicited(struct net_pkt *pkt)
{
	struct net_icmpv6_na_hdr na_hdr;
	int ret;

	ret = net_icmpv6_get_na_hdr(pkt, &na_hdr);
	if (ret < 0) {
		NET_ERR("could not get na_hdr");
		return false;
	}

	return na_hdr.flags & NET_ICMPV6_NA_FLAG_SOLICITED;
}

static inline bool net_is_router(struct net_pkt *pkt)
{
	struct net_icmpv6_na_hdr na_hdr;
	int ret;

	ret = net_icmpv6_get_na_hdr(pkt, &na_hdr);
	if (ret < 0) {
		NET_ERR("could not get na_hdr");
		return false;
	}

	return na_hdr.flags & NET_ICMPV6_NA_FLAG_ROUTER;
}

static inline bool net_is_override(struct net_pkt *pkt)
{
	struct net_icmpv6_na_hdr na_hdr;
	int ret;

	ret = net_icmpv6_get_na_hdr(pkt, &na_hdr);
	if (ret < 0) {
		NET_ERR("could not get na_hdr");
		return false;
	}

	return na_hdr.flags & NET_ICMPV6_NA_FLAG_OVERRIDE;
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

struct iface_cb_data {
	net_nbr_cb_t cb;
	void *user_data;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct iface_cb_data *data = user_data;
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref || nbr->iface != iface) {
			continue;
		}

		data->cb(nbr, data->user_data);
	}
}

void net_ipv6_nbr_foreach(net_nbr_cb_t cb, void *user_data)
{
	struct iface_cb_data cb_data = {
		.cb = cb,
		.user_data = user_data,
	};

	/* Return the neighbors according to network interface. This makes it
	 * easier in the callback to use the neighbor information.
	 */
	net_if_foreach(iface_cb, &cb_data);
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

		if (iface && nbr->iface != iface) {
			continue;
		}

		if (net_ipv6_addr_cmp(&net_ipv6_nbr_data(nbr)->addr, addr)) {
			return nbr;
		}
	}

	return NULL;
}

static inline void nbr_clear_ns_pending(struct net_ipv6_nbr_data *data)
{
	data->send_ns = 0;

	if (data->pending) {
		net_pkt_unref(data->pending);
		data->pending = NULL;
	}
}

static inline void nbr_free(struct net_nbr *nbr)
{
	NET_DBG("nbr %p", nbr);

	nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));

	net_ipv6_nbr_data(nbr)->reachable = 0;
	net_ipv6_nbr_data(nbr)->reachable_timeout = 0;

	net_nbr_unref(nbr);
	net_nbr_unlink(nbr, NULL);
}

bool net_ipv6_nbr_rm(struct net_if *iface, struct in6_addr *addr)
{
	struct net_nbr *nbr;
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	struct net_event_ipv6_nbr info;
#endif

	nbr = nbr_lookup(&net_neighbor.table, iface, addr);
	if (!nbr) {
		return false;
	}

	/* Remove any routes with nbr as nexthop in first place */
	net_route_del_by_nexthop(iface, addr);

	nbr_free(nbr);

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	info.idx = -1;
	net_ipaddr_copy(&info.addr, addr);
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_NBR_DEL,
					iface, (void *) &info,
					sizeof(struct net_event_ipv6_nbr));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_NBR_DEL, iface);
#endif

	return true;
}

#define NS_REPLY_TIMEOUT K_SECONDS(1)

static void ipv6_ns_reply_timeout(struct k_work *work)
{
	s64_t current = k_uptime_get();
	struct net_nbr *nbr = NULL;
	struct net_ipv6_nbr_data *data;
	int i;

	ARG_UNUSED(work);

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		s64_t remaining;
		nbr = get_nbr(i);

		if (!nbr || !nbr->ref) {
			continue;
		}

		data = net_ipv6_nbr_data(nbr);
		if (!data) {
			continue;
		}

		if (!data->send_ns) {
			continue;
		}

		remaining = data->send_ns + NS_REPLY_TIMEOUT - current;

		if (remaining > 0) {
			if (!k_delayed_work_remaining_get(
						&ipv6_ns_reply_timer)) {
				k_delayed_work_submit(&ipv6_ns_reply_timer,
						remaining);
			}

			continue;
		}

		data->send_ns = 0;

		/* We did not receive reply to a sent NS */
		if (!data->pending) {
			/* Silently return, this is not an error as the work
			 * cannot be cancelled in certain cases.
			 */
			continue;
		}

		NET_DBG("NS nbr %p pending %p timeout to %s", nbr,
			data->pending,
			net_sprint_ipv6_addr(
					&NET_IPV6_HDR(data->pending)->dst));

		/* To unref when pending variable was set */
		net_pkt_unref(data->pending);

		/* To unref the original pkt allocation */
		net_pkt_unref(data->pending);

		data->pending = NULL;

		net_nbr_unref(nbr);
	}
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
	net_ipv6_nbr_data(nbr)->send_ns = 0;

#if defined(CONFIG_NET_IPV6_ND)
	net_ipv6_nbr_data(nbr)->reachable = 0;
	net_ipv6_nbr_data(nbr)->reachable_timeout = 0;
#endif
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
		NET_DBG("%s %s from %s to %s", action, pkt_str,         \
			net_sprint_ipv6_addr(src),                      \
			net_sprint_ipv6_addr(dst));                     \
	} while (0)

#define dbg_addr_recv(pkt_str, src, dst)	\
	dbg_addr("Received", pkt_str, src, dst)

#define dbg_addr_sent(pkt_str, src, dst)	\
	dbg_addr("Sent", pkt_str, src, dst)

#define dbg_addr_with_tgt(action, pkt_str, src, dst, target)		\
	do {								\
		NET_DBG("%s %s from %s to %s, target %s", action,       \
			pkt_str,                                        \
			net_sprint_ipv6_addr(src),                      \
			net_sprint_ipv6_addr(dst),                      \
			net_sprint_ipv6_addr(target));                  \
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
	int ret;
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	struct net_event_ipv6_nbr info;
#endif

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

	if (net_nbr_link(nbr, iface, lladdr) == -EALREADY &&
	    net_ipv6_nbr_data(nbr)->state != NET_IPV6_NBR_STATE_STATIC) {
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
		ret = net_ipv6_send_ns(iface, NULL, NULL, NULL, addr, false);
		if (ret < 0) {
			NET_DBG("Cannot send NS (%d)", ret);
		}
	}

	NET_DBG("[%d] nbr %p state %d router %d IPv6 %s ll %s iface %p",
		nbr->idx, nbr, state, is_router,
		net_sprint_ipv6_addr(addr),
		net_sprint_ll_addr(lladdr->addr, lladdr->len),
		nbr->iface);

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	info.idx = nbr->idx;
	net_ipaddr_copy(&info.addr, addr);
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_NBR_ADD,
					iface, (void *) &info,
					sizeof(struct net_event_ipv6_nbr));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_NBR_ADD, iface);
#endif

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
#else
const char *net_ipv6_nbr_state2str(enum net_ipv6_nbr_state state)
{
	return "<unknown state>";
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

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
	 * the packet, but only if both src and dest are using
	 * same technology meaning that link address length is the same.
	 */
	if (net_pkt_forwarding(pkt) &&
	    net_pkt_orig_iface(pkt) == net_pkt_iface(pkt)) {
		return pkt;
	}

	reserve = net_if_get_ll_reserve(net_pkt_iface(pkt), addr);
	if (reserve == net_pkt_ll_reserve(pkt)) {
		return pkt;
	}

	NET_DBG("Adjust reserve old %d new %d",
		net_pkt_ll_reserve(pkt), reserve);

	/* Normally these debug prints are not needed so we do not print them
	 * always. If your packets get dropped for some reason by L2, then
	 * you can enable this block to see the IPv6 and LL addresses that
	 * are used.
	 */
	if (0) {
		NET_DBG("ll src %s",
			net_sprint_ll_addr(net_pkt_lladdr_src(pkt)->addr,
					   net_pkt_lladdr_src(pkt)->len));
		NET_DBG("ll dst %s",
			net_sprint_ll_addr(net_pkt_lladdr_dst(pkt)->addr,
					   net_pkt_lladdr_dst(pkt)->len));
		NET_DBG("ip src %s",
			net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src));
		NET_DBG("ip dst %s",
			net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
	}

	net_pkt_set_ll_reserve(pkt, reserve);

	orig_frag = pkt->frags;
	copy_len = orig_frag->len;
	pos = 0;

	pkt->frags = NULL;
	room_len = 0;
	frag = NULL;

	while (orig_frag) {
		if (!room_len) {
			frag = net_pkt_get_frag(pkt, NET_BUF_TIMEOUT);
			if (!frag) {
				net_pkt_unref(pkt);
				net_pkt_frag_unref(orig_frag);
				return NULL;
			}

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

static struct in6_addr *check_route(struct net_if *iface,
				    struct in6_addr *dst,
				    bool *try_route)
{
	struct in6_addr *nexthop = NULL;
	struct net_route_entry *route;
	struct net_if_router *router;

	route = net_route_lookup(iface, dst);
	if (route) {
		nexthop = net_route_get_nexthop(route);

		NET_DBG("Route %p nexthop %s", route,
			nexthop ? net_sprint_ipv6_addr(nexthop) : "<unknown>");

		if (!nexthop) {
			net_route_del(route);

			net_rpl_global_repair(route);

			NET_DBG("No route to host %s",
				net_sprint_ipv6_addr(dst));

			return NULL;
		}
	} else {
		/* No specific route to this host, use the default
		 * route instead.
		 */
		router = net_if_ipv6_router_find_default(NULL, dst);
		if (!router) {
			NET_DBG("No default route to %s",
				net_sprint_ipv6_addr(dst));

			/* Try to send the packet anyway */
			nexthop = dst;
			if (try_route) {
				*try_route = true;
			}

			return nexthop;
		}

		nexthop = &router->address.in6_addr;

		NET_DBG("Router %p nexthop %s", router,
			net_sprint_ipv6_addr(nexthop));
	}

	return nexthop;
}

struct net_pkt *net_ipv6_prepare_for_send(struct net_pkt *pkt)
{
	struct in6_addr *nexthop = NULL;
	struct net_if *iface = NULL;
	struct net_nbr *nbr;
	int ret;

	NET_ASSERT(pkt && pkt->frags);

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	/* If we have already fragmented the packet, the fragment id will
	 * contain a proper value and we can skip other checks.
	 */
	if (net_pkt_ipv6_fragment_id(pkt) == 0) {
		size_t pkt_len = net_pkt_get_len(pkt);

		if (pkt_len > NET_IPV6_MTU) {
			ret = net_ipv6_send_fragmented_pkt(net_pkt_iface(pkt),
							   pkt, pkt_len);
			if (ret < 0) {
				NET_DBG("Cannot fragment IPv6 pkt (%d)", ret);

				if (ret == -ENOMEM) {
					/* Try to send the packet if we could
					 * not allocate enough network packets
					 * and hope the original large packet
					 * can be sent ok.
					 */
					goto ignore_frag_error;
				}
			}

			/* We "fake" the sending of the packet here so that
			 * tcp.c:tcp_retry_expired() will increase the ref
			 * count when re-sending the packet. This is crucial
			 * thing to do here and will cause free memory access
			 * if not done.
			 */
			net_pkt_set_sent(pkt, true);

			/* We need to unref here because we simulate the packet
			 * sending.
			 */
			net_pkt_unref(pkt);

			/* No need to continue with the sending as the packet
			 * is now split and its fragments will be sent
			 * separately to network.
			 */
			return NULL;
		}
	}
ignore_frag_error:
#endif /* CONFIG_NET_IPV6_FRAGMENT */

	/* Workaround Linux bug, see:
	 * https://github.com/zephyrproject-rtos/zephyr/issues/3111
	 */
	if (atomic_test_bit(net_pkt_iface(pkt)->if_dev->flags,
			    NET_IF_POINTOPOINT)) {
		/* Update RPL header */
		if (net_rpl_update_header(pkt, &NET_IPV6_HDR(pkt)->dst) < 0) {
			net_pkt_unref(pkt);
			return NULL;
		}

		return pkt;
	}

	/* If the IPv6 destination address is not link local, then try to get
	 * the next hop from routing table if we have multi interface routing
	 * enabled. The reason for this is that the neighbor cache will not
	 * contain public IPv6 address information so in that case we should
	 * not enter this branch.
	 */
	if ((net_pkt_lladdr_dst(pkt)->addr &&
	     ((IS_ENABLED(CONFIG_NET_ROUTING) &&
	      net_is_ipv6_ll_addr(&NET_IPV6_HDR(pkt)->dst)) ||
	      !IS_ENABLED(CONFIG_NET_ROUTING))) ||
	    net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
		/* Update RPL header */
		if (net_rpl_update_header(pkt, &NET_IPV6_HDR(pkt)->dst) < 0) {
			net_pkt_unref(pkt);
			return NULL;
		}

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
		bool try_route = false;

		nexthop = check_route(NULL, &NET_IPV6_HDR(pkt)->dst,
				      &try_route);
		if (!nexthop) {
			net_pkt_unref(pkt);
			return NULL;
		}

		if (try_route) {
			goto try_send;
		}
	}

	if (!iface) {
		/* This means that the dst was not onlink, so try to
		 * figure out the interface using nexthop instead.
		 */
		if (net_if_ipv6_addr_onlink(&iface, nexthop)) {
			net_pkt_set_iface(pkt, iface);
		} else {
			iface = net_pkt_iface(pkt);
		}

		/* If the above check returns null, we try to send
		 * the packet and hope for the best.
		 */
	}

try_send:
	if (net_rpl_update_header(pkt, nexthop) < 0) {
		net_pkt_unref(pkt);
		return NULL;
	}

	nbr = nbr_lookup(&net_neighbor.table, iface, nexthop);

	NET_DBG("Neighbor lookup %p (%d) iface %p addr %s state %s", nbr,
		nbr ? nbr->idx : NET_NBR_LLADDR_UNKNOWN, iface,
		net_sprint_ipv6_addr(nexthop),
		nbr ? net_ipv6_nbr_state2str(net_ipv6_nbr_data(nbr)->state) :
		"-");

	if (nbr && nbr->idx != NET_NBR_LLADDR_UNKNOWN) {
		struct net_linkaddr_storage *lladdr;

		lladdr = net_nbr_get_lladdr(nbr->idx);

		net_pkt_lladdr_dst(pkt)->addr = lladdr->addr;
		net_pkt_lladdr_dst(pkt)->len = lladdr->len;

		NET_DBG("Neighbor %p addr %s", nbr,
			net_sprint_ll_addr(lladdr->addr, lladdr->len));

		/* Start the NUD if we are in STALE state.
		 * See RFC 4861 ch 7.3.3 for details.
		 */
#if defined(CONFIG_NET_IPV6_ND)
		if (net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_STALE) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_DELAY);

			net_ipv6_nbr_data(nbr)->reachable = k_uptime_get();
			net_ipv6_nbr_data(nbr)->reachable_timeout =
							DELAY_FIRST_PROBE_TIME;

			if (!k_delayed_work_remaining_get(
						&ipv6_nd_reachable_timer)) {
				k_delayed_work_submit(
					&ipv6_nd_reachable_timer,
					DELAY_FIRST_PROBE_TIME);
			}
		}
#endif

		return update_ll_reserve(pkt, nexthop);
	}

#if defined(CONFIG_NET_IPV6_ND)
	/* We need to send NS and wait for NA before sending the packet. */
	ret = net_ipv6_send_ns(net_pkt_iface(pkt), pkt,
			       &NET_IPV6_HDR(pkt)->src, NULL,
			       nexthop, false);
	if (ret < 0) {
		/* In case of an error, the NS send function will unref
		 * the pkt.
		 */
		NET_DBG("Cannot send NS (%d)", ret);
		return NULL;
	}

	NET_DBG("pkt %p (frag %p) will be sent later", pkt, pkt->frags);
#else
	ARG_UNUSED(ret);

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
	if (net_if_get_link_addr(iface)->len == 6) {
		return 8;
	} else if (net_if_get_link_addr(iface)->len == 8) {
		return 16;
	}

	/* What else could it be? */
	NET_ASSERT_INFO(0, "Invalid link address length %d",
			net_if_get_link_addr(iface)->len);

	return 0;
}

static inline void set_llao(struct net_linkaddr *lladdr,
			    u8_t *llao, u8_t llao_len, u8_t type)
{
	llao[NET_ICMPV6_OPT_TYPE_OFFSET] = type;
	llao[NET_ICMPV6_OPT_LEN_OFFSET] = llao_len >> 3;

	memcpy(&llao[NET_ICMPV6_OPT_DATA_OFFSET], lladdr->addr, lladdr->len);

	(void)memset(&llao[NET_ICMPV6_OPT_DATA_OFFSET + lladdr->len], 0,
		     llao_len - lladdr->len - 2);
}

static void setup_headers(struct net_pkt *pkt, u8_t nd6_len,
			  u8_t icmp_type)
{
	net_buf_add(pkt->frags,
		    sizeof(struct net_ipv6_hdr) +
		    sizeof(struct net_icmp_hdr));

	NET_IPV6_HDR(pkt)->vtc = 0x60;
	NET_IPV6_HDR(pkt)->tcflow = 0;
	NET_IPV6_HDR(pkt)->flow = 0;
	NET_IPV6_HDR(pkt)->len = htons(NET_ICMPH_LEN + nd6_len);

	NET_IPV6_HDR(pkt)->nexthdr = IPPROTO_ICMPV6;
	NET_IPV6_HDR(pkt)->hop_limit = NET_IPV6_ND_HOP_LIMIT;

	/* In this special case where we know there are no long extension
	 * headers, so we can use this header cast.
	 */
	net_pkt_icmp_data(pkt)->type = icmp_type;
	net_pkt_icmp_data(pkt)->code = 0;
}

static inline struct net_nbr *handle_ns_neighbor(struct net_pkt *pkt,
						 u8_t ll_len,
						 u16_t sllao_offset)
{
	struct net_linkaddr_storage lladdr;
	struct net_linkaddr nbr_lladdr;
	struct net_buf *frag;
	u16_t pos;

	lladdr.len = 8 * ll_len - 2;

	frag = net_frag_read(pkt->frags, sllao_offset,
			     &pos, lladdr.len, lladdr.addr);
	if (!frag && pos == 0xffff) {
		return NULL;
	}

	nbr_lladdr.len = lladdr.len;
	nbr_lladdr.addr = lladdr.addr;

	/**
	 * IEEE802154 lladdress is 8 bytes long, so it requires
	 * 2 * 8 bytes - 2 - padding.
	 * The formula above needs to be adjusted.
	 */
	if (net_pkt_lladdr_src(pkt)->len < nbr_lladdr.len) {
		nbr_lladdr.len = net_pkt_lladdr_src(pkt)->len;
	}

	return nbr_add(pkt, &nbr_lladdr, false, NET_IPV6_NBR_STATE_INCOMPLETE);
}

int net_ipv6_send_na(struct net_if *iface, const struct in6_addr *src,
		     const struct in6_addr *dst, const struct in6_addr *tgt,
		     u8_t flags)
{
	struct net_icmpv6_na_hdr na_hdr;
	struct net_pkt *pkt;
	struct net_buf *frag;
	u8_t llao_len;
	int ret;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				     ND_NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_frag(pkt, ND_NET_BUF_TIMEOUT);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	llao_len = get_llao_len(iface);

	net_pkt_set_ipv6_ext_len(pkt, 0);

	setup_headers(pkt, sizeof(struct net_icmpv6_na_hdr) + llao_len,
		      NET_ICMPV6_NA);

	net_buf_add(frag, sizeof(struct net_icmpv6_na_hdr) + llao_len);

	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, src);
	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, dst);
	net_ipaddr_copy(&na_hdr.tgt, tgt);

	set_llao(net_if_get_link_addr(net_pkt_iface(pkt)),
		 (u8_t *)net_pkt_icmp_data(pkt) + sizeof(struct net_icmp_hdr) +
					      sizeof(struct net_icmpv6_na_hdr),
		 llao_len, NET_ICMPV6_ND_OPT_TLLAO);

	na_hdr.flags = flags;
	ret = net_icmpv6_set_na_hdr(pkt, &na_hdr);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	pkt->frags->len = NET_IPV6ICMPH_LEN +
		sizeof(struct net_icmpv6_na_hdr) + llao_len;

	ret = net_icmpv6_set_chksum(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	dbg_addr_sent_tgt("Neighbor Advertisement",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &na_hdr.tgt);

	if (net_send_data(pkt) < 0) {
		goto drop;
	}

	net_stats_update_ipv6_nd_sent(net_pkt_iface(pkt));

	return 0;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));
	net_pkt_unref(pkt);

	return -EINVAL;
}

static void ns_routing_info(struct net_pkt *pkt,
			    struct in6_addr *nexthop,
			    struct in6_addr *tgt)
{
#if defined(CONFIG_NET_DEBUG_IPV6) && (CONFIG_SYS_LOG_NET_LEVEL > 3)
	char out[NET_IPV6_ADDR_LEN];

	snprintk(out, sizeof(out), "%s", net_sprint_ipv6_addr(nexthop));

	if (net_ipv6_addr_cmp(nexthop, tgt)) {
		NET_DBG("Routing to %s iface %p", out, net_pkt_iface(pkt));
	} else {
		NET_DBG("Routing to %s via %s iface %p",
			net_sprint_ipv6_addr(tgt), out, net_pkt_iface(pkt));
	}
#endif
}

static enum net_verdict handle_ns_input(struct net_pkt *pkt)
{
	u16_t total_len = net_pkt_get_len(pkt);
	u8_t prev_opt_len = 0;
	u8_t flags = 0;
	bool routing = false;
	struct net_icmpv6_nd_opt_hdr nd_opt_hdr;
	struct net_icmpv6_ns_hdr ns_hdr;
	struct net_if_addr *ifaddr;
	struct in6_addr *tgt;
	const struct in6_addr *src;
	size_t left_len;
	int ret;

	ret = net_icmpv6_get_ns_hdr(pkt, &ns_hdr);
	if (ret < 0) {
		NET_ERR("NULL NS header - dropping");
		goto drop;
	}

	dbg_addr_recv_tgt("Neighbor Solicitation",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &ns_hdr.tgt);

	net_stats_update_ipv6_nd_recv(net_pkt_iface(pkt));

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_ns_hdr))) ||
	    (NET_IPV6_HDR(pkt)->hop_limit != NET_IPV6_ND_HOP_LIMIT)) {
		if (net_is_ipv6_addr_mcast(&ns_hdr.tgt)) {
			struct net_icmp_hdr icmp_hdr;

			ret = net_icmpv6_get_hdr(pkt, &icmp_hdr);
			if (ret < 0 || icmp_hdr.code != 0) {
				NET_DBG("Preliminary check failed %u/%zu, "
					"code %u, hop %u",
					total_len,
					(sizeof(struct net_ipv6_hdr) +
					 sizeof(struct net_icmp_hdr) +
					 sizeof(struct net_icmpv6_ns_hdr)),
					icmp_hdr.code,
					NET_IPV6_HDR(pkt)->hop_limit);
				goto drop;
			}
		}
	}

	net_pkt_set_ipv6_ext_opt_len(pkt, sizeof(struct net_icmpv6_ns_hdr));

	left_len = net_pkt_get_len(pkt) - (sizeof(struct net_ipv6_hdr) +
					   sizeof(struct net_icmp_hdr));

	ret = net_icmpv6_get_nd_opt_hdr(pkt, &nd_opt_hdr);

	while (!ret && net_pkt_ipv6_ext_opt_len(pkt) < left_len) {
		if (!nd_opt_hdr.len) {
			break;
		}

		switch (nd_opt_hdr.type) {
		case NET_ICMPV6_ND_OPT_SLLAO:
			if (net_is_ipv6_addr_unspecified(
				    &NET_IPV6_HDR(pkt)->src)) {
				goto drop;
			}

			if (nd_opt_hdr.len > 2) {
				NET_ERR("Too long source link-layer address "
					"in NS option");
				goto drop;
			}

			if (!handle_ns_neighbor(pkt, nd_opt_hdr.len,
						net_pkt_ip_hdr_len(pkt) +
						net_pkt_ipv6_ext_len(pkt) +
						sizeof(struct net_icmp_hdr) +
						net_pkt_ipv6_ext_opt_len(pkt) +
						1 + 1)) {
				goto drop;
			}
			break;

		default:
			NET_DBG("Unknown ND option 0x%x", nd_opt_hdr.type);
			break;
		}

		prev_opt_len = net_pkt_ipv6_ext_opt_len(pkt);

		net_pkt_set_ipv6_ext_opt_len(pkt,
					     net_pkt_ipv6_ext_opt_len(pkt) +
					     (nd_opt_hdr.len << 3));

		if (prev_opt_len >= net_pkt_ipv6_ext_opt_len(pkt)) {
			NET_ERR("Corrupted NS message");
			goto drop;
		}

		ret = net_icmpv6_get_nd_opt_hdr(pkt, &nd_opt_hdr);
	}

	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		ifaddr = net_if_ipv6_addr_lookup(&ns_hdr.tgt, NULL);
	} else {
		ifaddr = net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
							  &ns_hdr.tgt);
	}

	if (!ifaddr) {
		if (IS_ENABLED(CONFIG_NET_ROUTING)) {
			struct in6_addr *nexthop;

			nexthop = check_route(NULL, &ns_hdr.tgt, NULL);
			if (nexthop) {
				ns_routing_info(pkt, nexthop, &ns_hdr.tgt);

				/* Note that the target is not the address of
				 * the "nethop" as that is a link-local address
				 * which is not routable.
				 */
				tgt = &ns_hdr.tgt;

				/* Source address must be one of our real
				 * interface address where the packet was
				 * received.
				 */
				src = net_if_ipv6_select_src_addr(
					net_pkt_iface(pkt),
					&NET_IPV6_HDR(pkt)->src);
				if (!src) {
					NET_DBG("No interface address for "
						"dst %s iface %p",
						net_sprint_ipv6_addr(
						      &NET_IPV6_HDR(pkt)->src),
						net_pkt_iface(pkt));
					goto drop;
				}

				routing = true;
				goto nexthop_found;
			}
		}

		NET_DBG("No such interface address %s",
			net_sprint_ipv6_addr(&ns_hdr.tgt));
		goto drop;
	} else {
		tgt = &ifaddr->address.in6_addr;

		/* As we swap the addresses later, the source will correctly
		 * have our address.
		 */
		src = &NET_IPV6_HDR(pkt)->src;
	}

nexthop_found:

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
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, &ns_hdr.tgt);
		flags = NET_ICMPV6_NA_FLAG_SOLICITED |
			NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	}

	if (routing) {
		/* No need to do NUD here when the target is being routed. */
		goto send_na;
	}

	/* Neighbor Unreachability Detection (NUD) */
	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		ifaddr = net_if_ipv6_addr_lookup(&NET_IPV6_HDR(pkt)->dst,
						 NULL);
	} else {
		ifaddr = net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
						      &NET_IPV6_HDR(pkt)->dst);
	}

	if (ifaddr) {
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst,
				&NET_IPV6_HDR(pkt)->src);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, &ns_hdr.tgt);
		src = &NET_IPV6_HDR(pkt)->src;
		tgt = &ifaddr->address.in6_addr;
		flags = NET_ICMPV6_NA_FLAG_SOLICITED |
			NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	} else {
		NET_DBG("NUD failed");
		goto drop;
	}

send_na:
	ret = net_ipv6_send_na(net_pkt_iface(pkt),
			       src,
			       &NET_IPV6_HDR(pkt)->dst,
			       tgt,
			       flags);
	if (!ret) {
		net_pkt_unref(pkt);
		return NET_OK;
	}

	NET_DBG("Cannot send NA (%d)", ret);

	return NET_DROP;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));

	return NET_DROP;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
static void ipv6_nd_reachable_timeout(struct k_work *work)
{
	s64_t current = k_uptime_get();
	struct net_nbr *nbr = NULL;
	struct net_ipv6_nbr_data *data = NULL;
	int ret;
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		s64_t remaining;

		nbr = get_nbr(i);
		if (!nbr || !nbr->ref) {
			continue;
		}

		data = net_ipv6_nbr_data(nbr);
		if (!data) {
			continue;
		}

		if (!data->reachable) {
			continue;
		}

		remaining = data->reachable + data->reachable_timeout - current;
		if (remaining > 0) {
			if (!k_delayed_work_remaining_get(
						&ipv6_nd_reachable_timer)) {
				k_delayed_work_submit(&ipv6_nd_reachable_timer,
						      remaining);
			}

			continue;
		}

		data->reachable = 0;

		if (net_rpl_get_interface() && nbr->iface ==
		    net_rpl_get_interface()) {
			/* The address belongs to RPL network, no need to
			 * activate full neighbor reachable rules in this case.
			 * Mark the neighbor always reachable.
			 */
			data->state = NET_IPV6_NBR_STATE_REACHABLE;
			continue;
		}

		switch (data->state) {
		case NET_IPV6_NBR_STATE_STATIC:
			NET_ASSERT_INFO(false, "Static entry shall never timeout");
			break;

		case NET_IPV6_NBR_STATE_INCOMPLETE:
			if (data->ns_count >= MAX_MULTICAST_SOLICIT) {
				nbr_free(nbr);
			} else {
				data->ns_count++;

				NET_DBG("nbr %p incomplete count %u", nbr,
					data->ns_count);

				ret = net_ipv6_send_ns(nbr->iface, NULL, NULL,
						       NULL, &data->addr,
						       false);
				if (ret < 0) {
					NET_DBG("Cannot send NS (%d)", ret);
				}
			}
			break;

		case NET_IPV6_NBR_STATE_REACHABLE:
			data->state = NET_IPV6_NBR_STATE_STALE;

			NET_DBG("nbr %p moving %s state to STALE (%d)",
				nbr, net_sprint_ipv6_addr(&data->addr),
				data->state);
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
				nbr, net_sprint_ipv6_addr(&data->addr),
				data->state);

			/* Intentionally continuing to probe state */

		case NET_IPV6_NBR_STATE_PROBE:
			if (data->ns_count >= MAX_UNICAST_SOLICIT) {
				struct net_if_router *router;

				router = net_if_ipv6_router_lookup(nbr->iface,
								   &data->addr);
				if (router && !router->is_infinite) {
					NET_DBG("nbr %p address %s PROBE ended (%d)",
						nbr,
						net_sprint_ipv6_addr(
								&data->addr),
						data->state);

					net_if_ipv6_router_rm(router);
					nbr_free(nbr);
				}
			} else {
				data->ns_count++;

				NET_DBG("nbr %p probe count %u", nbr,
					data->ns_count);

				ret = net_ipv6_send_ns(nbr->iface, NULL, NULL,
						       NULL, &data->addr,
						       false);
				if (ret < 0) {
					NET_DBG("Cannot send NS (%d)", ret);
				}

				net_ipv6_nbr_data(nbr)->reachable =
								k_uptime_get();
				net_ipv6_nbr_data(nbr)->reachable_timeout =
								RETRANS_TIMER;

				if (!k_delayed_work_remaining_get(
						&ipv6_nd_reachable_timer)) {
					k_delayed_work_submit(
						&ipv6_nd_reachable_timer,
						RETRANS_TIMER);
				}
			}
			break;
		}
	}
}

void net_ipv6_nbr_set_reachable_timer(struct net_if *iface,
				      struct net_nbr *nbr)
{
	u32_t time;

	time = net_if_ipv6_get_reachable_time(iface);

	NET_ASSERT_INFO(time, "Zero reachable timeout!");

	NET_DBG("Starting reachable timer nbr %p data %p time %d ms",
		nbr, net_ipv6_nbr_data(nbr), time);

	net_ipv6_nbr_data(nbr)->reachable = k_uptime_get();
	net_ipv6_nbr_data(nbr)->reachable_timeout = time;

	if (!k_delayed_work_remaining_get(&ipv6_nd_reachable_timer)) {
		k_delayed_work_submit(&ipv6_nd_reachable_timer, time);
	}
}
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static inline bool handle_na_neighbor(struct net_pkt *pkt,
				      struct net_icmpv6_na_hdr *na_hdr,
				      u16_t tllao_offset)
{
	bool lladdr_changed = false;
	struct net_nbr *nbr;
	struct net_linkaddr_storage lladdr = { 0 };
	struct net_linkaddr_storage *cached_lladdr;
	struct net_pkt *pending;
	struct net_buf *frag;
	u16_t pos;

	nbr = nbr_lookup(&net_neighbor.table, net_pkt_iface(pkt),
			 &na_hdr->tgt);

	NET_DBG("Neighbor lookup %p iface %p addr %s", nbr,
		net_pkt_iface(pkt),
		net_sprint_ipv6_addr(&na_hdr->tgt));

	if (!nbr) {
		nbr_print();

		NET_DBG("No such neighbor found, msg discarded");
		return false;
	}

	if (tllao_offset) {
		lladdr.len = net_if_get_link_addr(net_pkt_iface(pkt))->len;

		frag = net_frag_read(pkt->frags, tllao_offset,
				     &pos, lladdr.len, lladdr.addr);
		if (!frag && pos == 0xffff) {
			return false;
		}
	}

	if (nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
		struct net_linkaddr nbr_lladdr;

		if (!tllao_offset) {
			NET_DBG("No target link layer address.");
			return false;
		}

		nbr_lladdr.len = lladdr.len;
		nbr_lladdr.addr = lladdr.addr;

		if (net_nbr_link(nbr, net_pkt_iface(pkt), &nbr_lladdr)) {
			nbr_free(nbr);
			return false;
		}

		NET_DBG("[%d] nbr %p state %d IPv6 %s ll %s",
			nbr->idx, nbr, net_ipv6_nbr_data(nbr)->state,
			net_sprint_ipv6_addr(&na_hdr->tgt),
			net_sprint_ll_addr(nbr_lladdr.addr, nbr_lladdr.len));
	}

	cached_lladdr = net_nbr_get_lladdr(nbr->idx);
	if (!cached_lladdr) {
		NET_DBG("No lladdr but index defined");
		return false;
	}

	if (tllao_offset) {
		lladdr_changed = memcmp(lladdr.addr,
					cached_lladdr->addr,
					cached_lladdr->len);
	}

	/* Update the cached address if we do not yet known it */
	if (net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_INCOMPLETE) {
		if (!tllao_offset) {
			return false;
		}

		if (lladdr_changed) {
			dbg_update_neighbor_lladdr_raw(lladdr.addr,
						       cached_lladdr,
						       &na_hdr->tgt);

			net_linkaddr_set(cached_lladdr, lladdr.addr,
					 cached_lladdr->len);
		}

		if (net_is_solicited(pkt)) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);
			net_ipv6_nbr_data(nbr)->ns_count = 0;

			/* We might have active timer from PROBE */
			net_ipv6_nbr_data(nbr)->reachable = 0;
			net_ipv6_nbr_data(nbr)->reachable_timeout = 0;

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
	    (!net_is_override(pkt) && tllao_offset && !lladdr_changed)) {

		if (lladdr_changed) {
			dbg_update_neighbor_lladdr_raw(
				lladdr.addr, cached_lladdr, &na_hdr->tgt);

			net_linkaddr_set(cached_lladdr, lladdr.addr,
					 cached_lladdr->len);
		}

		if (net_is_solicited(pkt)) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);

			/* We might have active timer from PROBE */
			net_ipv6_nbr_data(nbr)->reachable = 0;
			net_ipv6_nbr_data(nbr)->reachable_timeout = 0;

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
	u16_t tllao_offset = 0;
	u8_t prev_opt_len = 0;
	struct net_icmpv6_nd_opt_hdr nd_opt_hdr;
	struct net_icmpv6_na_hdr na_hdr;
	struct net_if_addr *ifaddr;
	size_t left_len;
	int ret;

	ret = net_icmpv6_get_na_hdr(pkt, &na_hdr);
	if (ret < 0) {
		NET_ERR("NULL NA header - dropping");
		goto drop;
	}

	dbg_addr_recv_tgt("Neighbor Advertisement",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &na_hdr.tgt);

	net_stats_update_ipv6_nd_recv(net_pkt_iface(pkt));

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_na_hdr) +
			  sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	    (NET_IPV6_HDR(pkt)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    net_is_ipv6_addr_mcast(&na_hdr.tgt) ||
	    (net_is_solicited(pkt) &&
	     net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst))) {
		struct net_icmp_hdr icmp_hdr;

		ret = net_icmpv6_get_hdr(pkt, &icmp_hdr);
		if (ret < 0 || icmp_hdr.code != 0) {
			goto drop;
		}
	}

	net_pkt_set_ipv6_ext_opt_len(pkt, sizeof(struct net_icmpv6_na_hdr));

	left_len = net_pkt_get_len(pkt) - (sizeof(struct net_ipv6_hdr) +
					   sizeof(struct net_icmp_hdr));

	ret = net_icmpv6_get_nd_opt_hdr(pkt, &nd_opt_hdr);

	while (!ret && net_pkt_ipv6_ext_opt_len(pkt) < left_len) {
		if (!nd_opt_hdr.len) {
			break;
		}

		switch (nd_opt_hdr.type) {
		case NET_ICMPV6_ND_OPT_TLLAO:
			tllao_offset = net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_icmp_hdr) +
				net_pkt_ipv6_ext_opt_len(pkt) + 1 + 1;
			break;

		default:
			NET_DBG("Unknown ND option 0x%x", nd_opt_hdr.type);
			break;
		}

		prev_opt_len = net_pkt_ipv6_ext_opt_len(pkt);

		net_pkt_set_ipv6_ext_opt_len(pkt,
					     net_pkt_ipv6_ext_opt_len(pkt) +
					     (nd_opt_hdr.len << 3));

		if (prev_opt_len >= net_pkt_ipv6_ext_opt_len(pkt)) {
			NET_ERR("Corrupted NA message");
			goto drop;
		}

		ret = net_icmpv6_get_nd_opt_hdr(pkt, &nd_opt_hdr);
	}

	ifaddr = net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
						  &na_hdr.tgt);
	if (ifaddr) {
		NET_DBG("Interface %p already has address %s",
			net_pkt_iface(pkt),
			net_sprint_ipv6_addr(&na_hdr.tgt));

#if defined(CONFIG_NET_IPV6_DAD)
		if (ifaddr->addr_state == NET_ADDR_TENTATIVE) {
			dad_failed(net_pkt_iface(pkt), &na_hdr.tgt);
		}
#endif /* CONFIG_NET_IPV6_DAD */

		goto drop;
	}

	if (!handle_na_neighbor(pkt, &na_hdr, tllao_offset)) {
		goto drop;
	}

	net_stats_update_ipv6_nd_sent(net_pkt_iface(pkt));

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));

	return NET_DROP;
}

int net_ipv6_send_ns(struct net_if *iface,
		     struct net_pkt *pending,
		     struct in6_addr *src,
		     struct in6_addr *dst,
		     struct in6_addr *tgt,
		     bool is_my_address)
{
	struct net_icmpv6_ns_hdr ns_hdr;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_nbr *nbr;
	u8_t llao_len;
	int ret;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				     ND_NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_frag(pkt, ND_NET_BUF_TIMEOUT);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_ext_len(pkt, 0);

	net_pkt_ll_clear(pkt);

	llao_len = get_llao_len(net_pkt_iface(pkt));

	setup_headers(pkt, sizeof(struct net_icmpv6_ns_hdr) + llao_len,
		      NET_ICMPV6_NS);

	net_buf_add(frag, sizeof(struct net_icmpv6_ns_hdr));

	if (!dst) {
		net_ipv6_addr_create_solicited_node(tgt,
						    &NET_IPV6_HDR(pkt)->dst);
	} else {
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, dst);
	}

	net_ipaddr_copy(&ns_hdr.tgt, tgt);
	ret = net_icmpv6_set_ns_hdr(pkt, &ns_hdr);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	if (is_my_address) {
		u16_t len = ntohs(NET_IPV6_HDR(pkt)->len);
		/* DAD */
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				net_ipv6_unspecified_address());
		NET_IPV6_HDR(pkt)->len = htons(len - llao_len);
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
			if (pending) {
				net_pkt_unref(pending);
			}

			goto drop;
		}

		net_buf_add(frag, llao_len);

		set_llao(net_if_get_link_addr(net_pkt_iface(pkt)),
			 (u8_t *)net_pkt_icmp_data(pkt) +
					sizeof(struct net_icmp_hdr) +
					sizeof(struct net_icmpv6_ns_hdr),
			 llao_len, NET_ICMPV6_ND_OPT_SLLAO);
	}

	ret = net_icmpv6_set_chksum(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	nbr = nbr_lookup(&net_neighbor.table, net_pkt_iface(pkt), &ns_hdr.tgt);
	if (!nbr) {
		nbr_print();

		nbr = nbr_new(net_pkt_iface(pkt), &ns_hdr.tgt, false,
			      NET_IPV6_NBR_STATE_INCOMPLETE);
		if (!nbr) {
			NET_DBG("Could not create new neighbor %s",
				net_sprint_ipv6_addr(&ns_hdr.tgt));
			if (pending) {
				net_pkt_unref(pending);
			}

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

		net_ipv6_nbr_data(nbr)->send_ns = k_uptime_get();

		/* Let's start the timer if necessary */
		if (!k_delayed_work_remaining_get(&ipv6_ns_reply_timer)) {
			k_delayed_work_submit(&ipv6_ns_reply_timer,
					      NS_REPLY_TIMEOUT);
		}
	}

	dbg_addr_sent_tgt("Neighbor Solicitation",
			  &NET_IPV6_HDR(pkt)->src,
			  &NET_IPV6_HDR(pkt)->dst,
			  &ns_hdr.tgt);

	if (net_send_data(pkt) < 0) {
		NET_DBG("Cannot send NS %p (pending %p)", pkt, pending);

		if (pending) {
			nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
		}

		goto drop;
	}

	net_stats_update_ipv6_nd_sent(net_pkt_iface(pkt));

	return 0;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));
	net_pkt_unref(pkt);

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
	int ret;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				     ND_NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_frag(pkt, ND_NET_BUF_TIMEOUT);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

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

	net_buf_add(frag, sizeof(struct net_icmpv6_rs_hdr));

	if (!unspec_src) {
		net_buf_add(frag, llao_len);

		set_llao(net_if_get_link_addr(net_pkt_iface(pkt)),
			 (u8_t *)net_pkt_icmp_data(pkt) +
					 sizeof(struct net_icmp_hdr) +
					 sizeof(struct net_icmpv6_rs_hdr),
			 llao_len, NET_ICMPV6_ND_OPT_SLLAO);
	}

	ret = net_icmpv6_set_chksum(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	dbg_addr_sent("Router Solicitation",
		      &NET_IPV6_HDR(pkt)->src,
		      &NET_IPV6_HDR(pkt)->dst);

	if (net_send_data(pkt) < 0) {
		goto drop;
	}

	net_stats_update_ipv6_nd_sent(net_pkt_iface(pkt));

	return 0;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));
	net_pkt_unref(pkt);

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
	if (net_pkt_lladdr_src(pkt)->len < lladdr.len) {
		lladdr.len = net_pkt_lladdr_src(pkt)->len;
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

static u32_t time_diff(u32_t time1, u32_t time2)
{
	return (u32_t)abs((s32_t)time1 - (s32_t)time2);
}

static inline u32_t remaining_lifetime(struct net_if_addr *ifaddr)
{
	u64_t remaining;

	if (ifaddr->lifetime.timer_timeout == 0) {
		return 0;
	}

	remaining = (u64_t)ifaddr->lifetime.timer_timeout +
		(u64_t)ifaddr->lifetime.wrap_counter *
		(u64_t)NET_TIMEOUT_MAX_VALUE -
		(u64_t)time_diff(k_uptime_get_32(),
				 ifaddr->lifetime.timer_start);

	return (u32_t)(remaining / K_MSEC(1000));
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
		     remaining_lifetime(ifaddr))) {
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
		(void)memset(context.prefix.s6_addr + context.context_len, 0,
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
	struct net_icmpv6_ra_hdr ra_hdr;
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
	int ret;

	dbg_addr_recv("Router Advertisement",
		      &NET_IPV6_HDR(pkt)->src,
		      &NET_IPV6_HDR(pkt)->dst);

	net_stats_update_ipv6_nd_recv(net_pkt_iface(pkt));

	if ((total_len < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_ra_hdr) +
			  sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	    (NET_IPV6_HDR(pkt)->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	    !net_is_ipv6_ll_addr(&NET_IPV6_HDR(pkt)->src)) {
		struct net_icmp_hdr icmp_hdr;

		ret = net_icmpv6_get_hdr(pkt, &icmp_hdr);
		if (ret < 0 || icmp_hdr.code != 0) {
			goto drop;
		}
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

	ret = net_icmpv6_get_ra_hdr(pkt, &ra_hdr);
	if (ret < 0) {
		NET_ERR("could not get ra_hdr");
		goto drop;
	}

	if (reachable_time && reachable_time <= MAX_REACHABLE_TIME &&
	    (net_if_ipv6_get_reachable_time(net_pkt_iface(pkt)) !=
	     ra_hdr.reachable_time)) {
		net_if_ipv6_set_base_reachable_time(net_pkt_iface(pkt),
						    reachable_time);

		net_if_ipv6_set_reachable_time(
			net_pkt_iface(pkt)->config.ip.ipv6);
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

			if (mtu < MIN_IPV6_MTU || mtu > MAX_IPV6_MTU) {
				NET_ERR("Unsupported MTU %u, min is %u, "
					"max is %u",
					mtu, MIN_IPV6_MTU, MAX_IPV6_MTU);
				goto drop;
			}

			net_if_set_mtu(net_pkt_iface(pkt), mtu);

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
	k_delayed_work_cancel(&net_pkt_iface(pkt)->config.ip.ipv6->rs_timer);

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));

	return NET_DROP;
}
#endif /* CONFIG_NET_IPV6_ND */

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

void net_ipv6_nbr_init(void)
{
#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	net_icmpv6_register_handler(&ns_input_handler);
	net_icmpv6_register_handler(&na_input_handler);
	k_delayed_work_init(&ipv6_ns_reply_timer, ipv6_ns_reply_timeout);
#endif
#if defined(CONFIG_NET_IPV6_ND)
	net_icmpv6_register_handler(&ra_input_handler);
	k_delayed_work_init(&ipv6_nd_reachable_timer,
			    ipv6_nd_reachable_timeout);
#endif
}
