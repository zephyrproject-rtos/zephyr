/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/psa/key_ids.h>
#include "psa/crypto.h"

#include "wifi_credentials_internal.h"

LOG_MODULE_REGISTER(wifi_credentials_backend, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

BUILD_ASSERT(CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES <= ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_SIZE,
	     "Wi-Fi credentials management PSA key ID range exceeds officially allocated range.");

int wifi_credentials_backend_init(void)
{
	psa_status_t ret;
	uint8_t buf[ENTRY_MAX_LEN];

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		size_t length_read = 0;
		size_t key_id = i + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN;

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

	psa_set_key_id(&key_attributes, idx + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN);
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
	psa_status_t ret = psa_destroy_key(idx + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN);

	if (ret != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed, err: %d", ret);
		return -EFAULT;
	}

	return 0;
}

int wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	size_t length_read = 0;
	size_t key_id = idx + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN;
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
