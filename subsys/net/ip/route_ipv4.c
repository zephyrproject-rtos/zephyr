/** @file
 * @brief IPv4 route handling.
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
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>

#include "net_private.h"
#include "route_ipv4.h"

static struct net_route_entry route_ipv4_entries[CONFIG_NET_IPV4_MAX_ROUTES];
static struct net_route_table route_ipv4_table;

static struct net_addr route_ipv4_addr(const struct net_in_addr *addr)
{
	struct net_addr ret = {
		.family = NET_AF_INET,
	};

	net_ipaddr_copy(&ret.in_addr, addr);

	return ret;
}

static void route_ipv4_notify_add(struct net_route_entry *route)
{
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	struct net_event_ipv4_route info;

	net_ipaddr_copy(&info.addr, &route->addr.in_addr);
	net_ipaddr_copy(&info.nexthop, &route->nexthop.in_addr);
	info.prefix_len = route->prefix_len;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ROUTE_ADD, route->iface,
					&info, sizeof(info));
#else
	net_mgmt_event_notify(NET_EVENT_IPV4_ROUTE_ADD, route->iface);
#endif
}

static void route_ipv4_notify_del(struct net_route_entry *route)
{
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	struct net_event_ipv4_route info;

	net_ipaddr_copy(&info.addr, &route->addr.in_addr);
	net_ipaddr_copy(&info.nexthop, &route->nexthop.in_addr);
	info.prefix_len = route->prefix_len;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ROUTE_DEL, route->iface,
					&info, sizeof(info));
#else
	net_mgmt_event_notify(NET_EVENT_IPV4_ROUTE_DEL, route->iface);
#endif
}

static const struct net_route_table_ops route_ipv4_ops = {
	.notify_add = route_ipv4_notify_add,
	.notify_del = route_ipv4_notify_del,
};

struct net_route_entry *net_route_ipv4_lookup(struct net_if *iface,
					      struct net_in_addr *dst)
{
	struct net_addr addr = route_ipv4_addr(dst);

	return net_route_table_lookup(&route_ipv4_table, iface, &addr);
}

struct net_route_entry *net_route_ipv4_add(struct net_if *iface,
					   struct net_in_addr *addr,
					   uint8_t mask_len,
					   struct net_in_addr *nexthop,
					   uint32_t lifetime,
					   uint8_t preference)
{
	struct net_addr route_addr;
	struct net_addr nexthop_addr;

	NET_ASSERT(addr);
	NET_ASSERT(iface);

	if (net_ipv4_addr_cmp(addr, net_ipv4_unspecified_address())) {
		NET_DBG("Route cannot be towards unspecified address");
		return NULL;
	}

	route_addr = route_ipv4_addr(addr);
	if (nexthop != NULL) {
		nexthop_addr = route_ipv4_addr(nexthop);
	}

	return net_route_table_add(&route_ipv4_table, iface, &route_addr,
				   mask_len, nexthop != NULL ? &nexthop_addr : NULL,
				   lifetime,
				   preference);
}

int net_route_ipv4_del(struct net_route_entry *route)
{
	return net_route_table_del(&route_ipv4_table, route);
}

int net_route_ipv4_del_by_nexthop(struct net_if *iface,
				  struct net_in_addr *nexthop)
{
	struct net_addr addr = route_ipv4_addr(nexthop);

	return net_route_table_del_by_nexthop(&route_ipv4_table, iface, &addr);
}

void net_route_ipv4_update_lifetime(struct net_route_entry *route,
				    uint32_t lifetime)
{
	net_route_table_update_lifetime(&route_ipv4_table, route, lifetime);
}

struct net_in_addr *net_route_ipv4_get_nexthop(struct net_route_entry *route)
{
	if (!net_route_is_used(&route_ipv4_table, route) ||
	    !net_route_has_nexthop(route) ||
	    route->nexthop.family != NET_AF_INET) {
		return NULL;
	}

	return &route->nexthop.in_addr;
}

int net_route_ipv4_foreach(net_route_cb_t cb, void *user_data)
{
	return net_route_table_foreach(&route_ipv4_table, cb, user_data);
}

bool net_route_ipv4_get_info(struct net_if *iface,
			     struct net_in_addr *dst,
			     struct net_route_entry **route,
			     struct net_in_addr **nexthop)
{
	struct net_if_router *router;

	*route = net_route_ipv4_lookup(iface, dst);
	if (*route != NULL) {
		*nexthop = net_route_ipv4_get_nexthop(*route);
		if (*nexthop != NULL) {
			return true;
		}

		*nexthop = dst;
		return true;
	}

	if (net_if_ipv4_addr_onlink(NULL, dst)) {
		return false;
	}

	router = net_if_ipv4_router_find_default(NULL, dst);
	if (router == NULL) {
		return false;
	}

	*nexthop = &router->address.in_addr;
	return true;
}

int net_route_ipv4_packet(struct net_pkt *pkt, struct net_in_addr *nexthop)
{
	if (nexthop == NULL) {
		return -EINVAL;
	}

	net_pkt_set_forwarding(pkt, true);
	net_pkt_set_ipv4_ll_resolve_addr(pkt, nexthop);

	if (net_route_ll_addr_supported(net_pkt_iface(pkt))) {
		(void)net_linkaddr_copy(net_pkt_lladdr_src(pkt),
					net_pkt_lladdr_if(pkt));
	}

	(void)net_linkaddr_clear(net_pkt_lladdr_dst(pkt));

	return net_send_data(pkt);
}

void net_route_ipv4_init(void)
{
	net_route_table_init(&route_ipv4_table, route_ipv4_entries,
			     ARRAY_SIZE(route_ipv4_entries), &route_ipv4_ops);

	NET_DBG("Allocated %d IPv4 routing entries",
		CONFIG_NET_IPV4_MAX_ROUTES);
}
