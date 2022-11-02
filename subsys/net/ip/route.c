/** @file
 * @brief Route handling.
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_route, CONFIG_NET_ROUTE_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <limits.h>
#include <zephyr/types.h>
#include <zephyr/sys/slist.h>

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>

#include "net_private.h"
#include "ipv6.h"
#include "icmpv6.h"
#include "nbr.h"
#include "route.h"

#if !defined(NET_ROUTE_EXTRA_DATA_SIZE)
#define NET_ROUTE_EXTRA_DATA_SIZE 0
#endif

/* We keep track of the routes in a separate list so that we can remove
 * the oldest routes (at tail) if needed.
 */
static sys_slist_t routes;

/* Track currently active route lifetime timers */
static sys_slist_t active_route_lifetime_timers;

/* Timer that manages expired route entries. */
static struct k_work_delayable route_lifetime_timer;

static K_MUTEX_DEFINE(lock);

static void net_route_nexthop_remove(struct net_nbr *nbr)
{
	NET_DBG("Nexthop %p removed", nbr);
}

/*
 * This pool contains information next hop neighbors.
 */
NET_NBR_POOL_INIT(net_route_nexthop_pool,
		  CONFIG_NET_MAX_NEXTHOPS,
		  sizeof(struct net_route_nexthop),
		  net_route_nexthop_remove,
		  0);

static inline struct net_route_nexthop *net_nexthop_data(struct net_nbr *nbr)
{
	return (struct net_route_nexthop *)nbr->data;
}

static inline struct net_nbr *get_nexthop_nbr(struct net_nbr *start, int idx)
{
	NET_ASSERT(idx < CONFIG_NET_MAX_NEXTHOPS, "idx %d >= max %d",
		   idx, CONFIG_NET_MAX_NEXTHOPS);

	return (struct net_nbr *)((uint8_t *)start +
			((sizeof(struct net_nbr) + start->size) * idx));
}

static void release_nexthop_route(struct net_route_nexthop *route_nexthop)
{
	struct net_nbr *nbr = CONTAINER_OF(route_nexthop, struct net_nbr, __nbr);

	net_nbr_unref(nbr);
}

static struct net_nbr *get_nexthop_route(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_NEXTHOPS; i++) {
		struct net_nbr *nbr = get_nexthop_nbr(
			(struct net_nbr *)net_route_nexthop_pool, i);

		if (!nbr->ref) {
			nbr->data = nbr->__nbr;

			nbr->idx = NET_NBR_LLADDR_UNKNOWN;

			return net_nbr_ref(nbr);
		}
	}

	return NULL;
}

static void net_route_entry_remove(struct net_nbr *nbr)
{
	NET_DBG("Route %p removed", nbr);
}

static void net_route_entries_table_clear(struct net_nbr_table *table)
{
	NET_DBG("Route table %p cleared", table);
}

/*
 * This pool contains routing table entries.
 */
NET_NBR_POOL_INIT(net_route_entries_pool,
		  CONFIG_NET_MAX_ROUTES,
		  sizeof(struct net_route_entry),
		  net_route_entry_remove,
		  NET_ROUTE_EXTRA_DATA_SIZE);

NET_NBR_TABLE_INIT(NET_NBR_LOCAL, nbr_routes, net_route_entries_pool,
		   net_route_entries_table_clear);

static inline struct net_nbr *get_nbr(int idx)
{
	return &net_route_entries_pool[idx].nbr;
}

static inline struct net_route_entry *net_route_data(struct net_nbr *nbr)
{
	return (struct net_route_entry *)nbr->data;
}

struct net_nbr *net_route_get_nbr(struct net_route_entry *route)
{
	int i;

	NET_ASSERT(route);

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (nbr->data == (uint8_t *)route) {
			if (!nbr->ref) {
				k_mutex_unlock(&lock);
				return NULL;
			}

			k_mutex_unlock(&lock);
			return nbr;
		}
	}

	k_mutex_unlock(&lock);
	return NULL;
}

