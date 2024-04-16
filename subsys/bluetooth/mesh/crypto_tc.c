/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/cmac_mode.h>
#include <tinycrypt/ccm_mode.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>
#include <tinycrypt/hmac.h>

#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/crypto.h>

#define LOG_LEVEL CONFIG_BT_MESH_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_crypto_tc);

#include "mesh.h"
#include "crypto.h"
#include "prov.h"

static struct {
	bool is_ready;
	uint8_t private_key_be[PRIV_KEY_SIZE];
	uint8_t public_key_be[PUB_KEY_SIZE];
} dh_pair;

int bt_mesh_encrypt(const struct bt_mesh_key *key, const uint8_t plaintext[16],
		    uint8_t enc_data[16])
{
	return bt_encrypt_be(key->key, plaintext, enc_data);
}

int bt_mesh_ccm_encrypt(const struct bt_mesh_key *key, uint8_t nonce[13], const uint8_t *plaintext,
			size_t len, const uint8_t *aad, size_t aad_len, uint8_t *enc_data,
			size_t mic_size)
{
	return bt_ccm_encrypt(key->key, nonce, plaintext, len, aad, aad_len, enc_data, mic_size);
}

int bt_mesh_ccm_decrypt(const struct bt_mesh_key *key, uint8_t nonce[13], const uint8_t *enc_data,
			size_t len, const uint8_t *aad, size_t aad_len, uint8_t *plaintext,
			size_t mic_size)
{
	return bt_ccm_decrypt(key->key, nonce, enc_data, len, aad, aad_len, plaintext, mic_size);
}

int bt_mesh_aes_cmac_raw_key(const uint8_t key[16], struct bt_mesh_sg *sg, size_t sg_len,
			     uint8_t mac[16])
{
	struct tc_aes_key_sched_struct sched;
	struct tc_cmac_struct state;

	if (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	for (; sg_len; sg_len--, sg++) {
		if (tc_cmac_update(&state, sg->data, sg->len) == TC_CRYPTO_FAIL) {
			return -EIO;
		}
	}

	if (tc_cmac_final(mac, &state) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	return 0;
}

int bt_mesh_aes_cmac_mesh_key(const struct bt_mesh_key *key, struct bt_mesh_sg *sg,
			size_t sg_len, uint8_t mac[16])
{
	return bt_mesh_aes_cmac_raw_key(key->key, sg, sg_len, mac);
}

int bt_mesh_sha256_hmac_raw_key(const uint8_t key[32], struct bt_mesh_sg *sg, size_t sg_len,
				uint8_t mac[32])
{
	struct tc_hmac_state_struct h;

	if (tc_hmac_set_key(&h, key, 32) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_hmac_init(&h) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	for (; sg_len; sg_len--, sg++) {
		if (tc_hmac_update(&h, sg->data, sg->len) == TC_CRYPTO_FAIL) {
			return -EIO;
		}
	}

	if (tc_hmac_final(mac, 32, &h) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	return 0;
}

int bt_mesh_pub_key_gen(void)
{
	int rc = uECC_make_key(dh_pair.public_key_be,
			       dh_pair.private_key_be,
			       &curve_secp256r1);

	if (rc == TC_CRYPTO_FAIL) {
		dh_pair.is_ready = false;
		LOG_ERR("Failed to create public/private pair");
		return -EIO;
	}

	dh_pair.is_ready = true;

	return 0;
}

const uint8_t *bt_mesh_pub_key_get(void)
{
	return dh_pair.is_ready ? dh_pair.public_key_be : NULL;
}

int bt_mesh_dhkey_gen(const uint8_t *pub_key, const uint8_t *priv_key, uint8_t *dhkey)
{
	if (uECC_valid_public_key(pub_key, &curve_secp256r1)) {
		LOG_ERR("Public key is not valid");
		return -EIO;
	} else if (uECC_shared_secret(pub_key, priv_key ? priv_key :
							  dh_pair.private_key_be,
				      dhkey, &curve_secp256r1) != TC_CRYPTO_SUCCESS) {
		LOG_ERR("DHKey generation failed");
		return -EIO;
	}

	return 0;
}

__weak int default_CSPRNG(uint8_t *dst, unsigned int len)
{
	return !bt_rand(dst, len);
}

int bt_mesh_crypto_init(void)
{
	return 0;
}
