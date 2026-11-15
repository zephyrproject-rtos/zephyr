/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/init.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/psa/key_ids.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <psa/crypto.h>

#define STORED_SSID     "stored network"
#define STORED_PASSWORD "password of the stored network"
#define ADDED_SSID      "added network"
#define ADDED_PASSWORD  "password of the added network"

/* The priority of the library's initialization, which is at the POST_KERNEL level. */
#define EARLIER_THAN_LIBRARY 89
BUILD_ASSERT(CONFIG_APPLICATION_INIT_PRIORITY > EARLIER_THAN_LIBRARY);

static psa_status_t stored_status = PSA_ERROR_GENERIC_ERROR;
static bool password_found;

/*
 * Puts an entry where the PSA backend keeps the first one, before the library
 * initializes: as far as the library can tell, it was stored before a restart.
 */
static int store_entry_before_library_init(void)
{
	static struct wifi_credentials_personal entry = {
		.header.type = WIFI_SECURITY_TYPE_PSK,
		.header.ssid = STORED_SSID,
		.header.ssid_len = sizeof(STORED_SSID) - 1,
		.password = STORED_PASSWORD,
		.password_len = sizeof(STORED_PASSWORD) - 1,
	};
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;

	psa_set_key_id(&attributes, ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_EXPORT);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_algorithm(&attributes, PSA_ALG_NONE);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_RAW_DATA);
	psa_set_key_bits(&attributes, sizeof(entry) * 8);

	stored_status =
		psa_import_key(&attributes, (const uint8_t *)&entry, sizeof(entry), &key_id);
	return 0;
}
SYS_INIT(store_entry_before_library_init, POST_KERNEL, EARLIER_THAN_LIBRARY);

static int find_password(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			 void *param)
{
	static uint8_t value[CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE + 64];
	const char *password = param;
	const size_t password_len = strlen(password);
	ssize_t read;

	ARG_UNUSED(key);

	read = read_cb(cb_arg, value, MIN(len, sizeof(value)));
	zassert_true(read >= 0, "settings read failed: %zd", read);
	for (ssize_t i = 0; i + (ssize_t)password_len <= read; i++) {
		if (memcmp(&value[i], password, password_len) == 0) {
			password_found = true;
		}
	}
	return 0;
}

/* The library finds at its initialization what was stored before it. */
ZTEST(wifi_credentials_secure_storage, test_entry_stored_before_init)
{
	struct wifi_credentials_personal entry = {0};

	zassert_equal(stored_status, PSA_SUCCESS, "psa_import_key() failed (%d)", stored_status);
	zassert_false(wifi_credentials_is_empty());

	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(STORED_SSID,
								sizeof(STORED_SSID) - 1, &entry));
	zassert_equal(entry.header.type, WIFI_SECURITY_TYPE_PSK);
	zassert_equal(entry.password_len, sizeof(STORED_PASSWORD) - 1);
	zassert_mem_equal(entry.password, STORED_PASSWORD, sizeof(STORED_PASSWORD) - 1);
}

ZTEST(wifi_credentials_secure_storage, test_set_get_delete)
{
	struct wifi_credentials_personal entry = {0};

	zassert_ok(wifi_credentials_set_personal(ADDED_SSID, sizeof(ADDED_SSID) - 1,
						 WIFI_SECURITY_TYPE_PSK, NULL, 0, ADDED_PASSWORD,
						 sizeof(ADDED_PASSWORD) - 1, 0, 0, 0));

	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(ADDED_SSID, sizeof(ADDED_SSID) - 1,
								&entry));
	zassert_equal(entry.password_len, sizeof(ADDED_PASSWORD) - 1);
	zassert_mem_equal(entry.password, ADDED_PASSWORD, sizeof(ADDED_PASSWORD) - 1);

	if (IS_ENABLED(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_SETTINGS)) {
		/* The passwords are in the settings, sealed. */
		zassert_ok(settings_load_subtree_direct(NULL, find_password, STORED_PASSWORD));
		zassert_ok(settings_load_subtree_direct(NULL, find_password, ADDED_PASSWORD));
		zassert_false(password_found);
	}

	zassert_ok(wifi_credentials_delete_by_ssid(ADDED_SSID, sizeof(ADDED_SSID) - 1));
	zassert_equal(wifi_credentials_get_by_ssid_personal_struct(ADDED_SSID,
								   sizeof(ADDED_SSID) - 1, &entry),
		      -ENOENT);
}

ZTEST_SUITE(wifi_credentials_secure_storage, NULL, NULL, NULL, NULL, NULL);
