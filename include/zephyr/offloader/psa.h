/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Redirects PSA calls to the psa_offloader when CONFIG_OFFLOADER_PSA is enabled.
 *
 * Including this header redirects the covered PSA Crypto calls onto the offloader thread when
 * CONFIG_OFFLOADER_PSA is enabled, and is a plain include of <psa/crypto.h> otherwise.
 */
#ifndef _OFFLOADER_PSA_H_
#define _OFFLOADER_PSA_H_

#include <psa/crypto.h>

#ifdef CONFIG_OFFLOADER_PSA
#include <zephyr/offloader/shim/psa.h>

#define psa_crypto_init		offloader_psa_crypto_init
#define psa_import_key		offloader_psa_import_key
#define psa_destroy_key		offloader_psa_destroy_key
#define psa_generate_random	offloader_psa_generate_random
#define psa_hash_compute	offloader_psa_hash_compute
#define psa_sign_hash		offloader_psa_sign_hash
#define psa_verify_hash		offloader_psa_verify_hash
#define psa_aead_encrypt	offloader_psa_aead_encrypt
#define psa_aead_decrypt	offloader_psa_aead_decrypt
#define psa_mac_compute		offloader_psa_mac_compute
#define psa_cipher_encrypt	offloader_psa_cipher_encrypt
#define psa_cipher_decrypt	offloader_psa_cipher_decrypt

#endif /* CONFIG_OFFLOADER_PSA */

#endif /* _OFFLOADER_PSA_H_ */
