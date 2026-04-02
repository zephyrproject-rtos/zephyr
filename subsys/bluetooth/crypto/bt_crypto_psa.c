/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>

#include "psa/crypto.h"

#include "common/bt_str.h"
#include "bt_crypto.h"

#define LOG_LEVEL CONFIG_BT_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_crypto);

int bt_crypto_aes_cmac(const uint8_t *key, const uint8_t *in, size_t len, uint8_t *out)
{
	psa_key_id_t key_id;
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	size_t out_size;
	psa_status_t status, destroy_status;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attr, 128);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_MESSAGE |
					   PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&key_attr, PSA_ALG_CMAC);

	status = psa_import_key(&key_attr, key, 16, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import AES key %d", status);
		return -EIO;
	}

	status = psa_mac_compute(key_id, PSA_ALG_CMAC, in, len, out, 16, &out_size);
	destroy_status = psa_destroy_key(key_id);
	if ((status != PSA_SUCCESS) || (destroy_status != PSA_SUCCESS)) {
		LOG_ERR("Failed to compute MAC %d", status);
		return -EIO;
	}

	return 0;
}
