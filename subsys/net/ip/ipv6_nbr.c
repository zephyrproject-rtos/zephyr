/** @file
 * @brief IPv6 Neighbor related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* By default this prints too much data, set the value to 1 to see
 * neighbor cache contents.
 */
#define NET_DEBUG_NBR 0

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ipv6_nd, CONFIG_NET_IPV6_ND_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dns_resolve.h>
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

#if defined(CONFIG_NET_IPV6_NBR_CACHE) || defined(CONFIG_NET_IPV6_ND)
/* Global stale counter, whenever ipv6 neighbor enters into
 * stale state, stale counter is incremented by one.
 * When network stack tries to add new neighbor and if table
 * is full, oldest (oldest stale counter) neighbor in stale
 * state will be removed from the table and new entry will be
 * added.
 */
static uint32_t stale_counter;
#endif

#if defined(CONFIG_NET_IPV6_ND)
static struct k_work_delayable ipv6_nd_reachable_timer;
static void ipv6_nd_reachable_timeout(struct k_work *work);
static void ipv6_nd_restart_reachable_timer(struct net_nbr *nbr, int64_t time);
#endif

#if defined(CONFIG_NET_IPV6_NBR_CACHE)

/* Protocol constants from RFC 4861 Chapter 10 */
#define MAX_MULTICAST_SOLICIT 3
#define MAX_UNICAST_SOLICIT   3
#define DELAY_FIRST_PROBE_TIME (5 * MSEC_PER_SEC)
#define RETRANS_TIMER 1000 /* ms */

extern void net_neighbor_remove(struct net_nbr *nbr);
extern void net_neighbor_table_clear(struct net_nbr_table *table);

/** Neighbor Solicitation reply timer */
static struct k_work_delayable ipv6_ns_reply_timer;

NET_NBR_POOL_INIT(net_neighbor_pool,
		  CONFIG_NET_IPV6_MAX_NEIGHBORS,
		  sizeof(struct net_ipv6_nbr_data),
		  net_neighbor_remove);

NET_NBR_TABLE_INIT(NET_NBR_GLOBAL,
		   neighbor,
		   net_neighbor_pool,
		   net_neighbor_table_clear);

static K_MUTEX_DEFINE(nbr_lock);

void net_ipv6_nbr_lock(void)
{
	(void)k_mutex_lock(&nbr_lock, K_FOREVER);
}

void net_ipv6_nbr_unlock(void)
{
	k_mutex_unlock(&nbr_lock);
}

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

	if (net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_STALE) {
		if (stale_counter + 1 != UINT32_MAX) {
			net_ipv6_nbr_data(nbr)->stale_counter = stale_counter++;
		} else {
			/* Global stale counter reached UINT32_MAX, reset it and
			 * respective neighbors stale counter too.
			 */
			struct net_nbr *n = NULL;
			struct net_ipv6_nbr_data *data = NULL;
			int i;

			stale_counter = 0U;

			for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
				n = get_nbr(i);
				if (!n || !n->ref) {
					continue;
				}

				data = net_ipv6_nbr_data(nbr);
				if (!data) {
					continue;
				}

				if (data->state != NET_IPV6_NBR_STATE_STALE) {
					continue;
				}

				data->stale_counter = stale_counter++;
			}
		}
	}
}

struct iface_cb_data {
	net_nbr_cb_t cb;
	void *user_data;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct iface_cb_data *data = user_data;
	int i;

	net_ipv6_nbr_lock();

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref || nbr->iface != iface) {
			continue;
		}

		data->cb(nbr, data->user_data);
	}

	net_ipv6_nbr_unlock();
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

		NET_DBG("[%d] %p %d/%d/%d/%d/%d pending %p iface %p/%d "
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
				  const struct in6_addr *addr)
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

	net_ipv6_nbr_lock();

	nbr = nbr_lookup(&net_neighbor.table, iface, addr);
	if (!nbr) {
		net_ipv6_nbr_unlock();
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

	net_ipv6_nbr_unlock();
	return true;
}

#define NS_REPLY_TIMEOUT (1 * MSEC_PER_SEC)

static void ipv6_ns_reply_timeout(struct k_work *work)
{
	int64_t current = k_uptime_get();
	struct net_nbr *nbr = NULL;
	struct net_ipv6_nbr_data *data;
	int i;

	ARG_UNUSED(work);

	net_ipv6_nbr_lock();

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		int64_t remaining;
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
			if (!k_work_delayable_remaining_get(
				    &ipv6_ns_reply_timer)) {
				k_work_reschedule(&ipv6_ns_reply_timer,
						  K_MSEC(remaining));
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
			net_sprint_ipv6_addr(&NET_IPV6_HDR(data->pending)->dst));

		/* To unref when pending variable was set */
		net_pkt_unref(data->pending);

		/* To unref the original pkt allocation */
		net_pkt_unref(data->pending);

		data->pending = NULL;

		net_nbr_unref(nbr);
	}

	net_ipv6_nbr_unlock();
}

static void nbr_init(struct net_nbr *nbr, struct net_if *iface,
		     const struct in6_addr *addr, bool is_router,
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
			       const struct in6_addr *addr, bool is_router,
			       enum net_ipv6_nbr_state state)
{
	struct net_nbr *nbr = net_nbr_get(&net_neighbor.table);

	if (!nbr) {
		return NULL;
	}

	nbr_init(nbr, iface, addr, is_router, state);

	NET_DBG("nbr %p iface %p/%d state %d IPv6 %s",
		nbr, iface, net_if_get_by_iface(iface), state,
		net_sprint_ipv6_addr(addr));

	return nbr;
}

static void dbg_update_neighbor_lladdr(const struct net_linkaddr *new_lladdr,
				       const struct net_linkaddr_storage *old_lladdr,
				       const struct in6_addr *addr)
{
	char out[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ll_addr(old_lladdr->addr, old_lladdr->len));

	NET_DBG("Updating neighbor %s lladdr %s (was %s)",
		net_sprint_ipv6_addr(addr),
		net_sprint_ll_addr(new_lladdr->addr, new_lladdr->len),
		out);
}

static void dbg_update_neighbor_lladdr_raw(uint8_t *new_lladdr,
				       struct net_linkaddr_storage *old_lladdr,
				       struct in6_addr *addr)
{
	struct net_linkaddr lladdr = {
		.len = old_lladdr->len,
		.addr = new_lladdr,
	};

	dbg_update_neighbor_lladdr(&lladdr, old_lladdr, addr);
}

#define dbg_addr(action, pkt_str, src, dst, pkt)			\
	do {								\
		NET_DBG("%s %s from %s to %s iface %p/%d",		\
			action, pkt_str,				\
			net_sprint_ipv6_addr(src),		\
			net_sprint_ipv6_addr(dst),		\
			net_pkt_iface(pkt),				\
			net_if_get_by_iface(net_pkt_iface(pkt)));	\
	} while (0)

#define dbg_addr_recv(pkt_str, src, dst, pkt)	\
	dbg_addr("Received", pkt_str, src, dst, pkt)

#define dbg_addr_sent(pkt_str, src, dst, pkt)	\
	dbg_addr("Sent", pkt_str, src, dst, pkt)

#define dbg_addr_with_tgt(action, pkt_str, src, dst, target, pkt)	\
	do {								\
		NET_DBG("%s %s from %s to %s, target %s iface %p/%d",	\
			action,						\
			pkt_str,                                        \
			net_sprint_ipv6_addr(src),		\
			net_sprint_ipv6_addr(dst),		\
			net_sprint_ipv6_addr(target),	\
			net_pkt_iface(pkt),				\
			net_if_get_by_iface(net_pkt_iface(pkt)));	\
	} while (0)

#define dbg_addr_recv_tgt(pkt_str, src, dst, tgt, pkt)		\
	dbg_addr_with_tgt("Received", pkt_str, src, dst, tgt, pkt)

#define dbg_addr_sent_tgt(pkt_str, src, dst, tgt, pkt)		\
	dbg_addr_with_tgt("Sent", pkt_str, src, dst, tgt, pkt)

static void ipv6_nd_remove_old_stale_nbr(void)
{
	struct net_nbr *nbr = NULL;
	struct net_ipv6_nbr_data *data = NULL;
	int nbr_idx = -1;
	uint32_t oldest = UINT32_MAX;
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		nbr = get_nbr(i);
		if (!nbr || !nbr->ref) {
			continue;
		}

		data = net_ipv6_nbr_data(nbr);
		if (!data || data->is_router ||
		    data->state != NET_IPV6_NBR_STATE_STALE) {
			continue;
		}

		if (nbr_idx == -1) {
			nbr_idx = i;
			oldest = data->stale_counter;
			continue;
		}

		if (oldest == MIN(oldest, data->stale_counter)) {
			continue;
		}

		nbr_idx = i;
		oldest = data->stale_counter;
	}

	if (nbr_idx != -1) {
		nbr = get_nbr(nbr_idx);
		if (!nbr) {
			return;
		}

		net_ipv6_nbr_rm(nbr->iface,
				&net_ipv6_nbr_data(nbr)->addr);
	}
}

