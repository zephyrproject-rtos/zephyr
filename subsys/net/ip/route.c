/** @file
 * @brief Generic route handling.
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Use highest log level if both IPv4 and IPv6 are defined */
#if defined(CONFIG_NET_IPV4_ROUTE) && defined(CONFIG_NET_IPV6_ROUTE)

#if CONFIG_NET_IPV4_ROUTE_LOG_LEVEL > CONFIG_NET_IPV6_ROUTE_LOG_LEVEL
#define ROUTE_LOG_LEVEL CONFIG_NET_IPV4_ROUTE_LOG_LEVEL
#else
#define ROUTE_LOG_LEVEL CONFIG_NET_IPV6_ROUTE_LOG_LEVEL
#endif

#elif defined(CONFIG_NET_IPV4_ROUTE)
#define ROUTE_LOG_LEVEL CONFIG_NET_IPV4_ROUTE_LOG_LEVEL
#elif defined(CONFIG_NET_IPV6_ROUTE)
#define ROUTE_LOG_LEVEL CONFIG_NET_IPV6_ROUTE_LOG_LEVEL
#else
#define ROUTE_LOG_LEVEL LOG_LEVEL_INF
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_route, ROUTE_LOG_LEVEL);

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>

#include "route.h"

static size_t route_addr_len(net_sa_family_t family)
{
	switch (family) {
	case NET_AF_INET:
		return NET_IPV4_ADDR_SIZE;
	case NET_AF_INET6:
		return NET_IPV6_ADDR_SIZE;
	default:
		return 0U;
	}
}

static uint8_t route_addr_bits(net_sa_family_t family)
{
	return (uint8_t)(route_addr_len(family) * 8U);
}

static const uint8_t *route_addr_bytes(const struct net_addr *addr)
{
	switch (addr->family) {
	case NET_AF_INET:
		return addr->in_addr.s4_addr;
	case NET_AF_INET6:
		return addr->in6_addr.s6_addr;
	default:
		return NULL;
	}
}

static bool route_addr_cmp(const struct net_addr *a, const struct net_addr *b)
{
	size_t len;

	if (a->family != b->family) {
		return false;
	}

	if (a->family == NET_AF_UNSPEC) {
		return true;
	}

	len = route_addr_len(a->family);
	if (len == 0U) {
		return false;
	}

	return memcmp(route_addr_bytes(a), route_addr_bytes(b), len) == 0;
}

static bool route_prefix_match(const struct net_addr *dst,
			       struct net_route_entry *route)
{
	const uint8_t *lhs;
	const uint8_t *rhs;
	uint8_t bytes;
	uint8_t bits;

	if (dst->family != route->addr.family) {
		return false;
	}

	lhs = route_addr_bytes(dst);
	rhs = route_addr_bytes(&route->addr);
	if (lhs == NULL || rhs == NULL) {
		return false;
	}

	bytes = route->prefix_len / 8U;
	bits = route->prefix_len % 8U;

	if (bytes > 0U && memcmp(lhs, rhs, bytes) != 0) {
		return false;
	}

	if (bits > 0U) {
		uint8_t mask = (uint8_t)(0xffU << (8U - bits));

		return (lhs[bytes] & mask) == (rhs[bytes] & mask);
	}

	return true;
}

static bool route_preference_is_lower(uint8_t old, uint8_t new)
{
	if (new == NET_ROUTE_PREFERENCE_RESERVED || (new & 0xfcU) != 0U) {
		return true;
	}

	old = (old + 1U) & 0x3U;
	new = (new + 1U) & 0x3U;

	return new < old;
}

static void route_copy_addr(struct net_addr *dst,
			    const struct net_addr *src)
{
	if (src == NULL) {
		memset(dst, 0, sizeof(*dst));
		dst->family = NET_AF_UNSPEC;
		return;
	}

	memcpy(dst, src, sizeof(*dst));
}

static bool route_apply_prefix(struct net_addr *addr, uint8_t prefix_len)
{
	size_t len = route_addr_len(addr->family);
	uint8_t full_bytes;
	uint8_t rem_bits;
	uint8_t *bytes;

	if (addr->family == NET_AF_UNSPEC) {
		return prefix_len == 0U;
	}

	if (len == 0U || prefix_len > route_addr_bits(addr->family)) {
		return false;
	}

	full_bytes = prefix_len / 8U;
	rem_bits = prefix_len % 8U;
	bytes = (uint8_t *)route_addr_bytes(addr);

	if (rem_bits > 0U && full_bytes < len) {
		bytes[full_bytes] &= (uint8_t)(0xffU << (8U - rem_bits));
		full_bytes++;
	}

	if (full_bytes < len) {
		memset(&bytes[full_bytes], 0, len - full_bytes);
	}

	return true;
}

static bool route_entry_in_table(struct net_route_table *table,
				 struct net_route_entry *route)
{
	return route >= table->entries &&
	       route < (table->entries + table->entry_count);
}

