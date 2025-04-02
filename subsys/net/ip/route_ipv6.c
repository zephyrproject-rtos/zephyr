/** @file
 * @brief IPv6 route handling.
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_route);

#include <errno.h>
#include <string.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>

#include "net_private.h"
#include "ipv6.h"
#include "route_ipv6.h"

static struct net_route_entry route_ipv6_entries[CONFIG_NET_IPV6_MAX_ROUTES];
static struct net_route_table route_ipv6_table;

static struct net_addr route_ipv6_addr(const struct net_in6_addr *addr)
{
	struct net_addr ret = {
		.family = NET_AF_INET6,
	};

	net_ipaddr_copy(&ret.in6_addr, addr);

	return ret;
}

static void route_ipv6_notify_add(struct net_route_entry *route)
{
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	struct net_event_ipv6_route info;

	net_ipaddr_copy(&info.addr, &route->addr.in6_addr);
	net_ipaddr_copy(&info.nexthop, &route->nexthop.in6_addr);
	info.prefix_len = route->prefix_len;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTE_ADD, route->iface,
					&info, sizeof(info));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_ROUTE_ADD, route->iface);
#endif
}

static void route_ipv6_notify_del(struct net_route_entry *route)
{
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	struct net_event_ipv6_route info;

	net_ipaddr_copy(&info.addr, &route->addr.in6_addr);
	net_ipaddr_copy(&info.nexthop, &route->nexthop.in6_addr);
	info.prefix_len = route->prefix_len;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTE_DEL, route->iface,
					&info, sizeof(info));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_ROUTE_DEL, route->iface);
#endif
}

static const struct net_route_table_ops route_ipv6_ops = {
	.notify_add = route_ipv6_notify_add,
	.notify_del = route_ipv6_notify_del,
};

struct net_route_entry *net_route_ipv6_lookup(struct net_if *iface,
					      struct net_in6_addr *dst)
{
	struct net_addr addr = route_ipv6_addr(dst);

	return net_route_table_lookup(&route_ipv6_table, iface, &addr);
}

struct net_route_entry *net_route_ipv6_add(struct net_if *iface,
					   struct net_in6_addr *addr,
					   uint8_t prefix_len,
					   struct net_in6_addr *nexthop,
					   uint32_t lifetime,
					   uint8_t preference)
{
	struct net_addr route_addr;
	struct net_addr nexthop_addr;

	NET_ASSERT(addr);
	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	if (net_ipv6_addr_cmp(addr, net_ipv6_unspecified_address())) {
		NET_DBG("Route cannot be towards unspecified address");
		return NULL;
	}

	route_addr = route_ipv6_addr(addr);
	nexthop_addr = route_ipv6_addr(nexthop);

	return net_route_table_add(&route_ipv6_table, iface, &route_addr,
				   prefix_len, &nexthop_addr, lifetime,
				   preference);
}

int net_route_ipv6_del(struct net_route_entry *route)
{
	return net_route_table_del(&route_ipv6_table, route);
}

int net_route_ipv6_del_by_nexthop(struct net_if *iface,
				  struct net_in6_addr *nexthop)
{
	struct net_addr addr = route_ipv6_addr(nexthop);

	return net_route_table_del_by_nexthop(&route_ipv6_table, iface, &addr);
}

void net_route_ipv6_update_lifetime(struct net_route_entry *route,
				    uint32_t lifetime)
{
	net_route_table_update_lifetime(&route_ipv6_table, route, lifetime);
}

struct net_in6_addr *net_route_ipv6_get_nexthop(struct net_route_entry *route)
{
	if (!net_route_is_used(&route_ipv6_table, route) ||
	    route->nexthop.family != NET_AF_INET6) {
		return NULL;
	}

	return &route->nexthop.in6_addr;
}

struct net_nbr *net_route_ipv6_get_nbr(struct net_route_entry *route)
{
	struct net_in6_addr *nexthop = net_route_ipv6_get_nexthop(route);

	if (nexthop == NULL) {
		return NULL;
	}

	return net_ipv6_nbr_lookup(route->iface, nexthop);
}

int net_route_ipv6_foreach(net_route_cb_t cb, void *user_data)
{
	return net_route_table_foreach(&route_ipv6_table, cb, user_data);
}

#if defined(CONFIG_NET_IPV6_ROUTE_MCAST)
static struct net_route_ipv6_entry_mcast
	route_mcast_entries[CONFIG_NET_IPV6_MAX_MCAST_ROUTES];

static int mcast_route_iface_lookup(struct net_route_ipv6_entry_mcast *entry,
				    struct net_if *iface)
{
	ARRAY_FOR_EACH(entry->ifaces, i) {
		if (entry->ifaces[i] == iface) {
			return i;
		}
	}

	return -1;
}

bool net_route_ipv6_mcast_iface_add(struct net_route_ipv6_entry_mcast *entry,
				    struct net_if *iface)
{
	if (!net_if_flag_is_set(iface, NET_IF_FORWARD_MULTICASTS)) {
		return false;
	}

	if (mcast_route_iface_lookup(entry, iface) >= 0) {
		return true;
	}

	ARRAY_FOR_EACH(entry->ifaces, i) {
		if (entry->ifaces[i] == NULL) {
			entry->ifaces[i] = iface;
			return true;
		}
	}

	return false;
}

bool net_route_ipv6_mcast_iface_del(struct net_route_ipv6_entry_mcast *entry,
				    struct net_if *iface)
{
	int pos = mcast_route_iface_lookup(entry, iface);

	if (pos < 0) {
		return false;
	}

	entry->ifaces[pos] = NULL;
	return true;
}

#if defined(CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS)
struct mcast_route_mld_event {
	struct net_in6_addr *addr;
	uint8_t mode;
};

static void send_mld_event(struct net_if *iface, void *user_data)
{
	struct mcast_route_mld_event *event = user_data;

	if (!iface->config.ip.ipv6 ||
	    net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD) ||
	    net_if_ipv6_maddr_lookup(event->addr, &iface)) {
		return;
	}

	net_ipv6_mld_send_single(iface, event->addr, event->mode);
}

static void propagate_mld_event(struct net_route_ipv6_entry_mcast *route,
				bool route_added)
{
	struct mcast_route_mld_event mld_event;

	if (route->prefix_len == 128U) {
		mld_event.addr = &route->group;
		mld_event.mode = route_added ?
			NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE :
			NET_IPV6_MLDv2_CHANGE_TO_INCLUDE_MODE;

		net_if_foreach(send_mld_event, &mld_event);
	}
}
#else
#define propagate_mld_event(...)
#endif

int net_route_ipv6_mcast_forward_packet(struct net_pkt *pkt,
					struct net_ipv6_hdr *hdr)
{
	int ret = 0;
	int err = 0;

	hdr->hop_limit--;

	ARRAY_FOR_EACH_PTR(route_mcast_entries, route) {
		struct net_pkt *pkt_cpy = NULL;

		if (!route->is_used ||
		    !net_ipv6_is_prefix(hdr->dst, route->group.s6_addr,
					route->prefix_len)) {
			continue;
		}

		ARRAY_FOR_EACH(route->ifaces, i) {
			if (route->ifaces[i] == NULL ||
			    pkt->iface == route->ifaces[i] ||
			    !net_if_flag_is_set(route->ifaces[i],
						NET_IF_FORWARD_MULTICASTS)) {
				continue;
			}

			pkt_cpy = net_pkt_shallow_clone(pkt, K_NO_WAIT);
			if (pkt_cpy == NULL) {
				err--;
				continue;
			}

			net_pkt_set_forwarding(pkt_cpy, true);
			net_pkt_set_orig_iface(pkt_cpy, pkt->iface);
			net_pkt_set_iface(pkt_cpy, route->ifaces[i]);

			if (net_send_data(pkt_cpy) >= 0) {
				ret++;
			} else {
				net_pkt_unref(pkt_cpy);
				err--;
			}
		}
	}

	return (err == 0) ? ret : err;
}

int net_route_ipv6_mcast_foreach(net_route_ipv6_mcast_cb_t cb,
				 struct net_in6_addr *skip,
				 void *user_data)
{
	int ret = 0;

	ARRAY_FOR_EACH_PTR(route_mcast_entries, route) {
		if (!route->is_used) {
			continue;
		}

		if (skip != NULL &&
		    net_ipv6_is_prefix(skip->s6_addr, route->group.s6_addr,
				       route->prefix_len)) {
			continue;
		}

		cb(route, user_data);
		ret++;
	}

	return ret;
}

struct net_route_ipv6_entry_mcast *net_route_ipv6_mcast_add(struct net_if *iface,
							    struct net_in6_addr *group,
							    uint8_t prefix_len)
{
	net_ipv6_nbr_lock();

	if (!net_if_flag_is_set(iface, NET_IF_FORWARD_MULTICASTS) ||
	    !net_ipv6_is_addr_mcast(group) ||
	    net_ipv6_is_addr_mcast_iface(group) ||
	    net_ipv6_is_addr_mcast_link(group)) {
		net_ipv6_nbr_unlock();
		return NULL;
	}

	ARRAY_FOR_EACH_PTR(route_mcast_entries, route) {
		if (!route->is_used) {
			net_ipaddr_copy(&route->group, group);

			ARRAY_FOR_EACH(route->ifaces, i) {
				route->ifaces[i] = NULL;
			}

			route->prefix_len = prefix_len;
			route->ifaces[0] = iface;
			route->is_used = true;

			propagate_mld_event(route, true);

			net_ipv6_nbr_unlock();
			return route;
		}
	}

	net_ipv6_nbr_unlock();
	return NULL;
}

bool net_route_ipv6_mcast_del(struct net_route_ipv6_entry_mcast *route)
{
	if (route > &route_mcast_entries[CONFIG_NET_IPV6_MAX_MCAST_ROUTES - 1] ||
	    route < &route_mcast_entries[0]) {
		return false;
	}

	NET_ASSERT(route->is_used,
		   "Multicast route %p to %s was already removed", route,
		   net_sprint_ipv6_addr(&route->group));

	propagate_mld_event(route, false);
	route->is_used = false;

	return true;
}

struct net_route_ipv6_entry_mcast *
net_route_ipv6_mcast_lookup(struct net_in6_addr *group)
{
	ARRAY_FOR_EACH_PTR(route_mcast_entries, route) {
		if (!route->is_used) {
			continue;
		}

		if (net_ipv6_is_prefix(group->s6_addr, route->group.s6_addr,
				       route->prefix_len)) {
			return route;
		}
	}

	return NULL;
}

struct net_route_ipv6_entry_mcast *
net_route_ipv6_mcast_lookup_by_iface(struct net_in6_addr *group,
				     struct net_if *iface)
{
	ARRAY_FOR_EACH_PTR(route_mcast_entries, route) {
		if (!route->is_used) {
			continue;
		}

		ARRAY_FOR_EACH(route->ifaces, i) {
			if (route->ifaces[i] == NULL || route->ifaces[i] != iface) {
				continue;
			}

			if (net_ipv6_is_prefix(group->s6_addr,
					       route->group.s6_addr,
					       route->prefix_len)) {
				return route;
			}
		}
	}

	return NULL;
}
#endif /* CONFIG_NET_IPV6_ROUTE_MCAST */

