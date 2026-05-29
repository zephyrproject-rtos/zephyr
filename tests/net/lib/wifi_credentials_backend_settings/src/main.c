/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zephyr/fff.h>

#include <zephyr/net/wifi_credentials.h>

#include "wifi_credentials_internal.h"

#define MAX_KEY_LEN 16

#define SSID1     "test1"
#define PSK1      "super secret"
#define SECURITY1 WIFI_SECURITY_TYPE_PSK
#define BSSID1    "abcdef"
#define FLAGS1    WIFI_CREDENTIALS_FLAG_BSSID

#define SSID2     "test2"
#define PSK2      NULL
#define SECURITY2 WIFI_SECURITY_TYPE_NONE
#define BSSID2    NULL
#define FLAGS2    0

DEFINE_FFF_GLOBALS;

K_MUTEX_DEFINE(wifi_credentials_mutex);

FAKE_VALUE_FUNC(int, settings_subsys_init);
FAKE_VALUE_FUNC(int, settings_save_one, const char *, const void *, size_t);
FAKE_VALUE_FUNC(int, settings_delete, const char *);
FAKE_VALUE_FUNC(int, settings_load_subtree_direct, const char *, settings_load_direct_cb, void *);
FAKE_VOID_FUNC(wifi_credentials_cache_ssid, size_t, const struct wifi_credentials_header *);

static uint8_t fake_settings_buf[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES][ENTRY_MAX_LEN];
static char fake_settings_buf_keys[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES][MAX_KEY_LEN];
static size_t fake_settings_buf_lens[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES];

typedef int (*settings_set_cb)(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg);

static const struct wifi_credentials_personal example1 = {
	.header = {
		.ssid = SSID1,
		.ssid_len = strlen(SSID1),
		.type = SECURITY1,
		.bssid = BSSID1,
		.flags = FLAGS1,
	},
	.password = PSK1,
	.password_len = strlen(PSK1),
};

static const struct wifi_credentials_personal example2 = {
	.header = {
		.ssid = SSID2,
		.ssid_len = strlen(SSID2),
		.type = SECURITY2,
		.flags = FLAGS2,
	},
};

/**
 * @brief load content of given settings index to given buffer
 *
 * @param cb_arg size_t *idx
 * @param data destination
 * @param len length
 * @return ssize_t MIN(length, length_available)
 */
ssize_t custom_settings_read_cb(void *cb_arg, void *data, size_t len)
{
	size_t *idx = cb_arg;

	zassert_true(len <= ENTRY_MAX_LEN, "Length exceeds ENTRY_MAX_LEN");
	memcpy(data, fake_settings_buf[*idx], len);
	return len;
}

static int custom_settings_save_one(const char *name, const void *value, size_t val_len)
{
	zassert_true(strlen(name) < MAX_KEY_LEN, "Name length exceeds MAX_KEY_LEN");
	zassert_true(val_len <= ENTRY_MAX_LEN, "Value length exceeds ENTRY_MAX_LEN");

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (strlen(fake_settings_buf_keys[i]) == 0 ||
		    strcmp(name, fake_settings_buf_keys[i]) == 0) {
			strcpy(fake_settings_buf_keys[i], name);
			memcpy(fake_settings_buf[i], value, val_len);
			fake_settings_buf_lens[i] = val_len;
			return 0;
		}
	}
	return -ENOBUFS;
}

static int custom_settings_delete(const char *name)
{
	zassert_true(strlen(name) < MAX_KEY_LEN, "Name length exceeds MAX_KEY_LEN");
	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (strcmp(name, fake_settings_buf_keys[i]) == 0) {
			memset(fake_settings_buf_keys[i], 0, MAX_KEY_LEN);
			memset(fake_settings_buf[i], 0, ENTRY_MAX_LEN);
			fake_settings_buf_lens[i] = 0;
			return 0;
		}
	}
	return -ENOENT;
}

static int custom_settings_load_subtree_direct(const char *subtree, settings_load_direct_cb cb,
					       void *param)
{
	size_t subtree_len = strlen(subtree);

	zassert_true(subtree_len < MAX_KEY_LEN, "Subtree length exceeds MAX_KEY_LEN");

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (strncmp(subtree, fake_settings_buf_keys[i], subtree_len) == 0) {
			const char *key = fake_settings_buf_keys[i] + subtree_len + 1;

			cb(key, fake_settings_buf_lens[i], custom_settings_read_cb, &i, param);
		}
	}
	return 0;
}

