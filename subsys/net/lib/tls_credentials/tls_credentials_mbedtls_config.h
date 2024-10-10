/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Internal helper file which allows support for MbedTLS features to be checked easily.
 */

#ifndef __TLS_CREDENTIALS_MBEDTLS_CONFIG_H
#define __TLS_CREDENTIALS_MBEDTLS_CONFIG_H

/* Grab mbedTLS headers if they are available so that we can check supported MbedTLS features. */

#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */
#endif /* CONFIG_MBEDTLS */

#endif /* __TLS_CREDENTIALS_MBEDTLS_CONFIG_H */