static struct net_nbr *add_nbr(struct net_if *iface,
			       const struct in6_addr *addr,
			       bool is_router,
			       enum net_ipv6_nbr_state state)
{
	struct net_nbr *nbr;

	nbr = nbr_lookup(&net_neighbor.table, iface, addr);
	if (nbr) {
		return nbr;
	}

	nbr = nbr_new(iface, addr, is_router, state);
	if (nbr) {
		return nbr;
	}

	/* Check if there are any stale neighbors, delete the oldest
	 * one and try to add new neighbor.
	 */
	ipv6_nd_remove_old_stale_nbr();

	nbr = nbr_new(iface, addr, is_router, state);
	if (!nbr) {
		return NULL;
	}

	return nbr;
}

struct net_nbr *net_ipv6_nbr_add(struct net_if *iface,
				 const struct in6_addr *addr,
				 const struct net_linkaddr *lladdr,
				 bool is_router,
				 enum net_ipv6_nbr_state state)
{
	struct net_nbr *nbr;
	int ret;
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	struct net_event_ipv6_nbr info;
#endif

	net_ipv6_nbr_lock();

	nbr = add_nbr(iface, addr, is_router, state);
	if (!nbr) {
		NET_ERR("Could not add router neighbor %s [%s]",
			net_sprint_ipv6_addr(addr),
			lladdr ? net_sprint_ll_addr(lladdr->addr, lladdr->len) : "unknown");
		goto out;
	}

	if (lladdr && net_nbr_link(nbr, iface, lladdr) == -EALREADY &&
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

	NET_DBG("[%d] nbr %p state %d router %d IPv6 %s ll %s iface %p/%d",
		nbr->idx, nbr, state, is_router,
		net_sprint_ipv6_addr(addr),
		lladdr ? net_sprint_ll_addr(lladdr->addr, lladdr->len) : "[unknown]",
		nbr->iface, net_if_get_by_iface(nbr->iface));

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	info.idx = nbr->idx;
	net_ipaddr_copy(&info.addr, addr);
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_NBR_ADD,
					iface, (void *) &info,
					sizeof(struct net_event_ipv6_nbr));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_NBR_ADD, iface);
#endif

out:
	net_ipv6_nbr_unlock();
	return nbr;
}

void net_neighbor_remove(struct net_nbr *nbr)
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

	net_ipv6_nbr_lock();

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (iface && nbr->iface != iface) {
			continue;
		}

		if (nbr->idx == idx) {
			net_ipv6_nbr_unlock();
			return &net_ipv6_nbr_data(nbr)->addr;
		}
	}

	net_ipv6_nbr_unlock();
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
	if (net_ipv6_is_ll_addr(addr)) {
		NET_ERR("DAD failed, no ll IPv6 address!");
		return false;
	}

	net_if_ipv6_dad_failed(iface, addr);

	return true;
}
#endif /* CONFIG_NET_IPV6_DAD */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
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

		NET_DBG("Route %p nexthop %s iface %p/%d",
			route,
			nexthop ? net_sprint_ipv6_addr(nexthop) :
			"<unknown>",
			iface, net_if_get_by_iface(iface));

		if (!nexthop) {
			net_route_del(route);

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

enum net_verdict net_ipv6_prepare_for_send(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct in6_addr *nexthop = NULL;
	struct net_if *iface = NULL;
	struct net_ipv6_hdr *ip_hdr;
	struct net_nbr *nbr;
	int ret;

	NET_ASSERT(pkt && pkt->buffer);

	ip_hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!ip_hdr) {
		return NET_DROP;
	}

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	/* If we have already fragmented the packet, the fragment id will
	 * contain a proper value and we can skip other checks.
	 */
	if (net_pkt_ipv6_fragment_id(pkt) == 0U) {
		uint16_t mtu = net_if_get_mtu(net_pkt_iface(pkt));
		size_t pkt_len = net_pkt_get_len(pkt);

		mtu = MAX(NET_IPV6_MTU, mtu);
		if (mtu < pkt_len) {
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

			/* We need to unref here because we simulate the packet
			 * sending.
			 */
			net_pkt_unref(pkt);

			/* No need to continue with the sending as the packet
			 * is now split and its fragments will be sent
			 * separately to network.
			 */
			return NET_CONTINUE;
		}
	}
ignore_frag_error:
#endif /* CONFIG_NET_IPV6_FRAGMENT */

	/* If the IPv6 destination address is not link local, then try to get
	 * the next hop from routing table if we have multi interface routing
	 * enabled. The reason for this is that the neighbor cache will not
	 * contain public IPv6 address information so in that case we should
	 * not enter this branch.
	 */
	if ((net_pkt_lladdr_dst(pkt)->addr &&
	     ((IS_ENABLED(CONFIG_NET_ROUTING) &&
	      net_ipv6_is_ll_addr((struct in6_addr *)ip_hdr->dst)) ||
	      !IS_ENABLED(CONFIG_NET_ROUTING))) ||
	    net_ipv6_is_addr_mcast((struct in6_addr *)ip_hdr->dst) ||
	    /* Workaround Linux bug, see:
	     * https://github.com/zephyrproject-rtos/zephyr/issues/3111
	     */
	    net_if_flag_is_set(net_pkt_iface(pkt), NET_IF_POINTOPOINT) ||
	    net_if_flag_is_set(net_pkt_iface(pkt), NET_IF_IPV6_NO_ND)) {
		return NET_OK;
	}

	if (net_if_ipv6_addr_onlink(&iface, (struct in6_addr *)ip_hdr->dst)) {
		nexthop = (struct in6_addr *)ip_hdr->dst;
		net_pkt_set_iface(pkt, iface);
	} else if (net_ipv6_is_ll_addr((struct in6_addr *)ip_hdr->dst)) {
		nexthop = (struct in6_addr *)ip_hdr->dst;
	} else {
		/* We need to figure out where the destination
		 * host is located.
		 */
		bool try_route = false;

		nexthop = check_route(NULL, (struct in6_addr *)ip_hdr->dst,
				      &try_route);
		if (!nexthop) {
			return NET_DROP;
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
			/* nexthop might be the nbr list, e.g. a link-local
			 * address of a connected peer.
			 */
			nbr = net_ipv6_nbr_lookup(NULL, nexthop);
			if (nbr) {
				iface = nbr->iface;
				net_pkt_set_iface(pkt, iface);
			} else {
				iface = net_pkt_iface(pkt);
			}
		}

		/* If the above check returns null, we try to send
		 * the packet and hope for the best.
		 */
	}

try_send:
	net_ipv6_nbr_lock();

	nbr = nbr_lookup(&net_neighbor.table, iface, nexthop);

	NET_DBG("Neighbor lookup %p (%d) iface %p/%d addr %s state %s", nbr,
		nbr ? nbr->idx : NET_NBR_LLADDR_UNKNOWN,
		iface, net_if_get_by_iface(iface),
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

			ipv6_nd_restart_reachable_timer(nbr,
							DELAY_FIRST_PROBE_TIME);
		}
#endif
		net_ipv6_nbr_unlock();
		return NET_OK;
	}

	net_ipv6_nbr_unlock();

#if defined(CONFIG_NET_IPV6_ND)
	/* We need to send NS and wait for NA before sending the packet. */
	ret = net_ipv6_send_ns(net_pkt_iface(pkt), pkt,
			       (struct in6_addr *)ip_hdr->src, NULL, nexthop,
			       false);
	if (ret < 0) {
		/* In case of an error, the NS send function will unref
		 * the pkt.
		 */
		NET_DBG("Cannot send NS (%d) iface %p/%d",
			ret, net_pkt_iface(pkt),
			net_if_get_by_iface(net_pkt_iface(pkt)));
	}

	NET_DBG("pkt %p (buffer %p) will be sent later to iface %p/%d",
		pkt, pkt->buffer, net_pkt_iface(pkt),
		net_if_get_by_iface(net_pkt_iface(pkt)));

	return NET_CONTINUE;
#else
	ARG_UNUSED(ret);

	NET_DBG("pkt %p (buffer %p) cannot be sent to iface %p/%d, "
		"dropping it.", pkt, pkt->buffer,
		net_pkt_iface(pkt), net_if_get_by_iface(net_pkt_iface(pkt)));

	return NET_DROP;
#endif /* CONFIG_NET_IPV6_ND */
}

struct net_nbr *net_ipv6_nbr_lookup(struct net_if *iface,
				    struct in6_addr *addr)
{
	struct net_nbr *nbr;

	net_ipv6_nbr_lock();
	nbr = nbr_lookup(&net_neighbor.table, iface, addr);
	net_ipv6_nbr_unlock();

	return nbr;
}

struct net_nbr *net_ipv6_get_nbr(struct net_if *iface, uint8_t idx)
{
	struct net_nbr *ret = NULL;
	int i;

	if (idx == NET_NBR_LLADDR_UNKNOWN) {
		return NULL;
	}

	net_ipv6_nbr_lock();

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (nbr->ref) {
			if (iface && nbr->iface != iface) {
				continue;
			}

			if (nbr->idx == idx) {
				ret = nbr;
				break;
			}
		}
	}

	net_ipv6_nbr_unlock();
	return ret;
}

