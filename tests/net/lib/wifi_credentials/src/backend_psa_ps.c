/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/psa/ps_ids.h>
#include <psa/protected_storage.h>

#define SSID1 "test1"
#define PSK1  "super secret"
#define SSID2 "test2"
#define PSK2  "another secret"
#define SSID_STORED "stored before first use"
#define PSK_STORED  "stored secret"

static const uint8_t bssid1[WIFI_MAC_ADDR_LEN] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

/* Count the entries in the Wi-Fi credentials PS UID range. */
static size_t count_stored_entries(void)
{
	size_t count = 0;

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; i++) {
		struct psa_storage_info_t info;
		psa_status_t ret;

		ret = psa_ps_get_info(ZEPHYR_PSA_WIFI_CREDENTIALS_PS_UID_RANGE_BEGIN + i, &info);
		if (ret == PSA_SUCCESS) {
			zassert_equal(info.size, sizeof(struct wifi_credentials_personal));
			count++;
		} else {
			zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST, "unexpected status %d", ret);
		}
	}

	return count;
}

static psa_status_t stored_set_status = PSA_ERROR_GENERIC_ERROR;
static int first_use_set_ret = -EINPROGRESS;
static int stored_lookup_ret = -EINPROGRESS;

static void *setup(void)
{
	struct wifi_credentials_personal creds = {
		.header = {
			.type = WIFI_SECURITY_TYPE_PSK,
			.ssid = SSID_STORED,
			.ssid_len = strlen(SSID_STORED),
		},
		.password = PSK_STORED,
		.password_len = strlen(PSK_STORED),
	};

	/* Store an entry before the library is used, as if it was stored before a reboot. */
	stored_set_status = psa_ps_set(ZEPHYR_PSA_WIFI_CREDENTIALS_PS_UID_RANGE_BEGIN,
				       sizeof(creds), &creds, PSA_STORAGE_FLAG_NONE);

	/* The first use of the library stores another entry, which must not overwrite it.
	 * This is done before before() deletes all entries.
	 */
	first_use_set_ret = wifi_credentials_set_personal(SSID1, strlen(SSID1),
							  WIFI_SECURITY_TYPE_PSK, NULL, 0, PSK1,
							  strlen(PSK1), 0, 0, 0);

	memset(&creds, 0, sizeof(creds));
	stored_lookup_ret = wifi_credentials_get_by_ssid_personal_struct(SSID_STORED,
									 strlen(SSID_STORED),
									 &creds);
	if (stored_lookup_ret == 0 && (creds.password_len != strlen(PSK_STORED) ||
				       memcmp(creds.password, PSK_STORED,
					      creds.password_len) != 0)) {
		stored_lookup_ret = -EBADMSG;
	}

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(wifi_credentials_delete_all());
	zassert_true(wifi_credentials_is_empty());
	zassert_equal(count_stored_entries(), 0);
}

ZTEST_SUITE(wifi_credentials_backend_psa_ps, NULL, setup, before, NULL, NULL);

/* Entries stored before the library is used must be loaded on first use. */
ZTEST(wifi_credentials_backend_psa_ps, test_load_on_first_use)
{
	zassert_equal(stored_set_status, PSA_SUCCESS);
	zassert_ok(first_use_set_ret);
	zassert_ok(stored_lookup_ret);
}

ZTEST(wifi_credentials_backend_psa_ps, test_set_get)
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
	zassert_equal(count_stored_entries(), 1);

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
ZTEST(wifi_credentials_backend_psa_ps, test_set_get_max_length)
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
	zassert_equal(count_stored_entries(), 1);

	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(ssid, sizeof(ssid), &creds));
	zassert_equal(creds.header.type, WIFI_SECURITY_TYPE_SAE);
	zassert_equal(creds.password_len, sizeof(password));
	zassert_mem_equal(creds.password, password, sizeof(password));
}

ZTEST(wifi_credentials_backend_psa_ps, test_overwrite)
{
	struct wifi_credentials_personal creds;

	zassert_ok(wifi_credentials_set_personal(SSID1, strlen(SSID1), WIFI_SECURITY_TYPE_PSK,
						 NULL, 0, PSK1, strlen(PSK1), 0, 0, 0));
	zassert_ok(wifi_credentials_set_personal(SSID1, strlen(SSID1), WIFI_SECURITY_TYPE_PSK,
						 NULL, 0, PSK2, strlen(PSK2), 0, 0, 0));
	zassert_equal(count_stored_entries(), 1);

	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(SSID1, strlen(SSID1), &creds));
	zassert_equal(creds.password_len, strlen(PSK2));
	zassert_mem_equal(creds.password, PSK2, creds.password_len);
}

ZTEST(wifi_credentials_backend_psa_ps, test_delete)
{
	struct wifi_credentials_personal creds;

	zassert_ok(wifi_credentials_set_personal(SSID1, strlen(SSID1), WIFI_SECURITY_TYPE_PSK,
						 NULL, 0, PSK1, strlen(PSK1), 0, 0, 0));
	zassert_ok(wifi_credentials_set_personal(SSID2, strlen(SSID2), WIFI_SECURITY_TYPE_NONE,
						 NULL, 0, NULL, 0, 0, 0, 0));
	zassert_equal(count_stored_entries(), 2);

	zassert_ok(wifi_credentials_delete_by_ssid(SSID1, strlen(SSID1)));
	zassert_equal(count_stored_entries(), 1);
	zassert_equal(wifi_credentials_get_by_ssid_personal_struct(SSID1, strlen(SSID1), &creds),
		      -ENOENT);
	zassert_ok(wifi_credentials_get_by_ssid_personal_struct(SSID2, strlen(SSID2), &creds));

	zassert_ok(wifi_credentials_delete_by_ssid(SSID2, strlen(SSID2)));
	zassert_equal(count_stored_entries(), 0);
	zassert_true(wifi_credentials_is_empty());
}
