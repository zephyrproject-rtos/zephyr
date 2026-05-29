/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/fff.h>

#include <zephyr/net/wifi_credentials.h>

#include "wifi_credentials_internal.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, wifi_credentials_store_entry, size_t, const void *, size_t)
FAKE_VALUE_FUNC(int, wifi_credentials_load_entry, size_t, void *, size_t)
FAKE_VALUE_FUNC(int, wifi_credentials_delete_entry, size_t)
FAKE_VALUE_FUNC(int, wifi_credentials_backend_init)

uint8_t fake_settings_buf[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES][ENTRY_MAX_LEN];

int custom_wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len)
{
	zassert_true(idx < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES, "Index out of bounds");
	memcpy(fake_settings_buf[idx], buf, MIN(ENTRY_MAX_LEN, buf_len));
	return 0;
}

int custom_wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	zassert_true(idx < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES, "Index out of bounds");
	memcpy(buf, fake_settings_buf[idx], MIN(ENTRY_MAX_LEN, buf_len));
	return 0;
}

#define SSID1     "test1"
#define PSK1      "super secret"
#define SECURITY1 WIFI_SECURITY_TYPE_PSK
#define BSSID1    "abcdef"
#define FLAGS1    WIFI_CREDENTIALS_FLAG_BSSID
#define CHANNEL1  1

#define SSID2     "test2"
#define SECURITY2 WIFI_SECURITY_TYPE_NONE
#define FLAGS2    0
#define CHANNEL2  2

#define SSID3     "test3"
#define PSK3      "extremely secret"
#define SECURITY3 WIFI_SECURITY_TYPE_SAE
#define FLAGS3    0
#define CHANNEL3  3

#define SSID4     "\0what's\0null\0termination\0anyway"
#define PSK4      PSK1
#define SECURITY4 SECURITY1
#define BSSID4    BSSID1
#define FLAGS4    FLAGS1
#define CHANNEL4  4

static void wifi_credentials_setup(void *unused)
{
	RESET_FAKE(wifi_credentials_store_entry);
	RESET_FAKE(wifi_credentials_load_entry);
	RESET_FAKE(wifi_credentials_delete_entry);
	wifi_credentials_store_entry_fake.custom_fake = custom_wifi_credentials_store_entry;
	wifi_credentials_load_entry_fake.custom_fake = custom_wifi_credentials_load_entry;
}

static void wifi_credentials_teardown(void *unused)
{
	wifi_credentials_delete_by_ssid(SSID1, ARRAY_SIZE(SSID1));
	wifi_credentials_delete_by_ssid(SSID2, ARRAY_SIZE(SSID2));
	wifi_credentials_delete_by_ssid(SSID3, ARRAY_SIZE(SSID3));
	wifi_credentials_delete_by_ssid(SSID4, ARRAY_SIZE(SSID4));
	wifi_credentials_delete_by_ssid("", 0);
}

/* Verify that attempting to retrieve a non-existent credentials entry raises -ENOENT. */
ZTEST(wifi_credentials, test_get_non_existing)
{
	int err;
	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;
	uint8_t channel = 0;
	uint32_t timeout = 0;

	err = wifi_credentials_get_by_ssid_personal(
		SSID1, sizeof(SSID1), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, -ENOENT, "Expected -ENOENT, got %d", err);
}

