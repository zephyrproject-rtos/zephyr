/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include "coap_oscore_ctx_cache.h"
#include "coap_edhoc.h"

#include <errno.h>
#include <string.h>

#if defined(CONFIG_UOSCORE)
#include <oscore/security_context.h>

/* Internal fixed pool for OSCORE contexts
 * RFC 9668 Section 3.3.1: Server derives OSCORE contexts from EDHOC.
 * Pool size matches CONFIG_COAP_OSCORE_CTX_CACHE_SIZE.
 */
K_MEM_SLAB_DEFINE_STATIC(oscore_ctx_pool, sizeof(struct context),
			 CONFIG_COAP_OSCORE_CTX_CACHE_SIZE, 4);
#endif

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
			coap_edhoc_secure_memzero(entry, sizeof(*entry));
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
#if defined(CONFIG_UOSCORE)
		/* Free the OSCORE context back to the pool */
		if (entry->oscore_ctx != NULL) {
			coap_oscore_ctx_free(entry->oscore_ctx);
		}
#endif
		coap_edhoc_secure_memzero(entry, sizeof(*entry));
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
#if defined(CONFIG_UOSCORE)
			/* Free the OSCORE context back to the pool */
			if (cache[i].oscore_ctx != NULL) {
				coap_oscore_ctx_free(cache[i].oscore_ctx);
			}
#endif
			coap_edhoc_secure_memzero(&cache[i], sizeof(cache[i]));
			evicted++;
		}
	}

	return evicted;
}

#if defined(CONFIG_UOSCORE)
struct context *coap_oscore_ctx_alloc(void)
{
	struct context *ctx;
	int ret;

	ret = k_mem_slab_alloc(&oscore_ctx_pool, (void **)&ctx, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to allocate OSCORE context from pool (%d)", ret);
		return NULL;
	}

	/* Zeroize the context before use */
	coap_edhoc_secure_memzero(ctx, sizeof(*ctx));

	return ctx;
}

void coap_oscore_ctx_free(struct context *ctx)
{
	if (ctx == NULL) {
		return;
	}

	/* Zeroize the context before returning to pool (security best practice) */
	coap_edhoc_secure_memzero(ctx, sizeof(*ctx));

	k_mem_slab_free(&oscore_ctx_pool, (void *)ctx);
}
#endif
