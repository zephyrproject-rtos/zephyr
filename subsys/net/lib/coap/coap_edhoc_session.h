/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDHOC session management for RFC 9668
 *
 * Session lookup/insert/evict by C_R with lifetime enforcement.
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_EDHOC_SESSION_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_EDHOC_SESSION_H_

#include <zephyr/net/coap/coap_service.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find EDHOC session by C_R
 *
 * @param cache Session cache array
 * @param cache_size Size of cache array
 * @param c_r Connection identifier C_R to search for
 * @param c_r_len Length of C_R
 * @return Pointer to session entry if found, NULL otherwise
 */
struct coap_edhoc_session *coap_edhoc_session_find(
	struct coap_edhoc_session *cache,
	size_t cache_size,
	const uint8_t *c_r,
	uint8_t c_r_len);

/**
 * @brief Insert or update EDHOC session
 *
 * Uses LRU eviction if cache is full. Evicts expired sessions first.
 *
 * @param cache Session cache array
 * @param cache_size Size of cache array
 * @param c_r Connection identifier C_R
 * @param c_r_len Length of C_R
 * @return Pointer to session entry (new or existing), NULL on error
 */
struct coap_edhoc_session *coap_edhoc_session_insert(
	struct coap_edhoc_session *cache,
	size_t cache_size,
	const uint8_t *c_r,
	uint8_t c_r_len);

/**
 * @brief Remove EDHOC session by C_R
 *
 * @param cache Session cache array
 * @param cache_size Size of cache array
 * @param c_r Connection identifier C_R
 * @param c_r_len Length of C_R
 */
void coap_edhoc_session_remove(
	struct coap_edhoc_session *cache,
	size_t cache_size,
	const uint8_t *c_r,
	uint8_t c_r_len);

/**
 * @brief Evict expired EDHOC sessions
 *
 * @param cache Session cache array
 * @param cache_size Size of cache array
 * @param now Current timestamp (k_uptime_get())
 * @param lifetime_ms Session lifetime in milliseconds
 * @return Number of sessions evicted
 */
int coap_edhoc_session_evict_expired(
	struct coap_edhoc_session *cache,
	size_t cache_size,
	int64_t now,
	int64_t lifetime_ms);

/**
 * @brief Set C_I (connection identifier for initiator) on an existing EDHOC session
 *
 * Used to store C_I after it's extracted from EDHOC message_1 or message_2.
 * Required for RFC 9528 Appendix A.1 Table 14 ID mapping.
 *
 * @param session Session entry to update
 * @param c_i Connection identifier C_I
 * @param c_i_len Length of C_I
 * @return 0 on success, negative errno on error
 */
int coap_edhoc_session_set_ci(
	struct coap_edhoc_session *session,
	const uint8_t *c_i,
	uint8_t c_i_len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_EDHOC_SESSION_H_ */
