/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/settings/settings.h>

#include "wifi_credentials_internal.h"

LOG_MODULE_REGISTER(wifi_credentials_backend, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

BUILD_ASSERT(ENTRY_MAX_LEN <= SETTINGS_MAX_VAL_LEN);

#define WIFI_CREDENTIALS_SBE_BASE_KEY "wifi_cred"
#define WIFI_CREDENTIALS_SBE_KEY_SIZE                                                              \
	sizeof(WIFI_CREDENTIALS_SBE_BASE_KEY "/" STRINGIFY(CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES))
#define WIFI_CREDENTIALS_SBE_KEY_FMT WIFI_CREDENTIALS_SBE_BASE_KEY "/%d"

/* Type of the callback argument used in the function below. */
struct zephyr_settings_backend_load_cb_arg {
	uint8_t *buf;
	size_t buf_len;
	size_t idx;
	bool found;
};

/* This callback function is used to retrieve credentials on demand. */
static int zephyr_settings_backend_load_val_cb(const char *key, size_t len,
					       settings_read_cb read_cb, void *cb_arg, void *param)
{
	struct zephyr_settings_backend_load_cb_arg *arg = param;
	int idx = atoi(key);

	if (arg->idx != idx) {
		LOG_DBG("Skipping index [%s]", key);
		return 0;
	}

	if (len != arg->buf_len) {
		LOG_ERR("Settings error: invalid settings length");
		return -EINVAL;
	}

	size_t length_read = read_cb(cb_arg, arg->buf, arg->buf_len);

	/* value validation */
	if (length_read < len) {
		LOG_ERR("Settings error: entry incomplete");
		return -ENODATA;
	}

	arg->found = true;

	return 0;
}

/* This callback function is used to initialize the SSID cache. */
static int zephyr_settings_backend_load_key_cb(const char *key, size_t len,
					       settings_read_cb read_cb, void *cb_arg, void *param)
{
	ARG_UNUSED(param);

	/* key validation */
	if (!key) {
		LOG_ERR("Settings error: no key");
		return -EINVAL;
	}

	int idx = atoi(key);

	if ((idx == 0 && strcmp(key, "0") != 0) || idx >= CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES) {
		LOG_ERR("Settings error: index too large");
		return -EINVAL;
	}

	if (len < sizeof(struct wifi_credentials_header)) {
		LOG_ERR("Settings error: invalid settings length");
		return -EINVAL;
	}

	uint8_t buf[ENTRY_MAX_LEN];
	size_t length_read = read_cb(cb_arg, buf, ARRAY_SIZE(buf));

	/* value validation */
	if (length_read < len) {
		LOG_ERR("Settings error: entry incomplete");
		return -ENODATA;
	}

	wifi_credentials_cache_ssid(idx, (struct wifi_credentials_header *)buf);
	return 0;
}

int wifi_credentials_backend_init(void)
{
	int ret = settings_subsys_init();

	if (ret) {
		LOG_ERR("Initializing settings subsystem failed: %d", ret);
		return ret;
	}

	ret = settings_load_subtree_direct(WIFI_CREDENTIALS_SBE_BASE_KEY,
					   zephyr_settings_backend_load_key_cb, NULL);

	return ret;
}

int wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len)
{
	char settings_name_buf[WIFI_CREDENTIALS_SBE_KEY_SIZE] = {0};

	int ret = snprintk(settings_name_buf, ARRAY_SIZE(settings_name_buf),
			   WIFI_CREDENTIALS_SBE_KEY_FMT, idx);

	if (ret < 0 || ret == ARRAY_SIZE(settings_name_buf)) {
		LOG_ERR("WiFi credentials settings key could not be generated, idx: %d", idx);
		return -EFAULT;
	}

	return settings_save_one(settings_name_buf, buf, buf_len);
}

int wifi_credentials_delete_entry(size_t idx)
{
	char settings_name_buf[WIFI_CREDENTIALS_SBE_KEY_SIZE] = {0};

	int ret = snprintk(settings_name_buf, ARRAY_SIZE(settings_name_buf),
			   WIFI_CREDENTIALS_SBE_KEY_FMT, idx);

	if (ret < 0 || ret == ARRAY_SIZE(settings_name_buf)) {
		LOG_ERR("WiFi credentials settings key could not be generated, idx: %d", idx);
		return -EFAULT;
	}

	return settings_delete(settings_name_buf);
}

int wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	struct zephyr_settings_backend_load_cb_arg arg = {
		.buf = buf,
		.buf_len = buf_len,
		.idx = idx,
		.found = false,
	};
	int ret;

	/* Browse through the settings entries with custom callback to load the whole entry. */
	ret = settings_load_subtree_direct(WIFI_CREDENTIALS_SBE_BASE_KEY,
					   zephyr_settings_backend_load_val_cb, &arg);

	if (ret) {
		return ret;
	}

	if (!arg.found) {
		return -ENOENT;
	}

	return 0;
}
