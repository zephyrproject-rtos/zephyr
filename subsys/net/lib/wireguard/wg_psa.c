/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include <psa/crypto.h>

#include "crypto/crypto.h"
#include "crypto/refc/hchacha20.h"

#define WG_PSA_KEY_SIZE   32
#define WG_PSA_NONCE_SIZE PSA_AEAD_NONCE_LENGTH(PSA_KEY_TYPE_CHACHA20,	\
						PSA_ALG_CHACHA20_POLY1305) /* 12 */
#define WG_PSA_TAG_SIZE   PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_CHACHA20,	\
					      PSA_BITS_TO_BYTES(WG_PSA_KEY_SIZE), \
					      PSA_ALG_CHACHA20_POLY1305) /* 16 */

BUILD_ASSERT(WG_PSA_NONCE_SIZE == 12, "Expected 12-byte nonce for X25519 and ChaCha20");
BUILD_ASSERT(WG_PSA_TAG_SIZE == 16, "Expected 16-byte tag for X25519 and ChaCha20");

static int wg_psa_init(void)
{
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		NET_DBG("PSA crypto init failed: %d", status);
		return -EIO;
	}

	return 0;
}

static int wg_psa_import_x25519_private(psa_key_id_t *key_id,
					const uint8_t *key_data)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	if (key_id == NULL || key_data == NULL) {
		return -EINVAL;
	}

	/* Set key attributes for X25519 private key */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, 255);

	/* Import the key */
	status = psa_import_key(&attr, key_data, WG_PSA_KEY_SIZE, key_id);
	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to import X25519 private key: %d", status);
		return -EINVAL;
	}

	NET_DBG("Imported X25519 private key (ID: %u)", *key_id);
	return 0;
}

static int wg_psa_import_x25519_public(psa_key_id_t *key_id,
				       const uint8_t *key_data)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	if (key_id == NULL || key_data == NULL) {
		return -EINVAL;
	}

	/* Set key attributes for X25519 public key */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, 255);

	/* Import the key */
	status = psa_import_key(&attr, key_data, WG_PSA_KEY_SIZE, key_id);
	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to import X25519 public key: %d", status);
		return -EINVAL;
	}

	NET_DBG("Imported X25519 public key (ID: %u)", *key_id);
	return 0;
}

static int wg_psa_import_chacha_key(psa_key_id_t *key_id,
				    const uint8_t *key_data,
				    psa_key_usage_t usage)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	if (key_id == NULL || key_data == NULL) {
		return -EINVAL;
	}

	/* Set key attributes for ChaCha20-Poly1305 AEAD */
	psa_set_key_usage_flags(&attr, usage);
	psa_set_key_algorithm(&attr, PSA_ALG_CHACHA20_POLY1305);
	psa_set_key_type(&attr, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&attr, 256);

	/* Import the key */
	status = psa_import_key(&attr, key_data, WG_PSA_KEY_SIZE, key_id);
	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to import ChaCha20 key: %d", status);
		return -EINVAL;
	}

	NET_DBG("Imported ChaCha20 key (ID: %u)", *key_id);
	return 0;
}

static int wg_psa_generate_x25519_keypair(psa_key_id_t *key_id,
					  uint8_t *public_key)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	size_t output_len;

	if (key_id == NULL || public_key == NULL) {
		return -EINVAL;
	}

	/* Generate X25519 keypair directly */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, 255);

	status = psa_generate_key(&attr, key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to generate key: %d", status);
		return -EIO;
	}

	/* Export the public key */
	status = psa_export_public_key(*key_id, public_key, WG_PSA_KEY_SIZE, &output_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to export public key: %d", status);
		psa_destroy_key(*key_id);
		*key_id = PSA_KEY_ID_NULL;
		return -EINVAL;
	}

	if (output_len != WG_PSA_KEY_SIZE) {
		NET_DBG("Unexpected public key size: %zu", output_len);
		psa_destroy_key(*key_id);
		*key_id = PSA_KEY_ID_NULL;
		return -EINVAL;
	}

	NET_DBG("Generated X25519 keypair (ID: %u)", *key_id);
	return 0;
}

