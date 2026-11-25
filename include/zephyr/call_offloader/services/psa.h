/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief PSA Crypto call offloading (see <zephyr/call_offloader/call_offloader.h>).
 */
#ifndef ZEPHYR_INCLUDE_CALL_OFFLOADER_SERVICES_PSA_H_
#define ZEPHYR_INCLUDE_CALL_OFFLOADER_SERVICES_PSA_H_

/** @cond INTERNAL_HIDDEN */
#ifndef CALL_OFFLOADER_PSA_NO_REDIRECT
#define PSA_WRAPPER_NAME(_fn) call_offloader_##_fn

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
#endif /* CALL_OFFLOADER_PSA_NO_REDIRECT */
/** @endcond */
#endif /* ZEPHYR_INCLUDE_CALL_OFFLOADER_SERVICES_PSA_H_ */