void net_routes_print(void)
{
	int i;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		NET_DBG("[%d] %p %d addr %s/%d",
			i, nbr, nbr->ref,
			net_sprint_ipv6_addr(&net_route_data(nbr)->addr),
			net_route_data(nbr)->prefix_len);
		NET_DBG("    iface %p idx %d ll %s",
			nbr->iface, nbr->idx,
			nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
			net_sprint_ll_addr(
				net_nbr_get_lladdr(nbr->idx)->addr,
				net_nbr_get_lladdr(nbr->idx)->len));
	}

	k_mutex_unlock(&lock);
}

static inline void nbr_free(struct net_nbr *nbr)
{
	NET_DBG("nbr %p", nbr);

	net_nbr_unref(nbr);
}

static struct net_nbr *nbr_new(struct net_if *iface,
			       struct in6_addr *addr,
			       uint8_t prefix_len)
{
	struct net_nbr *nbr = net_nbr_get(&net_nbr_routes.table);

	if (!nbr) {
		return NULL;
	}

	nbr->iface = iface;

	net_ipaddr_copy(&net_route_data(nbr)->addr, addr);
	net_route_data(nbr)->prefix_len = prefix_len;

	NET_DBG("[%d] nbr %p iface %p IPv6 %s/%d",
		nbr->idx, nbr, iface,
		net_sprint_ipv6_addr(&net_route_data(nbr)->addr),
		prefix_len);

	return nbr;
}

static struct net_nbr *nbr_nexthop_get(struct net_if *iface,
				       struct in6_addr *addr)
{
	/* Note that the nexthop host must be already in the neighbor
	 * cache. We just increase the ref count of an existing entry.
	 */
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(iface, addr);
	if (nbr == NULL) {
		NET_DBG("Next hop neighbor not found!");
		return NULL;
	}

	net_nbr_ref(nbr);

	NET_DBG("[%d] nbr %p iface %p IPv6 %s",
		nbr->idx, nbr, iface,
		net_sprint_ipv6_addr(addr));

	return nbr;
}

static int nbr_nexthop_put(struct net_nbr *nbr)
{
	NET_ASSERT(nbr);

	NET_DBG("[%d] nbr %p iface %p", nbr->idx, nbr, nbr->iface);

	net_nbr_unref(nbr);

	return 0;
}


#define net_route_info(str, route, dst)					\
	do {								\
	if (CONFIG_NET_ROUTE_LOG_LEVEL >= LOG_LEVEL_DBG) {		\
		struct in6_addr *naddr = net_route_get_nexthop(route);	\
									\
		NET_ASSERT(naddr, "Unknown nexthop address");	\
									\
		NET_DBG("%s route to %s via %s (iface %p)", str,	\
			net_sprint_ipv6_addr(dst),		\
			net_sprint_ipv6_addr(naddr),	\
			route->iface);					\
	} } while (0)

/* Route was accessed, so place it in front of the routes list */
static inline void update_route_access(struct net_route_entry *route)
{
	sys_slist_find_and_remove(&routes, &route->node);
	sys_slist_prepend(&routes, &route->node);
}

struct net_route_entry *net_route_lookup(struct net_if *iface,
					 struct in6_addr *dst)
{
	struct net_route_entry *route, *found = NULL;
	uint8_t longest_match = 0U;
	int i;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES && longest_match < 128; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (iface && nbr->iface != iface) {
			continue;
		}

		route = net_route_data(nbr);

		if (route->prefix_len >= longest_match &&
		    net_ipv6_is_prefix(dst->s6_addr,
				       route->addr.s6_addr,
				       route->prefix_len)) {
			found = route;
			longest_match = route->prefix_len;
		}
	}

	if (found) {
		net_route_info("Found", found, dst);

		update_route_access(found);
	}

	k_mutex_unlock(&lock);
	return found;
}

