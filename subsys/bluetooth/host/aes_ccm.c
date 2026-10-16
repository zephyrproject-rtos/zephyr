/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>
#include <psa/crypto_struct.h>
#include <psa/crypto_types.h>
#include <psa/crypto_values.h>

#include "common/bt_str.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
LOG_MODULE_REGISTER(bt_aes_ccm);

static int psa_status_to_errno(psa_status_t status)
{
	if (status == PSA_ERROR_INVALID_ARGUMENT) {
		return -EINVAL;
	}

	if (status == PSA_ERROR_INVALID_SIGNATURE) {
		return -EBADMSG;
	}

	return -EIO;
}

static int bt_ccm_psa_op(bool encrypt, const uint8_t key[16], uint8_t nonce[13],
			 const uint8_t *input, size_t len, const uint8_t *aad,
			 size_t aad_len, uint8_t *output, size_t mic_size)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
	psa_status_t status;
	psa_status_t destroy_status;
	psa_algorithm_t alg = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, mic_size);
	size_t out_len;
	int err;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 128);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, alg);

	status = psa_import_key(&attr, key, 16, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import AES key %d", status);
		return -EINVAL;
	}

	if (encrypt) {
		status = psa_aead_encrypt(key_id, alg, nonce, 13, aad, aad_len, input, len,
					  output, len + mic_size, &out_len);
		if (status != PSA_SUCCESS || out_len != len + mic_size) {
			err = psa_status_to_errno(status);
		} else {
			err = 0;
		}
	} else {
		status = psa_aead_decrypt(key_id, alg, nonce, 13, aad, aad_len, input,
					  len + mic_size, output, len, &out_len);
		if (status != PSA_SUCCESS || out_len != len) {
			err = psa_status_to_errno(status);
		} else {
			err = 0;
		}
	}

	destroy_status = psa_destroy_key(key_id);
	if (destroy_status != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy AES key %d", destroy_status);
		if (err == 0) {
			err = -EIO;
		}
	}

	return err;
}

int bt_ccm_decrypt(const uint8_t key[16], uint8_t nonce[13],
		   const uint8_t *enc_data, size_t len, const uint8_t *aad,
		   size_t aad_len, uint8_t *plaintext, size_t mic_size)
{
	if (aad_len >= 0xff00 || mic_size > 16 || len > UINT16_MAX) {
		return -EINVAL;
	}

	return bt_ccm_psa_op(false, key, nonce, enc_data, len, aad, aad_len, plaintext,
			     mic_size);
}

int bt_ccm_encrypt(const uint8_t key[16], uint8_t nonce[13],
		   const uint8_t *plaintext, size_t len, const uint8_t *aad,
		   size_t aad_len, uint8_t *enc_data, size_t mic_size)
{
	LOG_DBG("key %s", bt_hex(key, 16));
	LOG_DBG("nonce %s", bt_hex(nonce, 13));
	LOG_DBG("msg (len %zu) %s", len, bt_hex(plaintext, len));
	LOG_DBG("aad_len %zu mic_size %zu", aad_len, mic_size);

	if (aad_len >= 0xff00 || mic_size > 16 || len > UINT16_MAX) {
		return -EINVAL;
	}

	return bt_ccm_psa_op(true, key, nonce, plaintext, len, aad, aad_len, enc_data,
			     mic_size);
}
