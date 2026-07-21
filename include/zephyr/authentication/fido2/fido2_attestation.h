/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 attestation callback API.
 */

#ifndef ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_ATTESTATION_H_
#define ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_ATTESTATION_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief FIDO2 attestation
 * @ingroup fido2
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Packed attestation */
#define FIDO2_ATTESTATION_FMT_PACKED  "packed"
/** @brief No attestation */
#define FIDO2_ATTESTATION_FMT_NONE    "none"
/** Maximum attestation format identifier length. */
#define FIDO2_ATTESTATION_FMT_MAX_LEN 32

/**
 * @brief Attestation result produced by an attestation callback.
 */
struct fido2_attestation_result {
	/** Attestation statement format identifier (e.g., "packed", "none"). */
	const char *fmt;
	/** COSE algorithm identifier (e.g., -7 for ES256). Set to 0 to omit. */
	int32_t alg;
	/** Attestation signature. NULL to omit. */
	const uint8_t *sig;
	/** Length of @p sig in bytes. */
	size_t sig_len;
	/** DER-encoded attestation certificate for the x5c array. */
	const uint8_t *x5c;
	/** Length of @p x5c in bytes. 0 if not present. */
	size_t x5c_len;
};

/**
 * @brief Sign authenticatorData for a new credential.
 * @param auth_data         Raw authenticatorData bytes.
 * @param auth_data_len     Length of @p auth_data.
 * @param client_data_hash  32-byte SHA-256 of clientDataJSON.
 * @param credential_key_id PSA key handle of the credential key. Only for self-attestation.
 * @param result	    Attestation result to fill.
 * @retval 0	   On success.
 * @retval -errno  On failure; subsystem aborts MakeCredential.
 */
int fido2_attestation_sign(const uint8_t *auth_data, size_t auth_data_len,
			   const uint8_t *client_data_hash, uint32_t credential_key_id,
			   struct fido2_attestation_result *result);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_ATTESTATION_H_ */
