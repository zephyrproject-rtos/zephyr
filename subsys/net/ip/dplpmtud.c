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
#include <zephyr/net/dplpmtud.h>
#include <zephyr/net/net_ip.h>
#include "dplpmtud_internal.h"
#include "pmtu.h"

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

static inline uint16_t normalize_plpmtu(uint16_t plpmtu)
{
	if (plpmtu == 0U) {
		return NET_DPLPMTUD_BASE_PLPMTU;
	}

	return plpmtu;
}

static inline uint16_t normalize_max_plpmtu(uint16_t base_plpmtu, uint16_t max_plpmtu)
{
	if (max_plpmtu == 0U) {
		return base_plpmtu;
	}

	return max_plpmtu;
}

static inline uint16_t normalize_path_max_plpmtu(uint16_t max_plpmtu)
{
	if (max_plpmtu == 0U) {
		return UINT16_MAX;
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

/* DPLPMTUD tracks the PLPMTU (the UDP payload size a transport can send),
 * while the shared PMTU destination cache stores the IP-layer MTU and is also
 * consumed by TCP and the fragmentation code. Convert between the two at the
 * cache boundary using the per-family IP + UDP header overhead so we neither
 * over-probe nor write payload-sized values into the cache.
 */
static uint16_t dplpmtud_header_overhead(net_sa_family_t family)
{
	switch (family) {
	case NET_AF_INET:
		return (uint16_t)(sizeof(struct net_ipv4_hdr) +
				  sizeof(struct net_udp_hdr));
	case NET_AF_INET6:
		return (uint16_t)(sizeof(struct net_ipv6_hdr) +
				  sizeof(struct net_udp_hdr));
	default:
		return 0U;
	}
}

/* Convert an IP-layer MTU (PMTU cache) to a PLPMTU (UDP payload). Returns 0 if
 * the path cannot carry even an empty UDP datagram.
 */
static uint16_t pmtu_to_plpmtu(net_sa_family_t family, uint16_t pmtu)
{
	uint16_t overhead = dplpmtud_header_overhead(family);

	if (pmtu <= overhead) {
		return 0U;
	}

	return pmtu - overhead;
}

/* Convert a PLPMTU (UDP payload) to an IP-layer MTU for the PMTU cache. */
static uint16_t plpmtu_to_pmtu(net_sa_family_t family, uint16_t plpmtu)
{
	uint16_t overhead = dplpmtud_header_overhead(family);

	if ((uint32_t)plpmtu + overhead > UINT16_MAX) {
		return UINT16_MAX;
	}

	return plpmtu + overhead;
}

static int dplpmtud_entry_to_sockaddr(const struct net_dplpmtud_entry *entry,
				      struct net_sockaddr_storage *dst)
{
	struct net_sockaddr *sa = net_sad(dst);

	memset(dst, 0, sizeof(*dst));

	switch (entry->dst.family) {
	case NET_AF_INET:
		sa->sa_family = NET_AF_INET;
		net_ipaddr_copy(&net_sin(sa)->sin_addr, &entry->dst.in_addr);
		return 0;
	case NET_AF_INET6:
		sa->sa_family = NET_AF_INET6;
		net_ipaddr_copy(&net_sin6(sa)->sin6_addr, &entry->dst.in6_addr);
		return 0;
	default:
		return -EAFNOSUPPORT;
	}
}

static void sync_pmtu_cache_from_validated(const struct net_dplpmtud_entry *entry,
					   uint16_t validated_plpmtu,
					   uint16_t previous_validated)
{
	struct net_sockaddr_storage dst;

	if (!IS_ENABLED(CONFIG_NET_PMTU) || validated_plpmtu == previous_validated) {
		return;
	}

	if (dplpmtud_entry_to_sockaddr(entry, &dst) < 0) {
		return;
	}

	(void)net_pmtu_update_mtu_from_dplpmtud(
		net_sad(&dst),
		plpmtu_to_pmtu(entry->dst.family, validated_plpmtu));
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
	if (entry->validated_plpmtu < entry->base_plpmtu) {
		entry->state = NET_DPLPMTUD_STATE_ERROR;
		return;
	}

	if (entry->validated_plpmtu == entry->base_plpmtu) {
		if (entry->probe_in_flight) {
			entry->state = NET_DPLPMTUD_STATE_SEARCHING;
		} else {
			entry->state = NET_DPLPMTUD_STATE_BASE;
		}

		return;
	}

	if (entry->search_high <= entry->validated_plpmtu) {
		entry->state = NET_DPLPMTUD_STATE_SEARCH_COMPLETE;
		return;
	}

	entry->state = NET_DPLPMTUD_STATE_SEARCHING;
}

static inline void reset_probe_locked(struct net_dplpmtud_entry *entry)
{
	entry->probe_size = 0U;
	entry->probe_count = 0U;
	entry->probe_in_flight = false;
}

static inline void sync_search_bounds_locked(struct net_dplpmtud_entry *entry)
{
	entry->validated_plpmtu = MIN(entry->validated_plpmtu, entry->max_plpmtu);
	entry->search_low = entry->validated_plpmtu;
	entry->search_high = MIN(entry->search_high, entry->max_plpmtu);
	entry->search_high = MAX(entry->search_high, entry->search_low);

	if (entry->probe_size <= entry->validated_plpmtu ||
	    entry->probe_size > entry->search_high) {
		reset_probe_locked(entry);
	}
}

static inline void init_entry_defaults_locked(struct net_dplpmtud_entry *entry,
					      uint16_t base_plpmtu,
					      uint16_t max_plpmtu)
{
	entry->base_plpmtu = normalize_plpmtu(base_plpmtu);
	entry->max_plpmtu = normalize_max_plpmtu(entry->base_plpmtu, max_plpmtu);
	entry->validated_plpmtu = MIN(entry->base_plpmtu, entry->max_plpmtu);
	entry->search_low = entry->validated_plpmtu;
	entry->search_high = entry->max_plpmtu;
	reset_probe_locked(entry);
	entry->blackhole_count = 0U;
	entry->last_update = k_uptime_get_32();
	entry->state = NET_DPLPMTUD_STATE_BASE;
	update_state_locked(entry);
}

static inline bool family_enabled(net_sa_family_t family)
{
	switch (family) {
	case NET_AF_INET:
		return IS_ENABLED(CONFIG_NET_IPV4_PMTU_DPLPMTUD);
	case NET_AF_INET6:
		return IS_ENABLED(CONFIG_NET_IPV6_PMTU_DPLPMTUD);
	default:
		return false;
	}
}

static inline uint16_t get_path_limit(const struct net_dplpmtud_path *path,
				      const struct net_dplpmtud_entry *entry)
{
	return MIN(entry->max_plpmtu, path->max_plpmtu);
}

static int copy_path_dst(struct net_dplpmtud_path *path, const struct net_sockaddr *dst)
{
	switch (dst->sa_family) {
	case NET_AF_INET:
		memcpy(&path->dst, net_sas(dst), sizeof(struct net_sockaddr_in));
		return 0;
	case NET_AF_INET6:
		memcpy(&path->dst, net_sas(dst), sizeof(struct net_sockaddr_in6));
		return 0;
	default:
		return -EAFNOSUPPORT;
	}
}

static inline void invalidate_entry_locked(struct net_dplpmtud_entry *entry)
{
	entry->in_use = false;
	reset_probe_locked(entry);
	entry->last_update = 0U;
	entry->blackhole_count = 0U;
	entry->base_plpmtu = 0U;
	entry->validated_plpmtu = 0U;
	entry->max_plpmtu = 0U;
	entry->search_low = 0U;
	entry->search_high = 0U;
	entry->state = NET_DPLPMTUD_STATE_BASE;
	memset(&entry->dst, 0, sizeof(entry->dst));
}

static inline void reuse_entry_locked(struct net_dplpmtud_entry *entry,
				      const struct net_sockaddr *dst)
{
	memset(entry, 0, sizeof(*entry));
	entry->in_use = true;
	entry->last_update = k_uptime_get_32();

	if (copy_dst_addr(entry, dst) < 0) {
		invalidate_entry_locked(entry);
		return;
	}

	init_entry_defaults_locked(entry, NET_DPLPMTUD_BASE_PLPMTU,
				   NET_DPLPMTUD_BASE_PLPMTU);
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

static void sync_entry_from_pmtu_cache(struct net_dplpmtud_entry *entry,
				       const struct net_sockaddr *dst)
{
	int mtu;

	mtu = net_pmtu_get_mtu(dst);
	if (mtu > 0) {
		uint16_t plpmtu = pmtu_to_plpmtu(dst->sa_family, (uint16_t)mtu);

		/* Raise the search ceiling when the PMTU cache reports a larger
		 * path limit. Lower it only on PTB-style reductions below the
		 * current validated PLPMTU, not when the cache mirrors a
		 * DPLPMTUD write-back of the validated size.
		 */
		if (plpmtu > entry->max_plpmtu) {
			net_dplpmtud_set_max_plpmtu(entry, plpmtu);
		} else if (plpmtu < entry->validated_plpmtu) {
			net_dplpmtud_set_max_plpmtu(entry, plpmtu);
		}
	}
}

static struct net_dplpmtud_entry *lookup_path_entry(struct net_dplpmtud_path *path,
						    bool create)
{
	if (path == NULL || !path->in_use ||
	    !family_enabled(net_sad(&path->dst)->sa_family)) {
		return NULL;
	}

	if (create) {
		return net_dplpmtud_get_or_create_entry(net_sad(&path->dst));
	}

	return net_dplpmtud_get_entry(net_sad(&path->dst));
}

/* Resolve the cache entry for a path. The caller MUST hold @lock across both
 * this lookup and the subsequent read/update of the returned entry. Otherwise a
 * concurrent net_dplpmtud_get_or_create_entry() on another thread could evict
 * and reuse the entry (LRU) in the window between lookup and use, applying the
 * operation to the wrong destination. The per-operation helpers below take
 * @lock too, but k_mutex is recursive so the nested acquisition is a no-op
 * while the wrapper already holds it.
 */
static struct net_dplpmtud_entry *get_path_entry(struct net_dplpmtud_path *path,
						 bool create, bool sync_pmtu)
{
	struct net_dplpmtud_entry *entry;

	entry = lookup_path_entry(path, create);
	if (entry == NULL) {
		return NULL;
	}

	if (sync_pmtu) {
		sync_entry_from_pmtu_cache(entry, net_sad(&path->dst));
	}

	return entry;
}

int net_dplpmtud_init_path(struct net_dplpmtud_path *path,
			   const struct net_sockaddr *dst,
			   uint16_t max_plpmtu)
{
	struct net_dplpmtud_entry *entry;

	if (path == NULL || dst == NULL || !family_enabled(dst->sa_family)) {
		return -EINVAL;
	}

	memset(path, 0, sizeof(*path));

	if (copy_path_dst(path, dst) < 0) {
		return -EINVAL;
	}

	path->max_plpmtu = normalize_path_max_plpmtu(max_plpmtu);
	path->in_use = true;

	k_mutex_lock(&lock, K_FOREVER);
	entry = get_path_entry(path, true, true);
	k_mutex_unlock(&lock);

	if (entry == NULL) {
		memset(path, 0, sizeof(*path));
		return -ENOMEM;
	}

	return 0;
}

void net_dplpmtud_set_path_max_plpmtu(struct net_dplpmtud_path *path,
				      uint16_t max_plpmtu)
{
	if (path == NULL || !path->in_use) {
		return;
	}

	path->max_plpmtu = normalize_path_max_plpmtu(max_plpmtu);
}

int net_dplpmtud_get_path_mtu(struct net_dplpmtud_path *path)
{
	struct net_dplpmtud_entry *entry;
	int mtu;

	k_mutex_lock(&lock, K_FOREVER);

	entry = get_path_entry(path, false, false);
	if (entry == NULL) {
		mtu = -ENOENT;
	} else {
		mtu = MIN(get_path_limit(path, entry), entry->validated_plpmtu);
	}

	k_mutex_unlock(&lock);

	return mtu;
}

int net_dplpmtud_get_path_probe_size(struct net_dplpmtud_path *path)
{
	struct net_dplpmtud_entry *entry;
	uint16_t path_limit;
	uint16_t search_high;
	int probe_size;

	k_mutex_lock(&lock, K_FOREVER);

	entry = get_path_entry(path, false, false);
	if (entry == NULL) {
		k_mutex_unlock(&lock);
		return -ENOENT;
	}

	path_limit = get_path_limit(path, entry);

	if (entry->probe_in_flight ||
	    (entry->probe_size > entry->validated_plpmtu &&
	     entry->probe_count > 0U)) {
		probe_size = entry->probe_size <= path_limit ?
			(int)entry->probe_size : 0;
	} else {
		search_high = MIN(entry->search_high, path_limit);
		if (search_high <= entry->validated_plpmtu) {
			probe_size = 0;
		} else {
			probe_size = (int)next_probe_size(entry->search_low,
							  search_high);
		}
	}

	k_mutex_unlock(&lock);

	return probe_size;
}

bool net_dplpmtud_path_probe_in_flight(struct net_dplpmtud_path *path)
{
	struct net_dplpmtud_entry *entry;
	bool in_flight;

	k_mutex_lock(&lock, K_FOREVER);

	entry = lookup_path_entry(path, false);
	in_flight = (entry != NULL) ? entry->probe_in_flight : false;

	k_mutex_unlock(&lock);

	return in_flight;
}

int net_dplpmtud_on_path_probe_sent(struct net_dplpmtud_path *path,
				    uint16_t probe_size)
{
	struct net_dplpmtud_entry *entry;
	int ret;

	k_mutex_lock(&lock, K_FOREVER);

	entry = get_path_entry(path, true, true);
	if (entry == NULL) {
		ret = -ENOENT;
	} else if (probe_size > get_path_limit(path, entry)) {
		ret = -EMSGSIZE;
	} else {
		ret = net_dplpmtud_start_probe(entry, probe_size);
	}

	k_mutex_unlock(&lock);

	return ret;
}

int net_dplpmtud_on_path_probe_acked(struct net_dplpmtud_path *path,
				     uint16_t probe_size)
{
	struct net_dplpmtud_entry *entry;
	int ret;

	k_mutex_lock(&lock, K_FOREVER);

	entry = get_path_entry(path, false, false);
	ret = (entry == NULL) ? -ENOENT :
		net_dplpmtud_probe_acked(entry, probe_size);

	k_mutex_unlock(&lock);

	return ret;
}

int net_dplpmtud_on_path_probe_lost(struct net_dplpmtud_path *path,
				    uint16_t probe_size)
{
	struct net_dplpmtud_entry *entry;
	int ret;

	k_mutex_lock(&lock, K_FOREVER);

	entry = get_path_entry(path, false, false);
	ret = (entry == NULL) ? -ENOENT :
		net_dplpmtud_probe_lost(entry, probe_size);

	k_mutex_unlock(&lock);

	return ret;
}

void net_dplpmtud_note_path_blackhole(struct net_dplpmtud_path *path)
{
	struct net_dplpmtud_entry *entry;

	k_mutex_lock(&lock, K_FOREVER);

	entry = get_path_entry(path, false, false);
	if (entry != NULL) {
		net_dplpmtud_note_blackhole(entry);
	}

	k_mutex_unlock(&lock);
}

struct net_dplpmtud_entry *net_dplpmtud_get_entry(const struct net_sockaddr *dst)
{
	struct net_dplpmtud_entry *entry;

	if (dst == NULL || !family_enabled(dst->sa_family)) {
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

	if (dst == NULL || !family_enabled(dst->sa_family)) {
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

	reuse_entry_locked(entry, dst);
	if (!entry->in_use) {
		entry = NULL;
		goto out;
	}

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

	init_entry_defaults_locked(entry, base_plpmtu, max_plpmtu);

	k_mutex_unlock(&lock);
}

void net_dplpmtud_set_base_plpmtu(struct net_dplpmtud_entry *entry, uint16_t base_plpmtu)
{
	if (entry == NULL) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	entry->base_plpmtu = normalize_plpmtu(base_plpmtu);
	entry->max_plpmtu = normalize_max_plpmtu(entry->base_plpmtu, entry->max_plpmtu);
	sync_search_bounds_locked(entry);
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

	entry->max_plpmtu = normalize_max_plpmtu(entry->base_plpmtu, max_plpmtu);
	sync_search_bounds_locked(entry);
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
	uint16_t previous_validated;
	uint16_t validated_plpmtu;
	int ret = 0;

	if (entry == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (!entry->probe_in_flight || entry->probe_size != probe_size) {
		ret = -EINVAL;
		goto out;
	}

	previous_validated = entry->validated_plpmtu;
	validated_plpmtu = MIN(probe_size, entry->max_plpmtu);
	entry->validated_plpmtu = validated_plpmtu;
	entry->search_low = entry->validated_plpmtu;
	reset_probe_locked(entry);
	entry->last_update = k_uptime_get_32();
	update_state_locked(entry);

	sync_pmtu_cache_from_validated(entry, validated_plpmtu, previous_validated);

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
	uint16_t previous_validated;
	uint16_t validated_plpmtu;

	if (entry == NULL) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	previous_validated = entry->validated_plpmtu;
	entry->blackhole_count++;
	entry->validated_plpmtu = MIN(entry->base_plpmtu, entry->max_plpmtu);
	validated_plpmtu = entry->validated_plpmtu;
	entry->search_low = entry->validated_plpmtu;
	entry->search_high = entry->validated_plpmtu;
	reset_probe_locked(entry);
	entry->state = NET_DPLPMTUD_STATE_ERROR;
	entry->last_update = k_uptime_get_32();

	k_mutex_unlock(&lock);

	sync_pmtu_cache_from_validated(entry, validated_plpmtu, previous_validated);
}

void net_dplpmtud_sync_from_pmtu(const struct net_sockaddr *dst, uint16_t pmtu)
{
	struct net_dplpmtud_entry *entry;
	uint16_t plpmtu;

	if (dst == NULL || !family_enabled(dst->sa_family)) {
		return;
	}

	/* The PMTU cache stores the IP-layer MTU; DPLPMTUD works in PLPMTU
	 * (UDP payload) units.
	 */
	plpmtu = pmtu_to_plpmtu(dst->sa_family, pmtu);

	/* Hold @lock across the lookup/create and the update so the entry
	 * cannot be evicted and reused for another destination in between.
	 */
	k_mutex_lock(&lock, K_FOREVER);

	entry = net_dplpmtud_get_entry(dst);
	if (entry == NULL) {
		entry = net_dplpmtud_get_or_create_entry(dst);
		if (entry != NULL) {
			net_dplpmtud_init_entry(entry, NET_DPLPMTUD_BASE_PLPMTU, plpmtu);
		}
	} else {
		net_dplpmtud_set_max_plpmtu(entry, plpmtu);
	}

	if (entry != NULL && plpmtu < NET_DPLPMTUD_BASE_PLPMTU) {
		net_dplpmtud_note_blackhole(entry);
	}

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