static inline uint8_t get_llao_len(struct net_if *iface)
{
	uint8_t total_len = net_if_get_link_addr(iface)->len +
			 sizeof(struct net_icmpv6_nd_opt_hdr);

	return ROUND_UP(total_len, 8U);
}

static inline bool set_llao(struct net_pkt *pkt,
			    struct net_linkaddr *lladdr,
			    uint8_t llao_len, uint8_t type)
{
	struct net_icmpv6_nd_opt_hdr opt_hdr = {
		.type = type,
		.len  = llao_len >> 3,
	};

	if (net_pkt_write(pkt, &opt_hdr,
			  sizeof(struct net_icmpv6_nd_opt_hdr)) ||
	    net_pkt_write(pkt, lladdr->addr, lladdr->len) ||
	    net_pkt_memset(pkt, 0, llao_len - lladdr->len - 2)) {
		return false;
	}

	return true;
}

static bool read_llao(struct net_pkt *pkt,
		      uint8_t len,
		      struct net_linkaddr_storage *llstorage)
{
	uint8_t padding;

	llstorage->len = NET_LINK_ADDR_MAX_LENGTH;
	if (net_pkt_lladdr_src(pkt)->len < llstorage->len) {
		llstorage->len = net_pkt_lladdr_src(pkt)->len;
	}

	if (net_pkt_read(pkt, llstorage->addr, llstorage->len)) {
		return false;
	}

	padding = len * 8U - 2 - llstorage->len;
	if (padding) {
		if (net_pkt_skip(pkt, padding)) {
			return false;
		}
	}

	return true;
}

int net_ipv6_send_na(struct net_if *iface, const struct in6_addr *src,
		     const struct in6_addr *dst, const struct in6_addr *tgt,
		     uint8_t flags)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(na_access,
					      struct net_icmpv6_na_hdr);
	int ret = -ENOBUFS;
	struct net_icmpv6_na_hdr *na_hdr;
	struct net_pkt *pkt;
	uint8_t llao_len;

	llao_len = get_llao_len(iface);

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_icmpv6_na_hdr) +
					llao_len,
					AF_INET6, IPPROTO_ICMPV6,
					ND_NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_ND_HOP_LIMIT);

	if (net_ipv6_create(pkt, src, dst) ||
	    net_icmpv6_create(pkt, NET_ICMPV6_NA, 0)) {
		goto drop;
	}

	na_hdr = (struct net_icmpv6_na_hdr *)net_pkt_get_data(pkt, &na_access);
	if (!na_hdr) {
		goto drop;
	}

	/* Let's make sure reserved part is full of 0 */
	memset(na_hdr, 0, sizeof(struct net_icmpv6_na_hdr));

	na_hdr->flags = flags;
	net_ipv6_addr_copy_raw(na_hdr->tgt, (uint8_t *)tgt);

	if (net_pkt_set_data(pkt, &na_access)) {
		goto drop;
	}

	if (!set_llao(pkt, net_if_get_link_addr(iface),
		      llao_len, NET_ICMPV6_ND_OPT_TLLAO)) {
		goto drop;
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);

	dbg_addr_sent_tgt("Neighbor Advertisement", src, dst, &na_hdr->tgt,
			  pkt);

	if (net_send_data(pkt) < 0) {
		net_stats_update_ipv6_nd_drop(iface);
		ret = -EINVAL;

		goto drop;
	}

	net_stats_update_icmp_sent(net_pkt_iface(pkt));
	net_stats_update_ipv6_nd_sent(iface);

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

static void ns_routing_info(struct net_pkt *pkt,
			    struct in6_addr *nexthop,
			    struct in6_addr *tgt)
{
	if (CONFIG_NET_IPV6_LOG_LEVEL >= LOG_LEVEL_DBG) {
		char out[NET_IPV6_ADDR_LEN];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv6_addr(nexthop));

		if (net_ipv6_addr_cmp(nexthop, tgt)) {
			NET_DBG("Routing to %s iface %p/%d",
				out,
				net_pkt_iface(pkt),
				net_if_get_by_iface(net_pkt_iface(pkt)));
		} else {
			NET_DBG("Routing to %s via %s iface %p/%d",
				net_sprint_ipv6_addr(tgt),
				out,
				net_pkt_iface(pkt),
				net_if_get_by_iface(net_pkt_iface(pkt)));
		}
	}
}

static int handle_ns_input(struct net_icmp_ctx *ctx,
			   struct net_pkt *pkt,
			   struct net_icmp_ip_hdr *hdr,
			   struct net_icmp_hdr *icmp_hdr,
			   void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ns_access,
					      struct net_icmpv6_ns_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(nd_access, struct net_icmpv6_nd_opt_hdr);
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	uint16_t length = net_pkt_get_len(pkt);
	uint8_t flags = 0U;
	bool routing = false;
	struct net_icmpv6_nd_opt_hdr *nd_opt_hdr;
	struct net_icmpv6_ns_hdr *ns_hdr;
	struct net_if_addr *ifaddr;
	const struct in6_addr *na_src;
	const struct in6_addr *na_dst;
	struct in6_addr *tgt;
	struct net_linkaddr_storage src_lladdr_s;
	struct net_linkaddr src_lladdr;

	src_lladdr.len = 0;

	if (net_if_flag_is_set(net_pkt_iface(pkt), NET_IF_IPV6_NO_ND)) {
		goto drop;
	}

	ns_hdr = (struct net_icmpv6_ns_hdr *)net_pkt_get_data(pkt, &ns_access);
	if (!ns_hdr) {
		NET_ERR("DROP: NULL NS header");
		goto drop;
	}

	dbg_addr_recv_tgt("Neighbor Solicitation",
			  &ip_hdr->src, &ip_hdr->dst, &ns_hdr->tgt, pkt);

	net_stats_update_ipv6_nd_recv(net_pkt_iface(pkt));

	if (((length < (sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr) +
			  sizeof(struct net_icmpv6_ns_hdr))) ||
	    (ip_hdr->hop_limit != NET_IPV6_ND_HOP_LIMIT)) &&
	    (net_ipv6_is_addr_mcast((struct in6_addr *)ns_hdr->tgt) &&
	     icmp_hdr->code != 0U)) {
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &ns_access);

	net_pkt_set_ipv6_ext_opt_len(pkt, sizeof(struct net_icmpv6_ns_hdr));
	length -= (sizeof(struct net_ipv6_hdr) + sizeof(struct net_icmp_hdr));

	nd_opt_hdr = (struct net_icmpv6_nd_opt_hdr *)
				net_pkt_get_data(pkt, &nd_access);

	while (nd_opt_hdr && nd_opt_hdr->len > 0 &&
	       net_pkt_ipv6_ext_opt_len(pkt) < length) {
		uint8_t prev_opt_len;

		net_pkt_acknowledge_data(pkt, &nd_access);

		switch (nd_opt_hdr->type) {
		case NET_ICMPV6_ND_OPT_SLLAO:
			if (net_ipv6_is_addr_unspecified(
					(struct in6_addr *)ip_hdr->src)) {
				goto drop;
			}

			if (!read_llao(pkt, nd_opt_hdr->len, &src_lladdr_s)) {
				NET_ERR("DROP: failed to read LLAO");
				goto drop;
			}

			src_lladdr.len = src_lladdr_s.len;
			src_lladdr.addr = src_lladdr_s.addr;

			break;
		default:
			NET_DBG("Unknown ND option 0x%x", nd_opt_hdr->type);
			break;
		}

		prev_opt_len = net_pkt_ipv6_ext_opt_len(pkt);

		net_pkt_set_ipv6_ext_opt_len(pkt,
					     net_pkt_ipv6_ext_opt_len(pkt) +
					     (nd_opt_hdr->len << 3));

		if (prev_opt_len >= net_pkt_ipv6_ext_opt_len(pkt)) {
			NET_ERR("DROP: Corrupted NS message");
			goto drop;
		}

		nd_opt_hdr = (struct net_icmpv6_nd_opt_hdr *)
					net_pkt_get_data(pkt, &nd_access);
	}

	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		ifaddr = net_if_ipv6_addr_lookup((struct in6_addr *)ns_hdr->tgt,
						 NULL);
	} else {
		ifaddr = net_if_ipv6_addr_lookup_by_iface(
			    net_pkt_iface(pkt), (struct in6_addr *)ns_hdr->tgt);
	}

	if (!ifaddr) {
		if (IS_ENABLED(CONFIG_NET_ROUTING)) {
			struct in6_addr *nexthop;

			nexthop = check_route(NULL,
					      (struct in6_addr *)ns_hdr->tgt,
					      NULL);
			if (nexthop) {
				ns_routing_info(pkt, nexthop,
						(struct in6_addr *)ns_hdr->tgt);
				na_dst = (struct in6_addr *)ip_hdr->dst;
				/* Note that the target is not the address of
				 * the "nethop" as that is a link-local address
				 * which is not routable.
				 */
				tgt = (struct in6_addr *)ns_hdr->tgt;

				/* Source address must be one of our real
				 * interface address where the packet was
				 * received.
				 */
				na_src = net_if_ipv6_select_src_addr(
						net_pkt_iface(pkt),
						(struct in6_addr *)ip_hdr->src);
				if (!na_src) {
					NET_DBG("DROP: No interface address "
						"for dst %s iface %p/%d",
						net_sprint_ipv6_addr(&ip_hdr->src),
						net_pkt_iface(pkt),
						net_if_get_by_iface(
							net_pkt_iface(pkt)));
					goto drop;
				}

				routing = true;
				goto nexthop_found;
			}
		}

		NET_DBG("DROP: No such interface address %s",
			net_sprint_ipv6_addr(&ns_hdr->tgt));
		goto drop;
	} else {
		tgt = &ifaddr->address.in6_addr;
		na_src = (struct in6_addr *)ip_hdr->dst;
	}

