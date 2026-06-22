/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/authentication/fido2/fido2_types.h>
#include <zephyr/authentication/fido2/fido2_storage.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define FIDO2_SETTINGS_BASE        "fido2"
#define FIDO2_SETTINGS_CRED_PREFIX FIDO2_SETTINGS_BASE "/cred"
#define FIDO2_SETTINGS_KEY_MAX     20

static void build_key(char *buf, size_t size, int idx)
{
	snprintk(buf, size, FIDO2_SETTINGS_CRED_PREFIX "/%d", idx);
}

static int cred_slot_get(const uint8_t *cred_id, size_t cred_id_len, struct fido2_credential *out)
{
	struct fido2_credential cred;

	for (int i = 0; i < CONFIG_FIDO2_MAX_CREDENTIALS; ++i) {
		char key[FIDO2_SETTINGS_KEY_MAX];

		build_key(key, sizeof(key), i);
		if (settings_load_one(key, &cred, sizeof(cred)) != sizeof(cred)) {
			continue;
		}
		if (cred.id_len == cred_id_len && memcmp(cred.id, cred_id, cred_id_len) == 0) {
			if (out != NULL) {
				memcpy(out, &cred, sizeof(cred));
			}
			return i;
		}
	}

	return -ENOENT;
}

static int settings_backend_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Settings init failed: %d", ret);
		return ret;
	}

	LOG_INF("Credential storage: settings");
	return 0;
}

static int settings_backend_store(const struct fido2_credential *cred)
{
	struct fido2_credential temp;

	for (int i = 0; i < CONFIG_FIDO2_MAX_CREDENTIALS; ++i) {
		char key[FIDO2_SETTINGS_KEY_MAX];

		build_key(key, sizeof(key), i);
		if (settings_load_one(key, &temp, sizeof(temp)) == sizeof(temp)) {
			continue; /* slot occupied */
		}

		return settings_save_one(key, cred, sizeof(*cred));
	}

	return -ENOSPC;
}

static int settings_backend_load(const uint8_t *cred_id, size_t cred_id_len,
				 struct fido2_credential *cred)
{
	int idx;

	idx = cred_slot_get(cred_id, cred_id_len, cred);
	if (idx < 0) {
		return idx;
	}

	return 0;
}

static int settings_backend_remove(const uint8_t *cred_id, size_t cred_id_len,
				   struct fido2_credential *cred)
{
	int idx;
	char key[FIDO2_SETTINGS_KEY_MAX];

	idx = cred_slot_get(cred_id, cred_id_len, cred);
	if (idx < 0) {
		return idx;
	}

	build_key(key, sizeof(key), idx);

	return settings_delete(key);
}

static int settings_backend_find_by_rp(const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				       struct fido2_credential *creds, size_t max_creds,
				       size_t *count)
{
	struct fido2_credential cred;

	*count = 0;

	for (int i = 0; i < CONFIG_FIDO2_MAX_CREDENTIALS; ++i) {
		char key[FIDO2_SETTINGS_KEY_MAX];

		build_key(key, sizeof(key), i);
		if (settings_load_one(key, &cred, sizeof(cred)) != sizeof(cred)) {
			continue;
		}
		if (memcmp(cred.rp_id_hash, rp_id_hash, FIDO2_SHA256_SIZE) == 0) {
			if (*count < max_creds) {
				memcpy(creds + *count, &cred, sizeof(cred));
			}
			(*count)++;
		}
	}

	return 0;
}

static int settings_backend_sign_count_increment(const uint8_t *cred_id, size_t cred_id_len,
						 uint32_t *new_count)
{
	int idx;
	char key[FIDO2_SETTINGS_KEY_MAX];
	struct fido2_credential cred;

	idx = cred_slot_get(cred_id, cred_id_len, &cred);
	if (idx < 0) {
		return -ENOENT;
	}

	*new_count = ++cred.sign_count;

	build_key(key, sizeof(key), idx);
	return settings_save_one(key, &cred, sizeof(cred));
}

const struct fido2_storage_api fido2_storage_backend = {
	.init = settings_backend_init,
	.store = settings_backend_store,
	.load = settings_backend_load,
	.remove = settings_backend_remove,
	.find_by_rp = settings_backend_find_by_rp,
	.sign_count_increment = settings_backend_sign_count_increment,
};
