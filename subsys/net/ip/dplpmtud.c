/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IPv4/6 DPLPMTUD related functions
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_pmtu, CONFIG_NET_PMTU_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include "dplpmtud_internal.h"

#if defined(CONFIG_NET_IPV4_PMTU_DPLPMTUD)
#define NET_IPV4_DPLPMTUD_ENTRIES CONFIG_NET_IPV4_PMTU_DESTINATION_CACHE_ENTRIES
#else
#define NET_IPV4_DPLPMTUD_ENTRIES 0
#endif

#if defined(CONFIG_NET_IPV6_PMTU_DPLPMTUD)
#define NET_IPV6_DPLPMTUD_ENTRIES CONFIG_NET_IPV6_PMTU_DESTINATION_CACHE_ENTRIES
#else
#define NET_IPV6_DPLPMTUD_ENTRIES 0
#endif

#define NET_DPLPMTUD_MAX_ENTRIES (NET_IPV4_DPLPMTUD_ENTRIES + NET_IPV6_DPLPMTUD_ENTRIES)

static struct net_dplpmtud_entry dplpmtud_entries[NET_DPLPMTUD_MAX_ENTRIES];
static K_MUTEX_DEFINE(lock);

static inline uint16_t clamp_plpmtu(uint16_t base_plpmtu, uint16_t max_plpmtu)
{
	if (max_plpmtu < base_plpmtu) {
		return base_plpmtu;
	}

	return max_plpmtu;
}

static inline uint16_t next_probe_size(uint16_t low, uint16_t high)
{
	uint16_t delta;

	if (high <= low) {
		return 0U;
	}

	delta = high - low;

	return low + DIV_ROUND_UP(delta, 2);
}

static bool entry_matches_dst(const struct net_dplpmtud_entry *entry,
			      const struct net_sockaddr *dst)
{
	if (!entry->in_use || entry->dst.family != dst->sa_family) {
		return false;
	}

	if (dst->sa_family == NET_AF_INET) {
		return net_ipv4_addr_cmp(&entry->dst.in_addr, &net_sin(dst)->sin_addr);
	}

	if (dst->sa_family == NET_AF_INET6) {
		return net_ipv6_addr_cmp(&entry->dst.in6_addr, &net_sin6(dst)->sin6_addr);
	}

	return false;
}

static int copy_dst_addr(struct net_dplpmtud_entry *entry, const struct net_sockaddr *dst)
{
	switch (dst->sa_family) {
	case NET_AF_INET:
		entry->dst.family = NET_AF_INET;
		net_ipaddr_copy(&entry->dst.in_addr, &net_sin(dst)->sin_addr);
		return 0;
	case NET_AF_INET6:
		entry->dst.family = NET_AF_INET6;
		net_ipaddr_copy(&entry->dst.in6_addr, &net_sin6(dst)->sin6_addr);
		return 0;
	default:
		return -EAFNOSUPPORT;
	}
}

static void update_state_locked(struct net_dplpmtud_entry *entry)
{
	if (entry->state == NET_DPLPMTUD_STATE_ERROR) {
		return;
	}

	if (entry->validated_plpmtu <= entry->base_plpmtu) {
		entry->state = NET_DPLPMTUD_STATE_BASE;
		return;
	}

	if (entry->search_high <= entry->validated_plpmtu) {
		entry->state = NET_DPLPMTUD_STATE_SEARCH_COMPLETE;
		return;
	}

	entry->state = NET_DPLPMTUD_STATE_SEARCHING;
}

static struct net_dplpmtud_entry *get_entry_locked(const struct net_sockaddr *dst)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dplpmtud_entries); i++) {
		if (entry_matches_dst(&dplpmtud_entries[i], dst)) {
			return &dplpmtud_entries[i];
		}
	}

	return NULL;
}

static struct net_dplpmtud_entry *get_free_entry_locked(void)
{
	struct net_dplpmtud_entry *entry = NULL;
	uint32_t oldest = 0U;
	int i;

	for (i = 0; i < ARRAY_SIZE(dplpmtud_entries); i++) {
		if (!dplpmtud_entries[i].in_use) {
			return &dplpmtud_entries[i];
		}

		if (oldest == 0U || dplpmtud_entries[i].last_update < oldest) {
			oldest = dplpmtud_entries[i].last_update;
			entry = &dplpmtud_entries[i];
		}
	}

	return entry;
}