nexthop_found:

#if !defined(CONFIG_NET_IPV6_DAD)
	if (net_ipv6_is_addr_unspecified((struct in6_addr *)ip_hdr->src)) {
		goto drop;
	}

#else /* CONFIG_NET_IPV6_DAD */

	/* Do DAD */
	if (net_ipv6_is_addr_unspecified((struct in6_addr *)ip_hdr->src)) {

		if (!net_ipv6_is_addr_solicited_node((struct in6_addr *)ip_hdr->dst)) {
			NET_DBG("DROP: Not solicited node addr %s",
				net_sprint_ipv6_addr(&ip_hdr->dst));
			goto drop;
		}

		if (ifaddr->addr_state == NET_ADDR_TENTATIVE) {
			NET_DBG("DROP: DAD failed for %s iface %p/%d",
				net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
				net_pkt_iface(pkt),
				net_if_get_by_iface(net_pkt_iface(pkt)));

			dad_failed(net_pkt_iface(pkt),
				   &ifaddr->address.in6_addr);
			goto drop;
		}

		/* We reuse the received packet for the NA addresses*/
		net_ipv6_addr_create_ll_allnodes_mcast(
					(struct in6_addr *)ip_hdr->dst);
		net_ipaddr_copy((struct in6_addr *)ip_hdr->src,
				net_if_ipv6_select_src_addr(
					net_pkt_iface(pkt),
					(struct in6_addr *)ip_hdr->dst));

		na_src = (struct in6_addr *)ip_hdr->src;
		na_dst = (struct in6_addr *)ip_hdr->dst;
		flags = NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	}
#endif /* CONFIG_NET_IPV6_DAD */

	if (net_ipv6_is_my_addr((struct in6_addr *)ip_hdr->src)) {
		NET_DBG("DROP: Duplicate IPv6 %s address",
			net_sprint_ipv6_addr(&ip_hdr->src));
		goto drop;
	}

	/* Address resolution */
	if (net_ipv6_is_addr_solicited_node((struct in6_addr *)ip_hdr->dst)) {
		na_src = (struct in6_addr *)ns_hdr->tgt;
		na_dst = (struct in6_addr *)ip_hdr->src;
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
		ifaddr = net_if_ipv6_addr_lookup((struct in6_addr *)ip_hdr->dst,
						 NULL);
	} else {
		ifaddr = net_if_ipv6_addr_lookup_by_iface(
						net_pkt_iface(pkt),
						(struct in6_addr *)ip_hdr->dst);
	}

	if (ifaddr) {
		na_src = (struct in6_addr *)ns_hdr->tgt;
		na_dst = (struct in6_addr *)ip_hdr->src;
		tgt = &ifaddr->address.in6_addr;
		flags = NET_ICMPV6_NA_FLAG_SOLICITED |
			NET_ICMPV6_NA_FLAG_OVERRIDE;
		goto send_na;
	} else {
		NET_DBG("DROP: NUD failed");
		goto drop;
	}

send_na:
	if (src_lladdr.len) {
		if (!net_ipv6_nbr_add(net_pkt_iface(pkt),
				      (struct in6_addr *)ip_hdr->src,
				      &src_lladdr, false,
				      NET_IPV6_NBR_STATE_INCOMPLETE)) {
			goto drop;
		}
	}

	if (!net_ipv6_send_na(net_pkt_iface(pkt), na_src,
			      na_dst, tgt, flags)) {
		return 0;
	}

	NET_DBG("DROP: Cannot send NA");

	return -EIO;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));

	return -EIO;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
static void ipv6_nd_restart_reachable_timer(struct net_nbr *nbr, int64_t time)
{
	int64_t remaining;

	if (nbr) {
		net_ipv6_nbr_data(nbr)->reachable = k_uptime_get();
		net_ipv6_nbr_data(nbr)->reachable_timeout = time;
	}

	remaining = k_ticks_to_ms_ceil32(
		k_work_delayable_remaining_get(&ipv6_nd_reachable_timer));
	if (!remaining || remaining > time) {
		k_work_reschedule(&ipv6_nd_reachable_timer, K_MSEC(time));
	}
}

static void ipv6_nd_reachable_timeout(struct k_work *work)
{
	int64_t current = k_uptime_get();
	struct net_nbr *nbr = NULL;
	struct net_ipv6_nbr_data *data = NULL;
	int ret;
	int i;

	net_ipv6_nbr_lock();

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		int64_t remaining;

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
			ipv6_nd_restart_reachable_timer(NULL, remaining);
			continue;
		}

		data->reachable = 0;

		switch (data->state) {
		case NET_IPV6_NBR_STATE_STATIC:
			NET_ASSERT(false, "Static entry shall never timeout");
			break;

		case NET_IPV6_NBR_STATE_INCOMPLETE:
			if (data->ns_count >= MAX_MULTICAST_SOLICIT) {
				net_ipv6_nbr_rm(nbr->iface, &data->addr);
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
				nbr,
				net_sprint_ipv6_addr(&data->addr),
				data->state);
			break;

		case NET_IPV6_NBR_STATE_STALE:
			NET_DBG("nbr %p removing stale address %s",
				nbr,
				net_sprint_ipv6_addr(&data->addr));
			net_ipv6_nbr_rm(nbr->iface, &data->addr);
			break;

		case NET_IPV6_NBR_STATE_DELAY:
			data->state = NET_IPV6_NBR_STATE_PROBE;
			data->ns_count = 0U;

			NET_DBG("nbr %p moving %s state to PROBE (%d)",
				nbr,
				net_sprint_ipv6_addr(&data->addr),
				data->state);

			/* Intentionally continuing to probe state */
			__fallthrough;

		case NET_IPV6_NBR_STATE_PROBE:
			if (data->ns_count >= MAX_UNICAST_SOLICIT) {
				net_ipv6_nbr_rm(nbr->iface, &data->addr);
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

				ipv6_nd_restart_reachable_timer(nbr,
								RETRANS_TIMER);
			}
			break;
		}
	}

	net_ipv6_nbr_unlock();
}

void net_ipv6_nbr_set_reachable_timer(struct net_if *iface,
				      struct net_nbr *nbr)
{
	uint32_t time;

	time = net_if_ipv6_get_reachable_time(iface);

	NET_ASSERT(time, "Zero reachable timeout!");

	NET_DBG("Starting reachable timer nbr %p data %p time %d ms",
		nbr, net_ipv6_nbr_data(nbr), time);

	ipv6_nd_restart_reachable_timer(nbr, time);
}

