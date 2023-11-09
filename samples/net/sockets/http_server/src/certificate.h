/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

enum tls_tag {
	/** The Certificate Authority public key */
	HTTP_SERVER_CA_CERTIFICATE_TAG,
	/** Used for both the public and private server keys */
	HTTP_SERVER_CERTIFICATE_TAG,
	/** Used for both the public and private client keys */
	HTTP_SERVER_CLIENT_CERTIFICATE_TAG,
	PSK_TAG,
};

#if !defined(CONFIG_NET_SAMPLE_CERTS_WITH_SC)
static const unsigned char server_certificate[] = {
#include "https-server-cert.der.inc"
};

/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "https-server-key.der.inc"
};

#else

static const unsigned char ca_certificate[] = {
#include "ca.der.inc"
};

static const unsigned char server_certificate[] = {
#include "server.der.inc"
};

/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "server_privkey.der.inc"
};
#endif

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
#include CONFIG_NET_SAMPLE_PSK_HEADER_FILE
#endif

#endif /* __CERTIFICATE_H__ */