static void custom_wifi_credentials_cache_ssid(size_t idx,
					       const struct wifi_credentials_header *buf)
{
	char name[16] = "";

	snprintk(name, ARRAY_SIZE(name), "wifi_cred/%d", idx);
	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		if (strcmp(name, fake_settings_buf_keys[i]) == 0) {
			zassert_equal(memcmp(buf, &fake_settings_buf[i],
					     sizeof(struct wifi_credentials_header)),
				      0, "Buffer mismatch");
			return;
		}
	}
	zassert_true(false, "SSID not found in cache");
}

static void wifi_credentials_backend_settings_setup(void *_unused)
{
	RESET_FAKE(settings_save_one);
	RESET_FAKE(settings_delete);
	RESET_FAKE(settings_load_subtree_direct);
	RESET_FAKE(wifi_credentials_cache_ssid);
	settings_save_one_fake.custom_fake = custom_settings_save_one;
	settings_delete_fake.custom_fake = custom_settings_delete;
	settings_load_subtree_direct_fake.custom_fake = custom_settings_load_subtree_direct;
	wifi_credentials_cache_ssid_fake.custom_fake = custom_wifi_credentials_cache_ssid;
	memset(fake_settings_buf_lens, 0, ARRAY_SIZE(fake_settings_buf_lens) * sizeof(size_t));
	memset(fake_settings_buf_keys, 0, CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES * MAX_KEY_LEN);
	memset(fake_settings_buf, 0, CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES * ENTRY_MAX_LEN);
}

ZTEST(wifi_credentials_backend_settings, test_init)
{
	int ret;

	ret = wifi_credentials_store_entry(0, &example1, sizeof(struct wifi_credentials_personal));
	zassert_equal(ret, 0, "Failed to store entry 0");
	ret = wifi_credentials_store_entry(1, &example2, sizeof(struct wifi_credentials_personal));
	zassert_equal(ret, 0, "Failed to store entry 1");

	ret = wifi_credentials_backend_init();

	zassert_equal(ret, 0, "Backend init failed");
	zassert_equal(settings_subsys_init_fake.call_count, 1,
		      "settings_subsys_init call count mismatch");
	zassert_equal(wifi_credentials_cache_ssid_fake.call_count, 2,
		      "wifi_credentials_cache_ssid call count mismatch");
	zassert_equal(wifi_credentials_cache_ssid_fake.arg0_history[0], 0,
		      "First cache SSID index mismatch");
	zassert_equal(wifi_credentials_cache_ssid_fake.arg0_history[1], 1,
		      "Second cache SSID index mismatch");
}

ZTEST(wifi_credentials_backend_settings, test_add)
{
	int ret = wifi_credentials_store_entry(0, "abc", 3);

	zassert_equal(ret, 0, "Failed to add entry");
	zassert_equal(settings_save_one_fake.call_count, 1,
		      "settings_save_one call count mismatch");
	zassert_equal(strcmp(fake_settings_buf_keys[0], "wifi_cred/0"), 0, "Key mismatch");
	zassert_equal(strcmp(fake_settings_buf[0], "abc"), 0, "Value mismatch");
	zassert_equal(fake_settings_buf_lens[0], 3, "Length mismatch");
}

ZTEST(wifi_credentials_backend_settings, test_get)
{
	int ret;

	ret = wifi_credentials_store_entry(0, &example1, sizeof(struct wifi_credentials_personal));
	zassert_equal(ret, 0, "Failed to store entry 0");
	ret = wifi_credentials_store_entry(1, &example2, sizeof(struct wifi_credentials_personal));
	zassert_equal(ret, 0, "Failed to store entry 1");

	char buf[ENTRY_MAX_LEN] = {0};

	ret = wifi_credentials_load_entry(0, buf, ARRAY_SIZE(buf));
	zassert_equal(ret, 0, "Failed to load entry 0");
	zassert_equal(memcmp(&example1, buf, ARRAY_SIZE(buf)), 0, "Entry 0 data mismatch");
	ret = wifi_credentials_load_entry(1, buf, ARRAY_SIZE(buf));
	zassert_equal(ret, 0, "Failed to load entry 1");
	zassert_equal(memcmp(&example2, buf, ARRAY_SIZE(buf)), 0, "Entry 1 data mismatch");
}

ZTEST(wifi_credentials_backend_settings, test_delete)
{
	int ret;

	ret = wifi_credentials_store_entry(0, "abc", 3);
	zassert_equal(ret, 0, "Failed to store entry");

	ret = wifi_credentials_delete_entry(0);
	zassert_equal(ret, 0, "Failed to delete entry");
	zassert_equal(settings_delete_fake.call_count, 1, "settings_delete call count mismatch");
}

ZTEST_SUITE(wifi_credentials_backend_settings, NULL, NULL, wifi_credentials_backend_settings_setup,
	    NULL, NULL);
