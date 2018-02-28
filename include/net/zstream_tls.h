/**
 * @file
 * @brief TLS stream API definitions
 *
 * Implementation of stream wrapper for mbedTLS.
 */

/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_ZSTREAM_TLS_H
#define __NET_ZSTREAM_TLS_H

#if defined(CONFIG_MBEDTLS)

#include <sys/types.h>
#include <stdbool.h>

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"

#include <net/zstream.h>

/* Stream object implementation for TLS adapter. */
struct zstream_tls {
	const struct zstream_api *api;

	/* Wrapped stream */
	struct zstream *sock;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt ca_cert;
	mbedtls_x509_crt srv_cert;
	mbedtls_pk_context srv_priv_key;
};

int zstream_tls_init(struct zstream_tls *self, struct zstream *sock,
		     mbedtls_ssl_config *conf, const char *hostname);

#endif /* CONFIG_MBEDTLS */

#endif /* __NET_ZSTREAM_TLS_H */
