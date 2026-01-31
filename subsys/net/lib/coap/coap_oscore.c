/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coap_oscore, CONFIG_COAP_LOG_LEVEL);

#include <errno.h>
#include <string.h>
#include <zephyr/net/coap.h>

#include "oscore.h"
#include "common/oscore_edhoc_error.h"

/**
 * @brief Check if a CoAP message has the OSCORE option
 *
 * @param cpkt CoAP packet to check
 * @return true if OSCORE option is present, false otherwise
 */
bool coap_oscore_msg_has_oscore(const struct coap_packet *cpkt)
{
	struct coap_option option;
	int ret;

	ret = coap_find_options(cpkt, COAP_OPTION_OSCORE, &option, 1);
	return ret > 0;
}

/**
 * @brief Validate OSCORE message according to RFC 8613 Section 2
 *
 * RFC 8613 Section 2: "An endpoint receiving a CoAP message without payload
 * that also contains an OSCORE option SHALL treat it as malformed and reject it."
 *
 * @param cpkt CoAP packet to validate
 * @return 0 if valid, -EBADMSG if malformed
 */
int coap_oscore_validate_msg(const struct coap_packet *cpkt)
{
	uint16_t payload_len;
	const uint8_t *payload;

	if (!coap_oscore_msg_has_oscore(cpkt)) {
		/* Not an OSCORE message, no validation needed */
		return 0;
	}

	payload = coap_packet_get_payload(cpkt, &payload_len);
	if (payload == NULL || payload_len == 0) {
		/* RFC 8613 Section 2: OSCORE option present without payload is malformed */
		LOG_ERR("OSCORE message without payload is malformed (RFC 8613 Section 2)");
		return -EBADMSG;
	}

	return 0;
}

/**
 * @brief Map uoscore error codes to CoAP response codes
 *
 * Based on RFC 8613 Section 8.2:
 * - COSE decode/decompression fail => may respond 4.02 Bad Option
 * - Security context not found => may respond 4.01 Unauthorized
 * - Decryption fail => must stop processing; may respond 4.00 Bad Request
 *
 * @param oscore_err Error code from uoscore library
 * @return CoAP response code
 */
static uint8_t oscore_err_to_coap_code(enum err oscore_err)
{
	/* Since we don't have access to all uoscore error codes at compile time,
	 * we use a simple mapping: ok => success, anything else => Bad Request.
	 * The uoscore library will log specific error details.
	 */
	if (oscore_err == ok) {
		return COAP_RESPONSE_CODE_OK;
	}

	/* For any error, return Bad Request as the default per RFC 8613 Section 8.2 */
	return COAP_RESPONSE_CODE_BAD_REQUEST;
}

/**
 * @brief Protect a CoAP message with OSCORE (encrypt)
 *
 * Implements RFC 8613 Section 8.1 (Protecting the Request) and
 * Section 8.3 (Protecting the Response).
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
			struct context *ctx)
{
	enum err result;

	if (coap_msg == NULL || oscore_msg == NULL || oscore_msg_len == NULL || ctx == NULL) {
		return -EINVAL;
	}

	/* Call uoscore coap2oscore to encrypt the message */
	result = coap2oscore((uint8_t *)coap_msg, coap_msg_len,
			     oscore_msg, oscore_msg_len, ctx);

	if (result != ok) {
		LOG_ERR("OSCORE protection failed: %d", result);
		return -EACCES;
	}

	LOG_DBG("OSCORE protected message: %u -> %u bytes", coap_msg_len, *oscore_msg_len);
	return 0;
}

/**
 * @brief Verify and decrypt an OSCORE-protected message
 *
 * Implements RFC 8613 Section 8.2 (Verifying the Request) and
 * Section 8.4 (Verifying the Response).
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
		       struct context *ctx, uint8_t *error_code)
{
	enum err result;

	if (oscore_msg == NULL || coap_msg == NULL || coap_msg_len == NULL || ctx == NULL) {
		if (error_code != NULL) {
			*error_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		}
		return -EINVAL;
	}

	/* Call uoscore oscore2coap to decrypt and verify the message */
	result = oscore2coap((uint8_t *)oscore_msg, oscore_msg_len,
			     coap_msg, coap_msg_len, ctx);

	if (result != ok) {
		LOG_ERR("OSCORE verification failed: %d", result);
		if (error_code != NULL) {
			*error_code = oscore_err_to_coap_code(result);
		}
		return -EACCES;
	}

	LOG_DBG("OSCORE verified message: %u -> %u bytes", oscore_msg_len, *coap_msg_len);
	return 0;
}

/**
 * @brief Wrapper for OSCORE verification (weak symbol for test overrides)
 *
 * This allows tests to intercept OSCORE verification calls without modifying
 * the production code. By default, it just calls the real implementation.
 */
__weak int coap_oscore_verify_wrapper(const uint8_t *oscore_msg, uint32_t oscore_msg_len,
				       uint8_t *coap_msg, uint32_t *coap_msg_len,
				       struct context *ctx, uint8_t *error_code)
{
	return coap_oscore_verify(oscore_msg, oscore_msg_len, coap_msg, coap_msg_len,
				  ctx, error_code);
}