static inline bool route_preference_is_lower(uint8_t old, uint8_t new)
{
	if (new == NET_ROUTE_PREFERENCE_RESERVED || (new & 0xfc) != 0) {
		return true;
	}

	/* Transform valid preference values into comparable integers */
	old = (old + 1) & 0x3;
	new = (new + 1) & 0x3;

	return new < old;
}

struct net_route_entry *net_route_add(struct net_if *iface,
				      struct in6_addr *addr,
				      uint8_t prefix_len,
				      struct in6_addr *nexthop,
				      uint32_t lifetime,
				      uint8_t preference)
{
	struct net_linkaddr_storage *nexthop_lladdr;
	struct net_nbr *nbr, *nbr_nexthop, *tmp;
	struct net_route_nexthop *nexthop_route;
	struct net_route_entry *route = NULL;
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
       struct net_event_ipv6_route info;
#endif

	NET_ASSERT(addr);
	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	if (net_ipv6_addr_cmp(addr, net_ipv6_unspecified_address())) {
		NET_DBG("Route cannot be towards unspecified address");
		return NULL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);
	if (!nbr_nexthop) {
		NET_DBG("No such neighbor %s found",
			net_sprint_ipv6_addr(nexthop));
		goto exit;
	}

	if (nbr_nexthop && nbr_nexthop->idx != NET_NBR_LLADDR_UNKNOWN) {
		nexthop_lladdr = net_nbr_get_lladdr(nbr_nexthop->idx);
		NET_ASSERT(nexthop_lladdr);
		NET_DBG("Nexthop %s lladdr is %s", net_sprint_ipv6_addr(nexthop),
			net_sprint_ll_addr(nexthop_lladdr->addr, nexthop_lladdr->len));
	}

	route = net_route_lookup(iface, addr);
	if (route) {
		/* Update nexthop if not the same */
		struct in6_addr *nexthop_addr;

		nexthop_addr = net_route_get_nexthop(route);
		if (nexthop_addr && net_ipv6_addr_cmp(nexthop, nexthop_addr)) {
			NET_DBG("No changes, return old route %p", route);

			/* Reset lifetime timer. */
			net_route_update_lifetime(route, lifetime);

			route->preference = preference;

			goto exit;
		}

		if (route_preference_is_lower(route->preference, preference)) {
			NET_DBG("No changes, ignoring route with lower preference");
			route = NULL;
			goto exit;
		}

		NET_DBG("Old route to %s found",
			net_sprint_ipv6_addr(nexthop_addr));

		net_route_del(route);
	}

	nbr = nbr_new(iface, addr, prefix_len);
	if (!nbr) {
		/* Remove the oldest route and try again */
		sys_snode_t *last = sys_slist_peek_tail(&routes);

		sys_slist_find_and_remove(&routes, last);

		route = CONTAINER_OF(last,
				     struct net_route_entry,
				     node);

		if (CONFIG_NET_ROUTE_LOG_LEVEL >= LOG_LEVEL_DBG) {
			struct in6_addr *tmp;
			struct net_linkaddr_storage *llstorage;

			tmp = net_route_get_nexthop(route);
			nbr = net_ipv6_nbr_lookup(iface, tmp);
			if (nbr) {
				llstorage = net_nbr_get_lladdr(nbr->idx);

				NET_DBG("Removing the oldest route %s "
					"via %s [%s]",
					net_sprint_ipv6_addr(&route->addr),
					net_sprint_ipv6_addr(tmp),
					net_sprint_ll_addr(llstorage->addr,
							   llstorage->len));
			}
		}

		net_route_del(route);

		nbr = nbr_new(iface, addr, prefix_len);
		if (!nbr) {
			NET_ERR("Neighbor route alloc failed!");
			route = NULL;
			goto exit;
		}
	}

	tmp = get_nexthop_route();
	if (!tmp) {
		NET_ERR("No nexthop route available!");
		route = NULL;
		goto exit;
	}

	nexthop_route = net_nexthop_data(tmp);

	route = net_route_data(nbr);
	route->iface = iface;
	route->preference = preference;

	net_route_update_lifetime(route, lifetime);

	sys_slist_prepend(&routes, &route->node);

	tmp = nbr_nexthop_get(iface, nexthop);

	NET_ASSERT(tmp == nbr_nexthop);

	nexthop_route->nbr = tmp;

	sys_slist_init(&route->nexthop);
	sys_slist_prepend(&route->nexthop, &nexthop_route->node);

	net_route_info("Added", route, addr);

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	net_ipaddr_copy(&info.addr, addr);
	net_ipaddr_copy(&info.nexthop, nexthop);
	info.prefix_len = prefix_len;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTE_ADD,
					iface, (void *) &info,
					sizeof(struct net_event_ipv6_route));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_ROUTE_ADD, iface);
