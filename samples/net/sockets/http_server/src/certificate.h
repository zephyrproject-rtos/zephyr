/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

enum tls_tag {
	/** Used for both the public and private server keys */
	HTTP_SERVER_CERTIFICATE_TAG,
	/* Used for pre-shared key */
	PSK_TAG,
};

static const unsigned char server_certificate[] = {
#include "server_cert.der.inc"
};

/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "server_privkey.der.inc"
};

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
#include CONFIG_NET_SAMPLE_PSK_HEADER_FILE
#endif

#endif /* __CERTIFICATE_H__ */
