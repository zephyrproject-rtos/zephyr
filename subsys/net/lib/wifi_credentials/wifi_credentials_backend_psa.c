/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "psa/crypto.h"

#include "wifi_credentials_internal.h"

LOG_MODULE_REGISTER(wifi_credentials_backend, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

#define WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN                                               \
	(PSA_KEY_ID_USER_MIN + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_OFFSET)

BUILD_ASSERT((WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN + CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES) <=
		     PSA_KEY_ID_USER_MAX,
	     "WIFI credentials management PSA key id range exceeds PSA_KEY_ID_USER_MAX.");

int wifi_credentials_backend_init(void)
{
	psa_status_t ret;
	uint8_t buf[ENTRY_MAX_LEN];

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		size_t length_read = 0;
		size_t key_id = i + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN;

		ret = psa_export_key(key_id, buf, ARRAY_SIZE(buf), &length_read);
		if (ret == PSA_SUCCESS && length_read == ENTRY_MAX_LEN) {
			wifi_credentials_cache_ssid(i, (struct wifi_credentials_header *)buf);
		} else if (ret != PSA_ERROR_INVALID_HANDLE) {
			LOG_ERR("psa_export_key failed, err: %d", ret);
			return -EFAULT;
		}
	}

	return 0;
}

int wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len)
{
	psa_status_t ret;
	psa_key_attributes_t key_attributes = {0};
	psa_key_id_t key_id;

	psa_set_key_id(&key_attributes, idx + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN);
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_EXPORT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_NONE);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RAW_DATA);
	psa_set_key_bits(&key_attributes, buf_len * 8);

	ret = psa_import_key(&key_attributes, buf, buf_len, &key_id);
	if (ret == PSA_ERROR_ALREADY_EXISTS) {
		LOG_ERR("psa_import_key failed, duplicate key: %d", ret);
		return -EEXIST;
	} else if (ret != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed, err: %d", ret);
		return -EFAULT;
	}

	return 0;
}

int wifi_credentials_delete_entry(size_t idx)
{
	psa_status_t ret = psa_destroy_key(idx + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN);

	if (ret != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed, err: %d", ret);
		return -EFAULT;
	}

	return 0;
}

int wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	size_t length_read = 0;
	size_t key_id = idx + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN;
	psa_status_t ret;

	ret = psa_export_key(key_id, buf, buf_len, &length_read);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("psa_export_key failed, err: %d", ret);
		return -EFAULT;
	}

	if (buf_len != length_read) {
		return -EIO;
	}

	return 0;
}
