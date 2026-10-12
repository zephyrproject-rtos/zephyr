/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/psa/ps_ids.h>
#include <psa/protected_storage.h>

#include "wifi_credentials_internal.h"

LOG_MODULE_REGISTER(wifi_credentials_backend, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

BUILD_ASSERT(CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES <= ZEPHYR_PSA_WIFI_CREDENTIALS_PS_UID_RANGE_SIZE,
	     "Wi-Fi credentials management PSA PS UID range exceeds officially allocated range.");

#if defined(CONFIG_SECURE_STORAGE)
BUILD_ASSERT(ENTRY_MAX_LEN <= CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE,
	     "CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE is too small for a Wi-Fi credentials entry.");
#endif

#define PSA_PS_ERR_FMT "psa_ps_%s failed, err: %d"

static psa_storage_uid_t entry_uid(size_t idx)
{
	return ZEPHYR_PSA_WIFI_CREDENTIALS_PS_UID_RANGE_BEGIN + idx;
}

int wifi_credentials_backend_init(void)
{
	psa_status_t ret;
	uint8_t buf[ENTRY_MAX_LEN];

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		struct psa_storage_info_t info;
		size_t length_read = 0;

		ret = psa_ps_get_info(entry_uid(i), &info);
		if (ret == PSA_ERROR_DOES_NOT_EXIST) {
			continue;
		} else if (ret != PSA_SUCCESS) {
			LOG_ERR(PSA_PS_ERR_FMT, "get_info", ret);
			return -EFAULT;
		}

		/* An entry stored with a different configuration, for example with another
		 * CONFIG_WIFI_CREDENTIALS_SAE_PASSWORD_LENGTH, cannot be used. Skip it so that
		 * the other entries can still be used and its slot can be overwritten.
		 */
		if (info.size != ENTRY_MAX_LEN) {
			LOG_WRN("Skipping entry %zu, its size %zu does not match %zu", i,
				(size_t)info.size, ENTRY_MAX_LEN);
			continue;
		}

		ret = psa_ps_get(entry_uid(i), 0, sizeof(buf), buf, &length_read);
		if (ret != PSA_SUCCESS || length_read != ENTRY_MAX_LEN) {
			LOG_ERR(PSA_PS_ERR_FMT, "get", ret);
			return -EFAULT;
		}

		wifi_credentials_cache_ssid(i, (struct wifi_credentials_header *)buf);
	}

	return 0;
}

int wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len)
{
	psa_status_t ret = psa_ps_set(entry_uid(idx), buf_len, buf, PSA_STORAGE_FLAG_NONE);

	if (ret != PSA_SUCCESS) {
		LOG_ERR(PSA_PS_ERR_FMT, "set", ret);
		return -EFAULT;
	}

	return 0;
}

int wifi_credentials_delete_entry(size_t idx)
{
	psa_status_t ret = psa_ps_remove(entry_uid(idx));

	if (ret != PSA_SUCCESS) {
		LOG_ERR(PSA_PS_ERR_FMT, "remove", ret);
		return -EFAULT;
	}

	return 0;
}

int wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	size_t length_read = 0;
	psa_status_t ret = psa_ps_get(entry_uid(idx), 0, buf_len, buf, &length_read);

	if (ret != PSA_SUCCESS) {
		LOG_ERR(PSA_PS_ERR_FMT, "get", ret);
		return -EFAULT;
	}

	if (buf_len != length_read) {
		return -EIO;
	}

	return 0;
}
