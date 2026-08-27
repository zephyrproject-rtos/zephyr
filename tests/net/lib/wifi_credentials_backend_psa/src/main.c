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
#include <zephyr/init.h>
#include <zephyr/psa/key_ids.h>

#include <zephyr/fff.h>

#include <zephyr/net/wifi_credentials.h>

#include "wifi_credentials_internal.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

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

FAKE_VOID_FUNC(wifi_credentials_cache_ssid, size_t, const struct wifi_credentials_header *);
FAKE_VALUE_FUNC(psa_status_t, psa_export_key, mbedtls_svc_key_id_t, uint8_t *, size_t, size_t *);
FAKE_VALUE_FUNC(psa_status_t, psa_import_key, psa_key_attributes_t *, uint8_t *, size_t,
		mbedtls_svc_key_id_t *);
FAKE_VALUE_FUNC(psa_status_t, psa_destroy_key, mbedtls_svc_key_id_t);
FAKE_VOID_FUNC(psa_set_key_id, psa_key_attributes_t *, uint32_t);
FAKE_VOID_FUNC(psa_set_key_usage_flags, psa_key_attributes_t *, psa_key_usage_t);
FAKE_VOID_FUNC(psa_set_key_lifetime, psa_key_attributes_t *, psa_key_lifetime_t);
FAKE_VOID_FUNC(psa_set_key_algorithm, psa_key_attributes_t *, psa_algorithm_t);
FAKE_VOID_FUNC(psa_set_key_type, psa_key_attributes_t *, psa_key_type_t);
FAKE_VOID_FUNC(psa_set_key_bits, psa_key_attributes_t *, size_t);

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

static size_t idx;

psa_status_t custom_psa_export_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				   size_t *data_length)
{
	/* confirm that we read the requested amount of data */
	*data_length = data_size;
	return PSA_SUCCESS;
}

static void custom_psa_set_key_id(psa_key_attributes_t *attributes, mbedtls_svc_key_id_t key)
{
	zassert_equal(idx + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN, key, "Key ID mismatch");
}

void custom_psa_set_key_bits(psa_key_attributes_t *attributes, size_t bits)
{
	zassert_equal(sizeof(struct wifi_credentials_personal) * 8, bits, "Key bits mismatch");
}

void custom_psa_set_key_type(psa_key_attributes_t *attributes, psa_key_type_t type)
{
	zassert_equal(PSA_KEY_TYPE_RAW_DATA, type, "Key type mismatch");
}

void custom_psa_set_key_algorithm(psa_key_attributes_t *attributes, psa_algorithm_t alg)
{
	zassert_equal(PSA_ALG_NONE, alg, "Key algorithm mismatch");
}

void custom_psa_set_key_lifetime(psa_key_attributes_t *attributes, psa_key_lifetime_t lifetime)
{
	zassert_equal(PSA_KEY_LIFETIME_PERSISTENT, lifetime, "Key lifetime mismatch");
}

void custom_psa_set_key_usage_flags(psa_key_attributes_t *attributes, psa_key_usage_t usage_flags)
{
	zassert_equal(PSA_KEY_USAGE_EXPORT, usage_flags, "Key usage flags mismatch");
}

static void wifi_credentials_backend_psa_setup(void *_unused)
{
	RESET_FAKE(wifi_credentials_cache_ssid);
	RESET_FAKE(psa_export_key);
	RESET_FAKE(psa_import_key);
	RESET_FAKE(psa_destroy_key);
	psa_export_key_fake.custom_fake = custom_psa_export_key;
	psa_set_key_id_fake.custom_fake = custom_psa_set_key_id;
	psa_set_key_usage_flags_fake.custom_fake = custom_psa_set_key_usage_flags;
	psa_set_key_lifetime_fake.custom_fake = custom_psa_set_key_lifetime;
	psa_set_key_algorithm_fake.custom_fake = custom_psa_set_key_algorithm;
	psa_set_key_type_fake.custom_fake = custom_psa_set_key_type;
	psa_set_key_bits_fake.custom_fake = custom_psa_set_key_bits;
	idx = 0;
}

