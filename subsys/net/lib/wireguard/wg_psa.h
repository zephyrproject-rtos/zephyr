/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WG_PSA_H_
#define WG_PSA_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Initialize PSA crypto subsystem
 *
 * @return 0 on success, negative errno on failure
 */
int wg_psa_init(void);

/**
 * @brief Perform X25519 Diffie-Hellman key agreement
 *
 * @param shared_secret Output buffer for 32-byte shared secret
 * @param private_key 32-byte private key
 * @param public_key 32-byte public key
 * @return 0 on success, negative errno on failure
 */
int wg_psa_x25519(uint8_t *shared_secret,
		  const uint8_t *private_key,
		  const uint8_t *public_key);

/**
 * @brief Generate X25519 public key from private key
 *
 * @param public_key Output buffer for 32-byte public key
 * @param private_key 32-byte private key
 * @return 0 on success, negative errno on failure
 */
int wg_psa_x25519_public_key(uint8_t *public_key,
			     const uint8_t *private_key);

/**
 * @brief ChaCha20-Poly1305 AEAD encryption
 *
 * @param dst Output buffer (must be src_len + 16 bytes)
 * @param src Plaintext input
 * @param src_len Length of plaintext
 * @param ad Additional authenticated data
 * @param ad_len Length of additional data
 * @param nonce 64-bit nonce (will be prefixed with 4 zero bytes)
 * @param key 32-byte encryption key
 * @return 0 on success, negative errno on failure
 */
int wg_psa_aead_encrypt(uint8_t *dst,
			const uint8_t *src, size_t src_len,
			const uint8_t *ad, size_t ad_len,
			uint64_t nonce,
			const uint8_t *key);

/**
 * @brief ChaCha20-Poly1305 AEAD decryption
 *
 * @param dst Output buffer for plaintext (src_len - 16 bytes)
 * @param src Ciphertext input (includes 16-byte tag)
 * @param src_len Length of ciphertext (including tag)
 * @param ad Additional authenticated data
 * @param ad_len Length of additional data
 * @param nonce 64-bit nonce
 * @param key 32-byte decryption key
 * @return true on success, false on failure (auth failed)
 */
bool wg_psa_aead_decrypt(uint8_t *dst,
			 const uint8_t *src, size_t src_len,
			 const uint8_t *ad, size_t ad_len,
			 uint64_t nonce,
			 const uint8_t *key);

/**
 * @brief XChaCha20-Poly1305 AEAD encryption
 *
 * @param dst Output buffer (must be src_len + 16 bytes)
 * @param src Plaintext input
 * @param src_len Length of plaintext
 * @param ad Additional authenticated data
 * @param ad_len Length of additional data
 * @param nonce 64-bit nonce (will be prefixed with 4 zero bytes)
 * @param key 32-byte encryption key
 * @return 0 on success, negative errno on failure
 */
int wg_psa_xaead_encrypt(uint8_t *dst,
			 const uint8_t *src, size_t src_len,
			 const uint8_t *ad, size_t ad_len,
			 const uint8_t *nonce,
			 const uint8_t *key);

/**
 * @brief XChaCha20-Poly1305 AEAD decryption
 *
 * @param dst Output buffer for plaintext (src_len - 16 bytes)
 * @param src Ciphertext input (includes 16-byte tag)
 * @param src_len Length of ciphertext (including tag)
 * @param ad Additional authenticated data
 * @param ad_len Length of additional data
 * @param nonce 64-bit nonce
 * @param key 32-byte decryption key
 * @return true on success, false on failure (auth failed)
 */
bool wg_psa_xaead_decrypt(uint8_t *dst,
			  const uint8_t *src, size_t src_len,
			  const uint8_t *ad, size_t ad_len,
			  const uint8_t *nonce,
			  const uint8_t *key);

/**
 * @brief Generate cryptographically secure random bytes
 *
 * @param buf Output buffer
 * @param len Number of bytes to generate
 * @return 0 on success, negative errno on failure
 */
int wg_psa_random(uint8_t *buf, size_t len);

#endif /* WG_PSA_H_ */
