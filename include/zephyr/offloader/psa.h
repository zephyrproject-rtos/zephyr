/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief PSA Crypto call offloading (see <zephyr/offloader/engine.h>).
 */
#ifndef ZEPHYR_INCLUDE_OFFLOADER_PSA_H_
#define ZEPHYR_INCLUDE_OFFLOADER_PSA_H_

#ifdef CONFIG_OFFLOADER_PSA
#ifndef OFFLOADER_PSA_NO_REDIRECT
#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif
#define PSA_WRAPPER_NAME(_fn) offloader_##_fn
#define PSA_WRAPPER_DECLARE(_fn) __typeof__(_fn) PSA_WRAPPER_NAME(_fn);

PSA_WRAPPER_DECLARE(psa_crypto_init)
PSA_WRAPPER_DECLARE(psa_import_key)
PSA_WRAPPER_DECLARE(psa_destroy_key)
PSA_WRAPPER_DECLARE(psa_generate_random)
PSA_WRAPPER_DECLARE(psa_hash_compute)
PSA_WRAPPER_DECLARE(psa_sign_hash)
PSA_WRAPPER_DECLARE(psa_verify_hash)
PSA_WRAPPER_DECLARE(psa_aead_encrypt)
PSA_WRAPPER_DECLARE(psa_aead_decrypt)
PSA_WRAPPER_DECLARE(psa_mac_compute)
PSA_WRAPPER_DECLARE(psa_cipher_encrypt)
PSA_WRAPPER_DECLARE(psa_cipher_decrypt)
PSA_WRAPPER_DECLARE(psa_generate_key)
PSA_WRAPPER_DECLARE(psa_generate_key_custom)
PSA_WRAPPER_DECLARE(psa_sign_message)
PSA_WRAPPER_DECLARE(psa_verify_message)
PSA_WRAPPER_DECLARE(psa_asymmetric_encrypt)
PSA_WRAPPER_DECLARE(psa_asymmetric_decrypt)
PSA_WRAPPER_DECLARE(psa_raw_key_agreement)
PSA_WRAPPER_DECLARE(psa_export_public_key)
PSA_WRAPPER_DECLARE(psa_export_key)
PSA_WRAPPER_DECLARE(psa_hash_compare)
PSA_WRAPPER_DECLARE(psa_mac_verify)
PSA_WRAPPER_DECLARE(psa_key_derivation_key_agreement)
PSA_WRAPPER_DECLARE(psa_key_derivation_output_key)

#ifdef __cplusplus
}
#endif

#define psa_crypto_init				PSA_WRAPPER_NAME(psa_crypto_init)
#define psa_import_key				PSA_WRAPPER_NAME(psa_import_key)
#define psa_destroy_key				PSA_WRAPPER_NAME(psa_destroy_key)
#define psa_generate_random			PSA_WRAPPER_NAME(psa_generate_random)
#define psa_hash_compute			PSA_WRAPPER_NAME(psa_hash_compute)
#define psa_sign_hash				PSA_WRAPPER_NAME(psa_sign_hash)
#define psa_verify_hash				PSA_WRAPPER_NAME(psa_verify_hash)
#define psa_aead_encrypt			PSA_WRAPPER_NAME(psa_aead_encrypt)
#define psa_aead_decrypt			PSA_WRAPPER_NAME(psa_aead_decrypt)
#define psa_mac_compute				PSA_WRAPPER_NAME(psa_mac_compute)
#define psa_cipher_encrypt			PSA_WRAPPER_NAME(psa_cipher_encrypt)
#define psa_cipher_decrypt			PSA_WRAPPER_NAME(psa_cipher_decrypt)
#define psa_generate_key			PSA_WRAPPER_NAME(psa_generate_key)
#define psa_generate_key_custom			PSA_WRAPPER_NAME(psa_generate_key_custom)
#define psa_sign_message			PSA_WRAPPER_NAME(psa_sign_message)
#define psa_verify_message			PSA_WRAPPER_NAME(psa_verify_message)
#define psa_asymmetric_encrypt			PSA_WRAPPER_NAME(psa_asymmetric_encrypt)
#define psa_asymmetric_decrypt			PSA_WRAPPER_NAME(psa_asymmetric_decrypt)
#define psa_raw_key_agreement			PSA_WRAPPER_NAME(psa_raw_key_agreement)
#define psa_export_public_key			PSA_WRAPPER_NAME(psa_export_public_key)
#define psa_export_key				PSA_WRAPPER_NAME(psa_export_key)
#define psa_hash_compare			PSA_WRAPPER_NAME(psa_hash_compare)
#define psa_mac_verify				PSA_WRAPPER_NAME(psa_mac_verify)
#define psa_key_derivation_key_agreement	PSA_WRAPPER_NAME(psa_key_derivation_key_agreement)
#define psa_key_derivation_output_key		PSA_WRAPPER_NAME(psa_key_derivation_output_key)
#endif /* OFFLOADER_PSA_NO_REDIRECT */
#endif /* CONFIG_OFFLOADER_PSA */
#endif /* ZEPHYR_INCLUDE_OFFLOADER_PSA_H_ */