void net_ipv6_nbr_reachability_hint(struct net_if *iface,
				    const struct in6_addr *ipv6_addr)
{
	struct net_nbr *nbr = NULL;

	net_ipv6_nbr_lock();

	nbr = nbr_lookup(&net_neighbor.table, iface, ipv6_addr);

	NET_DBG("nbr %p got rechability hint", nbr);

	if (nbr && net_ipv6_nbr_data(nbr)->state != NET_IPV6_NBR_STATE_INCOMPLETE &&
	    net_ipv6_nbr_data(nbr)->state != NET_IPV6_NBR_STATE_STATIC) {
		ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);

		/* We might have active timer from PROBE */
		net_ipv6_nbr_data(nbr)->reachable = 0;
		net_ipv6_nbr_data(nbr)->reachable_timeout = 0;

		net_ipv6_nbr_set_reachable_timer(iface, nbr);
	}

	net_ipv6_nbr_unlock();
}
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static inline bool handle_na_neighbor(struct net_pkt *pkt,
				      struct net_icmpv6_na_hdr *na_hdr,
				      uint16_t tllao_offset)
{
	struct net_linkaddr_storage lladdr = { 0 };
	bool lladdr_changed = false;
	struct net_linkaddr_storage *cached_lladdr;
	struct net_pkt *pending;
	struct net_nbr *nbr;

	net_ipv6_nbr_lock();

	nbr = nbr_lookup(&net_neighbor.table, net_pkt_iface(pkt),
			 (struct in6_addr *)na_hdr->tgt);

	NET_DBG("Neighbor lookup %p iface %p/%d addr %s", nbr,
		net_pkt_iface(pkt), net_if_get_by_iface(net_pkt_iface(pkt)),
		net_sprint_ipv6_addr(&na_hdr->tgt));

	if (!nbr) {
		nbr_print();

		NET_DBG("No such neighbor found, msg discarded");
		goto err;
	}

	if (tllao_offset) {
		lladdr.len = net_pkt_lladdr_src(pkt)->len;

		net_pkt_cursor_init(pkt);

		if (net_pkt_skip(pkt, tllao_offset) ||
		    net_pkt_read(pkt, lladdr.addr, lladdr.len)) {
			goto err;
		}
	}

	if (nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
		struct net_linkaddr nbr_lladdr;

		if (!tllao_offset) {
			NET_DBG("No target link layer address.");
			goto err;
		}

		nbr_lladdr.len = lladdr.len;
		nbr_lladdr.addr = lladdr.addr;

		if (net_nbr_link(nbr, net_pkt_iface(pkt), &nbr_lladdr)) {
			nbr_free(nbr);
			goto err;
		}

		NET_DBG("[%d] nbr %p state %d IPv6 %s ll %s",
			nbr->idx, nbr, net_ipv6_nbr_data(nbr)->state,
			net_sprint_ipv6_addr(&na_hdr->tgt),
			net_sprint_ll_addr(nbr_lladdr.addr, nbr_lladdr.len));
	}

	cached_lladdr = net_nbr_get_lladdr(nbr->idx);
	if (!cached_lladdr) {
		NET_DBG("No lladdr but index defined");
		goto err;
	}

	if (tllao_offset) {
		lladdr_changed = memcmp(lladdr.addr,
					cached_lladdr->addr,
					cached_lladdr->len);
	}

	/* Update the cached address if we do not yet known it */
	if (net_ipv6_nbr_data(nbr)->state == NET_IPV6_NBR_STATE_INCOMPLETE) {
		if (!tllao_offset) {
			goto err;
		}

		if (lladdr_changed) {
			dbg_update_neighbor_lladdr_raw(
				lladdr.addr, cached_lladdr,
				(struct in6_addr *)na_hdr->tgt);

			net_linkaddr_set(cached_lladdr, lladdr.addr,
					 cached_lladdr->len);
		}

		if (na_hdr->flags & NET_ICMPV6_NA_FLAG_SOLICITED) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_REACHABLE);
			net_ipv6_nbr_data(nbr)->ns_count = 0U;

			/* We might have active timer from PROBE */
			net_ipv6_nbr_data(nbr)->reachable = 0;
			net_ipv6_nbr_data(nbr)->reachable_timeout = 0;

			net_ipv6_nbr_set_reachable_timer(net_pkt_iface(pkt),
							 nbr);
		} else {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		}

		net_ipv6_nbr_data(nbr)->is_router =
			(na_hdr->flags & NET_ICMPV6_NA_FLAG_ROUTER);

		goto send_pending;
	}

	/* We do not update the address if override bit is not set
	 * and we have a valid address in the cache.
	 */
	if (!(na_hdr->flags & NET_ICMPV6_NA_FLAG_OVERRIDE) && lladdr_changed) {
		if (net_ipv6_nbr_data(nbr)->state ==
		    NET_IPV6_NBR_STATE_REACHABLE) {
			ipv6_nbr_set_state(nbr, NET_IPV6_NBR_STATE_STALE);
		}

		goto err;
	}

	if (na_hdr->flags & NET_ICMPV6_NA_FLAG_OVERRIDE ||
	    (!(na_hdr->flags & NET_ICMPV6_NA_FLAG_OVERRIDE) &&
	     tllao_offset && !lladdr_changed)) {

		if (lladdr_changed) {
			dbg_update_neighbor_lladdr_raw(
				lladdr.addr, cached_lladdr,
				(struct in6_addr *)na_hdr->tgt);

			net_linkaddr_set(cached_lladdr, lladdr.addr,
					 cached_lladdr->len);
		}

		if (na_hdr->flags & NET_ICMPV6_NA_FLAG_SOLICITED) {
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

	if (net_ipv6_nbr_data(nbr)->is_router &&
	    !(na_hdr->flags & NET_ICMPV6_NA_FLAG_ROUTER)) {
		/* Update the routing if the peer is no longer
		 * a router.
		 */
		/* FIXME */
	}

	net_ipv6_nbr_data(nbr)->is_router =
		(na_hdr->flags & NET_ICMPV6_NA_FLAG_ROUTER);

send_pending:
	/* Next send any pending messages to the peer. */
	pending = net_ipv6_nbr_data(nbr)->pending;
	if (pending) {
		NET_DBG("Sending pending %p to lladdr %s", pending,
			net_sprint_ll_addr(cached_lladdr->addr, cached_lladdr->len));

		if (net_send_data(pending) < 0) {
			nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
		} else {
			net_ipv6_nbr_data(nbr)->pending = NULL;
		}

		net_pkt_unref(pending);
	}

	net_ipv6_nbr_unlock();
	return true;

err:
	net_ipv6_nbr_unlock();
	return false;
}

static int handle_na_input(struct net_icmp_ctx *ctx,
			   struct net_pkt *pkt,
			   struct net_icmp_ip_hdr *hdr,
			   struct net_icmp_hdr *icmp_hdr,
			   void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(na_access,
					      struct net_icmpv6_na_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(nd_access, struct net_icmpv6_nd_opt_hdr);
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	uint16_t length = net_pkt_get_len(pkt);
	uint16_t tllao_offset = 0U;
	struct net_icmpv6_nd_opt_hdr *nd_opt_hdr;
	struct net_icmpv6_na_hdr *na_hdr;
	struct net_if_addr *ifaddr;

	if (net_if_flag_is_set(net_pkt_iface(pkt), NET_IF_IPV6_NO_ND)) {
		goto drop;
	}

	na_hdr = (struct net_icmpv6_na_hdr *)net_pkt_get_data(pkt, &na_access);
	if (!na_hdr) {
		NET_ERR("DROP: NULL NA header");
		goto drop;
	}

	dbg_addr_recv_tgt("Neighbor Advertisement",
			  &ip_hdr->src, &ip_hdr->dst, &na_hdr->tgt, pkt);

	net_stats_update_ipv6_nd_recv(net_pkt_iface(pkt));

	if (((length < (sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_icmp_hdr) +
			sizeof(struct net_icmpv6_na_hdr) +
			sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	     (ip_hdr->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	     net_ipv6_is_addr_mcast((struct in6_addr *)na_hdr->tgt) ||
	     (na_hdr->flags & NET_ICMPV6_NA_FLAG_SOLICITED &&
	      net_ipv6_is_addr_mcast((struct in6_addr *)ip_hdr->dst))) &&
	    (icmp_hdr->code != 0U)) {
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &na_access);

	net_pkt_set_ipv6_ext_opt_len(pkt, sizeof(struct net_icmpv6_na_hdr));
	length -= (sizeof(struct net_ipv6_hdr) + sizeof(struct net_icmp_hdr));

	nd_opt_hdr = (struct net_icmpv6_nd_opt_hdr *)
				net_pkt_get_data(pkt, &nd_access);

	while (nd_opt_hdr && nd_opt_hdr->len &&
	       net_pkt_ipv6_ext_opt_len(pkt) < length) {
		uint8_t prev_opt_len;

		switch (nd_opt_hdr->type) {
		case NET_ICMPV6_ND_OPT_TLLAO:
			tllao_offset = net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_icmp_hdr) +
				net_pkt_ipv6_ext_opt_len(pkt) + 1 + 1;
			break;

		default:
			NET_DBG("Unknown ND option 0x%x", nd_opt_hdr->type);
			break;
		}

		prev_opt_len = net_pkt_ipv6_ext_opt_len(pkt);

		net_pkt_set_ipv6_ext_opt_len(pkt,
					     net_pkt_ipv6_ext_opt_len(pkt) +
					     (nd_opt_hdr->len << 3));

		if (prev_opt_len >= net_pkt_ipv6_ext_opt_len(pkt)) {
			NET_ERR("DROP: Corrupted NA message");
			goto drop;
		}

		net_pkt_acknowledge_data(pkt, &nd_access);
		nd_opt_hdr = (struct net_icmpv6_nd_opt_hdr *)
					net_pkt_get_data(pkt, &nd_access);
	}

	ifaddr = net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
						  (struct in6_addr *)na_hdr->tgt);
	if (ifaddr) {
		NET_DBG("Interface %p/%d already has address %s",
			net_pkt_iface(pkt),
			net_if_get_by_iface(net_pkt_iface(pkt)),
			net_sprint_ipv6_addr(&na_hdr->tgt));

#if defined(CONFIG_NET_IPV6_DAD)
		if (ifaddr->addr_state == NET_ADDR_TENTATIVE) {
			dad_failed(net_pkt_iface(pkt),
				   (struct in6_addr *)na_hdr->tgt);
		}
#endif /* CONFIG_NET_IPV6_DAD */

		goto drop;
	}

	if (!handle_na_neighbor(pkt, na_hdr, tllao_offset)) {
		/* Update the statistics but silently drop NA msg if the sender
		 * is not known or if there was an error in the message.
		 * Returning <0 will cause error message to be printed which
		 * is too much for this non error.
		 */
		net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));
	}

	return 0;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));

	return -EIO;
}

