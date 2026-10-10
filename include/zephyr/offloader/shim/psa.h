/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PSA Crypto calls executed on the offloading engine.
 *
 * Internal shim. Include <zephyr/offloader/psa.h> instead, which redirects the standard PSA Crypto
 * names here when CONFIG_OFFLOADER_PSA is enabled.
 */
#ifndef _OFFLOADER_SHIM_PSA_H_
#define _OFFLOADER_SHIM_PSA_H_

#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

psa_status_t offloader_psa_crypto_init(void);

psa_status_t offloader_psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				      size_t data_length, psa_key_id_t *key);

psa_status_t offloader_psa_destroy_key(psa_key_id_t key);

psa_status_t offloader_psa_generate_random(uint8_t *output, size_t output_size);

psa_status_t offloader_psa_hash_compute(psa_algorithm_t alg, const uint8_t *input,
					size_t input_length, uint8_t *hash, size_t hash_size,
					size_t *hash_length);

psa_status_t offloader_psa_sign_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
				     size_t hash_length, uint8_t *signature, size_t signature_size,
				     size_t *signature_length);

psa_status_t offloader_psa_verify_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
				       size_t hash_length, const uint8_t *signature,
				       size_t signature_length);

psa_status_t offloader_psa_aead_encrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
					size_t nonce_length, const uint8_t *additional_data,
					size_t additional_data_length, const uint8_t *plaintext,
					size_t plaintext_length, uint8_t *ciphertext,
					size_t ciphertext_size, size_t *ciphertext_length);

psa_status_t offloader_psa_aead_decrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
					size_t nonce_length, const uint8_t *additional_data,
					size_t additional_data_length, const uint8_t *ciphertext,
					size_t ciphertext_length, uint8_t *plaintext,
					size_t plaintext_size, size_t *plaintext_length);

psa_status_t offloader_psa_mac_compute(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, uint8_t *mac, size_t mac_size,
				       size_t *mac_length);

psa_status_t offloader_psa_cipher_encrypt(psa_key_id_t key, psa_algorithm_t alg,
					  const uint8_t *input, size_t input_length,
					  uint8_t *output, size_t output_size,
					  size_t *output_length);

psa_status_t offloader_psa_cipher_decrypt(psa_key_id_t key, psa_algorithm_t alg,
					  const uint8_t *input, size_t input_length,
					  uint8_t *output, size_t output_size,
					  size_t *output_length);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* _OFFLOADER_SHIM_PSA_H_ */
