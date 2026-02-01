/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Outer Block1 reassembly for EDHOC+OSCORE combined requests (RFC 9668 Section 3.3.2)
 *
 * This module implements RFC 9668 Section 3.3.2 "Step 0" processing:
 * When a combined request uses outer Block1, the server must reassemble
 * all blocks before proceeding with EDHOC+OSCORE processing.
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_COMBINED_BLOCKWISE_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_COMBINED_BLOCKWISE_H_

#include <zephyr/net/coap.h>
#include <zephyr/net/coap_service.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result codes for outer Block1 processing
 */
enum coap_edhoc_outer_block_result {
	/** Waiting for more blocks - 2.31 Continue response sent */
	COAP_EDHOC_OUTER_BLOCK_WAITING = 0,
	/** All blocks received - reassembled request ready */
	COAP_EDHOC_OUTER_BLOCK_COMPLETE = 1,
	/** Error occurred - error response sent */
	COAP_EDHOC_OUTER_BLOCK_ERROR = -1,
};

/**
 * @brief Process outer Block1 for EDHOC+OSCORE combined requests
 *
 * Implements RFC 9668 Section 3.3.2 Step 0: reassemble outer Block1
 * before processing the combined request.
 *
 * This function:
 * - Detects Block1 in combined requests (EDHOC option present or matching cache entry)
 * - Validates blockwise invariants per RFC 7959
 * - Enforces security limits (max payload, lifetime)
 * - Sends 2.31 Continue for intermediate blocks
 * - Reconstructs full request when last block received
 *
 * @param service CoAP service
 * @param request Current block request
 * @param buf Original request buffer
 * @param received Length of received data
 * @param client_addr Client address
 * @param client_addr_len Client address length
 * @param reconstructed_buf Output buffer for reconstructed request (when complete)
 * @param reconstructed_len Output length of reconstructed request (when complete)
 * @return COAP_EDHOC_OUTER_BLOCK_WAITING if waiting for more blocks,
 *         COAP_EDHOC_OUTER_BLOCK_COMPLETE if reassembly complete,
 *         COAP_EDHOC_OUTER_BLOCK_ERROR on error
 */
int coap_edhoc_outer_block_process(const struct coap_service *service,
				    struct coap_packet *request,
				    const uint8_t *buf,
				    size_t received,
				    const struct net_sockaddr *client_addr,
				    net_socklen_t client_addr_len,
				    uint8_t *reconstructed_buf,
				    size_t *reconstructed_len);

#if defined(CONFIG_ZTEST)
/**
 * @brief Test-only API: Find outer Block1 cache entry
 *
 * Exposed for unit tests to inspect cache state.
 */
struct coap_edhoc_outer_block_entry *
coap_edhoc_outer_block_find(struct coap_edhoc_outer_block_entry *cache,
			     size_t cache_size,
			     const struct net_sockaddr *addr,
			     net_socklen_t addr_len,
			     const uint8_t *token,
			     uint8_t tkl);

/**
 * @brief Test-only API: Clear outer Block1 cache entry
 *
 * Exposed for unit tests to reset cache state.
 */
void coap_edhoc_outer_block_clear(struct coap_edhoc_outer_block_entry *entry);
#endif /* CONFIG_ZTEST */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_COAP_EDHOC_COMBINED_BLOCKWISE_H_ */