#endif

exit:
	k_mutex_unlock(&lock);
	return route;
}

static void route_expired(struct net_route_entry *route)
{
	NET_DBG("Route to %s expired",
		net_sprint_ipv6_addr(&route->addr));

	sys_slist_find_and_remove(&active_route_lifetime_timers,
				  &route->lifetime.node);

	net_route_del(route);
}

static void route_lifetime_timeout(struct k_work *work)
{
	uint32_t next_update = UINT32_MAX;
	uint32_t current_time = k_uptime_get_32();
	struct net_route_entry *current, *next;

	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_route_lifetime_timers,
					  current, next, lifetime.node) {
		struct net_timeout *timeout = &current->lifetime;
		uint32_t this_update = net_timeout_evaluate(timeout,
							    current_time);

		if (this_update == 0U) {
			route_expired(current);
			continue;
		}

		if (this_update < next_update) {
			next_update = this_update;
		}
	}

	if (next_update != UINT32_MAX) {
		k_work_reschedule(&route_lifetime_timer, K_MSEC(next_update));
	}

	k_mutex_unlock(&lock);
}

void net_route_update_lifetime(struct net_route_entry *route, uint32_t lifetime)
{
	NET_DBG("Updating route lifetime of %s to %u secs",
		net_sprint_ipv6_addr(&route->addr),
		lifetime);

	if (!route) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (lifetime == NET_IPV6_ND_INFINITE_LIFETIME) {
		route->is_infinite = true;

		(void)sys_slist_find_and_remove(&active_route_lifetime_timers,
						&route->lifetime.node);
	} else {
		route->is_infinite = false;

		net_timeout_set(&route->lifetime, lifetime, k_uptime_get_32());

		(void)sys_slist_find_and_remove(&active_route_lifetime_timers,
						&route->lifetime.node);
		sys_slist_append(&active_route_lifetime_timers,
				 &route->lifetime.node);
		k_work_reschedule(&route_lifetime_timer, K_NO_WAIT);
	}

	k_mutex_unlock(&lock);
}

int net_route_del(struct net_route_entry *route)
{
	struct net_nbr *nbr;
	struct net_route_nexthop *nexthop_route;
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
       struct net_event_ipv6_route info;
#endif

	if (!route) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	net_ipaddr_copy(&info.addr, &route->addr);
	info.prefix_len = route->prefix_len;
	net_ipaddr_copy(&info.nexthop,
			net_route_get_nexthop(route));

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTE_DEL,
					route->iface, (void *) &info,
					sizeof(struct net_event_ipv6_route));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_ROUTE_DEL, route->iface);
#endif

	if (!route->is_infinite) {
		sys_slist_find_and_remove(&active_route_lifetime_timers,
					  &route->lifetime.node);

		if (sys_slist_is_empty(&active_route_lifetime_timers)) {
			k_work_cancel_delayable(&route_lifetime_timer);
		}
	}

	sys_slist_find_and_remove(&routes, &route->node);

	nbr = net_route_get_nbr(route);
	if (!nbr) {
		k_mutex_unlock(&lock);
		return -ENOENT;
	}

	net_route_info("Deleted", route, &route->addr);

	SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route, node) {
		if (!nexthop_route->nbr) {
			continue;
		}

		nbr_nexthop_put(nexthop_route->nbr);
		release_nexthop_route(nexthop_route);
	}

	nbr_free(nbr);

	k_mutex_unlock(&lock);
	return 0;
}

