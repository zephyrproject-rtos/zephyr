/** @file
 * @brief Network TLS definitions
 *
 * An API for applications to configure TLS credentials.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_TLS_H
#define __NET_TLS_H

/**
 * @brief Network TLS management
 * @defgroup net_tls Network TLS management
 * @ingroup networking
 * @{
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** TLS credential type */
enum net_tls_credential_type {
	NET_TLS_CREDENTIAL_UNUSED,
	NET_TLS_CREDENTIAL_CA_CERTIFICATE,
	NET_TLS_CREDENTIAL_PSK,
	NET_TLS_CREDENTIAL_PSK_ID
};

/** Secure tag, a reference for TLS credential */
typedef int sec_tag_t;

#if defined(CONFIG_NET_TLS) || defined(CONFIG_NET_DTLS)

/**
 * @brief Add a TLS credential.
 *
 * @details This function adds a TLS credential, that can be used
 *          by TLS/DTLS for authentication.
 *
 * @param tag     A security tag that credential will be referenced with.
 * @param type    A TLS/DTLS credential type.
 * @param cred    A TLS/DTLS credential.
 * @param credlen A TLS/DTLS credential length.
 *
 * @return 0 if ok, < 0 if error.
 */
int net_tls_credential_add(int tag, enum net_tls_credential_type type,
			   const void *cred, size_t credlen);

/**
 * @brief Get a TLS credential.
 *
 * @details This function gets an already registered TLS credential,
 *          referenced by @p tag secure tag of @p type.
 *
 * @param tag     A security tag of requested credential.
 * @param type    A TLS/DTLS credential type of requested credential.
 * @param cred    A TLS/DTLS credential.
 * @param credlen A TLS/DTLS credential length.
 *
 * @return 0 if ok, < 0 if error.
 */
int net_tls_credential_get(int tag, enum net_tls_credential_type type,
			   void *cred, size_t *credlen);

/**
 * @brief Delete a TLS credential.
 *
 * @details This function removes a TLS credential, referenced by @p tag
 *          secure tag of @p type.
 *
 * @param tag  A security tag corresponding to removed credential.
 * @param type A TLS/DTLS credential type of removed credential.
 *
 * @return 0 if ok, < 0 if error.
 */
int net_tls_credential_delete(int tag, enum net_tls_credential_type type);

#else

#define net_tls_credential_add(...)
#define net_tls_credential_get(...)
#define net_tls_credential_delete(...)

#endif /* defined(CONFIG_NET_TLS) || defined(CONFIG_NET_DTLS) */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __NET_TLS_H */
