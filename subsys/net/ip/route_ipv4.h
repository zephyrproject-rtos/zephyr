/** @file
 * @brief IPv4 route handler
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ROUTE_IPV4_H
#define __ROUTE_IPV4_H

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <zephyr/net/net_ip.h>

#include "route.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lookup route to a given destination.
 *
 * @param iface Network interface. If NULL, then check against all interfaces.
 * @param dst Destination IPv4 address.
 *
 * @return Return route entry related to a given destination address, NULL
 * if not found.
 */
#if defined(CONFIG_NET_NATIVE)
struct net_route_entry *net_route_ipv4_lookup(struct net_if *iface,
					      struct net_in_addr *dst);
#else
static inline struct net_route_entry *net_route_ipv4_lookup(struct net_if *iface,
							    struct net_in_addr *dst)
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
 * @param addr IPv4 address.
 * @param mask_len IPv4 netmask length
 * @param nexthop IPv4 address of the Next hop device, or NULL for an
 * on-link route.
 * @param lifetime Route lifetime in seconds.
 * @param preference Route preference.
 *
 * @return Return created route entry, NULL if could not be created.
 */
struct net_route_entry *net_route_ipv4_add(struct net_if *iface,
					   struct net_in_addr *addr,
					   uint8_t mask_len,
					   struct net_in_addr *nexthop,
					   uint32_t lifetime,
					   uint8_t preference);

/**
 * @brief Delete a route from routing table.
 *
 * @param entry Existing route entry.
 *
 * @return 0 if ok, <0 if error
 */
int net_route_ipv4_del(struct net_route_entry *entry);

/**
 * @brief Delete a route from routing table by nexthop.
 *
 * @param iface Network interface to use.
 * @param nexthop IPv4 address of the nexthop device.
 *
 * @return number of routes deleted, <0 if error
 */
int net_route_ipv4_del_by_nexthop(struct net_if *iface,
				  struct net_in_addr *nexthop);

/**
 * @brief Update the route lifetime.
 *
 * @param route Pointer to routing entry.
 * @param lifetime Route lifetime in seconds.
 *
 * @return 0 if ok, <0 if error
 */
void net_route_ipv4_update_lifetime(struct net_route_entry *route,
				    uint32_t lifetime);

/**
 * @brief Get nexthop IPv4 address tied to this route.
 *
 * There can be multiple routes to a host but this function
 * will only return the first one in this version.
 *
 * @param entry Route entry to use.
 *
 * @return IPv4 address of the nexthop, NULL if not found.
 */
struct net_in_addr *net_route_ipv4_get_nexthop(struct net_route_entry *entry);

/**
 * @brief Go through all the routing entries and call callback
 * for each entry that is in use.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 *
 * @return Total number of routing entries found.
 */
int net_route_ipv4_foreach(net_route_cb_t cb, void *user_data);

/**
 * @brief Return a route to destination via some intermediate host.
 *
 * @param iface Network interface
 * @param dst Destination IPv4 address
 * @param route Route entry to destination is returned.
 * @param nexthop Next hop neighbor IPv4 address is returned.
 *
 * @return True if there is a route to the destination, False otherwise
 */
bool net_route_ipv4_get_info(struct net_if *iface,
			     struct net_in_addr *dst,
			     struct net_route_entry **route,
			     struct net_in_addr **nexthop);

/**
 * @brief Send the network packet to network via some intermediate host.
 *
 * @param pkt Network packet to send.
 * @param nexthop Next hop neighbor IPv4 address.
 *
 * @return 0 if there was no error, <0 if the packet could not be sent.
 */
int net_route_ipv4_packet(struct net_pkt *pkt, struct net_in_addr *nexthop);

/**
 * @brief Send the network packet to network via the given interface.
 *
 * @param pkt Network packet to send.
 * @param iface The network interface the packet should be sent on.
 *
 * @return 0 if there was no error, <0 if the packet could not be sent.
 */
int net_route_packet_if(struct net_pkt *pkt, struct net_if *iface);

#if defined(CONFIG_NET_IPV4_ROUTE) && defined(CONFIG_NET_NATIVE)
void net_route_ipv4_init(void);
#else
#define net_route_ipv4_init(...)
#endif /* CONFIG_NET_IPV4_ROUTE */

#ifdef __cplusplus
}
#endif

#endif /* __ROUTE_IPV4_H */