int net_ipv6_send_ns(struct net_if *iface,
		     struct net_pkt *pending,
		     const struct in6_addr *src,
		     const struct in6_addr *dst,
		     const struct in6_addr *tgt,
		     bool is_my_address)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ns_access,
					      struct net_icmpv6_ns_hdr);
	struct net_pkt *pkt = NULL;
	int ret = -ENOBUFS;
	struct net_icmpv6_ns_hdr *ns_hdr;
	struct in6_addr node_dst;
	struct net_nbr *nbr;
	uint8_t llao_len;

	if (!dst) {
		net_ipv6_addr_create_solicited_node(tgt, &node_dst);
		dst = &node_dst;
	}

	llao_len = get_llao_len(iface);

	if (is_my_address) {
		src = net_ipv6_unspecified_address();
		llao_len = 0U;
	} else {
		if (!src) {
			src = net_if_ipv6_select_src_addr(iface, tgt);
		}

		if (net_ipv6_is_addr_unspecified(src)) {
			NET_DBG("No source address for NS (tgt %s)",
				net_sprint_ipv6_addr(tgt));
			ret = -EINVAL;

			goto drop;
		}
	}

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_icmpv6_ns_hdr) +
					llao_len,
					AF_INET6, IPPROTO_ICMPV6,
					ND_NET_BUF_TIMEOUT);
	if (!pkt) {
		ret = -ENOMEM;
		goto drop;
	}

	/* Avoid recursive loop with network packet capturing */
	if (IS_ENABLED(CONFIG_NET_CAPTURE) && pending) {
		net_pkt_set_captured(pkt, net_pkt_is_captured(pending));
	}

	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_ND_HOP_LIMIT);

	if (net_ipv6_create(pkt, src, dst) ||
	    net_icmpv6_create(pkt, NET_ICMPV6_NS, 0)) {
		goto drop;
	}

	ns_hdr = (struct net_icmpv6_ns_hdr *)net_pkt_get_data(pkt, &ns_access);
	if (!ns_hdr) {
		goto drop;
	}

	ns_hdr->reserved = 0U;
	net_ipv6_addr_copy_raw(ns_hdr->tgt, (uint8_t *)tgt);

	if (net_pkt_set_data(pkt, &ns_access)) {
		goto drop;
	}

	if (!is_my_address) {
		if (!set_llao(pkt, net_if_get_link_addr(iface),
			      llao_len, NET_ICMPV6_ND_OPT_SLLAO)) {
			goto drop;
		}
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);

	net_ipv6_nbr_lock();
	nbr = add_nbr(iface, tgt, false,
		      NET_IPV6_NBR_STATE_INCOMPLETE);
	if (!nbr) {
		NET_DBG("Could not create new neighbor %s",
			net_sprint_ipv6_addr(&ns_hdr->tgt));
		net_ipv6_nbr_unlock();
		goto drop;
	}

	if (pending) {
		if (!net_ipv6_nbr_data(nbr)->pending) {
			net_ipv6_nbr_data(nbr)->pending = net_pkt_ref(pending);
		} else {
			NET_DBG("Packet %p already pending for "
				"operation. Discarding pending %p and pkt %p",
				net_ipv6_nbr_data(nbr)->pending, pending, pkt);
			net_ipv6_nbr_unlock();
			goto drop;
		}

		NET_DBG("Setting timeout %d for NS", NS_REPLY_TIMEOUT);

		net_ipv6_nbr_data(nbr)->send_ns = k_uptime_get();

		/* Let's start the timer if necessary */
		if (!k_work_delayable_remaining_get(&ipv6_ns_reply_timer)) {
			k_work_reschedule(&ipv6_ns_reply_timer,
					  K_MSEC(NS_REPLY_TIMEOUT));
		}
	}

	dbg_addr_sent_tgt("Neighbor Solicitation", src, dst, &ns_hdr->tgt,
			  pkt);

	if (net_send_data(pkt) < 0) {
		NET_DBG("Cannot send NS %p (pending %p)", pkt, pending);

		if (pending) {
			nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
			pending = NULL;
		}

		net_ipv6_nbr_unlock();
		goto drop;
	}

	net_ipv6_nbr_unlock();

	net_stats_update_icmp_sent(net_pkt_iface(pkt));
	net_stats_update_ipv6_nd_sent(iface);

	return 0;

drop:
	if (pending) {
		net_pkt_unref(pending);
	}

	if (pkt) {
		net_pkt_unref(pkt);
	}

	net_stats_update_ipv6_nd_drop(iface);

	return ret;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
int net_ipv6_send_rs(struct net_if *iface)
{
	uint8_t llao_len = 0U;
	int ret = -ENOBUFS;
	const struct in6_addr *src;
	struct in6_addr dst;
	struct net_pkt *pkt;

	net_ipv6_addr_create_ll_allrouters_mcast(&dst);
	src = net_if_ipv6_select_src_addr(iface, &dst);

	if (!net_ipv6_is_addr_unspecified(src)) {
		llao_len = get_llao_len(iface);
	}

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_icmpv6_rs_hdr) +
					llao_len,
					AF_INET6, IPPROTO_ICMPV6,
					ND_NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_ND_HOP_LIMIT);

	if (net_ipv6_create(pkt, src, &dst) ||
	    net_icmpv6_create(pkt, NET_ICMPV6_RS, 0) ||
	    net_pkt_memset(pkt, 0, sizeof(struct net_icmpv6_rs_hdr))) {
		goto drop;
	}

	if (llao_len > 0) {
		if (!set_llao(pkt, net_if_get_link_addr(iface),
			      llao_len, NET_ICMPV6_ND_OPT_SLLAO)) {
			goto drop;
		}
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);

	dbg_addr_sent("Router Solicitation", src, &dst, pkt);

	if (net_send_data(pkt) < 0) {
		net_stats_update_ipv6_nd_drop(iface);
		ret = -EINVAL;

		goto drop;
	}

	net_stats_update_icmp_sent(net_pkt_iface(pkt));
	net_stats_update_ipv6_nd_sent(iface);

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

int net_ipv6_start_rs(struct net_if *iface)
{
	return net_ipv6_send_rs(iface);
}

static inline struct net_nbr *handle_ra_neighbor(struct net_pkt *pkt, uint8_t len)
{
	struct net_linkaddr lladdr;
	struct net_linkaddr_storage llstorage;

	if (!read_llao(pkt, len, &llstorage)) {
		return NULL;
	}

	lladdr.len = llstorage.len;
	lladdr.addr = llstorage.addr;

	return net_ipv6_nbr_add(net_pkt_iface(pkt),
				(struct in6_addr *)NET_IPV6_HDR(pkt)->src,
				&lladdr, true,
				NET_IPV6_NBR_STATE_STALE);
}

static inline void handle_prefix_onlink(struct net_pkt *pkt,
			struct net_icmpv6_nd_opt_prefix_info *prefix_info)
{
	struct net_if_ipv6_prefix *prefix;

	prefix = net_if_ipv6_prefix_lookup(net_pkt_iface(pkt),
					   (struct in6_addr *)prefix_info->prefix,
					   prefix_info->prefix_len);
	if (!prefix) {
		if (!prefix_info->valid_lifetime) {
			return;
		}

		prefix = net_if_ipv6_prefix_add(net_pkt_iface(pkt),
						(struct in6_addr *)prefix_info->prefix,
						prefix_info->prefix_len,
						prefix_info->valid_lifetime);
		if (prefix) {
			NET_DBG("Interface %p/%d add prefix %s/%d lifetime %u",
				net_pkt_iface(pkt),
				net_if_get_by_iface(net_pkt_iface(pkt)),
				net_sprint_ipv6_addr(&prefix_info->prefix),
				prefix_info->prefix_len,
				prefix_info->valid_lifetime);
		} else {
			NET_ERR("Prefix %s/%d could not be added to "
				"iface %p/%d",
				net_sprint_ipv6_addr(&prefix_info->prefix),
				prefix_info->prefix_len,
				net_pkt_iface(pkt),
				net_if_get_by_iface(net_pkt_iface(pkt)));

			return;
		}
	}

	switch (prefix_info->valid_lifetime) {
	case 0:
		NET_DBG("Interface %p/%d delete prefix %s/%d",
			net_pkt_iface(pkt),
			net_if_get_by_iface(net_pkt_iface(pkt)),
			net_sprint_ipv6_addr(&prefix_info->prefix),
			prefix_info->prefix_len);

		net_if_ipv6_prefix_rm(net_pkt_iface(pkt),
				      &prefix->prefix,
				      prefix->len);
		break;

	case NET_IPV6_ND_INFINITE_LIFETIME:
		NET_DBG("Interface %p/%d prefix %s/%d infinite",
			net_pkt_iface(pkt),
			net_if_get_by_iface(net_pkt_iface(pkt)),
			net_sprint_ipv6_addr(&prefix->prefix),
			prefix->len);

		net_if_ipv6_prefix_set_lf(prefix, true);
		break;

	default:
		NET_DBG("Interface %p/%d update prefix %s/%u lifetime %u",
			net_pkt_iface(pkt),
			net_if_get_by_iface(net_pkt_iface(pkt)),
			net_sprint_ipv6_addr(&prefix_info->prefix),
			prefix_info->prefix_len, prefix_info->valid_lifetime);

		net_if_ipv6_prefix_set_lf(prefix, false);
		net_if_ipv6_prefix_set_timer(prefix,
					     prefix_info->valid_lifetime);
		break;
	}
}

