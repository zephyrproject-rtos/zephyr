/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/net/coap.h>

#ifdef CONFIG_COAP_OSCORE
#include "oscore/security_context.h"

/**
 * @brief Check if a CoAP message has the OSCORE option
 *
 * @param cpkt CoAP packet to check
 * @return true if OSCORE option is present, false otherwise
 */
bool coap_oscore_msg_has_oscore(const struct coap_packet *cpkt);

/**
 * @brief Validate OSCORE option occurrence (RFC 8613 Section 2 + RFC 7252 Section 5.4.5)
 *
 * RFC 8613 Section 2: "The OSCORE option is critical... and not repeatable."
 * RFC 7252 Section 5.4.5: Non-repeatable options MUST NOT appear more than once.
 *
 * @param cpkt CoAP packet to validate
 * @param present Output parameter set to true if OSCORE option is present (exactly once)
 * @return 0 if valid (0 or 1 occurrence), -EBADMSG if repeated (>1 occurrence)
 */
int coap_oscore_validate_option(const struct coap_packet *cpkt, bool *present);

/**
 * @brief Validate OSCORE message according to RFC 8613 Section 2
 *
 * @param cpkt CoAP packet to validate
 * @return 0 if valid, -EBADMSG if malformed
 */
int coap_oscore_validate_msg(const struct coap_packet *cpkt);

/**
 * @brief Protect a CoAP message with OSCORE (encrypt)
 *
 * @param coap_msg Input CoAP message buffer
 * @param coap_msg_len Length of input CoAP message
 * @param oscore_msg Output buffer for OSCORE-protected message
 * @param oscore_msg_len On input: size of output buffer; on output: length of protected message
 * @param ctx OSCORE security context
 * @return 0 on success, negative errno on error
 */
int coap_oscore_protect(const uint8_t *coap_msg, uint32_t coap_msg_len,
			uint8_t *oscore_msg, uint32_t *oscore_msg_len,
			struct context *ctx);

/**
 * @brief Verify and decrypt an OSCORE-protected message
 *
 * @param oscore_msg Input OSCORE-protected message buffer
 * @param oscore_msg_len Length of input OSCORE message
 * @param coap_msg Output buffer for decrypted CoAP message
 * @param coap_msg_len On input: size of output buffer; on output: length of decrypted message
 * @param ctx OSCORE security context
 * @param error_code If verification fails, this is set to the appropriate CoAP error code
 * @return 0 on success, negative errno on error
 */
int coap_oscore_verify(const uint8_t *oscore_msg, uint32_t oscore_msg_len,
		       uint8_t *coap_msg, uint32_t *coap_msg_len,
		       struct context *ctx, uint8_t *error_code);

/**
 * @brief Wrapper for OSCORE verification (can be overridden in tests)
 *
 * This is a weak symbol that tests can override to inject test behavior.
 * By default, it calls the real coap_oscore_verify implementation.
 */
int coap_oscore_verify_wrapper(const uint8_t *oscore_msg, uint32_t oscore_msg_len,
				uint8_t *coap_msg, uint32_t *coap_msg_len,
				struct context *ctx, uint8_t *error_code);

#if defined(CONFIG_COAP_TEST_API_ENABLE)
/* Forward declaration for OSCORE exchange structure */
struct coap_oscore_exchange;

/**
 * @brief Find OSCORE exchange entry (for testing)
 */
struct coap_oscore_exchange *oscore_exchange_find(
	struct coap_oscore_exchange *cache,
	const struct net_sockaddr *addr,
	net_socklen_t addr_len,
	const uint8_t *token,
	uint8_t tkl);

/**
 * @brief Add or update OSCORE exchange entry (for testing)
 */
int oscore_exchange_add(struct coap_oscore_exchange *cache,
			const struct net_sockaddr *addr,
			net_socklen_t addr_len,
			const uint8_t *token,
			uint8_t tkl,
			bool is_observe,
			struct context *oscore_ctx);

/**
 * @brief Remove OSCORE exchange entry (for testing)
 */
void oscore_exchange_remove(struct coap_oscore_exchange *cache,
			    const struct net_sockaddr *addr,
			    net_socklen_t addr_len,
			    const uint8_t *token,
			    uint8_t tkl);

/**
 * @brief Test-only helper to expose OSCORE error to CoAP code mapping
 *
 * This function allows unit tests to verify the RFC 8613 error code mapping
 * without needing to construct actual OSCORE packets.
 *
 * @param oscore_err uOSCORE error code (enum err)
 * @return Mapped CoAP response code
 */
uint8_t coap_oscore_err_to_coap_code_for_test(enum err oscore_err);
#endif /* CONFIG_COAP_TEST_API_ENABLE */

#endif /* CONFIG_COAP_OSCORE */

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_H_ */
