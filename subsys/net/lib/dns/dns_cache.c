/*
 * Copyright (c) 2024 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/dns_resolve.h>
#include "dns_cache.h"

LOG_MODULE_REGISTER(net_dns_cache, CONFIG_DNS_RESOLVER_LOG_LEVEL);

static void dns_cache_clean(struct dns_cache const *cache);

int dns_cache_flush(struct dns_cache *cache)
{
	k_mutex_lock(cache->lock, K_FOREVER);
	for (size_t i = 0; i < cache->size; i++) {
		cache->entries[i].in_use = false;
	}
	k_mutex_unlock(cache->lock);

	return 0;
}

int dns_cache_add(struct dns_cache *cache, char const *query, struct dns_addrinfo const *addrinfo,
		  uint32_t ttl)
{
	k_timepoint_t closest_to_expiry = sys_timepoint_calc(K_FOREVER);
	size_t index_to_replace = 0;
	bool found_empty = false;

	if (cache == NULL || query == NULL || addrinfo == NULL || ttl == 0) {
		return -EINVAL;
	}

	if (strlen(query) >= CONFIG_DNS_RESOLVER_MAX_QUERY_LEN) {
		NET_WARN("Query string to big to be processed %u >= "
			 "CONFIG_DNS_RESOLVER_MAX_QUERY_LEN",
			 strlen(query));
		return -EINVAL;
	}

	k_mutex_lock(cache->lock, K_FOREVER);

	NET_DBG("Add \"%s\" with TTL %" PRIu32, query, ttl);

	dns_cache_clean(cache);

	for (size_t i = 0; i < cache->size; i++) {
		if (!cache->entries[i].in_use) {
			index_to_replace = i;
			found_empty = true;
			break;
		} else if (sys_timepoint_cmp(closest_to_expiry, cache->entries[i].expiry) > 0) {
			index_to_replace = i;
			closest_to_expiry = cache->entries[i].expiry;
		}
	}

	if (!found_empty) {
		NET_DBG("Overwrite \"%s\"", cache->entries[index_to_replace].query);
	}

	strncpy(cache->entries[index_to_replace].query, query,
		CONFIG_DNS_RESOLVER_MAX_QUERY_LEN - 1);
	cache->entries[index_to_replace].data = *addrinfo;
	cache->entries[index_to_replace].expiry = sys_timepoint_calc(K_SECONDS(ttl));
	cache->entries[index_to_replace].in_use = true;

	k_mutex_unlock(cache->lock);

	return 0;
}

int dns_cache_remove(struct dns_cache *cache, char const *query)
{
	NET_DBG("Remove all entries with query \"%s\"", query);
	if (strlen(query) >= CONFIG_DNS_RESOLVER_MAX_QUERY_LEN) {
		NET_WARN("Query string to big to be processed %u >= "
			 "CONFIG_DNS_RESOLVER_MAX_QUERY_LEN",
			 strlen(query));
		return -EINVAL;
	}

	k_mutex_lock(cache->lock, K_FOREVER);

	dns_cache_clean(cache);

	for (size_t i = 0; i < cache->size; i++) {
		if (cache->entries[i].in_use && strcmp(cache->entries[i].query, query) == 0) {
			cache->entries[i].in_use = false;
		}
	}

	k_mutex_unlock(cache->lock);

	return 0;
}

int dns_cache_find(struct dns_cache const *cache, const char *query, struct dns_addrinfo *addrinfo,
		   size_t addrinfo_array_len)
{
	size_t found = 0;

	NET_DBG("Find \"%s\"", query);
	if (cache == NULL || query == NULL || addrinfo == NULL || addrinfo_array_len <= 0) {
		return -EINVAL;
	}
	if (strlen(query) >= CONFIG_DNS_RESOLVER_MAX_QUERY_LEN) {
		NET_WARN("Query string to big to be processed %u >= "
			 "CONFIG_DNS_RESOLVER_MAX_QUERY_LEN",
			 strlen(query));
		return -EINVAL;
	}

	k_mutex_lock(cache->lock, K_FOREVER);

	dns_cache_clean(cache);

	for (size_t i = 0; i < cache->size; i++) {
		if (!cache->entries[i].in_use) {
			continue;
		}
		if (strcmp(cache->entries[i].query, query) != 0) {
			continue;
		}
		if (found >= addrinfo_array_len) {
			NET_WARN("Found \"%s\" but not enough space in provided buffer.", query);
			found++;
		} else {
			addrinfo[found] = cache->entries[i].data;
			found++;
			NET_DBG("Found \"%s\"", query);
		}
	}

	k_mutex_unlock(cache->lock);

	if (found > addrinfo_array_len) {
		return -ENOSR;
	}

	if (found == 0) {
		NET_DBG("Could not find \"%s\"", query);
	}
	return found;
}

/* Needs to be called when lock is already acquired */
static void dns_cache_clean(struct dns_cache const *cache)
{
	for (size_t i = 0; i < cache->size; i++) {
		if (!cache->entries[i].in_use) {
			continue;
		}

		if (sys_timepoint_expired(cache->entries[i].expiry)) {
			NET_DBG("Remove \"%s\"", cache->entries[i].query);
			cache->entries[i].in_use = false;
		}
	}
}