/* Verify that we can successfully set/get a network without a specified BSSID. */
ZTEST(wifi_credentials, test_single_no_bssid)
{
	int err;

	/* set network credentials without BSSID */
	err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, NULL, 0, PSK1,
					    sizeof(PSK1), 0, 0, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;
	uint8_t channel = 0;
	uint32_t timeout = 0;

	/* retrieve network credentials without BSSID */
	err = wifi_credentials_get_by_ssid_personal(
		SSID1, sizeof(SSID1), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
	zassert_equal(strncmp(PSK1, psk_buf, ARRAY_SIZE(psk_buf)), 0, "PSK mismatch");
	zassert_equal(flags, 0, "Flags mismatch");
	zassert_equal(channel, 0, "Channel mismatch");
	zassert_equal(security, SECURITY1, "Security type mismatch");

	err = wifi_credentials_delete_by_ssid(SSID1, sizeof(SSID1));
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
}

/* Verify that we can successfully set/get a network with a fixed BSSID. */
ZTEST(wifi_credentials, test_single_with_bssid)
{
	int err;

	/* set network credentials with BSSID */
	err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, BSSID1, 6, PSK1,
					    sizeof(PSK1), FLAGS1, CHANNEL1, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;
	uint8_t channel = 0;
	uint32_t timeout = 0;

	/* retrieve network credentials with BSSID */
	err = wifi_credentials_get_by_ssid_personal(
		SSID1, sizeof(SSID1), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
	zassert_equal(strncmp(PSK1, psk_buf, ARRAY_SIZE(psk_buf)), 0, "PSK mismatch");
	zassert_equal(psk_len, sizeof(PSK1), "PSK length mismatch");
	zassert_equal(strncmp(BSSID1, bssid_buf, ARRAY_SIZE(bssid_buf)), 0, "BSSID mismatch");
	zassert_equal(flags, WIFI_CREDENTIALS_FLAG_BSSID, "Flags mismatch");
	zassert_equal(channel, CHANNEL1, "Channel mismatch");
	zassert_equal(security, SECURITY1, "Security type mismatch");

	err = wifi_credentials_delete_by_ssid(SSID1, sizeof(SSID1));
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
}

/* Verify that we can successfully set/get an open network. */
ZTEST(wifi_credentials, test_single_without_psk)
{
	int err;

	/* set network credentials without PSK/BSSID */
	err = wifi_credentials_set_personal(SSID2, sizeof(SSID2), SECURITY2, NULL, 6, NULL, 0,
					    FLAGS2, CHANNEL2, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;
	uint8_t channel = 0;
	uint32_t timeout = 0;

	/* retrieve network credentials without PSK/BSSID */
	err = wifi_credentials_get_by_ssid_personal(
		SSID2, sizeof(SSID2), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
	zassert_equal(psk_len, 0, "PSK length mismatch");
	zassert_equal(flags, 0, "Flags mismatch");
	zassert_equal(channel, CHANNEL2, "Channel mismatch");

	err = wifi_credentials_delete_by_ssid(SSID2, sizeof(SSID2));
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
}

/* Verify that we can set/get a network that is only identified by a BSSID. */
ZTEST(wifi_credentials, test_single_without_ssid)
{
	int err;

	err = wifi_credentials_set_personal("", 0, SECURITY1, BSSID1, 6, PSK1, sizeof(PSK1), FLAGS1,
					    CHANNEL1, 0);
	zassert_equal(err, -EINVAL, "Expected -EINVAL, got %d", err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;
	uint8_t channel = 0;
	uint32_t timeout = 0;

	err = wifi_credentials_get_by_ssid_personal(
		"", 0, &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf, ARRAY_SIZE(psk_buf),
		&psk_len, &flags, &channel, &timeout);
	zassert_equal(err, -EINVAL, "Expected -EINVAL, got %d", err);

	err = wifi_credentials_delete_by_ssid("", 0);
	zassert_equal(err, -EINVAL, "Expected -EINVAL, got %d", err);
}

/* Verify that we can handle SSIDs that contain NULL characters. */
ZTEST(wifi_credentials, test_single_garbled_ssid)
{
	int err;

	err = wifi_credentials_set_personal(SSID4, sizeof(SSID4), SECURITY4, BSSID4, 6, PSK4,
					    sizeof(PSK4), FLAGS4, CHANNEL4, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;
	uint8_t channel = 0;
	uint32_t timeout = 0;

	err = wifi_credentials_get_by_ssid_personal(
		SSID4, sizeof(SSID4), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
	zassert_equal(strncmp(PSK4, psk_buf, ARRAY_SIZE(psk_buf)), 0, "PSK mismatch");
	zassert_equal(psk_len, sizeof(PSK4), "PSK length mismatch");
	zassert_equal(strncmp(BSSID4, bssid_buf, ARRAY_SIZE(bssid_buf)), 0, "BSSID mismatch");
	zassert_equal(security, SECURITY4, "Security type mismatch");
	zassert_equal(flags, FLAGS4, "Flags mismatch");
	zassert_equal(channel, CHANNEL4, "Channel mismatch");

	err = wifi_credentials_delete_by_ssid(SSID4, sizeof(SSID4));
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
}

/* Helper function for test_set_storage_limit, making sure that the SSID cache is correct. */
void verify_ssid_cache_cb(void *cb_arg, const char *ssid, size_t ssid_len)
{
	static int call_count;
	static const char *const ssids[] = {SSID3, SSID2};

	zassert_equal(strncmp(ssids[call_count++], ssid, ssid_len), 0, "SSID cache mismatch");
	zassert_is_null(cb_arg, "Callback argument is not NULL");
}

/* Verify that wifi_credentials behaves correctly when the storage limit is reached. */
ZTEST(wifi_credentials, test_storage_limit)
{
	int err;

	/* Set two networks */
	err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, BSSID1, 6, PSK1,
					    sizeof(PSK1), FLAGS1, CHANNEL1, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	err = wifi_credentials_set_personal(SSID2, sizeof(SSID2), SECURITY2, NULL, 6, NULL, 0,
					    FLAGS2, CHANNEL2, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;
	uint8_t channel = 0;
	uint32_t timeout = 0;

	/* Get two networks */
	err = wifi_credentials_get_by_ssid_personal(
		SSID1, sizeof(SSID1), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
	zassert_equal(strncmp(PSK1, psk_buf, ARRAY_SIZE(psk_buf)), 0, "PSK mismatch");
	zassert_equal(psk_len, sizeof(PSK1), "PSK length mismatch");
	zassert_equal(strncmp(BSSID1, bssid_buf, ARRAY_SIZE(bssid_buf)), 0, "BSSID mismatch");
	zassert_equal(security, SECURITY1, "Security type mismatch");
	zassert_equal(flags, FLAGS1, "Flags mismatch");
	zassert_equal(channel, CHANNEL1, "Channel mismatch");

	err = wifi_credentials_get_by_ssid_personal(
		SSID2, sizeof(SSID2), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
	zassert_equal(security, SECURITY2, "Security type mismatch");
	zassert_equal(flags, FLAGS2, "Flags mismatch");
	zassert_equal(channel, CHANNEL2, "Channel mismatch");

	/* Set third network */
	err = wifi_credentials_set_personal(SSID3, sizeof(SSID3), SECURITY3, NULL, 6, PSK3,
					    sizeof(PSK3), FLAGS3, CHANNEL3, 0);
	zassert_equal(err, -ENOBUFS, "Expected -ENOBUFS, got %d", err);

	/* Not enough space? Delete the first one. */
	wifi_credentials_delete_by_ssid(SSID1, ARRAY_SIZE(SSID1));
	err = wifi_credentials_set_personal(SSID3, sizeof(SSID3), SECURITY3, NULL, 6, PSK3,
					    sizeof(PSK3), FLAGS3, CHANNEL3, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	err = wifi_credentials_get_by_ssid_personal(
		SSID3, sizeof(SSID3), &security, bssid_buf, ARRAY_SIZE(bssid_buf), psk_buf,
		ARRAY_SIZE(psk_buf), &psk_len, &flags, &channel, &timeout);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);
	zassert_equal(security, SECURITY3, "Security type mismatch");
	zassert_equal(psk_len, sizeof(PSK3), "PSK length mismatch");
	zassert_equal(strncmp(PSK3, psk_buf, ARRAY_SIZE(psk_buf)), 0, "PSK mismatch");
	zassert_equal(flags, FLAGS3, "Flags mismatch");
	zassert_equal(channel, CHANNEL3, "Channel mismatch");

	wifi_credentials_for_each_ssid(verify_ssid_cache_cb, NULL);
}

/* Verify that all entries are deleted. */
ZTEST(wifi_credentials, test_delete_all_entries)
{
	/* Set two networks */
	int err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, BSSID1, 6, PSK1,
						sizeof(PSK1), FLAGS1, CHANNEL1, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	err = wifi_credentials_set_personal(SSID2, sizeof(SSID2), SECURITY2, NULL, 6, NULL, 0,
					    FLAGS2, CHANNEL2, 0);
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	/* Delete all networks */
	err = wifi_credentials_delete_all();
	zassert_equal(err, EXIT_SUCCESS, "Expected EXIT_SUCCESS, got %d", err);

	/* Verify that the storage is empty */
	zassert_true(wifi_credentials_is_empty(), "Storage is not empty");
}

ZTEST_SUITE(wifi_credentials, NULL, NULL, wifi_credentials_setup, wifi_credentials_teardown, NULL);
