/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDHOC support for CoAP (RFC 9668)
 *
 * This file provides helper functions for EDHOC+OSCORE combined requests
 * as specified in RFC 9668.
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_H_

#include <zephyr/net/coap.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Span structure for referencing a byte range
 */
struct coap_edhoc_span {
	const uint8_t *ptr;  /**< Pointer to data */
	size_t len;          /**< Length of data */
};

/**
 * @brief Check if a CoAP packet has the EDHOC option (21)
 *
 * Per RFC 9668 Section 3.1, the EDHOC option MUST be empty. If any value
 * is sent, the recipient MUST ignore it.
 *
 * @param cpkt CoAP packet to check
 * @return true if EDHOC option is present, false otherwise
 */
bool coap_edhoc_msg_has_edhoc(const struct coap_packet *cpkt);

/**
 * @brief Split EDHOC+OSCORE combined payload into EDHOC message_3 and OSCORE payload
 *
 * Per RFC 9668 Section 3.2.1, the combined payload format is:
 *   COMB_PAYLOAD = EDHOC_MSG_3 / OSCORE_PAYLOAD
 *
 * Where EDHOC_MSG_3 is a CBOR bstr (byte string). This function parses the
 * first CBOR data item to extract its exact byte length.
 *
 * @param payload Combined payload buffer
 * @param payload_len Length of combined payload
 * @param edhoc_msg3 Output span for EDHOC message_3 (includes CBOR encoding)
 * @param oscore_payload Output span for OSCORE payload
 * @return 0 on success, negative errno on error
 *         -EINVAL if payload cannot be split (malformed CBOR)
 */
int coap_edhoc_split_comb_payload(const uint8_t *payload, size_t payload_len,
				   struct coap_edhoc_span *edhoc_msg3,
				   struct coap_edhoc_span *oscore_payload);

/**
 * @brief Remove EDHOC option from a CoAP packet
 *
 * Per RFC 9668 Section 3.3.1 Step 7, the EDHOC option must be removed
 * before OSCORE verification.
 *
 * @param cpkt CoAP packet to modify
 * @return 0 on success, negative errno on error
 */
int coap_edhoc_remove_option(struct coap_packet *cpkt);

/**
 * @brief Encode an EDHOC error message as a CBOR Sequence
 *
 * Per RFC 9528 Section 6, an EDHOC error message is a CBOR Sequence:
 *   error = (ERR_CODE : int, ERR_INFO : any)
 *
 * Per RFC 9528 Section 6.2, for ERR_CODE = 1 (Unspecified Error),
 * ERR_INFO MUST be a tstr (text string).
 *
 * This function encodes the error message for use in CoAP error responses
 * per RFC 9668 Section 3.3.1 and RFC 9528 Appendix A.2.3.
 *
 * @param err_code EDHOC error code (e.g., 1 for Unspecified Error)
 * @param diag_msg Diagnostic message as a null-terminated string (ERR_INFO)
 * @param out_buf Output buffer for the CBOR Sequence
 * @param inout_len Input: size of out_buf; Output: actual encoded length
 * @return 0 on success, negative errno on error
 *         -EINVAL if parameters are invalid
 *         -ENOMEM if buffer is too small
 */
int coap_edhoc_encode_error(int err_code, const char *diag_msg,
			     uint8_t *out_buf, size_t *inout_len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_H_ */