struct net_dplpmtud_entry *net_dplpmtud_get_entry(const struct net_sockaddr *dst)
{
	struct net_dplpmtud_entry *entry;

	if (dst == NULL) {
		return NULL;
	}

	k_mutex_lock(&lock, K_FOREVER);
	entry = get_entry_locked(dst);
	k_mutex_unlock(&lock);

	return entry;
}

struct net_dplpmtud_entry *net_dplpmtud_get_or_create_entry(const struct net_sockaddr *dst)
{
	struct net_dplpmtud_entry *entry;
	int ret;

	if (dst == NULL) {
		return NULL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	entry = get_entry_locked(dst);
	if (entry != NULL) {
		goto out;
	}

	entry = get_free_entry_locked();
	if (entry == NULL) {
		goto out;
	}

	memset(entry, 0, sizeof(*entry));

	ret = copy_dst_addr(entry, dst);
	if (ret < 0) {
		entry = NULL;
		goto out;
	}

	entry->in_use = true;
	entry->last_update = k_uptime_get_32();
	entry->base_plpmtu = NET_DPLPMTUD_BASE_PLPMTU;
	entry->validated_plpmtu = NET_DPLPMTUD_BASE_PLPMTU;
	entry->max_plpmtu = NET_DPLPMTUD_BASE_PLPMTU;
	entry->search_low = NET_DPLPMTUD_BASE_PLPMTU;
	entry->search_high = NET_DPLPMTUD_BASE_PLPMTU;
	entry->state = NET_DPLPMTUD_STATE_BASE;

out:
	k_mutex_unlock(&lock);

	return entry;
}

void net_dplpmtud_init_entry(struct net_dplpmtud_entry *entry,
			     uint16_t base_plpmtu,
			     uint16_t max_plpmtu)
{
	if (entry == NULL) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	entry->base_plpmtu = MAX(base_plpmtu, (uint16_t)NET_DPLPMTUD_BASE_PLPMTU);
	entry->validated_plpmtu = entry->base_plpmtu;
	entry->max_plpmtu = clamp_plpmtu(entry->base_plpmtu, max_plpmtu);
	entry->search_low = entry->validated_plpmtu;
	entry->search_high = entry->max_plpmtu;
	entry->probe_size = 0U;
	entry->probe_count = 0U;
	entry->blackhole_count = 0U;
	entry->probe_in_flight = false;
	entry->last_update = k_uptime_get_32();
	entry->state = NET_DPLPMTUD_STATE_BASE;
	update_state_locked(entry);

	k_mutex_unlock(&lock);
}

void net_dplpmtud_set_base_plpmtu(struct net_dplpmtud_entry *entry, uint16_t base_plpmtu)
{
	if (entry == NULL) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	entry->base_plpmtu = MAX(base_plpmtu, (uint16_t)NET_DPLPMTUD_BASE_PLPMTU);
	entry->max_plpmtu = clamp_plpmtu(entry->base_plpmtu, entry->max_plpmtu);
	entry->validated_plpmtu = MAX(entry->validated_plpmtu, entry->base_plpmtu);
	entry->validated_plpmtu = MIN(entry->validated_plpmtu, entry->max_plpmtu);
	entry->search_low = MAX(entry->search_low, entry->validated_plpmtu);
	entry->search_low = MAX(entry->search_low, entry->base_plpmtu);
	entry->search_high = MIN(entry->search_high, entry->max_plpmtu);
	entry->search_high = MAX(entry->search_high, entry->search_low);

	if (entry->probe_size <= entry->validated_plpmtu ||
	    entry->probe_size > entry->search_high) {
		entry->probe_size = 0U;
		entry->probe_count = 0U;
		entry->probe_in_flight = false;
	}

	entry->last_update = k_uptime_get_32();
	update_state_locked(entry);

	k_mutex_unlock(&lock);
}

void net_dplpmtud_set_max_plpmtu(struct net_dplpmtud_entry *entry, uint16_t max_plpmtu)
{
	if (entry == NULL) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	entry->max_plpmtu = clamp_plpmtu(entry->base_plpmtu, max_plpmtu);
	entry->validated_plpmtu = MIN(entry->validated_plpmtu, entry->max_plpmtu);
	entry->search_low = MIN(entry->search_low, entry->validated_plpmtu);
	entry->search_low = MAX(entry->search_low, entry->base_plpmtu);
	entry->search_high = MIN(entry->search_high, entry->max_plpmtu);
	entry->search_high = MAX(entry->search_high, entry->search_low);

	if (entry->probe_size <= entry->validated_plpmtu ||
	    entry->probe_size > entry->search_high) {
		entry->probe_size = 0U;
		entry->probe_count = 0U;
		entry->probe_in_flight = false;
	}

	entry->last_update = k_uptime_get_32();
	update_state_locked(entry);

	k_mutex_unlock(&lock);
}

uint16_t net_dplpmtud_get_next_probe_size(struct net_dplpmtud_entry *entry)
{
	uint16_t probe_size;

	if (entry == NULL) {
		return 0U;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (entry->probe_in_flight) {
		probe_size = entry->probe_size;
	} else if (entry->probe_size > entry->validated_plpmtu &&
		   entry->probe_count > 0U) {
		probe_size = entry->probe_size;
	} else {
		probe_size = next_probe_size(entry->search_low, entry->search_high);
	}

	k_mutex_unlock(&lock);

	return probe_size;
}

int net_dplpmtud_start_probe(struct net_dplpmtud_entry *entry, uint16_t probe_size)
{
	int ret = 0;

	if (entry == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (entry->probe_in_flight) {
		ret = -EALREADY;
		goto out;
	}

	if (probe_size <= entry->validated_plpmtu || probe_size > entry->search_high) {
		ret = -EINVAL;
		goto out;
	}

	entry->probe_in_flight = true;
	entry->probe_size = probe_size;
	entry->probe_count++;
	entry->state = NET_DPLPMTUD_STATE_SEARCHING;
	entry->last_update = k_uptime_get_32();

out:
	k_mutex_unlock(&lock);

	return ret;
}

int net_dplpmtud_probe_acked(struct net_dplpmtud_entry *entry, uint16_t probe_size)
{
	int ret = 0;

	if (entry == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (!entry->probe_in_flight || entry->probe_size != probe_size) {
		ret = -EINVAL;
		goto out;
	}

	entry->validated_plpmtu = MAX(entry->validated_plpmtu, probe_size);
	entry->search_low = entry->validated_plpmtu;
	entry->probe_size = 0U;
	entry->probe_count = 0U;
	entry->probe_in_flight = false;
	entry->last_update = k_uptime_get_32();
	update_state_locked(entry);

out:
	k_mutex_unlock(&lock);

	return ret;
}

int net_dplpmtud_probe_lost(struct net_dplpmtud_entry *entry, uint16_t probe_size)
{
	int ret = 0;

	if (entry == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (!entry->probe_in_flight || entry->probe_size != probe_size) {
		ret = -EINVAL;
		goto out;
	}

	entry->probe_in_flight = false;
	entry->last_update = k_uptime_get_32();

	if (entry->probe_count < NET_DPLPMTUD_MAX_PROBE_RETRIES) {
		entry->state = NET_DPLPMTUD_STATE_SEARCHING;
		goto out;
	}

	entry->probe_size = 0U;
	entry->probe_count = 0U;

	if (probe_size > 0U) {
		entry->search_high = probe_size - 1U;
	}

	entry->search_high = MAX(entry->search_high, entry->search_low);
	update_state_locked(entry);

out:
	k_mutex_unlock(&lock);

	return ret;
}

void net_dplpmtud_note_blackhole(struct net_dplpmtud_entry *entry)
{
	if (entry == NULL) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	entry->blackhole_count++;
	entry->validated_plpmtu = entry->base_plpmtu;
	entry->search_low = entry->base_plpmtu;
	entry->search_high = entry->base_plpmtu;
	entry->probe_size = 0U;
	entry->probe_count = 0U;
	entry->probe_in_flight = false;
	entry->state = NET_DPLPMTUD_STATE_ERROR;
	entry->last_update = k_uptime_get_32();

	k_mutex_unlock(&lock);
}

int net_dplpmtud_foreach(net_dplpmtud_cb_t cb, void *user_data)
{
	int count = 0;
	int i;

	if (cb == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(dplpmtud_entries); i++) {
		if (!dplpmtud_entries[i].in_use) {
			continue;
		}

		cb(&dplpmtud_entries[i], user_data);
		count++;
	}

	k_mutex_unlock(&lock);

	return count;
}

void net_dplpmtud_init(void)
{
	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(dplpmtud_entries, i) {
		dplpmtud_entries[i].in_use = false;
	}

	k_mutex_unlock(&lock);
}