bool net_route_ipv6_get_info(struct net_if *iface,
			     struct net_in6_addr *dst,
			     struct net_route_entry **route,
			     struct net_in6_addr **nexthop)
{
	struct net_if_router *router;

	if (net_ipv6_nbr_lookup(iface, dst) != NULL) {
		*route = NULL;
		*nexthop = dst;
		return true;
	}

	*route = net_route_ipv6_lookup(iface, dst);
	if (*route != NULL) {
		*nexthop = net_route_ipv6_get_nexthop(*route);
		return *nexthop != NULL;
	}

	if (net_if_ipv6_addr_onlink(NULL, dst)) {
		return false;
	}

	router = net_if_ipv6_router_find_default(NULL, dst);
	if (router == NULL) {
		return false;
	}

	*nexthop = &router->address.in6_addr;
	return true;
}

int net_route_ipv6_packet(struct net_pkt *pkt, struct net_in6_addr *nexthop)
{
	struct net_linkaddr *lladdr = NULL;
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(NULL, nexthop);
	if (nbr == NULL) {
		NET_DBG("Cannot find %s neighbor", net_sprint_ipv6_addr(nexthop));
		return -ENOENT;
	}

	if (net_route_ll_addr_supported(nbr->iface) &&
	    net_route_ll_addr_supported(net_pkt_iface(pkt)) &&
	    net_route_ll_addr_supported(net_pkt_orig_iface(pkt))) {
		lladdr = net_nbr_get_lladdr(nbr->idx);
		if (lladdr == NULL) {
			NET_DBG("Cannot find %s neighbor link layer address.",
				net_sprint_ipv6_addr(nexthop));
			return -ESRCH;
		}

		if (net_pkt_lladdr_src(pkt)->len == 0U) {
			NET_DBG("Link layer source address not set");
			return -EINVAL;
		}

		if (!memcmp(net_pkt_lladdr_src(pkt)->addr, lladdr->addr,
			    lladdr->len)) {
			NET_ERR("Src ll and Dst ll are same");
			return -EINVAL;
		}
	}

	net_pkt_set_forwarding(pkt, true);

	if (net_route_ll_addr_supported(net_pkt_iface(pkt))) {
		(void)net_linkaddr_copy(net_pkt_lladdr_src(pkt),
					net_pkt_lladdr_if(pkt));
	}

	if (lladdr != NULL) {
		(void)net_linkaddr_copy(net_pkt_lladdr_dst(pkt), lladdr);
	}

	net_pkt_set_iface(pkt, nbr->iface);

	return net_send_data(pkt);
}

void net_route_ipv6_init(void)
{
	net_route_table_init(&route_ipv6_table, route_ipv6_entries,
			     ARRAY_SIZE(route_ipv6_entries), &route_ipv6_ops);

#if defined(CONFIG_NET_IPV6_ROUTE_MCAST)
	memset(route_mcast_entries, 0, sizeof(route_mcast_entries));
#endif

	NET_DBG("Allocated %d IPv6 routing entries",
		CONFIG_NET_IPV6_MAX_ROUTES);
}
