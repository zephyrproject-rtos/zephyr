/** @file
 * @brief Route handling.
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_ROUTE)
#define SYS_LOG_DOMAIN "net/route"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <limits.h>
#include <stdint.h>
#include <misc/slist.h>

#include <net/nbuf.h>
#include <net/net_core.h>
#include <net/net_stats.h>
#include <net/net_ip.h>

#include "net_private.h"
#include "ipv6.h"
#include "icmpv6.h"
#include "nbr.h"
#include "route.h"
#include "rpl.h"

#if !defined(NET_ROUTE_EXTRA_DATA_SIZE)
#define NET_ROUTE_EXTRA_DATA_SIZE 0
#endif

/* We keep track of the routes in a separate list so that we can remove
 * the oldest routes (at tail) if needed.
 */
static sys_slist_t routes;

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
	NET_ASSERT_INFO(idx < CONFIG_NET_MAX_NEXTHOPS, "idx %d >= max %d",
			idx, CONFIG_NET_MAX_NEXTHOPS);

	return (struct net_nbr *)((void *)start +
			((sizeof(struct net_nbr) + start->size) * idx));
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

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (nbr->data == (uint8_t *)route) {
			if (!nbr->ref) {
				return NULL;
			}

			return nbr;
		}
	}

	return NULL;
}

#if defined(CONFIG_NET_DEBUG_ROUTE)
void net_routes_print(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		NET_DBG("[%d] %p %d addr %s/%d iface %p idx %d "
			"ll %s",
			i, nbr, nbr->ref,
			net_sprint_ipv6_addr(&net_route_data(nbr)->addr),
			net_route_data(nbr)->prefix_len,
			nbr->iface, nbr->idx,
			nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
			net_sprint_ll_addr(
				net_nbr_get_lladdr(nbr->idx)->addr,
				net_nbr_get_lladdr(nbr->idx)->len));
	}
}
#else
#define net_routes_print(...)
#endif /* CONFIG_NET_DEBUG_ROUTE */

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
	struct net_nbr *nbr = net_ipv6_nbr_lookup(iface, addr);

	NET_ASSERT_INFO(nbr, "Next hop neighbor not found!");
	NET_ASSERT_INFO(nbr->idx != NET_NBR_LLADDR_UNKNOWN,
			"Nexthop %s not in neighbor cache!",
			net_sprint_ipv6_addr(addr));

	net_nbr_ref(nbr);

	NET_DBG("[%d] nbr %p iface %p IPv6 %s",
		nbr->idx, nbr, iface, net_sprint_ipv6_addr(addr));

	return nbr;
}

static int nbr_nexthop_put(struct net_nbr *nbr)
{
	NET_ASSERT(nbr);

	NET_DBG("[%d] nbr %p iface %p", nbr->idx, nbr, nbr->iface);

	net_nbr_unref(nbr);

	return 0;
}

#if defined(CONFIG_NET_DEBUG_ROUTE)
#define net_route_info(str, route, dst)					\
	do {								\
		char out[NET_IPV6_ADDR_LEN];				\
		struct in6_addr *naddr = net_route_get_nexthop(route);	\
									\
		NET_ASSERT_INFO(naddr, "Unknown nexthop address");	\
									\
		snprintk(out, sizeof(out), "%s",			\
			 net_sprint_ipv6_addr(dst));			\
		NET_DBG("%s route to %s via %s (iface %p)", str, out,	\
			net_sprint_ipv6_addr(naddr), route->iface);	\
	} while (0)
#else
#define net_route_info(...)
#endif /* CONFIG_NET_DEBUG_ROUTE */

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
	uint8_t longest_match = 0;
	int i;

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
		    net_is_ipv6_prefix((uint8_t *)dst,
				       (uint8_t *)&route->addr,
				       route->prefix_len)) {
			found = route;
			longest_match = route->prefix_len;
		}
	}

	if (found) {
		net_route_info("Found", found, dst);

		update_route_access(found);
	}

	return found;
}

