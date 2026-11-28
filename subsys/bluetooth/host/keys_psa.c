/* keys_psa.c - Bluetooth host keys in PSA persistent storage */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/psa/key_ids.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>

#include <psa/crypto.h>

#include "keys_psa.h"

#define LOG_LEVEL CONFIG_BT_KEYS_LOG_LEVEL
LOG_MODULE_DECLARE(bt_keys);

#define KEY_ID_BEGIN ZEPHYR_PSA_BT_HOST_KEY_ID_RANGE_BEGIN

/* The factor 2 leaves room for the replacement of every key (copy-on-write).
 */
#define KEY_ID_COUNT (2 * (3 * CONFIG_BT_MAX_PAIRED + CONFIG_BT_ID_MAX))

BUILD_ASSERT(KEY_ID_COUNT <= ZEPHYR_PSA_BT_HOST_KEY_ID_RANGE_SIZE,
	     "Too many Bluetooth host PSA keys for the key ID range");

static ATOMIC_DEFINE(used_ids, KEY_ID_COUNT);

static bool id_in_range(psa_key_id_t id)
{
	return (id >= KEY_ID_BEGIN) && (id < KEY_ID_BEGIN + KEY_ID_COUNT);
}

static psa_key_id_t id_alloc(void)
{
	for (size_t i = 0; i < KEY_ID_COUNT; i++) {
		if (!atomic_test_and_set_bit(used_ids, i)) {
			return KEY_ID_BEGIN + i;
		}
	}

	return PSA_KEY_ID_NULL;
}

static void id_assign(psa_key_id_t id)
{
	atomic_set_bit(used_ids, id - KEY_ID_BEGIN);
}

static void id_free(psa_key_id_t id)
{
	atomic_clear_bit(used_ids, id - KEY_ID_BEGIN);
}

static bool status_is_missing(psa_status_t status)
{
	return (status == PSA_ERROR_DOES_NOT_EXIST) || (status == PSA_ERROR_INVALID_HANDLE);
}

int bt_keys_psa_init(void)
{
	const psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init() failed %d", status);
		return -EIO;
	}

	return 0;
}

static psa_status_t export_key(psa_key_id_t id, uint8_t key[BT_KEYS_PSA_KEY_SIZE])
{
	size_t len;
	psa_status_t status;

	status = psa_export_key(id, key, BT_KEYS_PSA_KEY_SIZE, &len);
	if ((status == PSA_SUCCESS) && (len != BT_KEYS_PSA_KEY_SIZE)) {
		status = PSA_ERROR_DATA_CORRUPT;
	}

	return status;
}

int bt_keys_psa_put(const uint8_t key[BT_KEYS_PSA_KEY_SIZE], psa_key_id_t *id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t new_id;
	psa_key_id_t out_id;
	psa_status_t status;

	__ASSERT_NO_MSG(key != NULL);
	__ASSERT_NO_MSG(id != NULL);

	new_id = id_alloc();
	if (new_id == PSA_KEY_ID_NULL) {
		LOG_ERR("No free PSA key ID");
		return -ENOMEM;
	}

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, BT_KEYS_PSA_KEY_SIZE * 8);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, new_id);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_EXPORT);

	status = psa_import_key(&attr, key, BT_KEYS_PSA_KEY_SIZE, &out_id);
	if (status == PSA_ERROR_ALREADY_EXISTS) {
		LOG_WRN("PSA key 0x%08x already exists, replacing it", (uint32_t)new_id);
		(void)psa_destroy_key(new_id);
		status = psa_import_key(&attr, key, BT_KEYS_PSA_KEY_SIZE, &out_id);
	}

	psa_reset_key_attributes(&attr);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import PSA key 0x%08x (status %d)", (uint32_t)new_id, status);
		id_free(new_id);
		return -EIO;
	}

	*id = out_id;

	return 0;
}

int bt_keys_psa_get(psa_key_id_t id, uint8_t key[BT_KEYS_PSA_KEY_SIZE])
{
	psa_status_t status;

	__ASSERT_NO_MSG(key != NULL);

	if (!id_in_range(id)) {
		LOG_ERR("PSA key ID 0x%08x out of range", (uint32_t)id);
		return -EINVAL;
	}

	status = export_key(id, key);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to export PSA key 0x%08x (status %d)", (uint32_t)id, status);
		(void)memset(key, 0, BT_KEYS_PSA_KEY_SIZE);
		return status_is_missing(status) ? -ENOENT : -EIO;
	}

	id_assign(id);

	return 0;
}

bool bt_keys_psa_matches(psa_key_id_t id, const uint8_t key[BT_KEYS_PSA_KEY_SIZE])
{
	uint8_t stored[BT_KEYS_PSA_KEY_SIZE];
	bool match;

	__ASSERT_NO_MSG(key != NULL);

	if (!id_in_range(id)) {
		return false;
	}

	if (export_key(id, stored) != PSA_SUCCESS) {
		return false;
	}

	match = memcmp(stored, key, sizeof(stored)) == 0;
	(void)memset(stored, 0, sizeof(stored));

	return match;
}

void bt_keys_psa_del(psa_key_id_t id)
{
	psa_status_t status;

	if (id == PSA_KEY_ID_NULL) {
		return;
	}

	if (!id_in_range(id)) {
		LOG_WRN("PSA key ID 0x%08x out of range", (uint32_t)id);
		return;
	}

	status = psa_destroy_key(id);
	if ((status != PSA_SUCCESS) && !status_is_missing(status)) {
		LOG_ERR("Failed to destroy PSA key 0x%08x (status %d)", (uint32_t)id, status);
	}

	id_free(id);
}

void bt_keys_psa_reset(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(used_ids); i++) {
		(void)atomic_clear(&used_ids[i]);
	}
}
