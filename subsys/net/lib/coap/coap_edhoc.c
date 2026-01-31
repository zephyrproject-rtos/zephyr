/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/coap.h>
#include "coap_edhoc.h"

#include <errno.h>
#include <string.h>

bool coap_edhoc_msg_has_edhoc(const struct coap_packet *cpkt)
{
	struct coap_option option[2];
	int ret;

	if (cpkt == NULL) {
		return false;
	}

	/* RFC 9668 Section 3.1: EDHOC option MUST occur at most once and MUST be empty.
	 * If any value is sent, the recipient MUST ignore it.
	 */
	ret = coap_find_options(cpkt, COAP_OPTION_EDHOC, option, 2);
	
	/* If more than one EDHOC option is present, treat as malformed */
	if (ret > 1) {
		LOG_ERR("Multiple EDHOC options present (%d), violates RFC 9668 Section 3.1", ret);
		return false;
	}

	/* Return true if exactly one EDHOC option is present (value is ignored per RFC 9668) */
	return ret == 1;
}

int coap_edhoc_split_comb_payload(const uint8_t *payload, size_t payload_len,
				   struct coap_edhoc_span *edhoc_msg3,
				   struct coap_edhoc_span *oscore_payload)
{
	if (payload == NULL || edhoc_msg3 == NULL || oscore_payload == NULL) {
		return -EINVAL;
	}

	if (payload_len == 0) {
		return -EINVAL;
	}

	/* RFC 9668 Section 3.2.1: COMB_PAYLOAD = EDHOC_MSG_3 / OSCORE_PAYLOAD
	 * EDHOC_MSG_3 is a CBOR bstr (byte string).
	 *
	 * CBOR encoding for byte strings (major type 2):
	 * - 0x40-0x57: length 0-23 in lower 5 bits (1 byte header)
	 * - 0x58: 1-byte length follows (2 byte header)
	 * - 0x59: 2-byte length follows (3 byte header)
	 * - 0x5a: 4-byte length follows (5 byte header)
	 * - 0x5b: 8-byte length follows (9 byte header)
	 */
	uint8_t initial_byte = payload[0];
	uint8_t major_type = (initial_byte >> 5) & 0x07;
	uint8_t additional_info = initial_byte & 0x1f;
	size_t header_len;
	size_t data_len;

	/* Check if it's a byte string (major type 2) */
	if (major_type != 2) {
		LOG_ERR("EDHOC_MSG_3 must be CBOR byte string (major type 2), got %d",
			major_type);
		return -EINVAL;
	}

	/* Parse length encoding */
	if (additional_info < 24) {
		/* Length 0-23 encoded in additional info */
		header_len = 1;
		data_len = additional_info;
	} else if (additional_info == 24) {
		/* 1-byte length follows */
		if (payload_len < 2) {
			return -EINVAL;
		}
		header_len = 2;
		data_len = payload[1];
	} else if (additional_info == 25) {
		/* 2-byte length follows (big-endian) */
		if (payload_len < 3) {
			return -EINVAL;
		}
		header_len = 3;
		data_len = ((uint16_t)payload[1] << 8) | payload[2];
	} else if (additional_info == 26) {
		/* 4-byte length follows (big-endian) */
		if (payload_len < 5) {
			return -EINVAL;
		}
		header_len = 5;
		data_len = ((uint32_t)payload[1] << 24) |
			   ((uint32_t)payload[2] << 16) |
			   ((uint32_t)payload[3] << 8) |
			   payload[4];
	} else if (additional_info == 27) {
		/* 8-byte length follows - reject as unreasonably large */
		LOG_ERR("EDHOC_MSG_3 length encoding too large (8-byte length)");
		return -EINVAL;
	} else {
		/* Reserved or invalid */
		LOG_ERR("Invalid CBOR additional info: %d", additional_info);
		return -EINVAL;
	}

	/* Check if the entire EDHOC_MSG_3 fits in the payload */
	size_t edhoc_msg3_total_len = header_len + data_len;
	if (edhoc_msg3_total_len > payload_len) {
		LOG_ERR("EDHOC_MSG_3 length (%zu) exceeds payload (%zu)",
			edhoc_msg3_total_len, payload_len);
		return -EINVAL;
	}

	/* Set output spans */
	edhoc_msg3->ptr = payload;
	edhoc_msg3->len = edhoc_msg3_total_len;

	oscore_payload->ptr = payload + edhoc_msg3_total_len;
	oscore_payload->len = payload_len - edhoc_msg3_total_len;

	/* RFC 9668 requires both EDHOC_MSG_3 and OSCORE_PAYLOAD to be present */
	if (oscore_payload->len == 0) {
		LOG_ERR("OSCORE_PAYLOAD missing in combined payload");
		return -EINVAL;
	}

	return 0;
}

int coap_edhoc_remove_option(struct coap_packet *cpkt)
{
	if (cpkt == NULL) {
		return -EINVAL;
	}

	/* Use existing CoAP option removal function */
	return coap_packet_remove_option(cpkt, COAP_OPTION_EDHOC);
}