struct net_route_entry *net_route_add(struct net_if *iface,
				      struct in6_addr *addr,
				      uint8_t prefix_len,
				      struct in6_addr *nexthop)
{
	struct net_linkaddr_storage *nexthop_lladdr;
	struct net_nbr *nbr, *nbr_nexthop, *tmp;
	struct net_route_nexthop *nexthop_route;
	struct net_route_entry *route;

	NET_ASSERT(addr);
	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	if (net_ipv6_addr_cmp(addr, net_ipv6_unspecified_address())) {
		NET_DBG("Route cannot be towards unspecified address");
		return NULL;
	}

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);
	if (!nbr_nexthop) {
		NET_DBG("No such neighbor %s found",
			net_sprint_ipv6_addr(nexthop));
		return NULL;
	}

	nexthop_lladdr = net_nbr_get_lladdr(nbr_nexthop->idx);

	NET_ASSERT(nexthop_lladdr);

	NET_DBG("Nexthop %s lladdr is %s", net_sprint_ipv6_addr(nexthop),
		net_sprint_ll_addr(nexthop_lladdr->addr, nexthop_lladdr->len));

	route = net_route_lookup(iface, addr);
	if (route) {
		/* Update nexthop if not the same */
		struct in6_addr *nexthop_addr;

		nexthop_addr = net_route_get_nexthop(route);
		if (nexthop_addr && net_ipv6_addr_cmp(nexthop, nexthop_addr)) {
			NET_DBG("No changes, return old route %p", route);
			return route;
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
#if defined(CONFIG_NET_DEBUG_ROUTE)
		do {
			char out[NET_IPV6_ADDR_LEN];
			struct in6_addr *tmp;
			struct net_linkaddr_storage *llstorage;

			snprintk(out, sizeof(out), "%s",
				 net_sprint_ipv6_addr(&route->addr));

			tmp = net_route_get_nexthop(route);
			nbr = net_ipv6_nbr_lookup(iface, tmp);
			llstorage = net_nbr_get_lladdr(nbr->idx);

			NET_DBG("Removing the oldest route %s via %s [%s]",
				out, net_sprint_ipv6_addr(tmp),
				net_sprint_ll_addr(llstorage->addr,
						   llstorage->len));
		} while (0);
#endif /* CONFIG_NET_DEBUG_ROUTE */

		net_route_del(route);

		nbr = nbr_new(iface, addr, prefix_len);
		if (!nbr) {
			NET_ERR("Neighbor route alloc failed!");
			return NULL;
		}
	}

	tmp = get_nexthop_route();
	if (!tmp) {
		NET_ERR("No nexthop route available!");
		return NULL;
	}

	nexthop_route = net_nexthop_data(tmp);

	route = net_route_data(nbr);
	route->iface = iface;

	sys_slist_prepend(&routes, &route->node);

	tmp = nbr_nexthop_get(iface, nexthop);

	NET_ASSERT(tmp == nbr_nexthop);

	nexthop_route->nbr = tmp;

	sys_slist_init(&route->nexthop);
	sys_slist_prepend(&route->nexthop, &nexthop_route->node);

	net_route_info("Added", route, addr);

	/* TODO: Send notification that we added a route */

	return route;
}

int net_route_del(struct net_route_entry *route)
{
	struct net_nbr *nbr;
	sys_snode_t *test;

	if (!route) {
		return -EINVAL;
	}

	sys_slist_find_and_remove(&routes, &route->node);

	nbr = net_route_get_nbr(route);
	if (!nbr) {
		return -ENOENT;
	}

	net_route_info("Deleted", route, &route->addr);

	SYS_SLIST_FOR_EACH_NODE(&route->nexthop, test) {
		struct net_route_nexthop *nexthop_route;

		nexthop_route = CONTAINER_OF(test,
					     struct net_route_nexthop,
					     node);

		if (!nexthop_route->nbr) {
			continue;
		}

		nbr_nexthop_put(nexthop_route->nbr);
	}

	nbr_free(nbr);

	/* TODO: Send notification that we deleted a route */

	return 0;
}

