/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"
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

	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_set_adv_random_addr, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test setting advertising random address while 'CONFIG_BT_EXT_ADV' isn't enabled
 *
 *  Constraints:
 *   - set_random_address() returns 0 (success)
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_random_addr() returns 0 (success)
 */
ZTEST(bt_id_set_adv_random_addr, test_no_ext_adv)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	/* This will make set_random_address() succeeds and returns 0 */
	bt_addr_copy(&bt_dev.random_addr.a, &BT_RPA_LE_ADDR->a);

	err = bt_id_set_adv_random_addr(&adv_param, &BT_RPA_LE_ADDR->a);

	expect_not_called_bt_hci_cmd_create();
	expect_not_called_bt_hci_cmd_send_sync();
	expect_not_called_net_buf_simple_add();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting advertising random address while 'CONFIG_BT_EXT_ADV' is enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *   - 'BT_ADV_PARAMS_SET' flag in advertising parameters reference isn't set
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_random_addr() returns 0 (success)
 *   - Random address field in advertising parameters reference is loaded with the address
 *   - 'BT_ADV_RANDOM_ADDR_PENDING' flag is set
 */
ZTEST(bt_id_set_adv_random_addr, test_ext_adv_enabled)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	atomic_clear_bit(adv_param.flags, BT_ADV_PARAMS_SET);

	err = bt_id_set_adv_random_addr(&adv_param, &BT_RPA_LE_ADDR->a);

	expect_not_called_bt_hci_cmd_create();
	expect_not_called_bt_hci_cmd_send_sync();
	expect_not_called_net_buf_simple_add();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);

	zassert_true(atomic_test_bit(adv_param.flags, BT_ADV_RANDOM_ADDR_PENDING),
		     "Flags were not correctly set");

	zassert_mem_equal(&adv_param.random_addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}

/*
 *  Test setting advertising random address while 'CONFIG_BT_EXT_ADV' is enabled
 *  and 'BT_ADV_PARAMS_SET' flag in advertising parameters reference is set.
 *
 *  Constraints:
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *   - 'BT_ADV_PARAMS_SET' flag in advertising parameters reference is set
 *   - bt_hci_cmd_create() returns a valid buffer pointer
 *   - bt_hci_cmd_send_sync() returns 0 (success)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_random_addr() returns 0 (success)
 */
ZTEST(bt_id_set_adv_random_addr, test_ext_adv_enabled_hci_set_adv_set_random_addr)
{
	int err;
	struct net_buf net_buff;
	struct bt_hci_cp_le_set_adv_set_random_addr cp;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(adv_param.flags, BT_ADV_PARAMS_SET);

	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	err = bt_id_set_adv_random_addr(&adv_param, &BT_RPA_LE_ADDR->a);

	expect_single_call_net_buf_simple_add(&net_buff.b, sizeof(cp));
	expect_single_call_bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_SET_RANDOM_ADDR, sizeof(cp));
	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_SET_RANDOM_ADDR);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_equal(cp.handle, adv_param.handle, "Incorrect handle value was set");
	zassert_mem_equal(&cp.bdaddr, &BT_RPA_LE_ADDR->a, sizeof(bt_addr_t),
			  "Incorrect address was set");
	zassert_mem_equal(&adv_param.random_addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}
