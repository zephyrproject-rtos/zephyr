/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/radio/radio_cal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(radio_cal, CONFIG_RADIO_CAL_LOG_LEVEL);

/*
 * Calibration items are persisted through the settings subsystem under a
 * dedicated subtree so they share the standard storage backend (and partition)
 * with the rest of the system without any private key-id management. The
 * settings subsystem owns the namespace, so radio_cal data coexists with
 * application and other subsystem data on the same storage.
 */
#define RADIO_CAL_SUBTREE "radio_cal"

static bool cal_ready;

K_MUTEX_DEFINE(cal_lock);

static int cal_storage_init(void)
{
	int rc;

	if (cal_ready) {
		return 0;
	}

	rc = settings_subsys_init();
	if (rc) {
		LOG_ERR("settings init failed (%d)", rc);
		return rc;
	}

	cal_ready = true;
	return 0;
}

/* Build the full settings name "radio_cal/<key>" into buf. */
static int cal_make_name(char *buf, size_t buf_len, const char *key)
{
	int n = snprintk(buf, buf_len, RADIO_CAL_SUBTREE "/%s", key);

	if (n < 0 || (size_t)n >= buf_len) {
		return -ENAMETOOLONG;
	}

	return 0;
}

int radio_cal_store(const char *key, const void *data, size_t len)
{
	char name[SETTINGS_MAX_NAME_LEN + 1];
	int rc;

	if (key == NULL || data == NULL || len == 0) {
		return -EINVAL;
	}

	rc = cal_make_name(name, sizeof(name), key);
	if (rc) {
		return rc;
	}

	k_mutex_lock(&cal_lock, K_FOREVER);

	rc = cal_storage_init();
	if (rc) {
		goto out;
	}

	rc = settings_save_one(name, data, len);

out:
	k_mutex_unlock(&cal_lock);
	return rc;
}

int radio_cal_load(const char *key, void *data, size_t len)
{
	char name[SETTINGS_MAX_NAME_LEN + 1];
	ssize_t rc;

	if (key == NULL || data == NULL || len == 0) {
		return -EINVAL;
	}

	rc = cal_make_name(name, sizeof(name), key);
	if (rc) {
		return rc;
	}

	k_mutex_lock(&cal_lock, K_FOREVER);

	rc = cal_storage_init();
	if (rc) {
		goto out;
	}

	rc = settings_load_one(name, data, len);
	/*
	 * settings_load_one returns the actual stored size, or a negative
	 * value on failure. A missing entry yields 0 (nothing loaded). Only an
	 * exact-size match is a usable calibration blob.
	 */
	if (rc == (ssize_t)len) {
		rc = 0;
	} else if (rc >= 0) {
		rc = -EIO;
	}

out:
	k_mutex_unlock(&cal_lock);
	return (int)rc;
}

int radio_cal_erase(const char *key)
{
	char name[SETTINGS_MAX_NAME_LEN + 1];
	int rc;

	if (key == NULL) {
		return -EINVAL;
	}

	rc = cal_make_name(name, sizeof(name), key);
	if (rc) {
		return rc;
	}

	k_mutex_lock(&cal_lock, K_FOREVER);

	rc = cal_storage_init();
	if (rc) {
		goto out;
	}

	rc = settings_delete(name);

out:
	k_mutex_unlock(&cal_lock);
	return rc;
}
