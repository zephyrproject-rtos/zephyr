/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/adv.h"
#include "mocks/hci_core.h"
#include "mocks/settings.h"
#include "mocks/settings_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_id_delete_bt_settings, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test deleting an ID, but not the last one
 *  As 'CONFIG_BT_SETTINGS' is enabled, settings should be saved by calling bt_settings_store_id()
 *  and bt_settings_store_irk() if 'BT_DEV_READY' flag in bt_dev.flags is set
 *
 *  Constraints:
 *   - ID value used is neither corresponds to default index nor the last index
 *   - 'CONFIG_BT_SETTINGS' is enabled
 *   - 'BT_DEV_READY' flag in bt_dev.flags is set
 *
 *  Expected behaviour:
 *   - bt_dev.id_addr[] at index equals to the ID value used is cleared
 *   - bt_dev.irk[] at index equals to the ID value used is cleared (if privacy is enabled)
 *   - bt_settings_store_id() and bt_settings_store_irk() are called to save settings
 *   - bt_dev.id_count is kept unchanged
 *   - bt_id_delete() returns 0
 */
ZTEST(bt_id_delete_bt_settings, test_delete_non_default_no_last_item_settings_enabled)
{
	int err;
	uint8_t id;
	int id_count;
#if defined(CONFIG_BT_PRIVACY)
	uint8_t zero_irk[16] = {0};
#endif

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SETTINGS);

	bt_dev.id_count = 3;
	id = 1;
	id_count = bt_dev.id_count;

	bt_addr_le_copy(&bt_dev.id_addr[0], BT_RPA_LE_ADDR);
	bt_addr_le_copy(&bt_dev.id_addr[1], BT_STATIC_RANDOM_LE_ADDR_1);
	bt_addr_le_copy(&bt_dev.id_addr[2], BT_STATIC_RANDOM_LE_ADDR_2);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_id_delete(id);

	expect_single_call_bt_settings_store_id();
	expect_single_call_bt_settings_store_irk();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(bt_dev.id_count == id_count, "Incorrect ID count %d was set", bt_dev.id_count);

	zassert_mem_equal(&bt_dev.id_addr[id], BT_ADDR_LE_ANY, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
#if defined(CONFIG_BT_PRIVACY)
	zassert_mem_equal(bt_dev.irk[id], zero_irk, sizeof(zero_irk),
			  "Incorrect IRK value was set");
#endif
}

/*
 *  Test deleting last ID. As 'CONFIG_BT_SETTINGS' is enabled, settings should
 *  be saved by calling bt_settings_store_id() and bt_settings_store_irk() if
 *  'BT_DEV_READY' flag in bt_dev.flags is set
 *
 *  Constraints:
 *   - ID value used corresponds to the last item in the list bt_dev.id_addr[]
 *   - 'CONFIG_BT_SETTINGS' is enabled
 *   - 'BT_DEV_READY' flag in bt_dev.flags is set
 *
 *  Expected behaviour:
 *   - bt_dev.id_addr[] at index equals to the ID value used is cleared
 *   - bt_dev.irk[] at index equals to the ID value used is cleared (if privacy
 *     is enabled)
 *   - bt_settings_store_id() and bt_settings_store_irk() are called to save settings
 *   - bt_dev.id_count is decremented
 *   - bt_id_delete() returns 0
 */
ZTEST(bt_id_delete_bt_settings, test_delete_last_id_settings_enabled)
{
	int err;
	uint8_t id;
	int id_count;
#if defined(CONFIG_BT_PRIVACY)
	uint8_t zero_irk[16] = {0};
#endif

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SETTINGS);

	bt_dev.id_count = 2;
	id = bt_dev.id_count - 1;
	id_count = bt_dev.id_count;

	bt_addr_le_copy(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1);
	bt_addr_le_copy(&bt_dev.id_addr[1], BT_STATIC_RANDOM_LE_ADDR_2);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_id_delete(id);

	expect_single_call_bt_settings_store_id();
	expect_single_call_bt_settings_store_irk();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(bt_dev.id_count == (id_count - 1), "Incorrect ID count %d was set",
		     bt_dev.id_count);

	zassert_mem_equal(&bt_dev.id_addr[id], BT_ADDR_LE_ANY, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
#if defined(CONFIG_BT_PRIVACY)
	zassert_mem_equal(bt_dev.irk[id], zero_irk, sizeof(zero_irk),
			  "Incorrect IRK value was set");
#endif
}
