/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ITS_TRANSFORM_AEAD_GET_H
#define SECURE_STORAGE_ITS_TRANSFORM_AEAD_GET_H

/** @file zephyr/secure_storage/its/transform/aead_get.h The AEAD ITS transform module API.
 *
 * The functions declared in this header allow customization
 * of the AEAD implementation of the ITS transform module.
 * They are not meant to be called directly other than by the AEAD ITS transform module.
 * This header may be included when providing a custom implementation of one
 * or more of these functions (@kconfig{CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_*_CUSTOM}).
 */
#include <zephyr/secure_storage/its/common.h>
#include <psa/crypto_types.h>

/** @brief Returns the key type and algorithm to use for the AEAD operations.
 *
 * @param[out] key_type The key type to use.
 * @param[out] alg      The algorithm to use.
 */
void secure_storage_its_transform_aead_get_scheme(psa_key_type_t *key_type, psa_algorithm_t *alg);

/** @brief Returns the encryption key to use for an ITS entry's AEAD operations.
 *
 * @param[in]  uid The UID of the ITS entry for whom the returned key is used.
 * @param[out] key The encryption key.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_its_transform_aead_get_key(
		secure_storage_its_uid_t uid,
		uint8_t key[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE]);

/** @brief Generates a nonce for an AEAD operation.
 *
 * @param[out] nonce The generated nonce.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_its_transform_aead_get_nonce(
		uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE]);

#endif
