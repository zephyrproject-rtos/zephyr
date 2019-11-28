/** @file
 * @brief Route handler
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ROUTE_H
#define __ROUTE_H

#include <kernel.h>
#include <sys/slist.h>

#include <net/net_ip.h>

#include "nbr.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	u8_t prefix_len;
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
#if defined(CONFIG_NET_NATIVE)
struct net_route_entry *net_route_lookup(struct net_if *iface,
					 struct in6_addr *dst);
#else
static inline struct net_route_entry *net_route_lookup(struct net_if *iface,
						       struct in6_addr *dst)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(dst);

	return NULL;
}
#endif

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
				      u8_t prefix_len,
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
 * @brief Delete a route from routing table by nexthop if the routing engine
 * specific data matches.
 *
 * @detail The routing engine specific data could be the RPL data.
 *
 * @param iface Network interface to use.
 * @param nexthop IPv6 address of the nexthop device.
 * @param data Routing engine specific data.
 *
 * @return number of routes deleted, <0 if error
 */
int net_route_del_by_nexthop_data(struct net_if *iface,
				  struct in6_addr *nexthop,
				  void *data);

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

/**
 * @brief Get generic neighbor entry from route entry.
 *
 * @param route Pointer to routing entry.
 *
 * @return Generic neighbor entry.
 */
struct net_nbr *net_route_get_nbr(struct net_route_entry *route);

typedef void (*net_route_cb_t)(struct net_route_entry *entry,
			       void *user_data);

/**
 * @brief Go through all the routing entries and call callback
 * for each entry that is in use.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 *
 * @return Total number of routing entries found.
 */
int net_route_foreach(net_route_cb_t cb, void *user_data);

/**
 * @brief Multicast route entry.
 */
struct net_route_entry_mcast {
	/** Network interface for the route. */
	struct net_if *iface;

	/** Extra routing engine specific data */
	void *data;

	/** IPv6 multicast group of the route. */
	struct in6_addr group;

	/** Routing entry lifetime in seconds. */
	u32_t lifetime;

	/** Is this entry in user or not */
	bool is_used;
};

typedef void (*net_route_mcast_cb_t)(struct net_route_entry_mcast *entry,
				     void *user_data);

/**
 * @brief Go through all the multicast routing entries and call callback
 * for each entry that is in use.
 *
 * @param cb User supplied callback function to call.
 * @param skip Do not call callback for this address.
 * @param user_data User specified data.
 *
 * @return Total number of multicast routing entries that are in use.
 */
int net_route_mcast_foreach(net_route_mcast_cb_t cb,
			    struct in6_addr *skip,
			    void *user_data);

/**
 * @brief Add a multicast routing entry.
 *
 * @param iface Network interface to use.
 * @param group IPv6 multicast address.
 *
 * @return Multicast routing entry.
 */
struct net_route_entry_mcast *net_route_mcast_add(struct net_if *iface,
						  struct in6_addr *group);

/**
 * @brief Delete a multicast routing entry.
 *
 * @param route Multicast routing entry.
 *
 * @return True if entry was deleted, false otherwise.
 */
bool net_route_mcast_del(struct net_route_entry_mcast *route);

/**
 * @brief Lookup a multicast routing entry.
 *
 * @param group IPv6 multicast group address
 *
 * @return Routing entry corresponding this multicast group.
 */
struct net_route_entry_mcast *
net_route_mcast_lookup(struct in6_addr *group);

/**
 * @brief Return a route to destination via some intermediate host.
 *
 * @param iface Network interface
 * @param dst Destination IPv6 address
 * @param route Route entry to destination is returned.
 * @param nexthop Next hop neighbor IPv6 address is returned.
 *
 * @return True if there is a route to the destination, False otherwise
 */
bool net_route_get_info(struct net_if *iface,
			struct in6_addr *dst,
			struct net_route_entry **route,
			struct in6_addr **nexthop);

/**
 * @brief Send the network packet to network via some intermediate host.
 *
 * @param pkt Network packet to send.
 * @param nexthop Next hop neighbor IPv6 address.
 *
 * @return 0 if there was no error, <0 if the packet could not be sent.
 */
int net_route_packet(struct net_pkt *pkt, struct in6_addr *nexthop);

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_NATIVE)
void net_route_init(void);
#else
#define net_route_init(...)
#endif /* CONFIG_NET_ROUTE */

#ifdef __cplusplus
}
#endif

#endif /* __ROUTE_H */
