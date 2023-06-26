/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/settings.h>
#include <asm-generic/errno.h>

#include "settings.h"

#include "common.h"
#include "zephyr/sys/util.h"
#include <stddef.h>
#include <string.h>

LOG_MODULE_DECLARE(bsim_bt_settings_cleanup, LOG_LEVEL_DBG);

const char *expected_settings_key[] = {
	"hash",
	"id",
	"keys/0000000000000",
	"sc/0000000000000",
	"cf/0000000000000",
	"ccc/0000000000000",
	"mesh/dummy/key",
};

static int print_all_settings_cb(const char *key, size_t len, settings_read_cb read_cb,
				 void *cb_arg, void *param)
{
	ssize_t err;
	uint8_t data[100] = {0};

	err = read_cb(cb_arg, data, len);
	if (err != len) {
		FAIL("Failed to read data (err %d)\n", err);
	}

	LOG_INF("key: 'bt/%s'", key);
	LOG_HEXDUMP_INF(data, len, "value:");

	return 0;
}

static void print_all_settings(void)
{
	int err;

	err = settings_load_subtree_direct("bt", print_all_settings_cb, NULL);
	if (err) {
		FAIL("Failed to load 'bt' subtree (err %s)\n", err);
	}
}

static int check_settings_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			     void *param)
{
	bool key_is_expected = false;
	uint8_t *key_not_deleted = (uint8_t *)param;

	for (size_t i = 0; i < ARRAY_SIZE(expected_settings_key); i++) {
		if (strcmp(expected_settings_key[i], key) == 0 && key_not_deleted[i] == 0) {
			key_not_deleted[i] = 1;
			key_is_expected = true;
		}
	}

	if (!key_is_expected) {
		FAIL("Key 'bt/%s' should have been deleted\n", key);
	}

	return 0;
}

static void check_settings(void)
{
	int err;
	uint8_t key_not_deleted[ARRAY_SIZE(expected_settings_key)] = {0};

	err = settings_load_subtree_direct("bt", check_settings_cb, key_not_deleted);
	if (err) {
		FAIL("Failed to load 'bt' subtree (err %d)\n", err);
	}

	for (size_t i = 0; i < ARRAY_SIZE(expected_settings_key); i++) {
		if (key_not_deleted[i] == 0) {
			FAIL("Key '%s' should not have been deleted\n", expected_settings_key[i]);
		}
	}
}

static void populate_settings(void)
{
	int err;
	uint8_t dummy_value = 0;
	bt_addr_le_t dummy_addr;
	bt_addr_le_t ids_addr[CONFIG_BT_ID_MAX] = {0};

	BUILD_ASSERT(CONFIG_BT_ID_MAX >= 3);

	bt_addr_le_copy(&dummy_addr, BT_ADDR_LE_ANY);

	err = bt_addr_le_create_static(&ids_addr[0]);
	if (err) {
		FAIL("Failed to create static address (err %d)\n", err);
	}

	err = bt_addr_le_create_static(&ids_addr[2]);
	if (err) {
		FAIL("Failed to create static address (err %d)\n", err);
	}

	err = settings_save_one("bt/id", ids_addr, sizeof(ids_addr));
	if (err) {
		FAIL("Failed to save ID (err %d)\n", err);
	}

	bt_settings_store_keys(0, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_sc(0, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_cf(0, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_ccc(0, &dummy_addr, &dummy_value, sizeof(dummy_value));

	bt_settings_store_keys(1, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_sc(1, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_cf(1, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_ccc(1, &dummy_addr, &dummy_value, sizeof(dummy_value));

	settings_save_one("bt/dummy/key", &dummy_value, sizeof(dummy_value));

	settings_save_one("bt/i", &dummy_value, sizeof(dummy_value));
	settings_save_one("bt/idd", &dummy_value, sizeof(dummy_value));

	/* without keys for id 2, this settings should be deleted */
	bt_settings_store_sc(2, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_cf(2, &dummy_addr, &dummy_value, sizeof(dummy_value));
	bt_settings_store_ccc(2, &dummy_addr, &dummy_value, sizeof(dummy_value));

	settings_save_one("bt/mesh/dummy/key", &dummy_value, sizeof(dummy_value));
}

void run_tester(void)
{
	int id;
	int err;
	char addr_str[BT_ADDR_STR_LEN];
	size_t id_count = CONFIG_BT_ID_MAX;
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX] = {0};

	LOG_DBG("Starting test... (tester)");

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
	}

	LOG_DBG("Bluetooth initialised");

	err = settings_load();
	if (err) {
		FAIL("Failed to load settings (err %d)\n", err);
	}

	err = bt_id_create(NULL, NULL);
	if (err < 0) {
		FAIL("Failed to create new identity (err %d)\n", err);
	}
	id = err;

	err = bt_id_create(NULL, NULL);
	if (err < 0) {
		FAIL("Failed to create new identity (err %d)\n", err);
	}

	err = bt_id_delete(id);
	if (err) {
		FAIL("Failed to delete identity %d (err %d)\n", id, err);
	}

	bt_id_get(addrs, &id_count);

	for (size_t i = 0; i < CONFIG_BT_ID_MAX; i++) {
		bt_addr_le_to_str(&addrs[i], addr_str, BT_ADDR_STR_LEN);
		LOG_DBG("ID[%d]: %s", i, addr_str);
	}

	populate_settings();

	print_all_settings();

	PASS("Test passed (tester)\n");
}

void run_dut(void)
{
	int err;
	bool settings_cleanup_dry_run = false;

	LOG_DBG("Starting test... (dut)");

	err = settings_subsys_init();
	if (err) {
		FAIL("settings_subsys_init failed (err %d)\n", err);
	}

	err = bt_settings_cleanup(settings_cleanup_dry_run);
	if (err) {
		FAIL("Failed to clean settings (err %d)\n", err);
	}

	print_all_settings();

	check_settings();

	PASS("Test passed (dut)\n");
}
