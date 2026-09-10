/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <psa/crypto.h>
#include <mbedtls/platform_util.h>
#include <string.h>

#include "crypto.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_crypto, CONFIG_LORAWAN_LOG_LEVEL);

#define LWAN_MIC_SIZE		4
#define AES_KEY_SIZE		16
#define AES_BLOCK_SIZE		16
#define KEY_TYPE_APP_S		0x02
#define KEY_TYPE_FNWK_S_INT	0x01
#define CTR_AI_TYPE		0x01

int lwan_crypto_init(void)
{
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed: %d", (int)status);
		return -EIO;
	}

	return 0;
}

static psa_key_id_t import_aes_key(psa_key_usage_t usage,
				   psa_algorithm_t algorithm,
				   const uint8_t key[16])
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;

	psa_set_key_usage_flags(&attr, usage);
	psa_set_key_algorithm(&attr, algorithm);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 128);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	status = psa_import_key(&attr, key, AES_KEY_SIZE, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key (0x%08x) failed: %d",
			(unsigned int)algorithm, (int)status);
		return PSA_KEY_ID_NULL;
	}

	return key_id;
}

psa_key_id_t lwan_crypto_import_cmac_key(const uint8_t key[16])
{
	return import_aes_key(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC, key);
}

psa_key_id_t lwan_crypto_import_ecb_key(const uint8_t key[16])
{
	return import_aes_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key);
}

static int ecb_encrypt_block(psa_key_id_t key_id,
			     const uint8_t in[16], uint8_t out[16])
{
	psa_status_t status;
	size_t out_len;

	status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
				    in, AES_BLOCK_SIZE, out, AES_BLOCK_SIZE,
				    &out_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_encrypt failed: %d", (int)status);
		return -EIO;
	}

	return 0;
}

int lwan_crypto_compute_mic(psa_key_id_t cmac_key, const uint8_t *msg,
			    size_t msg_len, uint8_t mic[4])
{
	psa_status_t status;
	uint8_t cmac[AES_BLOCK_SIZE];
	size_t cmac_len;

	status = psa_mac_compute(cmac_key, PSA_ALG_CMAC, msg, msg_len,
				 cmac, sizeof(cmac), &cmac_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_mac_compute failed: %d", (int)status);
		return -EIO;
	}

	memcpy(mic, cmac, LWAN_MIC_SIZE);

	return 0;
}

int lwan_crypto_decrypt_join_accept(psa_key_id_t ecb_key,
				    uint8_t *payload, size_t len)
{
	int ret;

	/*
	 * LoRaWAN join accept "decryption" uses AES encrypt (spec quirk).
	 * Process each 16-byte block in-place.
	 */
	if (len != AES_BLOCK_SIZE && len != 2 * AES_BLOCK_SIZE) {
		LOG_ERR("Invalid join accept length: %zu", len);
		return -EINVAL;
	}

	for (size_t i = 0; i < len; i += AES_BLOCK_SIZE) {
		ret = ecb_encrypt_block(ecb_key, &payload[i], &payload[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int lwan_crypto_derive_session_keys(psa_key_id_t nwk_ecb_key,
				    const uint8_t join_nonce[3],
				    const uint8_t net_id[3],
				    uint16_t dev_nonce,
				    psa_key_id_t *app_s_ecb,
				    psa_key_id_t *fnwk_cmac,
				    psa_key_id_t *fnwk_ecb)
{
	uint8_t raw_key[AES_KEY_SIZE];
	uint8_t input[AES_BLOCK_SIZE];
	int ret;

	*app_s_ecb = PSA_KEY_ID_NULL;
	*fnwk_cmac = PSA_KEY_ID_NULL;
	*fnwk_ecb = PSA_KEY_ID_NULL;

	/*
	 * LoRaWAN 1.0.x key derivation:
	 *   key = aes128_encrypt(NwkKey, type | JoinNonce | NetID | DevNonce | pad)
	 * Where type = 0x02 for AppSKey, 0x01 for FNwkSIntKey
	 */
	memset(input, 0, sizeof(input));
	input[0] = KEY_TYPE_APP_S;
	memcpy(&input[1], join_nonce, 3);
	memcpy(&input[4], net_id, 3);
	sys_put_le16(dev_nonce, &input[7]);

	/* Derive and import AppSKey */
	ret = ecb_encrypt_block(nwk_ecb_key, input, raw_key);
	if (ret != 0) {
		goto err;
	}

	*app_s_ecb = lwan_crypto_import_ecb_key(raw_key);
	if (*app_s_ecb == PSA_KEY_ID_NULL) {
		ret = -EIO;
		goto err;
	}

	/* Derive and import FNwkSIntKey (needs both CMAC and ECB) */
	input[0] = KEY_TYPE_FNWK_S_INT;
	ret = ecb_encrypt_block(nwk_ecb_key, input, raw_key);
	if (ret != 0) {
		goto err;
	}

	*fnwk_cmac = lwan_crypto_import_cmac_key(raw_key);
	if (*fnwk_cmac == PSA_KEY_ID_NULL) {
		ret = -EIO;
		goto err;
	}

	*fnwk_ecb = lwan_crypto_import_ecb_key(raw_key);
	if (*fnwk_ecb == PSA_KEY_ID_NULL) {
		ret = -EIO;
		goto err;
	}

	mbedtls_platform_zeroize(raw_key, sizeof(raw_key));
	return 0;

err:
	mbedtls_platform_zeroize(raw_key, sizeof(raw_key));
	psa_destroy_key(*app_s_ecb);
	psa_destroy_key(*fnwk_cmac);
	psa_destroy_key(*fnwk_ecb);
	*app_s_ecb = PSA_KEY_ID_NULL;
	*fnwk_cmac = PSA_KEY_ID_NULL;
	*fnwk_ecb = PSA_KEY_ID_NULL;
	return ret;
}

int lwan_crypto_payload_encrypt(psa_key_id_t ecb_key, uint32_t dev_addr,
				uint32_t fcnt, uint8_t dir,
				uint8_t *payload, size_t len)
{
	uint8_t a_block[AES_BLOCK_SIZE];
	uint8_t s_block[AES_BLOCK_SIZE];
	int ret;

	/*
	 * LoRaWAN payload encryption uses AES-128 in CTR mode:
	 *   Ai = 0x01 | 0x00000000 | dir | DevAddr(4,LE) | FCnt(4,LE) | 0x00 | i
	 *   Si = aes128_encrypt(key, Ai)
	 *   encrypted = payload XOR S1||S2||...
	 */
	memset(a_block, 0, sizeof(a_block));
	a_block[0] = CTR_AI_TYPE;
	a_block[5] = dir;
	sys_put_le32(dev_addr, &a_block[6]);
	sys_put_le32(fcnt, &a_block[10]);

	for (size_t i = 0; i < len; i += AES_BLOCK_SIZE) {
		a_block[15] = (uint8_t)((i / AES_BLOCK_SIZE) + 1);

		ret = ecb_encrypt_block(ecb_key, a_block, s_block);
		if (ret != 0) {
			return ret;
		}

		size_t chunk = MIN(len - i, (size_t)AES_BLOCK_SIZE);

		for (size_t j = 0; j < chunk; j++) {
			payload[i + j] ^= s_block[j];
		}
	}

	return 0;
}
