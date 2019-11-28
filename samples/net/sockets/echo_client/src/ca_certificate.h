/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CA_CERTIFICATE_H__
#define __CA_CERTIFICATE_H__

#define CA_CERTIFICATE_TAG 1
#define PSK_TAG 2

#define TLS_PEER_HOSTNAME "localhost"

/* This is the same cert as what is found in net-tools/echo-apps-cert.pem file
 */
static const unsigned char ca_certificate[] = {
#include "echo-apps-cert.der.inc"
};

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
#include CONFIG_NET_SAMPLE_PSK_HEADER_FILE
#endif

#endif /* __CA_CERTIFICATE_H__ */
