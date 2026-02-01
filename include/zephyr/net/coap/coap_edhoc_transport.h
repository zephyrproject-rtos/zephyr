/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDHOC over CoAP transport (RFC 9528 Appendix A.2)
 *
 * This file provides support for EDHOC message transfer over CoAP
 * as specified in RFC 9528 Appendix A.2 ("Transferring EDHOC over CoAP").
 */

#ifndef ZEPHYR_INCLUDE_NET_COAP_COAP_EDHOC_TRANSPORT_H_
#define ZEPHYR_INCLUDE_NET_COAP_COAP_EDHOC_TRANSPORT_H_

#include <zephyr/net/coap.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CoAP EDHOC Transport API
 * @defgroup coap_edhoc_transport CoAP EDHOC Transport API
 * @since 4.1
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/**
 * @brief Well-known EDHOC resource path
 *
 * Per RFC 9528 Appendix A.2, EDHOC messages are transferred via POST
 * requests to the /.well-known/edhoc resource.
 */
#define COAP_WELL_KNOWN_EDHOC_PATH \
	((const char * const[]) { ".well-known", "edhoc", NULL })

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_COAP_COAP_EDHOC_TRANSPORT_H_ */
