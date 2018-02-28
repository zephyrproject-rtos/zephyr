/**
 * @file
 * @brief TLS configuration API definitions
 *
 * Convenience configuration API for mbedTLS.
 */

/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mbedtls/ssl.h>

struct ztls_cert_key_pair {
	mbedtls_x509_crt cert;
	mbedtls_pk_context priv_key;
};


int ztls_get_tls_client_conf(mbedtls_ssl_config **out_conf);
int ztls_get_tls_server_conf(mbedtls_ssl_config **out_conf);

int ztls_conf_add_own_cert_key_pair(mbedtls_ssl_config *conf,
				    struct ztls_cert_key_pair *pair);

int ztls_parse_cert_key_pair(struct ztls_cert_key_pair *pair,
			     const unsigned char *cert,
			     size_t cert_len,
			     const unsigned char *priv_key,
			     size_t priv_key_len);
