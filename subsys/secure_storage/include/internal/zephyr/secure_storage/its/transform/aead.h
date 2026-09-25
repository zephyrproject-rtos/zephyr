/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ITS_TRANSFORM_AEAD_H
#define SECURE_STORAGE_ITS_TRANSFORM_AEAD_H

/** @file zephyr/secure_storage/its/transform/aead.h The AEAD ITS transform module API.
 *
 * The functions declared in this header allow customization
 * of the AEAD implementation of the ITS transform module.
 * They are not meant to be called directly other than by the AEAD ITS transform module.
 * This header file may and must be included when providing a custom implementation of one
 * or more of these functions (@kconfig_regex{CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_.*_CUSTOM}).
 */
#include <zephyr/secure_storage/its/common.h>
#include <psa/crypto_types.h>

/** @brief ITS transform AEAD tag size */
#define SECURE_STORAGE_ITS_TRANSFORM_AEAD_TAG_SIZE                     \
		(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_OUTPUT_OVERHEAD   \
		 - CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE)

/** @brief Encrypts or decrypts an ITS entry.
 *
 * @param[in]  operation    The operation to perform (`PSA_KEY_USAGE_{ENCRYPT,DECRYPT}`).
 * @param[in]  uid          The UID of the ITS entry to process.
 * @param[in]  nonce        The nonce to use in the AEAD operation.
 * @param[in]  add_data_len The size of `add_data` in bytes.
 * @param[in]  add_data     The additional data to authenticate.
 * @param[in]  input_len    The size of `input` in bytes.
 * @param[in]  input        The plaintext (when encrypting) or the ciphertext (when decrypting).
 * @param[in]  output_size  The size of the `output` buffer in bytes.
 * @param[out] output       The ciphertext (when encrypting) or the plaintext (when decrypting).
 * @param[out] output_len   On success, the size of `output` in bytes.
 *
 * On encryption, `output_len` must be exactly `input_len` plus the tag size, i.e.
 * `CONFIG_SECURE_STORAGE_ITS_TRANSFORM_OUTPUT_OVERHEAD` minus
 * `CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE`; decryption must remove the same expansion.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_its_transform_aead_crypt(
		psa_key_usage_t operation, secure_storage_its_uid_t uid,
		const uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE],
		size_t add_data_len, const uint8_t *add_data, size_t input_len,
		const uint8_t *input, size_t output_size, uint8_t *output, size_t *output_len);

#if defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_IS_CONFIGURABLE) || defined(__DOXYGEN__)

/** @brief Returns the key type and algorithm to use for the AEAD operations.
 *
 * @param[out] key_type The key type to use.
 * @param[out] alg      The algorithm to use.
 */
void secure_storage_its_transform_aead_get_scheme(psa_key_type_t *key_type, psa_algorithm_t *alg);

#endif /* CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_IS_CONFIGURABLE */

#if !defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_CRYPT_CUSTOM) || defined(__DOXYGEN__)

/** @brief Returns the encryption key to use for an ITS entry's AEAD operations.
 *
 * @param[in]  uid The UID of the ITS entry for which the key is used.
 * @param[out] key The encryption key.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_its_transform_aead_get_key(
		secure_storage_its_uid_t uid,
		uint8_t key[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE]);

#endif /* SECURE_STORAGE_ITS_TRANSFORM_AEAD_CRYPT_CUSTOM */

/** @brief Generates a nonce for an AEAD operation.
 *
 * @param[out] nonce The generated nonce.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_its_transform_aead_get_nonce(
		uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE]);

#endif