static int wg_psa_x25519_hybrid(uint8_t *shared_secret,
				psa_key_id_t private_key_id,
				const uint8_t *public_key)
{
	psa_status_t status;
	size_t output_len;

	if (shared_secret == NULL || public_key == NULL) {
		return -EINVAL;
	}

	status = psa_raw_key_agreement(PSA_ALG_ECDH, private_key_id,
				       public_key, WG_PSA_KEY_SIZE,
				       shared_secret, WG_PSA_KEY_SIZE,
				       &output_len);

	if (status != PSA_SUCCESS) {
		NET_DBG("X25519 key agreement failed: %d", status);
		return -EINVAL;
	}

	if (output_len != WG_PSA_KEY_SIZE) {
		NET_DBG("Unexpected shared secret size: %zu", output_len);
		return -EINVAL;
	}

	return 0;
}

static int wg_psa_aead_encrypt_by_id(uint8_t *dst,
				     const uint8_t *src, size_t src_len,
				     const uint8_t *ad, size_t ad_len,
				     uint64_t nonce,
				     psa_key_id_t key_id)
{
	psa_status_t status;
	uint8_t nonce_buf[WG_PSA_NONCE_SIZE];
	size_t output_len;

	if (dst == NULL) {
		return -EINVAL;
	}

	if (key_id == PSA_KEY_ID_NULL) {
		NET_DBG("AEAD encrypt: key_id is NULL");
		return -EINVAL;
	}

	/* Build nonce: 4 bytes of zeros + 8 bytes little-endian counter */
	memset(nonce_buf, 0, 4);
	sys_put_le64(nonce, nonce_buf + 4);

	NET_DBG("AEAD encrypt: key_id=%u, src_len=%zu, nonce=%llu",
		key_id, src_len, nonce);

	status = psa_aead_encrypt(key_id, PSA_ALG_CHACHA20_POLY1305,
				  nonce_buf, sizeof(nonce_buf),
				  ad, ad_len,
				  src, src_len,
				  dst, src_len + WG_PSA_TAG_SIZE,
				  &output_len);

	if (status != PSA_SUCCESS) {
		NET_DBG("AEAD encrypt failed: %d (key_id=%u, src_len=%zu)",
			status, key_id, src_len);
		return -EINVAL;
	}

	if (output_len != src_len + WG_PSA_TAG_SIZE) {
		NET_DBG("Unexpected ciphertext size: %zu", output_len);
		return -EINVAL;
	}

	return 0;
}

static bool wg_psa_aead_decrypt_by_id(uint8_t *dst,
				      const uint8_t *src, size_t src_len,
				      const uint8_t *ad, size_t ad_len,
				      uint64_t nonce,
				      psa_key_id_t key_id)
{
	psa_status_t status;
	uint8_t nonce_buf[WG_PSA_NONCE_SIZE];
	size_t output_len;
	size_t plaintext_len;

	if (dst == NULL || src == NULL) {
		return false;
	}

	if (src_len < WG_PSA_TAG_SIZE) {
		NET_DBG("Ciphertext too short: %zu", src_len);
		return false;
	}

	plaintext_len = src_len - WG_PSA_TAG_SIZE;

	/* Build nonce: 4 bytes of zeros + 8 bytes little-endian counter */
	memset(nonce_buf, 0, 4);
	sys_put_le64(nonce, nonce_buf + 4);

	status = psa_aead_decrypt(key_id, PSA_ALG_CHACHA20_POLY1305,
				  nonce_buf, sizeof(nonce_buf),
				  ad, ad_len,
				  src, src_len,
				  dst, plaintext_len,
				  &output_len);

	if (status != PSA_SUCCESS) {
		/* Authentication failure - this is normal for invalid packets */
		return false;
	}

	if (output_len != plaintext_len) {
		NET_DBG("Unexpected plaintext size: %zu", output_len);
		return false;
	}

	return true;
}