#define TWO_HOURS (2 * 60 * 60)

static inline uint32_t remaining_lifetime(struct net_if_addr *ifaddr)
{
	return net_timeout_remaining(&ifaddr->lifetime, k_uptime_get_32());
}

static inline void handle_prefix_autonomous(struct net_pkt *pkt,
			struct net_icmpv6_nd_opt_prefix_info *prefix_info)
{
	struct net_if *iface = net_pkt_iface(pkt);
	struct in6_addr addr = { };
	struct net_if_addr *ifaddr;

	/* Create IPv6 address using the given prefix and iid. We first
	 * setup link local address, and then copy prefix over first 8
	 * bytes of that address.
	 */
	net_ipv6_addr_create_iid(&addr, net_if_get_link_addr(iface));
	memcpy(&addr, prefix_info->prefix, sizeof(struct in6_addr) / 2);

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

			net_if_ipv6_addr_update_lifetime(
				ifaddr, prefix_info->valid_lifetime);
		} else {
			NET_DBG("Timer updating for address %s "
				"lifetime %u secs",
				net_sprint_ipv6_addr(&addr),
				TWO_HOURS);

			net_if_ipv6_addr_update_lifetime(ifaddr, TWO_HOURS);
		}

		net_if_addr_set_lf(ifaddr, false);
	} else {
		if (prefix_info->valid_lifetime ==
		    NET_IPV6_ND_INFINITE_LIFETIME) {
			net_if_ipv6_addr_add(iface, &addr,
					     NET_ADDR_AUTOCONF, 0);
		} else {
			net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF,
					     prefix_info->valid_lifetime);
		}
	}

	/* If privacy extensions are enabled, then start the procedure for that
	 * too.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6_PE) && iface->pe_enabled) {
		net_ipv6_pe_start(iface,
				  (const struct in6_addr *)prefix_info->prefix,
				  prefix_info->valid_lifetime,
				  prefix_info->preferred_lifetime);
	}
}

static inline bool handle_ra_prefix(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(rapfx_access,
				   struct net_icmpv6_nd_opt_prefix_info);
	struct net_icmpv6_nd_opt_prefix_info *pfx_info;
	uint32_t valid_lifetime, preferred_lifetime;

	pfx_info = (struct net_icmpv6_nd_opt_prefix_info *)
				net_pkt_get_data(pkt, &rapfx_access);
	if (!pfx_info) {
		return false;
	}

	net_pkt_acknowledge_data(pkt, &rapfx_access);

	valid_lifetime = ntohl(pfx_info->valid_lifetime);
	preferred_lifetime = ntohl(pfx_info->preferred_lifetime);

	if (valid_lifetime >= preferred_lifetime &&
	    !net_ipv6_is_ll_addr((struct in6_addr *)pfx_info->prefix)) {
		if (pfx_info->flags & NET_ICMPV6_RA_FLAG_ONLINK) {
			handle_prefix_onlink(pkt, pfx_info);
		}

		if ((pfx_info->flags & NET_ICMPV6_RA_FLAG_AUTONOMOUS) &&
		    valid_lifetime &&
		    (pfx_info->prefix_len == NET_IPV6_DEFAULT_PREFIX_LEN)) {
			handle_prefix_autonomous(pkt, pfx_info);
		}
	}

	return true;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
/* 6lowpan Context Option RFC 6775, 4.2 */
static inline bool handle_ra_6co(struct net_pkt *pkt, uint8_t len)
{
	NET_PKT_DATA_ACCESS_DEFINE(ctx_access, struct net_icmpv6_nd_opt_6co);
	struct net_icmpv6_nd_opt_6co *context;

	context = (struct net_icmpv6_nd_opt_6co *)
				net_pkt_get_data(pkt, &ctx_access);
	if (!context) {
		return false;
	}

	/* RFC 6775, 4.2
	 * Context Length: 8-bit unsigned integer.  The number of leading
	 * bits in the Context Prefix field that are valid.  The value ranges
	 * from 0 to 128.  If it is more than 64, then the Length MUST be 3.
	 */
	if ((context->context_len > 64 && len != 3U) ||
	    (context->context_len <= 64U && len != 2U)) {
		return false;
	}

	context->context_len = context->context_len / 8U;

	/* context_len: The number of leading bits in the Context Prefix
	 * field that are valid. Rest must be set to 0 by the sender and
	 * ignored by the receiver. But since there is no way to make sure
	 * the sender followed the rule, let's make sure rest is set to 0.
	 */
	if (context->context_len != sizeof(context->prefix)) {
		(void)memset(context->prefix + context->context_len, 0,
			     sizeof(context->prefix) - context->context_len);
	}

	net_6lo_set_context(net_pkt_iface(pkt), context);

	return true;
}
#endif

static inline bool handle_ra_route_info(struct net_pkt *pkt, uint8_t len)
{
	NET_PKT_DATA_ACCESS_DEFINE(routeinfo_access,
				   struct net_icmpv6_nd_opt_route_info);
	struct net_icmpv6_nd_opt_route_info *route_info;
	struct net_route_entry *route;
	struct in6_addr prefix_buf = { 0 };
	uint8_t prefix_field_len = (len - 1) * 8;
	uint32_t route_lifetime;
	uint8_t prefix_len;
	uint8_t preference;
	int ret;

	route_info = (struct net_icmpv6_nd_opt_route_info *)
				net_pkt_get_data(pkt, &routeinfo_access);
	if (!route_info) {
		return false;
	}

	ret = net_pkt_acknowledge_data(pkt, &routeinfo_access);
	if (ret < 0) {
		return false;
	}

	prefix_len = route_info->prefix_len;
	route_lifetime = ntohl(route_info->route_lifetime);
	preference = route_info->flags.prf;

	ret = net_pkt_read(pkt, &prefix_buf, prefix_field_len);
	if (ret < 0) {
		NET_ERR("Error reading prefix, %d", ret);
		return false;
	}

	if (route_lifetime == 0) {
		route = net_route_lookup(net_pkt_orig_iface(pkt), &prefix_buf);
		if (route != NULL) {
			ret = net_route_del(route);
			if (ret < 0) {
				NET_DBG("Failed to delete route");
			}
		}
	} else {
		route = net_route_add(net_pkt_orig_iface(pkt),
				      &prefix_buf,
				      prefix_len,
				      (struct in6_addr *)NET_IPV6_HDR(pkt)->src,
				      route_lifetime,
				      preference);
		if (route == NULL) {
			NET_DBG("Failed to add route");
		}
	}

	return true;
}

#if defined(CONFIG_NET_IPV6_RA_RDNSS)
static inline bool handle_ra_rdnss(struct net_pkt *pkt, uint8_t len)
{
	NET_PKT_DATA_ACCESS_DEFINE(rdnss_access, struct net_icmpv6_nd_opt_rdnss);
	struct net_icmpv6_nd_opt_rdnss *rdnss;
	struct dns_resolve_context *ctx;
	struct sockaddr_in6 dns = {
		.sin6_family = AF_INET6
	};
	const struct sockaddr *dns_servers[] = {
		(struct sockaddr *)&dns, NULL
	};
	size_t rdnss_size;
	int ret;

	rdnss = (struct net_icmpv6_nd_opt_rdnss *) net_pkt_get_data(pkt, &rdnss_access);
	if (!rdnss) {
		return false;
	}

	ret = net_pkt_acknowledge_data(pkt, &rdnss_access);
	if (ret < 0) {
		return false;
	}

	rdnss_size = len * 8U - 2 - sizeof(struct net_icmpv6_nd_opt_rdnss);
	if ((rdnss_size % NET_IPV6_ADDR_SIZE) != 0) {
		return false;
	}

	/* Recursive DNS servers option may present 1 or more addresses,
	 * each 16 bytes in length. DNS servers should be listed in order
	 * of preference, choose the first and skip the rest.
	 */
	ret = net_pkt_read(pkt, dns.sin6_addr.s6_addr, NET_IPV6_ADDR_SIZE);
	if (ret < 0) {
		NET_ERR("Failed to read RDNSS address, %d", ret);
		return false;
	}

	/* Skip the rest of the DNS servers. */
	if (net_pkt_skip(pkt, rdnss_size - NET_IPV6_ADDR_SIZE)) {
		NET_ERR("Failed to skip RDNSS address, %d", ret);
		return false;
	}

	/* TODO: Handle lifetime. */
	ctx = dns_resolve_get_default();
	ret = dns_resolve_reconfigure(ctx, NULL, dns_servers);
	if (ret < 0) {
		NET_DBG("Failed to set RDNSS resolve address: %d", ret);
	}

	return true;
}
#endif

