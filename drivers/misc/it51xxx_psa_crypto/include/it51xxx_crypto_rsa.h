/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_RSA_H_
#define ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_RSA_H_

/**
 * @file it51xxx_crypto_rsa.h
 * @brief Provide hash signing, signature verification and RSA initialization.
 */

#include "psa/crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sign a precomputed hash of a message
 *
 * @param attributes Attributes of the key to use
 * @param key_buffer Key material buffer
 * @param key_buffer_size Size in bytes of the key
 * @param alg Algorithm to use
 * @param hash Hash to sign buffer
 * @param hash_length Size in bytes of the data to sign
 * @param signature Buffer to hold the signature data
 * @param signature_size Size in bytes of the signature buffer
 * @param signature_length Size in bytes of the signature
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t ite_it51xxx_sign_hash(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
				   uint8_t *signature, size_t signature_size,
				   size_t *signature_length);

/**
 * @brief Verify a message signature on a hash
 *
 * @param attributes Attributes of the key to use
 * @param key_buffer Key material buffer
 * @param key_buffer_size Size in bytes of the key
 * @param alg Algorithm to use
 * @param hash Hash to sign buffer
 * @param hash_length Size in bytes of the data to sign
 * @param signature Signature to verify
 * @param signature_length Size in bytes of the signature
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t ite_it51xxx_verify_hash(const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
				     const uint8_t *signature, size_t signature_length);

/**
 * @brief Encrypt message using RSA public key
 *
 * @param attributes Attributes of the key to use
 * @param key_buffer Key material buffer
 * @param key_buffer_size Size in bytes of the key
 * @param alg Algorithm to use
 * @param input Message to encrypt
 * @param input_length Size in bytes of the message
 * @param salt Salt or label, if supported by the algorithm; otherwise NULL
 * @param salt_length Size in bytes of the salt or label; zero if unused
 * @param output Buffer to hold the ciphertext
 * @param output_size Size in bytes of the output buffer
 * @param output_length On success, size in bytes of the ciphertext
 *
 * @return PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *         failure
 */
psa_status_t ite_it51xxx_asymmetric_encrypt(const psa_key_attributes_t *attributes,
					    const uint8_t *key_buffer, size_t key_buffer_size,
					    psa_algorithm_t alg, const uint8_t *input,
					    size_t input_length, const uint8_t *salt,
					    size_t salt_length, uint8_t *output, size_t output_size,
					    size_t *output_length);

/**
 * @brief Decrypt message using RSA private key
 *
 * @param attributes Attributes of the key to use
 * @param key_buffer Key material buffer
 * @param key_buffer_size Size in bytes of the key
 * @param alg Algorithm to use
 * @param input Ciphertext to decrypt
 * @param input_length Size in bytes of the ciphertext
 * @param salt Salt or label, if supported by the algorithm; otherwise NULL
 * @param salt_length Size in bytes of the salt or label; zero if unused
 * @param output Buffer to hold the plaintext
 * @param output_size Size in bytes of the output buffer
 * @param output_length On success, size in bytes of the plaintext
 *
 * @return PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *         failure
 */
psa_status_t ite_it51xxx_asymmetric_decrypt(const psa_key_attributes_t *attributes,
					    const uint8_t *key_buffer, size_t key_buffer_size,
					    psa_algorithm_t alg, const uint8_t *input,
					    size_t input_length, const uint8_t *salt,
					    size_t salt_length, uint8_t *output, size_t output_size,
					    size_t *output_length);

/**
 * @brief Initialize RSA hardware
 *
 * @retval PSA_SUCCESS on success.
 */
psa_status_t it51xxx_crypto_rsa_init(void);

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_RSA_H_ */
