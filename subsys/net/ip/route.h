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

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_timeout.h>

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

	/** Route lifetime timer. */
	struct net_timeout lifetime;

	/** IPv6 address/prefix of the route. */
	struct in6_addr addr;

	/** IPv6 address/prefix length. */
	uint8_t prefix_len;

	uint8_t preference : 2;

	/** Is the route valid forever */
	uint8_t is_infinite : 1;
};

/* Route preference values, as defined in RFC 4191 */
#define NET_ROUTE_PREFERENCE_HIGH     0x01
#define NET_ROUTE_PREFERENCE_MEDIUM   0x00
#define NET_ROUTE_PREFERENCE_LOW      0x03 /* -1 if treated as 2 bit signed int */
#define NET_ROUTE_PREFERENCE_RESERVED 0x02

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
 * @param lifetime Route lifetime in seconds.
 * @param preference Route preference.
 *
 * @return Return created route entry, NULL if could not be created.
 */
struct net_route_entry *net_route_add(struct net_if *iface,
				      struct in6_addr *addr,
				      uint8_t prefix_len,
				      struct in6_addr *nexthop,
				      uint32_t lifetime,
				      uint8_t preference);

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
 * @brief Update the route lifetime.
 *
 * @param route Pointer to routing entry.
 * @param lifetime Route lifetime in seconds.
 *
 * @return 0 if ok, <0 if error
 */
void net_route_update_lifetime(struct net_route_entry *route, uint32_t lifetime);

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

#if defined(CONFIG_NET_ROUTE_MCAST)
/**
 * @brief Multicast route entry.
 */
struct net_route_entry_mcast {
	/** Network interfaces for the route. */
	struct net_if *ifaces[CONFIG_NET_MCAST_ROUTE_MAX_IFACES];

	/** Extra routing engine specific data */
	void *data;

	/** IPv6 multicast group of the route. */
	struct in6_addr group;

	/** Routing entry lifetime in seconds. */
	uint32_t lifetime;

	/** Is this entry in use or not */
	bool is_used;

	/** IPv6 multicast group prefix length. */
	uint8_t prefix_len;
};
#else
struct net_route_entry_mcast;
#endif

typedef void (*net_route_mcast_cb_t)(struct net_route_entry_mcast *entry,
				     void *user_data);

/**
 * @brief Forwards a multicast packet by checking the local multicast
 * routing table
 *
 * @param pkt The original received ipv6 packet to forward
 * @param hdr The IPv6 header of the packet
 *
 * @return Number of interfaces which forwarded the packet, or a negative
 * value in case of an error.
 */
int net_route_mcast_forward_packet(struct net_pkt *pkt,
				   const struct net_ipv6_hdr *hdr);

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
 * @param prefix_len Length of the IPv6 group that must match.
 *
 * @return Multicast routing entry.
 */
struct net_route_entry_mcast *net_route_mcast_add(struct net_if *iface,
						  struct in6_addr *group,
						  uint8_t prefix_len);

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
 * @brief Add an interface to multicast routing entry.
 *
 * @param entry Multicast routing entry.
 * @param iface Network interface to be added.
 *
 * @return True if the interface was added or found on the
 *         list, false otherwise.
 */
bool net_route_mcast_iface_add(struct net_route_entry_mcast *entry, struct net_if *iface);

/**
 * @brief Delete an interface from multicast routing entry.
 *
 * @param entry Multicast routing entry.
 * @param iface Network interface to be deleted.
 *
 * @return True if entry was deleted, false otherwise.
 */
bool net_route_mcast_iface_del(struct net_route_entry_mcast *entry, struct net_if *iface);

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

/**
 * @brief Send the network packet to network via the given interface.
 *
 * @param pkt Network packet to send.
 * @param iface The network interface the packet should be sent on.
 *
 * @return 0 if there was no error, <0 if the packet could not be sent.
 */
int net_route_packet_if(struct net_pkt *pkt, struct net_if *iface);

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_NATIVE)
void net_route_init(void);
#else
#define net_route_init(...)
#endif /* CONFIG_NET_ROUTE */

#ifdef __cplusplus
}
#endif

#endif /* __ROUTE_H */