static int handle_ra_input(struct net_icmp_ctx *ctx,
			   struct net_pkt *pkt,
			   struct net_icmp_ip_hdr *hdr,
			   struct net_icmp_hdr *icmp_hdr,
			   void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ra_access,
					      struct net_icmpv6_ra_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(nd_access, struct net_icmpv6_nd_opt_hdr);
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	uint16_t length = net_pkt_get_len(pkt);
	struct net_nbr *nbr = NULL;
	struct net_icmpv6_nd_opt_hdr *nd_opt_hdr;
	struct net_icmpv6_ra_hdr *ra_hdr;
	struct net_if_router *router;
	uint32_t mtu, reachable_time, retrans_timer;
	uint16_t router_lifetime;

	ARG_UNUSED(user_data);

	if (net_if_flag_is_set(net_pkt_iface(pkt), NET_IF_IPV6_NO_ND)) {
		goto drop;
	}

	ra_hdr = (struct net_icmpv6_ra_hdr *)net_pkt_get_data(pkt, &ra_access);
	if (!ra_hdr) {
		NET_ERR("DROP: NULL RA header");
		goto drop;
	}

	dbg_addr_recv("Router Advertisement", &ip_hdr->src, &ip_hdr->dst, pkt);

	net_stats_update_ipv6_nd_recv(net_pkt_iface(pkt));

	if (((length < (sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_icmp_hdr) +
			sizeof(struct net_icmpv6_ra_hdr) +
			sizeof(struct net_icmpv6_nd_opt_hdr))) ||
	     (ip_hdr->hop_limit != NET_IPV6_ND_HOP_LIMIT) ||
	     !net_ipv6_is_ll_addr((struct in6_addr *)ip_hdr->src)) &&
		icmp_hdr->code != 0U) {
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &ra_access);

	router_lifetime = ntohs(ra_hdr->router_lifetime);
	reachable_time = ntohl(ra_hdr->reachable_time);
	retrans_timer = ntohl(ra_hdr->retrans_timer);

	if (ra_hdr->cur_hop_limit) {
		net_if_ipv6_set_hop_limit(net_pkt_iface(pkt),
					  ra_hdr->cur_hop_limit);
		NET_DBG("New hop limit %d",
			net_if_ipv6_get_hop_limit(net_pkt_iface(pkt)));
	}

	if (reachable_time && reachable_time <= MAX_REACHABLE_TIME &&
	    (net_if_ipv6_get_reachable_time(net_pkt_iface(pkt)) !=
	     reachable_time)) {
		net_if_ipv6_set_base_reachable_time(net_pkt_iface(pkt),
						    reachable_time);
		net_if_ipv6_set_reachable_time(
			net_pkt_iface(pkt)->config.ip.ipv6);
	}

	if (retrans_timer) {
		net_if_ipv6_set_retrans_timer(net_pkt_iface(pkt),
					      ra_hdr->retrans_timer);
	}

	net_pkt_set_ipv6_ext_opt_len(pkt, sizeof(struct net_icmpv6_ra_hdr));
	length -= (sizeof(struct net_ipv6_hdr) + sizeof(struct net_icmp_hdr));

	nd_opt_hdr = (struct net_icmpv6_nd_opt_hdr *)
				net_pkt_get_data(pkt, &nd_access);

	/* Add neighbor cache entry using link local address, regardless of link layer address
	 * presence in Router Advertisement.
	 */
	nbr = net_ipv6_nbr_add(net_pkt_iface(pkt), (struct in6_addr *)NET_IPV6_HDR(pkt)->src, NULL,
				true, NET_IPV6_NBR_STATE_INCOMPLETE);

	while (nd_opt_hdr) {
		net_pkt_acknowledge_data(pkt, &nd_access);

		switch (nd_opt_hdr->type) {
		case NET_ICMPV6_ND_OPT_SLLAO:
			/* Update existing neighbor cache entry with link layer address. */
			nbr = handle_ra_neighbor(pkt, nd_opt_hdr->len);
			if (!nbr) {
				goto drop;
			}

			break;
		case NET_ICMPV6_ND_OPT_MTU:
			/* MTU has reserved 2 bytes, so skip it. */
			if (net_pkt_skip(pkt, 2) ||
			    net_pkt_read_be32(pkt, &mtu)) {
				goto drop;
			}

			if (mtu < MIN_IPV6_MTU || mtu > MAX_IPV6_MTU) {
				NET_ERR("DROP: Unsupported MTU %u, min is %u, "
					"max is %u",
					mtu, MIN_IPV6_MTU, MAX_IPV6_MTU);
				goto drop;
			}

			net_if_set_mtu(net_pkt_iface(pkt), mtu);

			break;
		case NET_ICMPV6_ND_OPT_PREFIX_INFO:
			if (nd_opt_hdr->len != 4) {
				NET_ERR("DROP: Invalid %s length (%d)",
					"prefix opt", nd_opt_hdr->len);
				goto drop;
			}

			if (!handle_ra_prefix(pkt)) {
				goto drop;
			}

			break;
#if defined(CONFIG_NET_6LO_CONTEXT)
		case NET_ICMPV6_ND_OPT_6CO:
			/* RFC 6775, 4.2 (Length)*/
			if (!(nd_opt_hdr->len == 2U || nd_opt_hdr->len == 3U)) {
				NET_ERR("DROP: Invalid %s length %d",
					"6CO", nd_opt_hdr->len);
				goto drop;
			}

			if (!handle_ra_6co(pkt, nd_opt_hdr->len)) {
				goto drop;
			}

			break;
#endif
		case NET_ICMPV6_ND_OPT_ROUTE:
			if (!IS_ENABLED(CONFIG_NET_ROUTE)) {
				NET_DBG("Route option skipped");
				goto skip;
			}

			/* RFC 4191, ch. 2.3 */
			if (nd_opt_hdr->len == 0U || nd_opt_hdr->len > 3U) {
				NET_ERR("DROP: Invalid %s length (%d)",
					"route info opt", nd_opt_hdr->len);
				goto drop;
			}

			if (!handle_ra_route_info(pkt, nd_opt_hdr->len)) {
				goto drop;
			}

			break;
#if defined(CONFIG_NET_IPV6_RA_RDNSS)
		case NET_ICMPV6_ND_OPT_RDNSS:
			if (!handle_ra_rdnss(pkt, nd_opt_hdr->len)) {
				goto drop;
			}
			break;
#endif

		case NET_ICMPV6_ND_OPT_DNSSL:
			NET_DBG("DNSSL option skipped");
			goto skip;

		default:
			NET_DBG("Unknown ND option 0x%x", nd_opt_hdr->type);
		skip:
			if (net_pkt_skip(pkt, nd_opt_hdr->len * 8U - 2)) {
				goto drop;
			}

			break;
		}

		nd_opt_hdr = (struct net_icmpv6_nd_opt_hdr *)
					net_pkt_get_data(pkt, &nd_access);
	}

	router = net_if_ipv6_router_lookup(net_pkt_iface(pkt),
					   (struct in6_addr *)ip_hdr->src);
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

			net_if_ipv6_router_update_lifetime(
					router, router_lifetime);
		}
	} else {
		net_if_ipv6_router_add(net_pkt_iface(pkt),
				       (struct in6_addr *)ip_hdr->src,
				       router_lifetime);
	}

	net_ipv6_nbr_lock();
	if (nbr && net_ipv6_nbr_data(nbr)->pending) {
		NET_DBG("Sending pending pkt %p to %s",
			net_ipv6_nbr_data(nbr)->pending,
			net_sprint_ipv6_addr(&NET_IPV6_HDR(net_ipv6_nbr_data(nbr)->pending)->dst));

		if (net_send_data(net_ipv6_nbr_data(nbr)->pending) < 0) {
			net_pkt_unref(net_ipv6_nbr_data(nbr)->pending);
		}

		nbr_clear_ns_pending(net_ipv6_nbr_data(nbr));
	}
	net_ipv6_nbr_unlock();

	/* Cancel the RS timer on iface */
	net_if_stop_rs(net_pkt_iface(pkt));

	return 0;

drop:
	net_stats_update_ipv6_nd_drop(net_pkt_iface(pkt));

	return -EIO;
}
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static struct net_icmp_ctx ns_ctx;
static struct net_icmp_ctx na_ctx;
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
static struct net_icmp_ctx ra_ctx;
#endif /* CONFIG_NET_IPV6_ND */

void net_ipv6_nbr_init(void)
{
	int ret;

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	ret = net_icmp_init_ctx(&ns_ctx, NET_ICMPV6_NS, 0, handle_ns_input);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV6_NS),
			ret);
	}

	ret = net_icmp_init_ctx(&na_ctx, NET_ICMPV6_NA, 0, handle_na_input);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV6_NA),
			ret);
	}

	k_work_init_delayable(&ipv6_ns_reply_timer, ipv6_ns_reply_timeout);
#endif
#if defined(CONFIG_NET_IPV6_ND)
	ret = net_icmp_init_ctx(&ra_ctx, NET_ICMPV6_RA, 0, handle_ra_input);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV6_RA),
			ret);
	}

	k_work_init_delayable(&ipv6_nd_reachable_timer,
			      ipv6_nd_reachable_timeout);
#endif

	ARG_UNUSED(ret);
}