int net_route_del_by_nexthop(struct net_if *iface, struct in6_addr *nexthop)
{
	int count = 0, status = 0;
	struct net_nbr *nbr_nexthop;
	sys_snode_t *test;
	int i, ret;

	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_route_entry *route = net_route_data(nbr);

		SYS_SLIST_FOR_EACH_NODE(&route->nexthop, test) {
			struct net_route_nexthop *nexthop_route;

			nexthop_route = CONTAINER_OF(test,
						     struct net_route_nexthop,
						     node);

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
	sys_snode_t *test;
	int i, ret;

	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_route_entry *route = net_route_data(nbr);

		SYS_SLIST_FOR_EACH_NODE(&route->nexthop, test) {
			struct net_route_nexthop *nexthop_route;
			void *extra_data;

			nexthop_route = CONTAINER_OF(test,
						     struct net_route_nexthop,
						     node);

			if (nexthop_route->nbr != nbr_nexthop) {
				continue;
			}

			if (nbr->extra_data_size == 0) {
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

	if (count) {
		return count;
	}

	return status;
}

struct in6_addr *net_route_get_nexthop(struct net_route_entry *route)
{
	sys_snode_t *test;

	NET_ASSERT(route);

	SYS_SLIST_FOR_EACH_NODE(&route->nexthop, test) {
		struct net_route_nexthop *nexthop_route;
		struct in6_addr *addr;

		nexthop_route = CONTAINER_OF(test,
					     struct net_route_nexthop,
					     node);

		NET_ASSERT(nexthop_route->nbr->idx != NET_NBR_LLADDR_UNKNOWN);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			continue;
		}

		addr = net_ipv6_nbr_lookup_by_index(route->iface,
						    nexthop_route->nbr->idx);
		NET_ASSERT(addr);

		return addr;
	}

	return NULL;
}

int net_route_foreach(net_route_cb_t cb, void *user_data)
{
	int i, ret = 0;

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_route_entry *route = net_route_data(nbr);

		cb(route, user_data);

		ret++;
	}

	return ret;
}

#if defined(CONFIG_NET_ROUTE_MCAST)
/*
 * This array contains multicast routing entries.
 */
static
struct net_route_entry_mcast route_mcast_entries[CONFIG_NET_MAX_MCAST_ROUTES];

int net_route_mcast_foreach(net_route_mcast_cb_t cb,
			    struct in6_addr *skip,
			    void *user_data)
{
	int i, ret = 0;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (route->is_used) {
			if (skip && net_ipv6_addr_cmp(skip, &route->group)) {
				continue;
			}

			cb(route, user_data);

			ret++;
		}
	}

	return ret;
}

struct net_route_entry_mcast *net_route_mcast_add(struct net_if *iface,
						  struct in6_addr *group)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (!route->is_used) {
			net_ipaddr_copy(&route->group, group);

			route->iface = iface;
			route->is_used = true;

			return route;
		}
	}

	return NULL;
}

bool net_route_mcast_del(struct net_route_entry_mcast *route)
{
	if (route > &route_mcast_entries[CONFIG_NET_MAX_MCAST_ROUTES - 1] ||
	    route < &route_mcast_entries[0]) {
		return false;
	}

	NET_ASSERT_INFO(route->is_used,
			"Multicast route %d to %s was already removed", i,
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
			if (net_ipv6_addr_cmp(group, &route->group)) {
				return route;
			}
		}
	}

	return NULL;
}
#endif /* CONFIG_NET_ROUTE_MCAST */

void net_route_init(void)
{
	NET_DBG("Allocated %d routing entries (%zu bytes)",
		CONFIG_NET_MAX_ROUTES, sizeof(net_route_entries_pool));

	NET_DBG("Allocated %d nexthop entries (%zu bytes)",
		CONFIG_NET_MAX_NEXTHOPS, sizeof(net_route_nexthop_pool));
}
