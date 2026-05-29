/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/store.h>
#include <zephyr/secure_storage/its/store/settings_get.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <stdio.h>

LOG_MODULE_DECLARE(secure_storage, CONFIG_SECURE_STORAGE_LOG_LEVEL);

static int init_settings_subsys(void)
{
	const int ret = settings_subsys_init();

	if (ret) {
		LOG_DBG("Failed. (%d)", ret);
	}
	return ret;
}
SYS_INIT(init_settings_subsys, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_MAX_LEN <= SETTINGS_MAX_NAME_LEN);

#ifndef CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_CUSTOM

BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_MAX_LEN ==
	     sizeof(CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_PREFIX) - 1
	     + 1 + 1 /* caller ID + '/' */
	     + 2 * sizeof(psa_storage_uid_t) /* hex UID */);

void secure_storage_its_store_settings_get_name(
	secure_storage_its_uid_t uid,
	char name[static SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE])
{
	int ret;

#ifdef CONFIG_SECURE_STORAGE_64_BIT_UID
	ret = snprintf(name, SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE,
		       CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_PREFIX "%x/%llx",
		       uid.caller_id, (unsigned long long)uid.uid);
#else
	ret = snprintf(name, SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE,
		       CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_PREFIX "%x/%lx",
		       uid.caller_id, (unsigned long)uid.uid);
#endif
	__ASSERT_NO_MSG(ret > 0 && ret < SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE);
}

#endif /* !CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_CUSTOM */

psa_status_t secure_storage_its_store_set(secure_storage_its_uid_t uid,
					  size_t data_length, const void *data)
{
	int ret;
	char name[SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE];

	secure_storage_its_store_settings_get_name(uid, name);

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

struct load_params {
	const size_t data_size;
	uint8_t *const data;
	ssize_t ret;
};

static int load_direct_setting(const char *key, size_t len, settings_read_cb read_cb,
			       void *cb_arg, void *param)
{
	(void)key;
	struct load_params *load_params = param;

	load_params->ret = read_cb(cb_arg, load_params->data, MIN(load_params->data_size, len));
	return 0;
}

psa_status_t secure_storage_its_store_get(secure_storage_its_uid_t uid, size_t data_size,
					  void *data, size_t *data_length)
{
	psa_status_t ret;
	char name[SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE];
	struct load_params load_params = {.data_size = data_size, .data = data, .ret = -ENOENT};

	secure_storage_its_store_settings_get_name(uid, name);

	settings_load_subtree_direct(name, load_direct_setting, &load_params);
	if (load_params.ret > 0) {
		*data_length = load_params.ret;
		ret = PSA_SUCCESS;
	} else if (load_params.ret == 0 || load_params.ret == -ENOENT) {
		ret = PSA_ERROR_DOES_NOT_EXIST;
	} else {
		ret = PSA_ERROR_STORAGE_FAILURE;
	}
	LOG_DBG("%s %s for up to %zu bytes. (%zd)", (ret != PSA_ERROR_STORAGE_FAILURE) ?
		"Loaded" : "Failed to load", name, data_size, load_params.ret);
	return ret;
}

psa_status_t secure_storage_its_store_remove(secure_storage_its_uid_t uid)
{
	int ret;
	char name[SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE];

	secure_storage_its_store_settings_get_name(uid, name);

	ret = settings_delete(name);
	LOG_DBG("%s %s. (%d)", ret ? "Failed to delete" : "Deleted", name, ret);

	return ret ? PSA_ERROR_STORAGE_FAILURE : PSA_SUCCESS;
}
