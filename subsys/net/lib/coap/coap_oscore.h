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

#endif /* CONFIG_COAP_OSCORE */

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_H_ */
