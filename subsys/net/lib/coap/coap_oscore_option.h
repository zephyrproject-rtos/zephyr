/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OSCORE option parsing helpers
 *
 * Minimal OSCORE option value parser for extracting kid (OSCORE Sender ID).
 * Per RFC 8613 Section 6.1 and RFC 9668 Section 3.3.1 Step 3.
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_OPTION_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_OPTION_H_

#include <zephyr/net/coap.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extract OSCORE Sender ID (kid) from OSCORE option value
 *
 * Per RFC 8613 Section 6.1, the OSCORE option value uses a custom binary format:
 *   - Flag byte with bits: n (Partial IV present), k (kid present), h (kid context present)
 *   - Partial IV (length-prefixed, if n=1)
 *   - kid context (length-prefixed, if h=1)
 *   - kid (length-prefixed, if k=1)
 *
 * This function extracts the 'kid' field which is used as C_R in
 * RFC 9668 Section 3.3.1 Step 3.
 *
 * @param cpkt CoAP packet containing OSCORE option
 * @param kid Output buffer for kid bytes
 * @param kid_len On input: size of kid buffer; on output: actual kid length
 * @return 0 on success, negative errno on error
 *         -ENOENT if OSCORE option not present or kid not present
 *         -EINVAL if OSCORE option value is malformed
 *         -ENOMEM if kid buffer too small
 */
int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
				   uint8_t *kid, size_t *kid_len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_OSCORE_OPTION_H_ */
