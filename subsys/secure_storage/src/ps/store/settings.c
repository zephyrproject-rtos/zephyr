/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/ps/store.h>
#include <zephyr/secure_storage/ps/store/settings.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <stdio.h>

LOG_MODULE_DECLARE(secure_storage_ps, CONFIG_SECURE_STORAGE_LOG_LEVEL);

static int init_settings_subsys(void)
{
	const int ret = settings_subsys_init();

	if (ret) {
		LOG_DBG("Failed. (%d)", ret);
	}
	return ret;
}
SYS_INIT(init_settings_subsys, APPLICATION, CONFIG_SECURE_STORAGE_INIT_PRIORITY);

BUILD_ASSERT(CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_NAME_MAX_LEN <= SETTINGS_MAX_NAME_LEN);

#ifndef CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_NAME_CUSTOM

BUILD_ASSERT(CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_NAME_MAX_LEN ==
	     sizeof(CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_PREFIX) - 1
	     + 2 * sizeof(psa_storage_uid_t) /* hex UID */);

void secure_storage_ps_store_settings_get_name(
	psa_storage_uid_t uid,
	char name[static SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE])
{
	int ret;

#ifdef CONFIG_SECURE_STORAGE_64_BIT_UID
	ret = snprintf(name, SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE,
		       CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_PREFIX "%llx",
		       (unsigned long long)uid);
#else
	ret = snprintf(name, SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE,
		       CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_PREFIX "%lx",
		       (unsigned long)uid);
#endif
	__ASSERT_NO_MSG(ret > 0 && ret < SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE);
}

#endif /* !CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_NAME_CUSTOM */

psa_status_t secure_storage_ps_store_set(psa_storage_uid_t uid,
					  size_t data_length, const void *data)
{
	int ret;
	char name[SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE];

	secure_storage_ps_store_settings_get_name(uid, name);

	ret = settings_save_one(name, data, data_length);
	LOG_DBG("%s %s with %zu bytes. (%d)",
		(ret == 0) ? "Saved" : "Failed to save", name, data_length, ret);

	switch (ret) {
	case 0:
		return PSA_SUCCESS;
	case -ENOMEM:
	case -ENOSPC:
		return PSA_ERROR_INSUFFICIENT_STORAGE;
	default:
		return PSA_ERROR_STORAGE_FAILURE;
	}
}

psa_status_t secure_storage_ps_store_get(psa_storage_uid_t uid, size_t data_size,
					  void *data, size_t *data_length)
{
	psa_status_t ret;
	ssize_t settings_ret;
	char name[SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE];

	secure_storage_ps_store_settings_get_name(uid, name);

	settings_ret = settings_load_one(name, data, data_size);
	if (settings_ret > (ssize_t)data_size) {
		/* Note: 'settings_load_one()' fills 'data' with at most 'data_size'
		 * bytes but returns the length of the stored value, which can be
		 * larger. If that happens then return a data corrupted failure.
		 */
		ret = PSA_ERROR_DATA_CORRUPT;
	} else if (settings_ret > 0) {
		*data_length = settings_ret;
		ret = PSA_SUCCESS;
	} else if (settings_ret == 0 || settings_ret == -ENOENT) {
		ret = PSA_ERROR_DOES_NOT_EXIST;
	} else {
		ret = PSA_ERROR_STORAGE_FAILURE;
	}
	LOG_DBG("%s %s for up to %zu bytes. (%zd)", (ret == PSA_SUCCESS) ?
		"Loaded" : "Failed to load", name, data_size, settings_ret);
	return ret;
}

psa_status_t secure_storage_ps_store_remove(psa_storage_uid_t uid)
{
	int ret;
	char name[SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE];

	secure_storage_ps_store_settings_get_name(uid, name);

	ret = settings_delete(name);
	LOG_DBG("%s %s. (%d)", ret ? "Failed to delete" : "Deleted", name, ret);

	return ret ? PSA_ERROR_STORAGE_FAILURE : PSA_SUCCESS;
}
