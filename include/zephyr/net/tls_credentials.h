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

/** TLS Credential Keygen types
 * Used to specify the type of keypair to generate during keygen operations.
 */
enum tls_credential_keygen_type {
	/** Generate the default keypair type for the back-end */
	TLS_CREDENTIAL_KEYGEN_DEFAULT = 0,

	/** Do not generate a new keypair */
	TLS_CREDENTIAL_KEYGEN_EXISTING,

	/* The remaining keygen types are a subset of the key pair types defined in the IANA
	 * Supported Groups registry. See RFC 8447 for details.
	 */
	TLS_CREDENTIAL_KEYGEN_SECP256R1,
	TLS_CREDENTIAL_KEYGEN_SECP384R1,
	TLS_CREDENTIAL_KEYGEN_SECP521R1,
	TLS_CREDENTIAL_KEYGEN_X25519,
	TLS_CREDENTIAL_KEYGEN_X448
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

#if defined(CONFIG_TLS_CREDENTIAL_KEYGEN) || defined(__DOXYGEN__)

/**
 * @brief Generate client public/private keypair, store the private key, output the public key.
 *
 * @details This function generates a public/private keypair, installs the private key to the
 * 	    provided security tag, and outputs the public key. The security tag must not already
 * 	    have a private key installed. Some backends do not support this function.
 *
 * @param tag	  A security tag where the generated private key should be installed.
 * @param type	  The type of public/private key-pair to generate. Set to
 * 		  TLS_CREDENTIAL_KEYGEN_DEFAULT to use the default type for the credentials
 * 		  back-end. TLS_CREDENTIAL_KEYGEN_EXISTING is invalid.
 * @param key_buf A buffer where the public key will be written in ASN.1 DER format.
 * 		  (X.509 SubjectPublicKeyInfo entry. See RFC5280)
 * 		  This buffer may be temporarily used to hold the private key before it is saved
 * 		  into storage. Private key contents will always be erased before the function
 * 		  returns.
 * @param keylen  The size of the provided key buffer. This will be updated to the total number of
 * 		  bytes written to the key buffer.
 *
 * @retval 0 CSR successfully generated.
 * @retval -EEXIST There is already a private key installed at the requested security tag.
 * @retval -EFAULT CSR Generation failed.
 * @retval -EFBIG CSR does not fit in the buffer provided.
 * @retval -EINVAL Invalid input.
 * @retval -ENOTSUP The TLS credentials backend doesn't support keygen or the requested keygen type.
 */
int tls_credential_keygen(sec_tag_t tag, enum tls_credential_keygen_type type,
			  void *key_buf, size_t *keylen);

/**
 * @brief Check whether the specified TLS credential keygen type is supported.
 *
 *
 * @param type
 * @retval true if the provided keygen type can be used with tls_credential_keygen (including
 *  		TLS_CREDENTIAL_KEYGEN_DEFAULT).
 * @retval false otherwise.
 */
bool tls_credential_can_keygen(enum tls_credential_keygen_type type);

#else /* defined(CONFIG_TLS_CREDENTIAL_KEYGEN) */
#define tls_credential_keygen(...) (-ENOTSUP)
#define tls_credential_can_keygen(...) (false)
#endif /* defined(CONFIG_TLS_CREDENTIAL_KEYGEN) */

#if defined(CONFIG_TLS_CREDENTIAL_CSR) || defined(__DOXYGEN__)

/**
 * @brief Generate a certificate signing request (CSR).
 *
 * @details This function generates a PKCS#10 Certificate Signing Request. Some backends do not
 * 	    support this function. Can optionally generate a new private/public keypair and store
 * 	    the private key used for CSR signature. Some backends may require or not support this
 * 	    mode.
 *
 * @param tag    Security tag of the private key used for signing the CSR. If generating a new
 * 		 keypair, this is where the generated private key will be installed. In this case,
 * 		 the security tag must not already have a private key installed. Otherwise,
 * 		 specifies the sectag to load the needed private key from.
 * @param type	 The type of public/private key-pair to generate. Set to
 * 		 TLS_CREDENTIAL_KEYGEN_DEFAULT to use the default type for the credentials
 * 		 back-end. Set to TLS_CREDENTIAL_KEYGEN_EXISTING to instead use a pre-existing private
 * 		 key.
 * @param dn     Distinguished Name subject string for the CSR to be generated. Provided string
 * 		 will be copied. Should contain a comma-separated list of attribute types and
 * 		 values conformal with RFC 4514. For example:
 *               "C=US,ST=CA,O=Linux Foundation,CN=Zephyr RTOS Device 1"
 * @param csr    A buffer where the PKCS#10 CSR will be written (in ASN.1 DER format).
 * 		 This buffer may be temporarily used to hold private key data while it is in use.
 * 		 Private key contents are always be erased before the function returns.
 * @param csrlen The size of the provided csr buffer. If any data is written to the csr buffer,
 *               this will be updated to the total amount written in bytes.
 *
 * @retval 0 CSR successfully generated.
 * @retval -EEXIST There is already a private key installed at the requested security tag.
 * @retval -ENOENT Private key does not exist at the requested security tag.
 * @retval -EFAULT CSR Generation failed.
 * @retval -EFBIG CSR does not fit in the buffer provided.
 * @retval -EINVAL Invalid input.
 * @retval -ENOTSUP The TLS credentials backend doesn't support CSR generation.
 */
int tls_credential_csr(sec_tag_t tag, char *dn, enum tls_credential_keygen_type type,
		       void *csr, size_t *csrlen);

#else /* defined(CONFIG_TLS_CREDENTIAL_CSR) */
#define tls_credential_csr(...) (-ENOTSUP)
#endif /* defined(CONFIG_TLS_CREDENTIAL_CSR) */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_TLS_CREDENTIALS_H_ */