int net_route_del_by_nexthop(struct net_if *iface, struct in6_addr *nexthop)
{
	int count = 0, status = 0;
	struct net_nbr *nbr_nexthop;
	struct net_route_nexthop *nexthop_route;
	int i, ret;

	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	k_mutex_lock(&lock, K_FOREVER);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_route_entry *route = net_route_data(nbr);

		if (!route) {
			continue;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route,
					     node) {
			if (nexthop_route->nbr == nbr_nexthop) {
				/* This route contains this nexthop */
				ret = net_route_del(route);
				if (!ret) {
					count++;
				} else {
					status = ret;
				}
				break;
			}
		}
	}

	k_mutex_unlock(&lock);

	if (count) {
		return count;
	} else if (status < 0) {
		return status;
	}

	return 0;
}

int net_route_del_by_nexthop_data(struct net_if *iface,
				  struct in6_addr *nexthop,
				  void *data)
{
	int count = 0, status = 0;
	struct net_nbr *nbr_nexthop;
	struct net_route_nexthop *nexthop_route;
	int i, ret;

	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	k_mutex_lock(&lock, K_FOREVER);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);
	if (!nbr_nexthop) {
		k_mutex_unlock(&lock);
		return -EINVAL;
	}

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_route_entry *route = net_route_data(nbr);

		SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route,
					     node) {
			void *extra_data;

			if (nexthop_route->nbr != nbr_nexthop) {
				continue;
			}

			if (nbr->extra_data_size == 0U) {
				continue;
			}

			/* Routing engine specific extra data needs
			 * to match too.
			 */
			extra_data = net_nbr_extra_data(nbr_nexthop);
			if (extra_data != data) {
				continue;
			}

			ret = net_route_del(route);
			if (!ret) {
				count++;
			} else {
				status = ret;
			}

			break;
		}
	}

	k_mutex_unlock(&lock);

	if (count) {
		return count;
	}

	return status;
}

struct in6_addr *net_route_get_nexthop(struct net_route_entry *route)
{
	struct net_route_nexthop *nexthop_route;
	struct net_ipv6_nbr_data *ipv6_nbr_data;

	if (!route) {
		return NULL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route, node) {
		struct in6_addr *addr;

		ipv6_nbr_data = net_ipv6_nbr_data(nexthop_route->nbr);
		if (ipv6_nbr_data) {
			addr = &ipv6_nbr_data->addr;
			NET_ASSERT(addr);

			k_mutex_unlock(&lock);
			return addr;
		} else {
			NET_ERR("could not get neighbor data from next hop");
		}
	}

	k_mutex_unlock(&lock);
	return NULL;
}

int net_route_foreach(net_route_cb_t cb, void *user_data)
{
	int i, ret = 0;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_route_entry *route;
		struct net_nbr *nbr;

		nbr = get_nbr(i);
		if (!nbr) {
			continue;
		}

		if (!nbr->ref) {
			continue;
		}

		route = net_route_data(nbr);
		if (!route) {
			continue;
		}

		cb(route, user_data);

		ret++;
	}

	k_mutex_unlock(&lock);
	return ret;
}

#if defined(CONFIG_NET_ROUTE_MCAST)
/*
 * This array contains multicast routing entries.
 */
static
struct net_route_entry_mcast route_mcast_entries[CONFIG_NET_MAX_MCAST_ROUTES];

