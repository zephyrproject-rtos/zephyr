/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/store.h>
#include <zephyr/init.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <stdio.h>

SYS_INIT(settings_subsys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

enum { NAME_BUF_SIZE = sizeof(CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_PREFIX) - 1
		       + 2 * (sizeof(secure_storage_its_uid_t) + 1) };

BUILD_ASSERT(NAME_BUF_SIZE <= SETTINGS_MAX_NAME_LEN + 1);
BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE <= SETTINGS_MAX_VAL_LEN);

static void make_name(secure_storage_its_uid_t uid, char name[static NAME_BUF_SIZE])
{
	int ret;

	ret = snprintf(name, NAME_BUF_SIZE, "%s%x/%llx",
		       CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_PREFIX, uid.caller_id, uid.uid);
	__ASSERT_NO_MSG(ret > 0 && ret < NAME_BUF_SIZE);
}

psa_status_t secure_storage_its_store_set(secure_storage_its_uid_t uid,
					  size_t data_length, const void *data)
{
	char name[NAME_BUF_SIZE];

	make_name(uid, name);

	switch (settings_save_one(name, data, data_length)) {
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
	char name[NAME_BUF_SIZE];
	struct load_params load_params = {.data_size = data_size, .data = data, .ret = -ENOENT};

	make_name(uid, name);

	settings_load_subtree_direct(name, load_direct_setting, &load_params);
	if (load_params.ret > 0) {
		*data_length = load_params.ret;
		return PSA_SUCCESS;
	} else if (load_params.ret == 0 || load_params.ret == -ENOENT) {
		return PSA_ERROR_DOES_NOT_EXIST;
	} else {
		return PSA_ERROR_STORAGE_FAILURE;
	}
}

psa_status_t secure_storage_its_store_remove(secure_storage_its_uid_t uid)
{
	char name[NAME_BUF_SIZE];

	make_name(uid, name);

	if (settings_delete(name) != 0) {
		return PSA_ERROR_STORAGE_FAILURE;
	}
	return PSA_SUCCESS;
}
