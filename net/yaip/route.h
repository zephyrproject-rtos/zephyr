/** @file
 * @brief Route handler
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ROUTE_H
#define __ROUTE_H

#include <nanokernel.h>
#include <misc/slist.h>

#include <net/net_ip.h>

#include "nbr.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_ROUTE)

/**
 * @brief Next hop entry for a given route.
 */
struct net_route_nexthop {
	/** Pointer to nexthop that has same route to a specific
	 * neighbor.
	 */
	sys_snode_t node;

	/** Next hop neighbor */
	struct net_nbr *nbr;
};

/**
 * @brief Route entry to a specific neighbor.
 */
struct net_route_entry {
	/** Node information. The routes are also in separate list in
	 * order to keep track which one of them is the oldest so that
	 * we can remove it if we run out of available routes.
	 * The oldest one is the last entry in the list.
	 */
	sys_snode_t node;

	/** List of neighbors that the routes go through. */
	sys_slist_t nexthop;

	/** Network interface for the route. */
	struct net_if *iface;

	/** IPv6 address/prefix of the route. */
	struct in6_addr addr;

	/** IPv6 address/prefix length. */
	uint8_t prefix_len;
};

/**
 * @brief Lookup route to a given destination.
 *
 * @param iface Network interface. If NULL, then check against all interfaces.
 * @param dst Destination IPv6 address.
 *
 * @return Return route entry related to a given destination address, NULL
 * if not found.
 */
struct net_route_entry *net_route_lookup(struct net_if *iface,
					 struct in6_addr *dst);

/**
 * @brief Add a route to routing table.
 *
 * @param iface Network interface that this route is tied to.
 * @param addr IPv6 address.
 * @param prefix_len Length of the IPv6 address/prefix.
 * @param nexthop IPv6 address of the Next hop device.
 *
 * @return Return created route entry, NULL if could not be created.
 */
struct net_route_entry *net_route_add(struct net_if *iface,
				      struct in6_addr *addr,
				      uint8_t prefix_len,
				      struct in6_addr *nexthop);

/**
 * @brief Delete a route from routing table.
 *
 * @param entry Existing route entry.
 *
 * @return 0 if ok, <0 if error
 */
int net_route_del(struct net_route_entry *entry);

/**
 * @brief Delete a route from routing table by nexthop.
 *
 * @param iface Network interface to use.
 * @param nexthop IPv6 address of the nexthop device.
 *
 * @return number of routes deleted, <0 if error
 */
int net_route_del_by_nexthop(struct net_if *iface,
			     struct in6_addr *nexthop);

/**
 * @brief Get nexthop IPv6 address tied to this route.
 *
 * There can be multiple routes to a host but this function
 * will only return the first one in this version.
 *
 * @param entry Route entry to use.
 *
 * @return IPv6 address of the nexthop, NULL if not found.
 */
struct in6_addr *net_route_get_nexthop(struct net_route_entry *entry);

void net_route_init(void);
#else
#define net_route_init(...)
#endif /* CONFIG_NET_ROUTE */

#ifdef __cplusplus
}
#endif

#endif /* __ROUTE_H */