int net_route_mcast_forward_packet(struct net_pkt *pkt,
				   const struct net_ipv6_hdr *hdr)
{
	int i, ret = 0, err = 0;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; ++i) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];
		struct net_pkt *pkt_cpy = NULL;

		if (!route->is_used) {
			continue;
		}

		if (!net_if_flag_is_set(route->iface,
					NET_IF_FORWARD_MULTICASTS) ||
		    !net_ipv6_is_prefix(hdr->dst,
					route->group.s6_addr,
					route->prefix_len)         ||
		    (pkt->iface == route->iface)) {
			continue;
		}

		pkt_cpy = net_pkt_shallow_clone(pkt, K_NO_WAIT);

		if (pkt_cpy == NULL) {
			err--;
			continue;
		}

		net_pkt_set_forwarding(pkt_cpy, true);
		net_pkt_set_iface(pkt_cpy, route->iface);

		if (net_send_data(pkt_cpy) >= 0) {
			++ret;
		} else {
			net_pkt_unref(pkt_cpy);
			--err;
		}
	}

	return (err == 0) ? ret : err;
}

int net_route_mcast_foreach(net_route_mcast_cb_t cb,
			    struct in6_addr *skip,
			    void *user_data)
{
	int i, ret = 0;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (route->is_used) {
			if (skip && net_ipv6_is_prefix(skip->s6_addr,
						       route->group.s6_addr,
						       route->prefix_len)) {
				continue;
			}

			cb(route, user_data);

			ret++;
		}
	}

	return ret;
}

struct net_route_entry_mcast *net_route_mcast_add(struct net_if *iface,
						  struct in6_addr *group,
						  uint8_t prefix_len)
{
	int i;

	k_mutex_lock(&lock, K_FOREVER);

	if ((!net_if_flag_is_set(iface, NET_IF_FORWARD_MULTICASTS)) ||
			(!net_ipv6_is_addr_mcast(group)) ||
			(net_ipv6_is_addr_mcast_iface(group)) ||
			(net_ipv6_is_addr_mcast_link(group))) {
		k_mutex_unlock(&lock);
		return NULL;
	}

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (!route->is_used) {
			net_ipaddr_copy(&route->group, group);

			route->prefix_len = prefix_len;
			route->iface = iface;
			route->is_used = true;

			k_mutex_unlock(&lock);
			return route;
		}
	}

	k_mutex_unlock(&lock);
	return NULL;
}

bool net_route_mcast_del(struct net_route_entry_mcast *route)
{
	if (route > &route_mcast_entries[CONFIG_NET_MAX_MCAST_ROUTES - 1] ||
	    route < &route_mcast_entries[0]) {
		return false;
	}

	NET_ASSERT(route->is_used,
		   "Multicast route %p to %s was already removed", route,
		   net_sprint_ipv6_addr(&route->group));

	route->is_used = false;

	return true;
}

struct net_route_entry_mcast *
net_route_mcast_lookup(struct in6_addr *group)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (!route->is_used) {
			continue;
		}

		if (net_ipv6_is_prefix(group->s6_addr,
					route->group.s6_addr,
					route->prefix_len)) {
			return route;
		}
	}

	return NULL;
}
#endif /* CONFIG_NET_ROUTE_MCAST */

bool net_route_get_info(struct net_if *iface,
			struct in6_addr *dst,
			struct net_route_entry **route,
			struct in6_addr **nexthop)
{
	struct net_if_router *router;
	bool ret = false;

	k_mutex_lock(&lock, K_FOREVER);

	/* Search in neighbor table first, if not search in routing table. */
	if (net_ipv6_nbr_lookup(iface, dst)) {
		/* Found nexthop, no need to look into routing table. */
		*route = NULL;
		*nexthop = dst;

		ret = true;
		goto exit;
	}

	*route = net_route_lookup(iface, dst);
	if (*route) {
		*nexthop = net_route_get_nexthop(*route);
		if (!*nexthop) {
			goto exit;
		}

		ret = true;
		goto exit;
	} else {
		/* No specific route to this host, use the default
		 * route instead.
		 */
		router = net_if_ipv6_router_find_default(NULL, dst);
		if (!router) {
			goto exit;
		}

		*nexthop = &router->address.in6_addr;

		ret = true;
		goto exit;
	}

exit:
	k_mutex_unlock(&lock);
	return ret;
}

