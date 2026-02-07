/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 *
 * @brief OSCORE/EDHOC encryption for Zephyr's CoAP server using uoscore-uedhoc.
 */

#ifndef ZEPHYR_INCLUDE_NET_COAP_EDHOC_OSCORE_H_
#define ZEPHYR_INCLUDE_NET_COAP_EDHOC_OSCORE_H_

#include <zephyr/net/net_if.h>
#include <zephyr/net/coap.h>
#include <oscore.h>
#include <edhoc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum length of OSCORE master secret
 */
#define EDHOC_OSCORE_MASTER_SECRET_MAX_LEN     16

/**
 * Maximum length of OSCORE master salt
 */
#define EDHOC_OSCORE_MASTER_SALT_MAX_LEN       8

/**
 * OSCORE/EDHOC context containing protocol state and secrets
 *
 * @note Only responder role is currently supported.
 */
struct edhoc_oscore_ctx {
	struct context oscore_ctx; /**< OSCORE context */
	struct other_party_cred cred_initiator; /**< EDHOC credentials of remote initiator */
	struct edhoc_responder_context edhoc_ctx; /**< EDHOC context */
};

/**
 * @brief Register the CoAP service's EDHOC context with the EDHOC server
 * @param ctx EDHOC context to register with this instance
 * @returns 0 on success, negative error code on failure
 */
int edhoc_register_ctx(struct edhoc_oscore_ctx *ctx);

/**
 * @brief Get EDHOC payload to be transmitted over the wire from the EDHOC server
 * @param ctx EDHOC context
 * @param data pointer to EDHOC payload
 * @param data_len length of payload
 * @returns 0 on success, negative error code on failure
 * @note The queue length is 1, so only one packet can be in the queue at once
 */
int edhoc_tx_dequeue(struct edhoc_oscore_ctx *ctx, uint8_t *data, uint16_t *data_len);

/**
 * @brief Submit EDHOC payload that was received from the wire to the EDHOC server
 * @param ctx EDHOC context
 * @param data pointer to EDHOC payload
 * @param data_len length of payload
 * @returns 0 on success, negative error code on failure
 * @note The queue length is 1, so only one packet can be in the queue at once
 */
int edhoc_rx_enqueue(struct edhoc_oscore_ctx *ctx, const uint8_t *data, const uint16_t data_len);

/**
 * @brief Check if a CoAP packet has OSCORE option set
 * @param cpkt CoAP packet to check
 * @returns true if the packet is OSCORE protected, false otherwise
 */
static inline bool packet_is_oscore(const struct coap_packet *cpkt)
{
	/* RFC8613 Section 2 */
	return coap_get_option_int(cpkt, COAP_OPTION_OSCORE) >= 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_COAP_EDHOC_OSCORE_H_ */