ZTEST(wifi_credentials_backend_psa, test_init)
{
	int ret;

	ret = wifi_credentials_backend_init();

	zassert_equal(0, ret, "Initialization failed");
	zassert_equal(psa_export_key_fake.call_count, CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES,
		      "Export key call count mismatch");
	zassert_equal(wifi_credentials_cache_ssid_fake.call_count,
		      CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES, "Cache SSID call count mismatch");
}

ZTEST(wifi_credentials_backend_psa, test_add)
{
	int ret = wifi_credentials_store_entry(idx, &example1,
					       sizeof(struct wifi_credentials_personal));

	zassert_equal(0, ret, "Store entry failed");
	zassert_equal_ptr(psa_import_key_fake.arg1_val, &example1, "Import key arg1 mismatch");
	zassert_equal(psa_import_key_fake.arg2_val, sizeof(struct wifi_credentials_personal),
		      "Import key arg2 mismatch");

	idx++;

	ret = wifi_credentials_store_entry(idx, &example2,
					   sizeof(struct wifi_credentials_personal));

	zassert_equal(0, ret, "Store entry failed");
	zassert_equal_ptr(psa_import_key_fake.arg1_val, &example2, "Import key arg1 mismatch");
	zassert_equal(psa_import_key_fake.arg2_val, sizeof(struct wifi_credentials_personal),
		      "Import key arg2 mismatch");

	zassert_equal(psa_import_key_fake.call_count, 2, "Import key call count mismatch");
	zassert_equal(psa_set_key_id_fake.call_count, 2, "Set key ID call count mismatch");
	zassert_equal(psa_set_key_usage_flags_fake.call_count, 2,
		      "Set key usage flags call count mismatch");
	zassert_equal(psa_set_key_lifetime_fake.call_count, 2,
		      "Set key lifetime call count mismatch");
	zassert_equal(psa_set_key_algorithm_fake.call_count, 2,
		      "Set key algorithm call count mismatch");
	zassert_equal(psa_set_key_type_fake.call_count, 2, "Set key type call count mismatch");
	zassert_equal(psa_set_key_bits_fake.call_count, 2, "Set key bits call count mismatch");
}

ZTEST(wifi_credentials_backend_psa, test_get)
{
	int ret;
	psa_key_id_t key_id = idx + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN;
	uint8_t buf[ENTRY_MAX_LEN];

	ret = wifi_credentials_load_entry(idx, buf, ARRAY_SIZE(buf));

	zassert_equal(0, ret, "Load entry failed");
	zassert_equal(psa_export_key_fake.arg0_val, key_id, "Export key arg0 mismatch");
	zassert_equal_ptr(psa_export_key_fake.arg1_val, buf, "Export key arg1 mismatch");
	zassert_equal(psa_export_key_fake.arg2_val, ARRAY_SIZE(buf), "Export key arg2 mismatch");

	idx++;
	key_id = idx + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN;

	ret = wifi_credentials_load_entry(idx, buf, ARRAY_SIZE(buf));

	zassert_equal(0, ret, "Load entry failed");
	zassert_equal(psa_export_key_fake.arg0_val, key_id, "Export key arg0 mismatch");
	zassert_equal_ptr(psa_export_key_fake.arg1_val, buf, "Export key arg1 mismatch");
	zassert_equal(psa_export_key_fake.arg2_val, ARRAY_SIZE(buf), "Export key arg2 mismatch");

	zassert_equal(psa_export_key_fake.call_count, 2, "Export key call count mismatch");
}

ZTEST(wifi_credentials_backend_psa, test_delete)
{
	int ret;

	ret = wifi_credentials_delete_entry(idx);

	zassert_equal(0, ret, "Delete entry failed");
	zassert_equal(psa_destroy_key_fake.arg0_val, ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN,
		      "Destroy key arg0 mismatch");

	idx++;

	ret = wifi_credentials_delete_entry(1);

	zassert_equal(0, ret, "Delete entry failed");
	zassert_equal(psa_destroy_key_fake.arg0_val,
		      idx + ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN,
		      "Destroy key arg0 mismatch");

	zassert_equal(psa_destroy_key_fake.call_count, 2, "Destroy key call count mismatch");
}

ZTEST_SUITE(wifi_credentials_backend_psa, NULL, NULL, wifi_credentials_backend_psa_setup, NULL,
	    NULL);
