/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include "coap_oscore_ctx_cache.h"

#include <errno.h>
#include <string.h>

struct coap_oscore_ctx_cache_entry *coap_oscore_ctx_cache_find(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	const uint8_t *kid,
	uint8_t kid_len)
{
	if (cache == NULL || kid == NULL || kid_len == 0) {
		return NULL;
	}

	for (size_t i = 0; i < cache_size; i++) {
		if (cache[i].active &&
		    cache[i].kid_len == kid_len &&
		    memcmp(cache[i].kid, kid, kid_len) == 0) {
			return &cache[i];
		}
	}

	return NULL;
}

struct coap_oscore_ctx_cache_entry *coap_oscore_ctx_cache_insert(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	const uint8_t *kid,
	uint8_t kid_len)
{
	struct coap_oscore_ctx_cache_entry *entry = NULL;
	struct coap_oscore_ctx_cache_entry *oldest = NULL;
	int64_t oldest_time = INT64_MAX;
	int64_t now = k_uptime_get();

	if (cache == NULL || kid == NULL || kid_len == 0 || kid_len > 16) {
		return NULL;
	}

	/* Check if entry already exists */
	entry = coap_oscore_ctx_cache_find(cache, cache_size, kid, kid_len);
	if (entry != NULL) {
		/* Update timestamp */
		entry->timestamp = now;
		return entry;
	}

	/* Find empty or oldest entry */
	for (size_t i = 0; i < cache_size; i++) {
		if (!cache[i].active) {
			entry = &cache[i];
			break;
		}
		if (cache[i].timestamp < oldest_time) {
			oldest_time = cache[i].timestamp;
			oldest = &cache[i];
		}
	}

	/* Use empty entry or evict oldest */
	if (entry == NULL) {
		if (oldest != NULL) {
			LOG_DBG("Evicting oldest OSCORE context (age %lld ms)",
				now - oldest->timestamp);
			entry = oldest;
			memset(entry, 0, sizeof(*entry));
		} else {
			return NULL;
		}
	}

	/* Initialize new entry */
	memcpy(entry->kid, kid, kid_len);
	entry->kid_len = kid_len;
	entry->timestamp = now;
	entry->active = true;
	/* oscore_ctx should be set by caller */

	return entry;
}

void coap_oscore_ctx_cache_remove(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	const uint8_t *kid,
	uint8_t kid_len)
{
	struct coap_oscore_ctx_cache_entry *entry;

	entry = coap_oscore_ctx_cache_find(cache, cache_size, kid, kid_len);
	if (entry != NULL) {
		memset(entry, 0, sizeof(*entry));
	}
}

int coap_oscore_ctx_cache_evict_expired(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	int64_t now,
	int64_t lifetime_ms)
{
	int evicted = 0;

	if (cache == NULL) {
		return 0;
	}

	for (size_t i = 0; i < cache_size; i++) {
		if (cache[i].active && (now - cache[i].timestamp) > lifetime_ms) {
			LOG_DBG("Evicting expired OSCORE context (age %lld ms)",
				now - cache[i].timestamp);
			memset(&cache[i], 0, sizeof(cache[i]));
			evicted++;
		}
	}

	return evicted;
}
