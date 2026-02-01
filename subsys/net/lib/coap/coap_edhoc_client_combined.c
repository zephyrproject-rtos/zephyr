/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDHOC+OSCORE combined request construction for CoAP client (RFC 9668)
 *
 * Implements RFC 9668 Section 3.2.1 client-side combined request construction.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client.h>
#include "coap_edhoc.h"
#include <errno.h>
#include <string.h>

/**
 * @brief Construct EDHOC+OSCORE combined request from OSCORE-protected packet
 *
 * Per RFC 9668 Section 3.2.1, this function:
 * 1. Takes an already OSCORE-protected CoAP packet
 * 2. Extracts the OSCORE payload (ciphertext)
 * 3. Builds COMB_PAYLOAD = EDHOC_MSG_3 || OSCORE_PAYLOAD
 * 4. Constructs a new outer CoAP message with:
 *    - Same header fields (ver/type/tkl/token/code/MID)
 *    - All existing outer options from OSCORE-protected packet
 *    - EDHOC option (21) with empty value (added in correct numeric order)
 *    - Combined payload
 *
 * RFC 9668 Section 3.2.2: Block-wise constraints:
 * - Only applies to first inner Block1 (NUM == 0)
 * - If COMB_PAYLOAD exceeds MAX_UNFRAGMENTED_SIZE, returns -EMSGSIZE
 *
 * @param oscore_pkt OSCORE-protected CoAP packet
 * @param oscore_pkt_len Length of OSCORE-protected packet
 * @param edhoc_msg3 EDHOC message_3 as CBOR bstr encoding
 * @param edhoc_msg3_len Length of EDHOC message_3
 * @param combined_buf Output buffer for combined request
 * @param combined_buf_size Size of output buffer
 * @param combined_len Output: length of combined request
 * @return 0 on success, negative errno on error
 */
