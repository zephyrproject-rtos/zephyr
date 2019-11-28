/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#define SERVER_CERTIFICATE_TAG 1
#define PSK_TAG 2

static const unsigned char server_certificate[] = {
#include "echo-apps-cert.der.inc"
};

/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "echo-apps-key.der.inc"
};

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
#include CONFIG_NET_SAMPLE_PSK_HEADER_FILE
#endif

#endif /* __CERTIFICATE_H__ */