static int wg_psa_export_public_key(uint8_t *public_key, psa_key_id_t key_id)
{
	psa_status_t status;
	size_t output_len;

	if (public_key == NULL) {
		return -EINVAL;
	}

	status = psa_export_public_key(key_id, public_key, WG_PSA_KEY_SIZE, &output_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to export public key: %d", status);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Export raw key material (for symmetric keys like preshared key)
 *
 * This is needed for KDF operations that require the raw preshared key.
 */
static int wg_psa_export_key(uint8_t *key_data, size_t key_data_len,
			     psa_key_id_t key_id)
{
	psa_status_t status;
	size_t output_len;

	if (key_data == NULL || key_id == PSA_KEY_ID_NULL) {
		return -EINVAL;
	}

	status = psa_export_key(key_id, key_data, key_data_len, &output_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to export key: %d", status);
		return -EINVAL;
	}

	return 0;
}

static void wg_psa_destroy_key(psa_key_id_t key_id)
{
	psa_status_t status;

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		NET_WARN("Failed to destroy key %u: %d", key_id, status);
	} else {
		NET_DBG("Destroyed key ID: %u", key_id);
	}
}

static int wg_psa_derive_session_keys(psa_key_id_t *sending_key_id,
				      psa_key_id_t *receiving_key_id,
				      const uint8_t *chaining_key,
				      bool is_initiator)
{
	uint8_t tau1[WG_PSA_KEY_SIZE];
	uint8_t tau2[WG_PSA_KEY_SIZE];
	int ret;

	if (sending_key_id == NULL || receiving_key_id == NULL || chaining_key == NULL) {
		return -EINVAL;
	}

	/* Derive two keys using WireGuard KDF */
	wg_kdf2(tau1, tau2, chaining_key, NULL, 0);

	/* Import keys based on role */
	if (is_initiator) {
		/* Initiator: sending = tau1, receiving = tau2 */
		ret = wg_psa_import_chacha_key(sending_key_id, tau1,
					       PSA_KEY_USAGE_ENCRYPT);
		if (ret < 0) {
			goto cleanup;
		}

		ret = wg_psa_import_chacha_key(receiving_key_id, tau2,
					       PSA_KEY_USAGE_DECRYPT);
		if (ret < 0) {
			wg_psa_destroy_key(*sending_key_id);
			*sending_key_id = PSA_KEY_ID_NULL;
			goto cleanup;
		}
	} else {
		/* Responder: sending = tau2, receiving = tau1 */
		ret = wg_psa_import_chacha_key(sending_key_id, tau2,
					       PSA_KEY_USAGE_ENCRYPT);
		if (ret < 0) {
			goto cleanup;
		}

		ret = wg_psa_import_chacha_key(receiving_key_id, tau1,
					       PSA_KEY_USAGE_DECRYPT);
		if (ret < 0) {
			wg_psa_destroy_key(*sending_key_id);
			*sending_key_id = PSA_KEY_ID_NULL;
			goto cleanup;
		}
	}

	NET_DBG("Derived session keys (send: %u, recv: %u, initiator: %d)",
		*sending_key_id, *receiving_key_id, is_initiator);
	ret = 0;

cleanup:
	crypto_zero(tau1, sizeof(tau1));
	crypto_zero(tau2, sizeof(tau2));
	return ret;
}

static int wg_psa_aead_setup(uint64_t nonce, uint8_t nonce_buf[static WG_PSA_NONCE_SIZE],
			     int key_usage, const uint8_t *key, psa_key_id_t *key_id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Build nonce:  4 bytes of zeros + 8 bytes little-endian counter */
	memset(nonce_buf, 0, 4);
	sys_put_le64(nonce, nonce_buf + 4);

	psa_set_key_usage_flags(&attr, key_usage);
	psa_set_key_algorithm(&attr, PSA_ALG_CHACHA20_POLY1305);
	psa_set_key_type(&attr, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&attr, 256);

	status = psa_import_key(&attr, key, WG_PSA_KEY_SIZE, key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to import AEAD key: %d", status);
		return -EINVAL;
	}

	return 0;
}

static int wg_psa_aead_encrypt(uint8_t *dst,
			       const uint8_t *src, size_t src_len,
			       const uint8_t *ad, size_t ad_len,
			       uint64_t nonce,
			       const uint8_t *key)
{
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t nonce_buf[WG_PSA_NONCE_SIZE];
	size_t output_len;
	psa_status_t status;
	int ret;

	ret = wg_psa_aead_setup(nonce, nonce_buf, PSA_KEY_USAGE_ENCRYPT, key, &key_id);
	if (ret < 0) {
		return ret;
	}

	status = psa_aead_encrypt(key_id, PSA_ALG_CHACHA20_POLY1305,
				  nonce_buf, sizeof(nonce_buf),
				  ad, ad_len,
				  src, src_len,
				  dst, src_len + WG_PSA_TAG_SIZE,
				  &output_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		NET_DBG("AEAD encrypt failed: %d", status);
		return -EINVAL;
	}

	return 0;
}

static bool wg_psa_aead_decrypt(uint8_t *dst,
				const uint8_t *src, size_t src_len,
				const uint8_t *ad, size_t ad_len,
				uint64_t nonce,
				const uint8_t *key)
{
	psa_status_t status;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t nonce_buf[WG_PSA_NONCE_SIZE];
	size_t output_len;
	size_t plaintext_len;
	int ret;

	if (src_len < WG_PSA_TAG_SIZE) {
		return false;
	}

	plaintext_len = src_len - WG_PSA_TAG_SIZE;

	ret = wg_psa_aead_setup(nonce, nonce_buf, PSA_KEY_USAGE_DECRYPT, key, &key_id);
	if (ret < 0) {
		return ret;
	}

	status = psa_aead_decrypt(key_id, PSA_ALG_CHACHA20_POLY1305,
				  nonce_buf, sizeof(nonce_buf),
				  ad, ad_len,
				  src, src_len,
				  dst, plaintext_len,
				  &output_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		return false;
	}

	if (output_len != plaintext_len) {
		NET_DBG("Unexpected plaintext size: %zu", output_len);
		return false;
	}

	return true;
}

/*
 * XChaCha20-Poly1305 using hybrid approach:
 * - HChaCha20: reference implementation (not available in PSA)
 * - ChaCha20-Poly1305: PSA implementation
 */
static int wg_psa_xaead_encrypt(uint8_t *dst,
				const uint8_t *src, size_t src_len,
				const uint8_t *ad, size_t ad_len,
				const uint8_t *nonce,
				const uint8_t *key)
{
	uint8_t subkey[CHACHA20_KEY_SIZE];
	uint64_t new_nonce;
	int ret;

	/* Use HChaCha20 to derive subkey from first 16 bytes of nonce */
	hchacha20(subkey, nonce, key);

	/* Extract last 8 bytes of 24-byte nonce as little-endian uint64 */
	new_nonce = U8TO64_LITTLE(nonce + 16);

	/* Use PSA ChaCha20-Poly1305 with derived subkey */
	ret = wg_psa_aead_encrypt(dst, src, src_len, ad, ad_len, new_nonce, subkey);

	crypto_zero(subkey, sizeof(subkey));

	return ret;
}

static bool wg_psa_xaead_decrypt(uint8_t *dst,
				 const uint8_t *src, size_t src_len,
				 const uint8_t *ad, size_t ad_len,
				 const uint8_t *nonce,
				 const uint8_t *key)
{
	uint8_t subkey[CHACHA20_KEY_SIZE];
	uint64_t new_nonce;
	bool result;

	/* Use HChaCha20 to derive subkey from first 16 bytes of nonce */
	hchacha20(subkey, nonce, key);

	/* Extract last 8 bytes of 24-byte nonce as little-endian uint64 */
	new_nonce = U8TO64_LITTLE(nonce + 16);

	/* Use PSA ChaCha20-Poly1305 with derived subkey */
	result = wg_psa_aead_decrypt(dst, src, src_len, ad, ad_len, new_nonce, subkey);

	crypto_zero(subkey, sizeof(subkey));

	return result;
}

static int wg_psa_random(uint8_t *buf, size_t len)
{
	psa_status_t status;

	status = psa_generate_random(buf, len);
	if (status != PSA_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}