int net_route_packet(struct net_pkt *pkt, struct in6_addr *nexthop)
{
	struct net_linkaddr_storage *lladdr;
	struct net_nbr *nbr;
	int err;

	k_mutex_lock(&lock, K_FOREVER);

	nbr = net_ipv6_nbr_lookup(NULL, nexthop);
	if (!nbr) {
		NET_DBG("Cannot find %s neighbor",
			net_sprint_ipv6_addr(nexthop));
		err = -ENOENT;
		goto error;
	}

	lladdr = net_nbr_get_lladdr(nbr->idx);
	if (!lladdr) {
		NET_DBG("Cannot find %s neighbor link layer address.",
			net_sprint_ipv6_addr(nexthop));
		err = -ESRCH;
		goto error;
	}

#if defined(CONFIG_NET_L2_DUMMY)
	/* No need to do this check for dummy L2 as it does not have any
	 * link layer. This is done at runtime because we can have multiple
	 * network technologies enabled.
	 */
	if (net_if_l2(net_pkt_iface(pkt)) != &NET_L2_GET_NAME(DUMMY)) {
#endif
#if defined(CONFIG_NET_L2_PPP)
		/* PPP does not populate the lladdr fields */
		if (net_if_l2(net_pkt_iface(pkt)) != &NET_L2_GET_NAME(PPP)) {
#endif
			if (!net_pkt_lladdr_src(pkt)->addr) {
				NET_DBG("Link layer source address not set");
				err = -EINVAL;
				goto error;
			}

			/* Sanitycheck: If src and dst ll addresses are going
			 * to be same, then something went wrong in route
			 * lookup.
			 */
			if (!memcmp(net_pkt_lladdr_src(pkt)->addr, lladdr->addr,
				    lladdr->len)) {
				NET_ERR("Src ll and Dst ll are same");
				err = -EINVAL;
				goto error;
			}
#if defined(CONFIG_NET_L2_PPP)
		}
#endif
#if defined(CONFIG_NET_L2_DUMMY)
	}
#endif

	net_pkt_set_forwarding(pkt, true);

	/* Set the destination and source ll address in the packet.
	 * We set the destination address to be the nexthop recipient.
	 */
	net_pkt_lladdr_src(pkt)->addr = net_pkt_lladdr_if(pkt)->addr;
	net_pkt_lladdr_src(pkt)->type = net_pkt_lladdr_if(pkt)->type;
	net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;

	net_pkt_lladdr_dst(pkt)->addr = lladdr->addr;
	net_pkt_lladdr_dst(pkt)->type = lladdr->type;
	net_pkt_lladdr_dst(pkt)->len = lladdr->len;

	net_pkt_set_iface(pkt, nbr->iface);

	k_mutex_unlock(&lock);
	return net_send_data(pkt);

error:
	k_mutex_unlock(&lock);
	return err;
}

int net_route_packet_if(struct net_pkt *pkt, struct net_if *iface)
{
	/* The destination is reachable via iface. But since no valid nexthop
	 * is known, net_pkt_lladdr_dst(pkt) cannot be set here.
	 */

	net_pkt_set_orig_iface(pkt, net_pkt_iface(pkt));
	net_pkt_set_iface(pkt, iface);

	net_pkt_set_forwarding(pkt, true);

	net_pkt_lladdr_src(pkt)->addr = net_pkt_lladdr_if(pkt)->addr;
	net_pkt_lladdr_src(pkt)->type = net_pkt_lladdr_if(pkt)->type;
	net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;

	return net_send_data(pkt);
}

void net_route_init(void)
{
	NET_DBG("Allocated %d routing entries (%zu bytes)",
		CONFIG_NET_MAX_ROUTES, sizeof(net_route_entries_pool));

	NET_DBG("Allocated %d nexthop entries (%zu bytes)",
		CONFIG_NET_MAX_NEXTHOPS, sizeof(net_route_nexthop_pool));

	k_work_init_delayable(&route_lifetime_timer, route_lifetime_timeout);
}
