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
 * @brief Validate OSCORE option occurrence (RFC 8613 Section 2 + RFC 7252 Section 5.4.5)
 *
 * RFC 8613 Section 2: "The OSCORE option is critical... and not repeatable."
 * RFC 7252 Section 5.4.5: Non-repeatable options MUST NOT appear more than once;
 * each supernumerary occurrence MUST be treated like an unrecognized option.
 *
 * @param cpkt CoAP packet to validate
 * @param present Output parameter set to true if OSCORE option is present (exactly once)
 * @return 0 if valid (0 or 1 occurrence), -EBADMSG if repeated (>1 occurrence),
 *         negative errno for other errors
 */
int coap_oscore_validate_option(const struct coap_packet *cpkt, bool *present)
{
	struct coap_option options[2] = {0};
	int ret;

	if (cpkt == NULL || present == NULL) {
		return -EINVAL;
	}

	/* Try to find up to 2 OSCORE options to detect repetition
	 * Note: coap_find_options() may return -EINVAL if option parsing fails,
	 * which is different from finding multiple options. We only check for
	 * repetition if parsing succeeds.
	 */
	ret = coap_find_options(cpkt, COAP_OPTION_OSCORE, options, 2);

	if (ret < 0) {
		/* Error parsing options - propagate the error
		 * This could be due to malformed option encoding, not repetition
		 */
		*present = false;
		return ret;
	} else if (ret == 0) {
		/* No OSCORE option present */
		*present = false;
		return 0;
	} else if (ret == 1) {
		/* Exactly one OSCORE option - valid */
		*present = true;
		return 0;
	} else {
		/* ret > 1: Multiple OSCORE options detected
		 * RFC 7252 Section 5.4.5: Supernumerary occurrences of non-repeatable
		 * critical options MUST be treated like unrecognized options.
		 * RFC 7252 Section 5.4.1: Unrecognized critical options cause rejection.
		 */
		LOG_ERR("Multiple OSCORE options detected (%d occurrences), violates RFC 8613 Section 2",
			ret);
		*present = false;
		return -EBADMSG;
	}
}

/**
 * @brief Validate OSCORE message according to RFC 8613 Section 2
 *
 * RFC 8613 Section 2: "An endpoint receiving a CoAP message without payload
 * that also contains an OSCORE option SHALL treat it as malformed and reject it."
 *
 * RFC 8613 Section 2: "If the OSCORE flag bits are all zero (0x00), the option
 * value SHALL be empty (Option Length = 0)."
 *
 * @param cpkt CoAP packet to validate
 * @return 0 if valid, -EBADMSG if malformed
 */
int coap_oscore_validate_msg(const struct coap_packet *cpkt)
{
	struct coap_option option;
	uint16_t payload_len;
	const uint8_t *payload;
	bool has_oscore;
	int ret;

	/* RFC 8613 Section 2 + RFC 7252 Section 5.4.5: Validate OSCORE option occurrence */
	ret = coap_oscore_validate_option(cpkt, &has_oscore);
	if (ret < 0) {
		/* Multiple OSCORE options detected - reject per RFC 8613 Section 2 */
		return ret;
	}

	if (!has_oscore) {
		/* Not an OSCORE message, no validation needed */
		return 0;
	}

	/* Get the OSCORE option for further validation
	 * We know it exists and is unique from validation above
	 */
	ret = coap_find_options(cpkt, COAP_OPTION_OSCORE, &option, 1);
	if (ret != 1) {
		/* Should not happen since coap_oscore_validate_option() returned has_oscore=true */
		return -EINVAL;
	}

	/* RFC 8613 Section 2: If flags are all zero, option value must be empty */
	if (option.len > 0 && option.value[0] == 0x00) {
		LOG_ERR("OSCORE option with flags=0x00 must be empty (RFC 8613 Section 2)");
		return -EBADMSG;
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
 * Implements RFC 8613 Section 8.2 and Section 7.4 error code mapping:
 * - Decode/decompression/parse failures => 4.02 Bad Option (RFC 8613 §8.2 step 2 bullet 1)
 * - Security context not found => 4.01 Unauthorized (RFC 8613 §8.2 step 2 bullet 2)
 * - Replay protection failures => 4.01 Unauthorized (RFC 8613 §7.4)
 * - Decryption/integrity failures => 4.00 Bad Request (RFC 8613 §8.2 step 6)
 * - Unknown errors => 4.00 Bad Request (safe default)
 *
 * @param oscore_err Error code from uoscore library
 * @return CoAP response code
 */
static uint8_t oscore_err_to_coap_code(enum err oscore_err)
{
	switch (oscore_err) {
	case ok:
		return COAP_RESPONSE_CODE_OK;

	/* RFC 8613 Section 8.2 step 2 bullet 1: Decode/decompression/parse failures => 4.02 */
	case not_valid_input_packet:
	case oscore_inpkt_invalid_tkl:
	case oscore_inpkt_invalid_option_delta:
	case oscore_inpkt_invalid_optionlen:
	case oscore_inpkt_invalid_piv:
	case oscore_valuelen_to_long_error:
	case too_many_options:
	case cbor_decoding_error:
	case cbor_encoding_error:
		return COAP_RESPONSE_CODE_BAD_OPTION;

	/* RFC 8613 Section 8.2 step 2 bullet 2: Security context not found => 4.01 */
	case oscore_kid_recipient_id_mismatch:
		return COAP_RESPONSE_CODE_UNAUTHORIZED;

	/* RFC 8613 Section 7.4: Replay protection failures => 4.01 */
	case oscore_replay_window_protection_error:
	case oscore_replay_notification_protection_error:
	case first_request_after_reboot:
	case echo_validation_failed:
		return COAP_RESPONSE_CODE_UNAUTHORIZED;

	/* RFC 8613 Section 8.2 step 6: Decryption/integrity failures => 4.00 */
	/* All other errors default to 4.00 Bad Request (safe default) */
	default:
		return COAP_RESPONSE_CODE_BAD_REQUEST;
	}
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

#if defined(CONFIG_COAP_TEST_API_ENABLE)
/**
 * @brief Test-only helper to expose OSCORE error to CoAP code mapping
 *
 * This function is only available when CONFIG_COAP_TEST_API_ENABLE is set.
 * It allows unit tests to verify the RFC 8613 error code mapping without
 * needing to construct actual OSCORE packets.
 *
 * @param oscore_err uOSCORE error code
 * @return Mapped CoAP response code
 */
uint8_t coap_oscore_err_to_coap_code_for_test(enum err oscore_err)
{
	return oscore_err_to_coap_code(oscore_err);
}
#endif /* CONFIG_COAP_TEST_API_ENABLE */
