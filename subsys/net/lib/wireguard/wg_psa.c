/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <psa/crypto.h>
#include "wg_psa.h"

#include "crypto/crypto.h"
#include "crypto/refc/hchacha20.h"

LOG_MODULE_DECLARE(wireguard, CONFIG_WIREGUARD_LOG_LEVEL);

#define WG_PSA_KEY_SIZE         32
#define WG_PSA_NONCE_SIZE       12
#define WG_PSA_TAG_SIZE         16

static bool psa_initialized;

int wg_psa_init(void)
{
	psa_status_t status;

	if (psa_initialized) {
		return 0;
	}

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA crypto init failed: %d", status);
		return -EIO;
	}

	psa_initialized = true;
	return 0;
}

int wg_psa_x25519(uint8_t *shared_secret,
		  const uint8_t *private_key,
		  const uint8_t *public_key)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	size_t output_len;

	if (!psa_initialized) {
		if (wg_psa_init() != 0) {
			return -EIO;
		}
	}

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr,
			 PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, 255);

	status = psa_import_key(&attr, private_key, WG_PSA_KEY_SIZE, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import X25519 key:  %d", status);
		return -EINVAL;
	}

	status = psa_raw_key_agreement(PSA_ALG_ECDH, key_id,
				       public_key, WG_PSA_KEY_SIZE,
				       shared_secret, WG_PSA_KEY_SIZE,
				       &output_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		LOG_ERR("X25519 key agreement failed: %d", status);
		return -EINVAL;
	}

	return 0;
}

int wg_psa_x25519_public_key(uint8_t *public_key,
			     const uint8_t *private_key)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	size_t output_len;

	if (!psa_initialized) {
		if (wg_psa_init() != 0) {
			return -EIO;
		}
	}

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr,
			 PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, 255);

	status = psa_import_key(&attr, private_key, WG_PSA_KEY_SIZE, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import X25519 key: %d", status);
		return -EINVAL;
	}

	status = psa_export_public_key(key_id, public_key, WG_PSA_KEY_SIZE,
				       &output_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to export public key: %d", status);
		return -EINVAL;
	}

	return 0;
}

int wg_psa_aead_encrypt(uint8_t *dst,
			const uint8_t *src, size_t src_len,
			const uint8_t *ad, size_t ad_len,
			uint64_t nonce,
			const uint8_t *key)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t nonce_buf[WG_PSA_NONCE_SIZE];
	size_t output_len;

	if (!psa_initialized) {
		if (wg_psa_init() != 0) {
			return -EIO;
		}
	}

	/* Build nonce:  4 bytes of zeros + 8 bytes little-endian counter */
	memset(nonce_buf, 0, 4);
	sys_put_le64(nonce, nonce_buf + 4);

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_CHACHA20_POLY1305);
	psa_set_key_type(&attr, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&attr, 256);

	status = psa_import_key(&attr, key, WG_PSA_KEY_SIZE, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import AEAD key: %d", status);
		return -EINVAL;
	}

	status = psa_aead_encrypt(key_id, PSA_ALG_CHACHA20_POLY1305,
				  nonce_buf, sizeof(nonce_buf),
				  ad, ad_len,
				  src, src_len,
				  dst, src_len + WG_PSA_TAG_SIZE,
				  &output_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		LOG_ERR("AEAD encrypt failed: %d", status);
		return -EINVAL;
	}

	return 0;
}

bool wg_psa_aead_decrypt(uint8_t *dst,
			 const uint8_t *src, size_t src_len,
			 const uint8_t *ad, size_t ad_len,
			 uint64_t nonce,
			 const uint8_t *key)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t nonce_buf[WG_PSA_NONCE_SIZE];
	size_t output_len;
	size_t plaintext_len;

	if (src_len < WG_PSA_TAG_SIZE) {
		return false;
	}

	plaintext_len = src_len - WG_PSA_TAG_SIZE;

	if (!psa_initialized) {
		if (wg_psa_init() != 0) {
			return false;
		}
	}

	/* Build nonce: 4 bytes of zeros + 8 bytes little-endian counter */
	memset(nonce_buf, 0, 4);
	sys_put_le64(nonce, nonce_buf + 4);

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_CHACHA20_POLY1305);
	psa_set_key_type(&attr, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&attr, 256);

	status = psa_import_key(&attr, key, WG_PSA_KEY_SIZE, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import AEAD key: %d", status);
		return false;
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

	return true;
}

/*
 * XChaCha20-Poly1305 using hybrid approach:
 * - HChaCha20: reference implementation (not available in PSA)
 * - ChaCha20-Poly1305: PSA implementation
 */
int wg_psa_xaead_encrypt(uint8_t *dst,
			 const uint8_t *src, size_t src_len,
			 const uint8_t *ad, size_t ad_len,
			 const uint8_t *nonce,
			 const uint8_t *key)
{
	uint8_t subkey[CHACHA20_KEY_SIZE];
	uint64_t new_nonce;

	if (!psa_initialized) {
		if (wg_psa_init() != 0) {
			return -EIO;
		}
	}

	/* Use HChaCha20 to derive subkey from first 16 bytes of nonce */
	hchacha20(subkey, nonce, key);

	/* Extract last 8 bytes of 24-byte nonce as little-endian uint64 */
	new_nonce = U8TO64_LITTLE(nonce + 16);

	/* Use PSA ChaCha20-Poly1305 with derived subkey */
	wg_psa_aead_encrypt(dst, src, src_len, ad, ad_len, new_nonce, subkey);

	crypto_zero(subkey, sizeof(subkey));

	return 0;
}

bool wg_psa_xaead_decrypt(uint8_t *dst,
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

int wg_psa_random(uint8_t *buf, size_t len)
{
	psa_status_t status;

	if (!psa_initialized) {
		if (wg_psa_init() != 0) {
			return -EIO;
		}
	}

	status = psa_generate_random(buf, len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Random generation failed: %d", status);
		return -EIO;
	}

	return 0;
}
