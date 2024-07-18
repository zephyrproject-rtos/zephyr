/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief TLS credentials management
 *
 * An API for applications to configure TLS credentials.
 */

#ifndef ZEPHYR_INCLUDE_NET_TLS_CREDENTIALS_H_
#define ZEPHYR_INCLUDE_NET_TLS_CREDENTIALS_H_

/**
 * @brief TLS credentials management
 * @defgroup tls_credentials TLS credentials management
 * @since 1.13
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** TLS credential types */
enum tls_credential_type {
	/** Unspecified credential. */
	TLS_CREDENTIAL_NONE,

	/** A trusted CA certificate. Use this to authenticate remote servers.
	 *  Used with certificate-based ciphersuites.
	 */
	TLS_CREDENTIAL_CA_CERTIFICATE,

	/** A public server certificate. Use this to register your own server
	 *  certificate. Should be registered together with a corresponding
	 *  private key. Used with certificate-based ciphersuites.
	 */
	TLS_CREDENTIAL_SERVER_CERTIFICATE,

	/** Private key. Should be registered together with a corresponding
	 *  public certificate. Used with certificate-based ciphersuites.
	 */
	TLS_CREDENTIAL_PRIVATE_KEY,

	/** Pre-shared key. Should be registered together with a corresponding
	 *  PSK identity. Used with PSK-based ciphersuites.
	 */
	TLS_CREDENTIAL_PSK,

	/** Pre-shared key identity. Should be registered together with a
	 *  corresponding PSK. Used with PSK-based ciphersuites.
	 */
	TLS_CREDENTIAL_PSK_ID
};

/** Secure tag, a reference to TLS credential
 *
 * Secure tag can be used to reference credential after it was registered
 * in the system.
 *
 * @note Some TLS credentials come in pairs:
 *    - TLS_CREDENTIAL_SERVER_CERTIFICATE with TLS_CREDENTIAL_PRIVATE_KEY,
 *    - TLS_CREDENTIAL_PSK with TLS_CREDENTIAL_PSK_ID.
 *    Such pairs of credentials must be assigned the same secure tag to be
 *    correctly handled in the system.
 *
 * @note Negative values are reserved for internal use.
 */
typedef int sec_tag_t;

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
 * @retval 0 TLS credential successfully added.
 * @retval -EACCES Access to the TLS credential subsystem was denied.
 * @retval -ENOMEM Not enough memory to add new TLS credential.
 * @retval -EEXIST TLS credential of specific tag and type already exists.
 */
int tls_credential_add(sec_tag_t tag, enum tls_credential_type type,
		       const void *cred, size_t credlen);

/**
 * @brief Get a TLS credential.
 *
 * @details This function gets an already registered TLS credential,
 *          referenced by @p tag secure tag of @p type.
 *
 * @param tag     A security tag of requested credential.
 * @param type    A TLS/DTLS credential type of requested credential.
 * @param cred    A buffer for TLS/DTLS credential.
 * @param credlen A buffer size on input. TLS/DTLS credential length on output.
 *
 * @retval 0 TLS credential successfully obtained.
 * @retval -EACCES Access to the TLS credential subsystem was denied.
 * @retval -ENOENT Requested TLS credential was not found.
 * @retval -EFBIG Requested TLS credential does not fit in the buffer provided.
 */
int tls_credential_get(sec_tag_t tag, enum tls_credential_type type,
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
 * @retval 0 TLS credential successfully deleted.
 * @retval -EACCES Access to the TLS credential subsystem was denied.
 * @retval -ENOENT Requested TLS credential was not found.
 */
int tls_credential_delete(sec_tag_t tag, enum tls_credential_type type);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_TLS_CREDENTIALS_H_ */
