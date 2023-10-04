/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/keys_help_utils.h"
#include "mocks/settings.h"
#include "mocks/settings_expects.h"
#include "mocks/settings_store.h"
#include "mocks/settings_store_expects.h"
#include "mocks/util.h"
#include "mocks/util_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

DEFINE_FFF_GLOBALS;

static void tc_setup(void *f)
{
	/* Clear keys pool */
	clear_key_pool();

	/* Register resets */
	UTIL_FFF_FAKES_LIST(RESET_FAKE);
	SETTINGS_FFF_FAKES_LIST(RESET_FAKE);
	SETTINGS_STORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_keys_store_key_bt_settings_enabled, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Store an existing key (ID = 0) and verify the result.
 *  settings_save_one() returns 0 which represents success.
 *
 *  Constraints:
 *   - Key reference points to a valid item
 *   - Item ID is set to 0
 *   - Return value from settings_save_one() is 0
 *
 *  Expected behaviour:
 *   - bt_keys_store() returns 0 which represent success
 */
ZTEST(bt_keys_store_key_bt_settings_enabled, test_id_equal_0_with_no_error)
{
	int returned_code;
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_0;
	bt_addr_le_t *addr = BT_ADDR_LE_1;

	/* Add custom item to the keys pool */
	returned_key = bt_keys_get_addr(id, addr);
	zassert_true(returned_key != NULL, "bt_keys_get_addr() returned a non-valid reference");

	settings_save_one_fake.return_val = 0;

	/* Store the key */
	returned_code = bt_keys_store(returned_key);

	zassert_true(returned_code == 0, "bt_keys_store() returned a non-zero code");

	expect_not_called_u8_to_dec();
	expect_single_call_bt_settings_encode_key_with_null_key(&returned_key->addr);
	expect_single_call_settings_save_one(returned_key->storage_start);
}

/*
 *  Store an existing key (ID = 0) and verify the result.
 *  settings_save_one() returns a negative value of -1 which represents failure.
 *
 *  Constraints:
 *   - Key reference points to a valid item
 *   - Item ID is set to 0
 *   - Return value from settings_save_one() is -1
 *
 *  Expected behaviour:
 *   - bt_keys_store() returns a negative error code of -1
 */
ZTEST(bt_keys_store_key_bt_settings_enabled, test_id_equal_0_with_error)
{
	int returned_code;
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_0;
	bt_addr_le_t *addr = BT_ADDR_LE_1;

	/* Add custom item to the keys pool */
	returned_key = bt_keys_get_addr(id, addr);
	zassert_true(returned_key != NULL, "bt_keys_get_addr() returned a non-valid reference");

	settings_save_one_fake.return_val = -1;

	/* Store the key */
	returned_code = bt_keys_store(returned_key);

	zassert_true(returned_code == -1, "bt_keys_store() returned a non-zero code");

	expect_not_called_u8_to_dec();
	expect_single_call_bt_settings_encode_key_with_null_key(&returned_key->addr);
	expect_single_call_settings_save_one(returned_key->storage_start);
}

/*
 *  Store an existing key (ID != 0) and verify the result.
 *  settings_save_one() returns 0 which represents success.
 *
 *  Constraints:
 *   - Key reference points to a valid item
 *   - Item ID isn't set to 0
 *   - Return value from settings_save_one() is 0
 *
 *  Expected behaviour:
 *   - bt_keys_store() returns 0 which represent success
 */
ZTEST(bt_keys_store_key_bt_settings_enabled, test_id_not_equal_0_with_no_error)
{
	int returned_code;
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_1;
	bt_addr_le_t *addr = BT_ADDR_LE_1;

	/* Add custom item to the keys pool */
	returned_key = bt_keys_get_addr(id, addr);
	zassert_true(returned_key != NULL, "bt_keys_get_addr() returned a non-valid reference");

	settings_save_one_fake.return_val = 0;

	/* Store the key */
	returned_code = bt_keys_store(returned_key);

	zassert_true(returned_code == 0, "bt_keys_store() returned a non-zero code");

	expect_single_call_u8_to_dec(id);
	expect_single_call_bt_settings_encode_key_with_not_null_key(&returned_key->addr);
	expect_single_call_settings_save_one(returned_key->storage_start);
}

/*
 *  Store an existing key (ID != 0) and verify the result
 *  settings_save_one() returns a negative value of -1 which represents failure.
 *
 *  Constraints:
 *   - Key reference points to a valid item
 *   - Item ID isn't set to 0
 *   - Return value from settings_save_one() is -1
 *
 *  Expected behaviour:
 *   - bt_keys_store() returns a negative error code of -1
 */
ZTEST(bt_keys_store_key_bt_settings_enabled, test_id_not_equal_0_with_error)
{
	int returned_code;
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_1;
	bt_addr_le_t *addr = BT_ADDR_LE_1;

	/* Add custom item to the keys pool */
	returned_key = bt_keys_get_addr(id, addr);
	zassert_true(returned_key != NULL, "bt_keys_get_addr() returned a non-valid reference");

	settings_save_one_fake.return_val = -1;

	/* Store the key */
	returned_code = bt_keys_store(returned_key);

	zassert_true(returned_code == -1, "bt_keys_store() returned a non-zero code");

	expect_single_call_u8_to_dec(id);
	expect_single_call_bt_settings_encode_key_with_not_null_key(&returned_key->addr);
	expect_single_call_settings_save_one(returned_key->storage_start);
}
