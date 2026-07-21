/** @file
 * @brief DNS cache
 *
 * An cache holding dns records for faster dns resolving.
 */

/*
 * Copyright (c) 2024 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_DNS_CACHE_H_
#define ZEPHYR_INCLUDE_NET_DNS_CACHE_H_

#include <stdint.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

struct dns_cache_entry {
	char query[CONFIG_DNS_RESOLVER_MAX_QUERY_LEN];
	struct dns_addrinfo data;
	k_timepoint_t expiry;
	bool in_use;
};

struct dns_cache {
	size_t size;
	struct dns_cache_entry *entries;
	struct k_mutex *lock;
};

/**
 * @brief Statically define and initialize a DNS queue.
 *
 * The cache can be accessed outside the module where it is defined using:
 *
 * @code extern struct dns_cache <name>; @endcode
 *
 * @param name Name of the cache.
 */
#define DNS_CACHE_DEFINE(name, cache_size)                                                         \
	static K_MUTEX_DEFINE(name##_mutex);                                                       \
	static struct dns_cache_entry name##_entries[cache_size];                                  \
	static struct dns_cache name = {                                                           \
		.entries = name##_entries, .size = cache_size, .lock = &name##_mutex};

/**
 * @brief Flushes the dns cache removing all its entries.
 *
 * @param cache Cache to be flushed
 * @retval 0 on success
 * @retval On error, a negative value is returned.
 */
int dns_cache_flush(struct dns_cache *cache);

/**
 * @brief Adds a new entry to the dns cache removing the one closest to expiry
 * if no free space is available.
 *
 * @param cache Cache where the entry should be added.
 * @param query Query which should be persisted in the cache.
 * @param addrinfo Addrinfo resulting from the query which will be returned
 * upon cache hit.
 * @param ttl Time to live for the entry in seconds. This usually represents
 * the TTL of the RR.
 * @retval 0 on success
 * @retval On error, a negative value is returned.
 */
int dns_cache_add(struct dns_cache *cache, char const *query, struct dns_addrinfo const *addrinfo,
		  uint32_t ttl);

/**
 * @brief Removes all entries with the given query
 *
 * @param cache Cache where the entries should be removed.
 * @param query Query which should be searched for.
 * @retval 0 on success
 * @retval On error, a negative value is returned.
 */
int dns_cache_remove(struct dns_cache *cache, char const *query);

/**
 * @brief Tries to find the specified query entry within the cache.
 *
 * @param cache Cache where the entry should be searched.
 * @param query Query which should be searched for.
 * @param type Query type which will control the types of addresses that will be found.
 * @param addrinfo dns_addrinfo array which will be written if the query was found.
 * @param addrinfo_array_len Array size of the dns_addrinfo array
 * @retval on success the amount of dns_addrinfo written into the addrinfo array will be returned.
 * A cache miss will therefore return a 0.
 * @retval On error a negative value is returned.
 * -ENOSR means there was not enough space in the addrinfo array to accommodate all cache hits the
 * array will however be filled with valid data.
 */
int dns_cache_find(struct dns_cache const *cache, const char *query, enum dns_query_type type,
		   struct dns_addrinfo *addrinfo, size_t addrinfo_array_len);

#endif /* ZEPHYR_INCLUDE_NET_DNS_CACHE_H_ */
