/** @file
 * @brief Generic route definitions used by IPv4 and IPv6.
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ROUTE_H
#define __ROUTE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_timeout.h>

struct net_if;
struct net_nbr;
struct net_pkt;

/* Route preference values, as defined in RFC 4191 */
#define NET_ROUTE_PREFERENCE_HIGH     0x01
#define NET_ROUTE_PREFERENCE_MEDIUM   0x00
#define NET_ROUTE_PREFERENCE_LOW      0x03
#define NET_ROUTE_PREFERENCE_RESERVED 0x02

#define NET_ROUTE_INFINITE_LIFETIME UINT32_MAX

struct net_route_entry {
	/* Routes are kept in a separate list so the oldest one can be evicted. */
	sys_snode_t node;

	/* Interface used for this route. */
	struct net_if *iface;

	/* Lifetime timer state for this route. */
	struct net_timeout lifetime;

	/* Destination prefix. */
	struct net_addr addr;

	/* Next hop for the destination prefix. */
	struct net_addr nexthop;

	/* Prefix length for addr. */
	uint8_t prefix_len;

	uint8_t preference : 2;
	uint8_t is_infinite : 1;
	uint8_t is_used : 1;
};

typedef void (*net_route_cb_t)(struct net_route_entry *entry,
			       void *user_data);

struct net_route_table_ops {
	void (*notify_add)(struct net_route_entry *entry);
	void (*notify_del)(struct net_route_entry *entry);
};

struct net_route_table {
	struct k_mutex lock;
	sys_slist_t routes;
	sys_slist_t active_route_lifetime_timers;
	struct k_work_delayable route_lifetime_timer;
	struct net_route_entry *entries;
	size_t entry_count;
	const struct net_route_table_ops *ops;
};

void net_route_table_init(struct net_route_table *table,
			  struct net_route_entry *entries,
			  size_t entry_count,
			  const struct net_route_table_ops *ops);

bool net_route_is_used(struct net_route_table *table,
		       struct net_route_entry *route);

struct net_route_entry *net_route_table_lookup(struct net_route_table *table,
					       struct net_if *iface,
					       const struct net_addr *dst);

struct net_route_entry *net_route_table_add(struct net_route_table *table,
					    struct net_if *iface,
					    const struct net_addr *addr,
					    uint8_t prefix_len,
					    const struct net_addr *nexthop,
					    uint32_t lifetime,
					    uint8_t preference);

int net_route_table_del(struct net_route_table *table,
			struct net_route_entry *route);

int net_route_table_del_by_nexthop(struct net_route_table *table,
				   struct net_if *iface,
				   const struct net_addr *nexthop);

void net_route_table_update_lifetime(struct net_route_table *table,
				     struct net_route_entry *route,
				     uint32_t lifetime);

int net_route_table_foreach(struct net_route_table *table,
			    net_route_cb_t cb,
			    void *user_data);

bool net_route_has_nexthop(const struct net_route_entry *route);

bool net_route_ll_addr_supported(struct net_if *iface);

int net_route_packet_if(struct net_pkt *pkt, struct net_if *iface);

#endif /* __ROUTE_H */
