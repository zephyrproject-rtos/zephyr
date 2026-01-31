/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OSCORE context cache management for RFC 9668
 *
 * Context cache lookup/insert/evict by kid with lifetime enforcement.
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_CTX_CACHE_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_CTX_CACHE_H_

#include <zephyr/net/coap/coap_service.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find OSCORE context by kid
 *
 * @param cache Context cache array
 * @param cache_size Size of cache array
 * @param kid OSCORE Sender ID (kid) to search for
 * @param kid_len Length of kid
 * @return Pointer to cache entry if found, NULL otherwise
 */
struct coap_oscore_ctx_cache_entry *coap_oscore_ctx_cache_find(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	const uint8_t *kid,
	uint8_t kid_len);

/**
 * @brief Insert or update OSCORE context
 *
 * Uses LRU eviction if cache is full. Evicts expired contexts first.
 *
 * @param cache Context cache array
 * @param cache_size Size of cache array
 * @param kid OSCORE Sender ID (kid)
 * @param kid_len Length of kid
 * @return Pointer to cache entry (new or existing), NULL on error
 */
struct coap_oscore_ctx_cache_entry *coap_oscore_ctx_cache_insert(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	const uint8_t *kid,
	uint8_t kid_len);

/**
 * @brief Remove OSCORE context by kid
 *
 * @param cache Context cache array
 * @param cache_size Size of cache array
 * @param kid OSCORE Sender ID (kid)
 * @param kid_len Length of kid
 */
void coap_oscore_ctx_cache_remove(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	const uint8_t *kid,
	uint8_t kid_len);

/**
 * @brief Evict expired OSCORE contexts
 *
 * @param cache Context cache array
 * @param cache_size Size of cache array
 * @param now Current timestamp (k_uptime_get())
 * @param lifetime_ms Context lifetime in milliseconds
 * @return Number of contexts evicted
 */
int coap_oscore_ctx_cache_evict_expired(
	struct coap_oscore_ctx_cache_entry *cache,
	size_t cache_size,
	int64_t now,
	int64_t lifetime_ms);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_CTX_CACHE_H_ */
