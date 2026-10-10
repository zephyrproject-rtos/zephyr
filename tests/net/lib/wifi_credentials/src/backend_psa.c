/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/psa/key_ids.h>
#include <psa/crypto.h>

#define SSID1 "test1"
#define PSK1  "super secret"
#define SSID2 "test2"
#define PSK2  "another secret"

static const uint8_t bssid1[WIFI_MAC_ADDR_LEN] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

/* Count the persistent keys in the Wi-Fi credentials key ID range. */
static size_t count_stored_keys(void)
{
	size_t count = 0;

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; i++) {
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_status_t ret;

		ret = psa_get_key_attributes(ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN + i,
					     &attr);
		if (ret == PSA_SUCCESS) {
			zassert_equal(psa_get_key_lifetime(&attr), PSA_KEY_LIFETIME_PERSISTENT);
			count++;
		} else {
			zassert_equal(ret, PSA_ERROR_INVALID_HANDLE, "unexpected status %d", ret);
		}
		psa_reset_key_attributes(&attr);
	}

	return count;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(wifi_credentials_delete_all());
	zassert_true(wifi_credentials_is_empty());
	zassert_equal(count_stored_keys(), 0);
}

ZTEST_SUITE(wifi_credentials_backend_psa, NULL, NULL, before, NULL, NULL);

ZTEST(wifi_credentials_backend_psa, test_set_get)
{
	enum wifi_security_type type;
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
	char password[WIFI_CREDENTIALS_MAX_PASSWORD_LEN];
	size_t password_len;
	uint32_t flags;
	uint8_t channel;
	uint32_t timeout;

	zassert_ok(wifi_credentials_set_personal(SSID1, strlen(SSID1), WIFI_SECURITY_TYPE_PSK,
						 bssid1, sizeof(bssid1), PSK1, strlen(PSK1),
						 WIFI_CREDENTIALS_FLAG_BSSID, 6, 10));
	zassert_false(wifi_credentials_is_empty());
	zassert_equal(count_stored_keys(), 1);

	zassert_ok(wifi_credentials_get_by_ssid_personal(SSID1, strlen(SSID1), &type, bssid,
							 sizeof(bssid), password,
							 sizeof(password), &password_len, &flags,
							 &channel, &timeout));
	zassert_equal(type, WIFI_SECURITY_TYPE_PSK);
	zassert_mem_equal(bssid, bssid1, sizeof(bssid1));
	zassert_equal(password_len, strlen(PSK1));
	zassert_mem_equal(password, PSK1, password_len);
	zassert_equal(flags, WIFI_CREDENTIALS_FLAG_BSSID);
	zassert_equal(channel, 6);
	zassert_equal(timeout, 10);
}

/* An entry with the longest SSID and password must fit in the storage. */
ZTEST(wifi_credentials_backend_psa, test_set_get_max_length)
{
	char ssid[WIFI_SSID_MAX_LEN];
	char password[WIFI_CREDENTIALS_MAX_PASSWORD_LEN];
	struct wifi_credentials_personal creds;

	memset(ssid, 's', sizeof(ssid));
	memset(password, 'p', sizeof(password));

	zassert_ok(wifi_credentials_set_personal(ssid, sizeof(ssid), WIFI_SECURITY_TYPE_SAE,
						 bssid1, sizeof(bssid1), password,
						 sizeof(password), WIFI_CREDENTIALS_FLAG_BSSID, 6,
						 10));
	zassert_equal(count_stored_keys(), 1);

	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(ssid, sizeof(ssid), &creds));
	zassert_equal(creds.header.type, WIFI_SECURITY_TYPE_SAE);
	zassert_equal(creds.password_len, sizeof(password));
	zassert_mem_equal(creds.password, password, sizeof(password));
}

ZTEST(wifi_credentials_backend_psa, test_overwrite)
{
	struct wifi_credentials_personal creds;

	zassert_ok(wifi_credentials_set_personal(SSID1, strlen(SSID1), WIFI_SECURITY_TYPE_PSK,
						 NULL, 0, PSK1, strlen(PSK1), 0, 0, 0));
	zassert_ok(wifi_credentials_set_personal(SSID1, strlen(SSID1), WIFI_SECURITY_TYPE_PSK,
						 NULL, 0, PSK2, strlen(PSK2), 0, 0, 0));
	zassert_equal(count_stored_keys(), 1);

	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(SSID1, strlen(SSID1), &creds));
	zassert_equal(creds.password_len, strlen(PSK2));
	zassert_mem_equal(creds.password, PSK2, creds.password_len);
}

ZTEST(wifi_credentials_backend_psa, test_delete)
{
	struct wifi_credentials_personal creds;

	zassert_ok(wifi_credentials_set_personal(SSID1, strlen(SSID1), WIFI_SECURITY_TYPE_PSK,
						 NULL, 0, PSK1, strlen(PSK1), 0, 0, 0));
	zassert_ok(wifi_credentials_set_personal(SSID2, strlen(SSID2), WIFI_SECURITY_TYPE_NONE,
						 NULL, 0, NULL, 0, 0, 0, 0));
	zassert_equal(count_stored_keys(), 2);

	zassert_ok(wifi_credentials_delete_by_ssid(SSID1, strlen(SSID1)));
	zassert_equal(count_stored_keys(), 1);
	zassert_equal(wifi_credentials_get_by_ssid_personal_struct(SSID1, strlen(SSID1), &creds),
		      -ENOENT);
	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(SSID2, strlen(SSID2), &creds));

	zassert_ok(wifi_credentials_delete_by_ssid(SSID2, strlen(SSID2)));
	zassert_equal(count_stored_keys(), 0);
	zassert_true(wifi_credentials_is_empty());
}
