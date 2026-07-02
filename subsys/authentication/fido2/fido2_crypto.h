/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FIDO2_CRYPTO_H_
#define FIDO2_CRYPTO_H_

#include <zephyr/authentication/fido2/fido2_types.h>

/* P-256 public key export size */
#define FIDO2_P256_UNCOMPRESSED_KEY_SIZE 65
/* P-256 coordinate size */
#define FIDO2_P256_COORD_SIZE            32
/* EC point prefix */
#define FIDO2_EC_POINT_UNCOMPRESSED      0x04
/* ASN.1-encoded ECDSA signature length */
#define FIDO2_ECDSA_SIG_MAX_SIZE         72

/**
 * Initialize the crypto module.
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_init(void);

/**
 * Generate a new P-256 key pair and return the PSA key ID.
 *
 * @param key_id Output: assigned PSA key ID
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_generate_keypair(uint32_t *key_id);

/**
 * Sign data using ECDSA-SHA256 with the given key.
 *
 * @param key_id PSA key ID
 * @param hash 32-byte hash to sign
 * @param sig Output signature buffer
 * @param sig_size Size of signature buffer
 * @param sig_len Actual signature length written
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_sign(uint32_t key_id, const uint8_t hash[FIDO2_SHA256_SIZE], uint8_t *sig,
		      size_t sig_size, size_t *sig_len);

/**
 * Export the public key for a key pair.
 *
 * @param key_id PSA key ID
 * @param pub_key Output: uncompressed public key (65 bytes for P-256)
 * @param pub_key_size Buffer size
 * @param pub_key_len Actual bytes written
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_export_pubkey(uint32_t key_id, uint8_t *pub_key, size_t pub_key_size,
			       size_t *pub_key_len);

/**
 * Compute SHA-256 hash.
 *
 * @param data Input data
 * @param len Input length
 * @param hash Output: 32-byte hash
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_sha256(const uint8_t *data, size_t len, uint8_t hash[FIDO2_SHA256_SIZE]);

/**
 * Compute SHA-256(authData || clientDataHash).
 *
 * @param auth_data Raw authenticatorData bytes
 * @param auth_data_len Length of auth_data
 * @param client_data_hash 32-byte clientDataHash
 * @param out Output: 32-byte hash
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_hash_authdata(const uint8_t *auth_data, size_t auth_data_len,
			       const uint8_t client_data_hash[FIDO2_SHA256_SIZE],
			       uint8_t out[FIDO2_SHA256_SIZE]);

/**
 * Destroy a key.
 *
 * @param key_id PSA key ID to destroy
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_destroy_key(uint32_t key_id);

/**
 * Wrap a PSA key_id and rp_id_hash into a credential ID.
 *
 * @param key_id        PSA persistent key identifier
 * @param rp_id_hash    SHA-256 of the relying party ID
 * @param cred_id_out   Output buffer, FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE bytes
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_wrap_credential(uint32_t key_id, const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				 uint8_t *cred_id_out);

/**
 * Unwrap a credential ID back to key_id.
 *
 * @param cred_id       Credential ID, FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE bytes
 * @param rp_id_hash    SHA-256 of the relying party ID
 * @param key_id_out    Recovered PSA key identifier
 * @return 0 on success, non-zero on failure
 */
int fido2_crypto_unwrap_credential(const uint8_t *cred_id,
				   const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				   uint32_t *key_id_out);

#endif /* FIDO2_CRYPTO_H_ */
