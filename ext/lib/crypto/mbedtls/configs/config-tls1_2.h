/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * More complete mbedTLS configuration for TLS 1.2 (RFC 5246) for Zephyr,
 * extending config-mini-tls1_2.h.
 */

#ifndef MBEDTLS_CONFIG_TLS1_2_H
#define MBEDTLS_CONFIG_TLS1_2_H

#if 1

/* DHE config - slow but moderate code size impact (~5K x86) */
#define MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
#define MBEDTLS_DHM_C

#else

/* ECDHE config - faster but higher code size impact (~15K x86) */
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECP_C

#endif

#include <config-mini-tls1_2.h>

#endif /* MBEDTLS_CONFIG_TLS1_2_H */
