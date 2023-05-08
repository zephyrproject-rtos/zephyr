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
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));

	ADV_FFF_FAKES_LIST(RESET_FAKE);
	SETTINGS_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_delete, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test deleting an ID, but not the last one
 *
 *  Constraints:
 *   - ID value used is neither corresponds to default index nor the last index
 *
 *  Expected behaviour:
 *   - bt_dev.id_addr[] at index equals to the ID value used is cleared
 *   - bt_dev.irk[] at index equals to the ID value used is cleared (if privacy is enabled)
 *   - bt_id_delete() returns 0
 */
ZTEST(bt_id_delete, test_delete_non_default_no_last_item)
{
	int err;
	uint8_t id;
	int id_count;
#if defined(CONFIG_BT_PRIVACY)
	uint8_t zero_irk[16] = {0};
#endif

	bt_dev.id_count = 3;
	id = 1;
	id_count = bt_dev.id_count;

	bt_addr_le_copy(&bt_dev.id_addr[0], BT_RPA_LE_ADDR);
	bt_addr_le_copy(&bt_dev.id_addr[1], BT_STATIC_RANDOM_LE_ADDR_1);
	bt_addr_le_copy(&bt_dev.id_addr[2], BT_STATIC_RANDOM_LE_ADDR_2);

	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_id_delete(id);

	expect_not_called_bt_settings_save_id();

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
 *  Test deleting last ID
 *
 *  Constraints:
 *   - ID value used corresponds to the last item in the list bt_dev.id_addr[]
 *
 *  Expected behaviour:
 *   - bt_dev.id_addr[] at index equals to the ID value used is cleared
 *   - bt_dev.irk[] at index equals to the ID value used is cleared (if privacy is enabled)
 *   - bt_dev.id_count is decremented
 *   - bt_id_delete() returns 0
 */
ZTEST(bt_id_delete, test_delete_last_id)
{
	int err;
	uint8_t id;
	int id_count;
#if defined(CONFIG_BT_PRIVACY)
	uint8_t zero_irk[16] = {0};
#endif

	bt_dev.id_count = 2;
	id = bt_dev.id_count - 1;
	id_count = bt_dev.id_count;

	bt_addr_le_copy(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1);
	bt_addr_le_copy(&bt_dev.id_addr[1], BT_STATIC_RANDOM_LE_ADDR_2);

	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_id_delete(id);

	expect_not_called_bt_settings_save_id();

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