static struct net_route_entry *route_find_exact_locked(struct net_route_table *table,
						       struct net_if *iface,
						       const struct net_addr *addr,
						       uint8_t prefix_len)
{
	for (size_t i = 0; i < table->entry_count; i++) {
		struct net_route_entry *route = &table->entries[i];

		if (!route->is_used) {
			continue;
		}

		if (iface != NULL && route->iface != iface) {
			continue;
		}

		if (route->prefix_len != prefix_len) {
			continue;
		}

		if (route_addr_cmp(&route->addr, addr)) {
			return route;
		}
	}

	return NULL;
}

static void route_update_access_locked(struct net_route_table *table,
				       struct net_route_entry *route)
{
	(void)sys_slist_find_and_remove(&table->routes, &route->node);
	sys_slist_prepend(&table->routes, &route->node);
}

static void route_update_lifetime_locked(struct net_route_table *table,
					 struct net_route_entry *route,
					 uint32_t lifetime)
{
	if (lifetime == NET_ROUTE_INFINITE_LIFETIME) {
		route->is_infinite = true;
		(void)sys_slist_find_and_remove(&table->active_route_lifetime_timers,
						&route->lifetime.node);
		return;
	}

	route->is_infinite = false;

	net_timeout_set(&route->lifetime, lifetime, k_uptime_get_32());

	(void)sys_slist_find_and_remove(&table->active_route_lifetime_timers,
					&route->lifetime.node);
	sys_slist_append(&table->active_route_lifetime_timers,
			 &route->lifetime.node);
	k_work_reschedule(&table->route_lifetime_timer, K_NO_WAIT);
}

static int route_delete_locked(struct net_route_table *table,
			       struct net_route_entry *route)
{
	if (!route_entry_in_table(table, route) || !route->is_used) {
		return -ENOENT;
	}

	if (table->ops != NULL && table->ops->notify_del != NULL) {
		table->ops->notify_del(route);
	}

	if (!route->is_infinite) {
		(void)sys_slist_find_and_remove(&table->active_route_lifetime_timers,
						&route->lifetime.node);
		if (sys_slist_is_empty(&table->active_route_lifetime_timers)) {
			k_work_cancel_delayable(&table->route_lifetime_timer);
		}
	}

	(void)sys_slist_find_and_remove(&table->routes, &route->node);
	memset(route, 0, sizeof(*route));

	return 0;
}

static void route_lifetime_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct net_route_table *table =
		CONTAINER_OF(dwork, struct net_route_table, route_lifetime_timer);
	uint32_t next_update = UINT32_MAX;
	uint32_t current_time = k_uptime_get_32();
	struct net_route_entry *current;
	struct net_route_entry *next;

	k_mutex_lock(&table->lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&table->active_route_lifetime_timers,
					  current, next, lifetime.node) {
		struct net_timeout *timeout = &current->lifetime;
		uint32_t this_update = net_timeout_evaluate(timeout, current_time);

		if (this_update == 0U) {
			(void)route_delete_locked(table, current);
			continue;
		}

		if (this_update < next_update) {
			next_update = this_update;
		}
	}

	if (next_update != UINT32_MAX) {
		k_work_reschedule(&table->route_lifetime_timer, K_MSEC(next_update));
	}

	k_mutex_unlock(&table->lock);
}

void net_route_table_init(struct net_route_table *table,
			  struct net_route_entry *entries,
			  size_t entry_count,
			  const struct net_route_table_ops *ops)
{
	memset(entries, 0, sizeof(*entries) * entry_count);
	memset(table, 0, sizeof(*table));

	k_mutex_init(&table->lock);
	k_work_init_delayable(&table->route_lifetime_timer, route_lifetime_timeout);
	table->entries = entries;
	table->entry_count = entry_count;
	table->ops = ops;
}

bool net_route_is_used(struct net_route_table *table,
		       struct net_route_entry *route)
{
	bool ret;

	k_mutex_lock(&table->lock, K_FOREVER);
	ret = route_entry_in_table(table, route) && route->is_used;
	k_mutex_unlock(&table->lock);

	return ret;
}

struct net_route_entry *net_route_table_lookup(struct net_route_table *table,
					       struct net_if *iface,
					       const struct net_addr *dst)
{
	struct net_route_entry *found = NULL;
	uint8_t longest_match = 0U;

	k_mutex_lock(&table->lock, K_FOREVER);

	for (size_t i = 0; i < table->entry_count; i++) {
		struct net_route_entry *route = &table->entries[i];

		if (!route->is_used) {
			continue;
		}

		if (iface != NULL && route->iface != iface) {
			continue;
		}

		if (route->prefix_len < longest_match) {
			continue;
		}

		if (!route_prefix_match(dst, route)) {
			continue;
		}

		found = route;
		longest_match = route->prefix_len;
	}

	if (found != NULL) {
		route_update_access_locked(table, found);
	}

	k_mutex_unlock(&table->lock);

	return found;
}

