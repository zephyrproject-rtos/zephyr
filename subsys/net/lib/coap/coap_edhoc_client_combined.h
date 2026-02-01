/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal API for EDHOC+OSCORE combined request construction (RFC 9668)
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_CLIENT_COMBINED_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_CLIENT_COMBINED_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Construct EDHOC+OSCORE combined request from OSCORE-protected packet
 *
 * Per RFC 9668 Section 3.2.1, builds a combined request with:
 * - EDHOC option (21) with empty value
 * - COMB_PAYLOAD = EDHOC_MSG_3 || OSCORE_PAYLOAD
 *
 * @param oscore_pkt OSCORE-protected CoAP packet
 * @param oscore_pkt_len Length of OSCORE-protected packet
 * @param edhoc_msg3 EDHOC message_3 as CBOR bstr encoding
 * @param edhoc_msg3_len Length of EDHOC message_3
 * @param combined_buf Output buffer for combined request
 * @param combined_buf_size Size of output buffer
 * @param combined_len Output: length of combined request
 * @return 0 on success, negative errno on error
 *         -EMSGSIZE if combined payload exceeds MAX_UNFRAGMENTED_SIZE
 */
int coap_edhoc_client_build_combined_request(const uint8_t *oscore_pkt,
					      size_t oscore_pkt_len,
					      const uint8_t *edhoc_msg3,
					      size_t edhoc_msg3_len,
					      uint8_t *combined_buf,
					      size_t combined_buf_size,
					      size_t *combined_len);

/**
 * @brief Check if inner Block1 is first block (NUM == 0)
 *
 * Per RFC 9668 Section 3.2.2 Step 2.1, EDHOC option is only included
 * for the first inner Block1 (NUM == 0).
 *
 * @param plaintext_pkt Plaintext CoAP packet (before OSCORE protection)
 * @param plaintext_len Length of plaintext packet
 * @param is_first_block Output: true if first block or no Block1, false otherwise
 * @return 0 on success, negative errno on error
 */
int coap_edhoc_client_is_first_inner_block(const uint8_t *plaintext_pkt,
					    size_t plaintext_len,
					    bool *is_first_block);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_CLIENT_COMBINED_H_ */