int coap_edhoc_client_build_combined_request(const uint8_t *oscore_pkt,
					      size_t oscore_pkt_len,
					      const uint8_t *edhoc_msg3,
					      size_t edhoc_msg3_len,
					      uint8_t *combined_buf,
					      size_t combined_buf_size,
					      size_t *combined_len)
{
	struct coap_packet oscore_cpkt;
	struct coap_packet combined_cpkt;
	uint16_t oscore_payload_len;
	const uint8_t *oscore_payload;
	int ret;
	uint8_t ver, type, tkl, code;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;

	if (oscore_pkt == NULL || edhoc_msg3 == NULL || combined_buf == NULL ||
	    combined_len == NULL) {
		return -EINVAL;
	}

	/* Parse OSCORE-protected packet to extract header and payload */
	ret = coap_packet_parse(&oscore_cpkt, (uint8_t *)oscore_pkt, oscore_pkt_len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to parse OSCORE-protected packet (%d)", ret);
		return ret;
	}

	/* Get OSCORE payload (ciphertext) */
	oscore_payload = coap_packet_get_payload(&oscore_cpkt, &oscore_payload_len);
	if (oscore_payload == NULL || oscore_payload_len == 0) {
		LOG_ERR("OSCORE-protected packet has no payload");
		return -EINVAL;
	}

	/* RFC 9668 Section 3.2.2 Step 3.1: Check MAX_UNFRAGMENTED_SIZE constraint */
	size_t combined_payload_len = edhoc_msg3_len + oscore_payload_len;
	if (combined_payload_len > CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE) {
		LOG_ERR("Combined payload (%zu) exceeds MAX_UNFRAGMENTED_SIZE (%d)",
			combined_payload_len, CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE);
		return -EMSGSIZE;
	}

	/* Extract header fields from OSCORE-protected packet */
	ver = coap_header_get_version(&oscore_cpkt);
	type = coap_header_get_type(&oscore_cpkt);
	tkl = coap_header_get_token(&oscore_cpkt, token);
	code = coap_header_get_code(&oscore_cpkt);
	id = coap_header_get_id(&oscore_cpkt);

	/* Initialize combined packet with same header */
	ret = coap_packet_init(&combined_cpkt, combined_buf, combined_buf_size,
			       ver, type, tkl, token, code, id);
	if (ret < 0) {
		LOG_ERR("Failed to init combined packet (%d)", ret);
		return ret;
	}

	/* Copy all outer options from OSCORE-protected packet, then add EDHOC option
	 * RFC 9668 Section 3.2.1 Step 5: Include EDHOC option (empty)
	 * RFC 7252: Options must be added in numeric order
	 *
	 * Strategy: Use coap_find_options to iterate through all options,
	 * copy them in order, and insert EDHOC option at the correct position
	 */
	struct coap_option options[16];
	int num_options;
	bool edhoc_added = false;

	/* Find all options in OSCORE packet */
	num_options = coap_find_options(&oscore_cpkt, 0, options, ARRAY_SIZE(options));
	if (num_options < 0) {
		LOG_ERR("Failed to find options (%d)", num_options);
		return num_options;
	}

	/* Copy options in order, inserting EDHOC option at correct position */
	uint16_t current_opt = 0;
	for (int i = 0; i < num_options; i++) {
		uint16_t opt_num = current_opt + options[i].delta;
		current_opt = opt_num;

		/* Insert EDHOC option before any option with number > 21 */
		if (!edhoc_added && opt_num > COAP_OPTION_EDHOC) {
			ret = coap_packet_append_option(&combined_cpkt, COAP_OPTION_EDHOC,
							NULL, 0);
			if (ret < 0) {
				LOG_ERR("Failed to append EDHOC option (%d)", ret);
				return ret;
			}
			edhoc_added = true;
		}

		/* Copy option from OSCORE packet */
		ret = coap_packet_append_option(&combined_cpkt, opt_num,
						options[i].value, options[i].len);
		if (ret < 0) {
			LOG_ERR("Failed to copy option %u (%d)", opt_num, ret);
			return ret;
		}
	}

	/* If EDHOC option wasn't added yet (all options <= 21), add it now */
	if (!edhoc_added) {
		ret = coap_packet_append_option(&combined_cpkt, COAP_OPTION_EDHOC, NULL, 0);
		if (ret < 0) {
			LOG_ERR("Failed to append EDHOC option (%d)", ret);
			return ret;
		}
	}

	/* RFC 9668 Section 3.2.1 Step 3: Set payload to EDHOC_MSG_3 || OSCORE_PAYLOAD */
	ret = coap_packet_append_payload_marker(&combined_cpkt);
	if (ret < 0) {
		LOG_ERR("Failed to append payload marker (%d)", ret);
		return ret;
	}

	/* Append EDHOC_MSG_3 first */
	ret = coap_packet_append_payload(&combined_cpkt, edhoc_msg3, edhoc_msg3_len);
	if (ret < 0) {
		LOG_ERR("Failed to append EDHOC_MSG_3 (%d)", ret);
		return ret;
	}

	/* Append OSCORE_PAYLOAD */
	ret = coap_packet_append_payload(&combined_cpkt, oscore_payload, oscore_payload_len);
	if (ret < 0) {
		LOG_ERR("Failed to append OSCORE payload (%d)", ret);
		return ret;
	}

	*combined_len = combined_cpkt.offset;

	LOG_DBG("Built combined request: EDHOC_MSG_3 (%zu bytes) + OSCORE (%u bytes) = %zu bytes",
		edhoc_msg3_len, oscore_payload_len, *combined_len);

	return 0;
}

/**
 * @brief Check if inner Block1 is first block (NUM == 0)
 *
 * Per RFC 9668 Section 3.2.2 Step 2.1, EDHOC option is only included
 * for the first inner Block1 (NUM == 0).
 *
 * This function checks if the plaintext CoAP request (before OSCORE protection)
 * has a Block1 option with NUM == 0, or no Block1 option at all (treated as NUM == 0).
 *
 * @param plaintext_pkt Plaintext CoAP packet (before OSCORE protection)
 * @param plaintext_len Length of plaintext packet
 * @param is_first_block Output: true if first block or no Block1, false otherwise
 * @return 0 on success, negative errno on error
 */
int coap_edhoc_client_is_first_inner_block(const uint8_t *plaintext_pkt,
					    size_t plaintext_len,
					    bool *is_first_block)
{
	struct coap_packet cpkt;
	int ret;

	if (plaintext_pkt == NULL || is_first_block == NULL) {
		return -EINVAL;
	}

	ret = coap_packet_parse(&cpkt, (uint8_t *)plaintext_pkt, plaintext_len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to parse plaintext packet (%d)", ret);
		return ret;
	}

	/* Check for Block1 option */
	bool has_more;
	uint32_t block_number;
	ret = coap_get_block1_option(&cpkt, &has_more, &block_number);
	if (ret < 0) {
		/* No Block1 option - treat as first block (NUM == 0) */
		*is_first_block = true;
		return 0;
	}

	/* Block1 option present - check if NUM == 0 */
	*is_first_block = (block_number == 0);

	return 0;
}