struct net_route_entry *net_route_table_add(struct net_route_table *table,
					    struct net_if *iface,
					    const struct net_addr *addr,
					    uint8_t prefix_len,
					    const struct net_addr *nexthop,
					    uint32_t lifetime,
					    uint8_t preference)
{
	struct net_addr normalized_addr;
	struct net_addr normalized_nexthop;
	struct net_route_entry *route;

	k_mutex_lock(&table->lock, K_FOREVER);

	route_copy_addr(&normalized_addr, addr);
	route_copy_addr(&normalized_nexthop, nexthop);
	if (!route_apply_prefix(&normalized_addr, prefix_len)) {
		k_mutex_unlock(&table->lock);
		return NULL;
	}

	route = route_find_exact_locked(table, iface, &normalized_addr, prefix_len);
	if (route != NULL) {
		if (route_addr_cmp(&route->nexthop, &normalized_nexthop)) {
			route->preference = preference;
			route_update_lifetime_locked(table, route, lifetime);
			route_update_access_locked(table, route);
			k_mutex_unlock(&table->lock);
			return route;
		}

		if (route_preference_is_lower(route->preference, preference)) {
			k_mutex_unlock(&table->lock);
			return NULL;
		}

		(void)route_delete_locked(table, route);
	}

	route = NULL;

	for (size_t i = 0; i < table->entry_count; i++) {
		if (!table->entries[i].is_used) {
			route = &table->entries[i];
			break;
		}
	}

	if (route == NULL) {
		sys_snode_t *last = sys_slist_peek_tail(&table->routes);

		if (last == NULL) {
			k_mutex_unlock(&table->lock);
			return NULL;
		}

		route = CONTAINER_OF(last, struct net_route_entry, node);
		(void)route_delete_locked(table, route);
	}

	memset(route, 0, sizeof(*route));

	route->iface = iface;
	route->prefix_len = prefix_len;
	route->preference = preference;
	route->is_used = true;

	route_copy_addr(&route->addr, &normalized_addr);
	route_copy_addr(&route->nexthop, &normalized_nexthop);

	route_update_lifetime_locked(table, route, lifetime);
	sys_slist_prepend(&table->routes, &route->node);

	if (table->ops != NULL && table->ops->notify_add != NULL) {
		table->ops->notify_add(route);
	}

	k_mutex_unlock(&table->lock);

	return route;
}

int net_route_table_del(struct net_route_table *table,
			struct net_route_entry *route)
{
	int ret;

	k_mutex_lock(&table->lock, K_FOREVER);
	ret = route_delete_locked(table, route);
	k_mutex_unlock(&table->lock);

	return ret;
}

int net_route_table_del_by_nexthop(struct net_route_table *table,
				   struct net_if *iface,
				   const struct net_addr *nexthop)
{
	int count = 0;
	int status = -ENOENT;

	k_mutex_lock(&table->lock, K_FOREVER);

	for (size_t i = 0; i < table->entry_count; i++) {
		struct net_route_entry *route = &table->entries[i];
		int ret;

		if (!route->is_used) {
			continue;
		}

		if (iface != NULL && route->iface != iface) {
			continue;
		}

		if (!route_addr_cmp(&route->nexthop, nexthop)) {
			continue;
		}

		ret = route_delete_locked(table, route);
		if (ret == 0) {
			count++;
		} else {
			status = ret;
		}
	}

	k_mutex_unlock(&table->lock);

	if (count > 0) {
		return count;
	}

	return status;
}

void net_route_table_update_lifetime(struct net_route_table *table,
				     struct net_route_entry *route,
				     uint32_t lifetime)
{
	k_mutex_lock(&table->lock, K_FOREVER);

	if (route_entry_in_table(table, route) && route->is_used) {
		route_update_lifetime_locked(table, route, lifetime);
	}

	k_mutex_unlock(&table->lock);
}

int net_route_table_foreach(struct net_route_table *table,
			    net_route_cb_t cb,
			    void *user_data)
{
	int count = 0;

	k_mutex_lock(&table->lock, K_FOREVER);

	for (size_t i = 0; i < table->entry_count; i++) {
		struct net_route_entry *route = &table->entries[i];

		if (!route->is_used) {
			continue;
		}

		cb(route, user_data);
		count++;
	}

	k_mutex_unlock(&table->lock);

	return count;
}

bool net_route_has_nexthop(const struct net_route_entry *route)
{
	return route != NULL && route->nexthop.family != NET_AF_UNSPEC;
}

bool net_route_ll_addr_supported(struct net_if *iface)
{
#if defined(CONFIG_NET_L2_DUMMY)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return false;
	}
#endif
#if defined(CONFIG_NET_L2_PPP)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(PPP)) {
		return false;
	}
#endif
#if defined(CONFIG_NET_L2_OPENTHREAD)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(OPENTHREAD)) {
		return false;
	}
#endif

	return true;
}

int net_route_packet_if(struct net_pkt *pkt, struct net_if *iface)
{
	net_pkt_set_orig_iface(pkt, net_pkt_iface(pkt));
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_forwarding(pkt, true);

	if (net_route_ll_addr_supported(iface)) {
		memcpy(net_pkt_lladdr_src(pkt)->addr,
		       net_pkt_lladdr_if(pkt)->addr,
		       net_pkt_lladdr_if(pkt)->len);
		net_pkt_lladdr_src(pkt)->type = net_pkt_lladdr_if(pkt)->type;
		net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;
	}

	return net_send_data(pkt);
}
