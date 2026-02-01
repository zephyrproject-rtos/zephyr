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

int coap_edhoc_validate_option(const struct coap_packet *cpkt, bool *present)
{
	struct coap_option option[2];
	int ret;

	if (cpkt == NULL || present == NULL) {
		return -EINVAL;
	}

	/* RFC 9668 Section 3.1: EDHOC option MUST occur at most once.
	 * RFC 7252 Section 5.4.5: Supernumerary occurrences MUST be treated
	 * as unrecognized critical options.
	 */
	ret = coap_find_options(cpkt, COAP_OPTION_EDHOC, option, 2);
	if (ret < 0) {
		return ret;
	}

	if (ret == 0) {
		/* No EDHOC option present */
		*present = false;
		return 0;
	}

	/* At least one EDHOC option is present */
	*present = true;

	if (ret > 1) {
		/* Multiple EDHOC options - RFC 7252 Section 5.4.5 violation */
		LOG_ERR("Multiple EDHOC options present (%d), violates RFC 9668 Section 3.1", ret);
		return -EBADMSG;
	}

	/* Exactly one EDHOC option present - valid
	 * Note: Option value is ignored per RFC 9668 Section 3.1
	 */
	return 0;
}

bool coap_edhoc_msg_has_edhoc(const struct coap_packet *cpkt)
{
	struct coap_option option[2];
	int ret;

	if (cpkt == NULL) {
		return false;
	}

	/* RFC 9668 Section 3.1: EDHOC option MUST occur at most once and MUST be empty.
	 * If any value is sent, the recipient MUST ignore it.
	 *
	 * This function returns true if at least one EDHOC option is present,
	 * even if repeated (validation is done separately by coap_edhoc_validate_option).
	 */
	ret = coap_find_options(cpkt, COAP_OPTION_EDHOC, option, 2);
	
	/* Return true if at least one EDHOC option is present (value is ignored per RFC 9668) */
	return ret >= 1;
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

int coap_edhoc_encode_error(int err_code, const char *diag_msg,
			     uint8_t *out_buf, size_t *inout_len)
{
	if (out_buf == NULL || inout_len == NULL || diag_msg == NULL) {
		return -EINVAL;
	}

	size_t diag_len = strlen(diag_msg);
	size_t offset = 0;
	size_t tstr_header_len;

	/* RFC 9528 Section 6: error = (ERR_CODE : int, ERR_INFO : any)
	 * This is a CBOR Sequence (concatenation of two CBOR data items).
	 *
	 * First item: ERR_CODE as CBOR integer
	 * Second item: ERR_INFO as CBOR text string (tstr)
	 */

	/* Encode ERR_CODE as CBOR integer
	 * For err_code = 1, this is a single byte: 0x01
	 * CBOR encoding for small positive integers (0-23): major type 0, value in low 5 bits
	 */
	if (err_code < 0 || err_code > 23) {
		/* For simplicity, only support error codes 0-23 (single-byte encoding) */
		LOG_ERR("Error code %d not supported (must be 0-23)", err_code);
		return -EINVAL;
	}

	/* Calculate required buffer size based on diagnostic message length */
	if (diag_len < 24) {
		tstr_header_len = 1;
	} else if (diag_len < 256) {
		tstr_header_len = 2;
	} else if (diag_len < 65536) {
		tstr_header_len = 3;
	} else {
		LOG_ERR("Diagnostic message too long (%zu bytes)", diag_len);
		return -EINVAL;
	}

	if (*inout_len < 1 + tstr_header_len + diag_len) {
		LOG_ERR("Buffer too small (%zu < %zu)", *inout_len, 1 + tstr_header_len + diag_len);
		return -ENOMEM;
	}

	/* Encode ERR_CODE: CBOR major type 0 (unsigned integer), value in low 5 bits */
	out_buf[offset++] = (uint8_t)err_code;

	/* Encode ERR_INFO as CBOR text string (major type 3) */
	if (diag_len < 24) {
		/* Length 0-23 encoded in additional info */
		out_buf[offset++] = 0x60 | (uint8_t)diag_len;
	} else if (diag_len < 256) {
		/* 1-byte length follows */
		out_buf[offset++] = 0x78;
		out_buf[offset++] = (uint8_t)diag_len;
	} else {
		/* 2-byte length follows (big-endian) */
		out_buf[offset++] = 0x79;
		out_buf[offset++] = (uint8_t)(diag_len >> 8);
		out_buf[offset++] = (uint8_t)(diag_len & 0xFF);
	}

	/* Copy diagnostic message */
	memcpy(out_buf + offset, diag_msg, diag_len);
	offset += diag_len;

	*inout_len = offset;
	return 0;
}
