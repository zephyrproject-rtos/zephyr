/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>

#include "psa/crypto.h"

#include "common/bt_str.h"

#include "hci_core.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_host_crypto);

int prng_init(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init() failed %d", status);
		return -EIO;
	}
	return 0;
}

#if defined(CONFIG_BT_HOST_CRYPTO_PRNG)
int bt_rand(void *buf, size_t len)
{
	psa_status_t status = psa_generate_random(buf, len);

	if (status == PSA_SUCCESS) {
		return 0;
	}

	LOG_ERR("psa_generate_random() failed %d", status);
	return -EIO;
}
#else /* !CONFIG_BT_HOST_CRYPTO_PRNG */
int bt_rand(void *buf, size_t len)
{
	CHECKIF(buf == NULL || len == 0) {
		return -EINVAL;
	}

	return bt_hci_le_rand(buf, len);
}
#endif /* CONFIG_BT_HOST_CRYPTO_PRNG */

int bt_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
	psa_status_t status, destroy_status;
	size_t out_len;
	uint8_t tmp[16];

	CHECKIF(key == NULL || plaintext == NULL || enc_data == NULL) {
		return -EINVAL;
	}

	LOG_DBG("key %s", bt_hex(key, 16));
	LOG_DBG("plaintext %s", bt_hex(plaintext, 16));

	sys_memcpy_swap(tmp, key, 16);

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 128);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECB_NO_PADDING);
	status = psa_import_key(&attr, tmp, 16, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import AES key %d", status);
		return -EINVAL;
	}

	sys_memcpy_swap(tmp, plaintext, 16);

	status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, tmp, 16,
					enc_data, 16, &out_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("AES encryption failed %d", status);
	}

	destroy_status = psa_destroy_key(key_id);
	if (destroy_status != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy AES key %d", destroy_status);
	}

	if ((status != PSA_SUCCESS) || (destroy_status != PSA_SUCCESS)) {
		return -EIO;
	}

	sys_mem_swap(enc_data, 16);

	LOG_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
	psa_status_t status, destroy_status;
	size_t out_len;

	CHECKIF(key == NULL || plaintext == NULL || enc_data == NULL) {
		return -EINVAL;
	}

	LOG_DBG("key %s", bt_hex(key, 16));
	LOG_DBG("plaintext %s", bt_hex(plaintext, 16));

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 128);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECB_NO_PADDING);
	status = psa_import_key(&attr, key, 16, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import AES key %d", status);
		return -EINVAL;
	}

	status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
				plaintext, 16, enc_data, 16, &out_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("AES encryption failed %d", status);
	}

	destroy_status = psa_destroy_key(key_id);
	if (destroy_status != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy AES key %d", destroy_status);
	}

	if ((status != PSA_SUCCESS) || (destroy_status != PSA_SUCCESS)) {
		return -EIO;
	}

	LOG_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}
