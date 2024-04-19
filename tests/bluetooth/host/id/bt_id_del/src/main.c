/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/adv.h"
#include "mocks/adv_expects.h"
#include "mocks/conn.h"
#include "mocks/conn_expects.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/keys.h"
#include "mocks/keys_expects.h"
#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"
#include "mocks/scan.h"
#include "mocks/scan_expects.h"
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
	CONN_FFF_FAKES_LIST(RESET_FAKE);
	KEYS_FFF_FAKES_LIST(RESET_FAKE);
	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_del, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test deleting key from the resolving list when size of controller resolving list is zero
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to 0
 *   - bt_dev.le.rl_entries is greater than 0
 *
 *  Expected behaviour:
 *   - Passed key state is updated by clearing 'BT_KEYS_ID_ADDED' bit
 */
ZTEST(bt_id_del, test_zero_controller_list_size)
{
	struct bt_keys keys = {0};
	uint8_t expected_rl_entries;

	bt_dev.le.rl_size = 0;
	bt_dev.le.rl_entries = 1;
	expected_rl_entries = bt_dev.le.rl_entries - 1;
	keys.state |= BT_KEYS_ID_ADDED;

	bt_id_del(&keys);

	expect_not_called_bt_conn_lookup_state_le();

	zassert_equal(expected_rl_entries, bt_dev.le.rl_entries, "Incorrect entries count");
	zassert_false(keys.state & BT_KEYS_ID_ADDED, "Incorrect key state");
}

/*
 *  Test deleting key from the resolving list when size of controller resolving list isn't zero
 *  and number of entries in the resolving list is greater than controller resolving list size.
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to 0
 *   - bt_dev.le.rl_entries is greater than (bt_dev.le.rl_size + 1)
 *
 *  Expected behaviour:
 *   - Passed key state is updated by clearing 'BT_KEYS_ID_ADDED' bit
 */
ZTEST(bt_id_del, test_resolving_list_entries_greater_than_controller_list_size)
{
	struct bt_keys keys = {0};
	uint8_t expected_rl_entries;

	bt_dev.le.rl_size = 1;
	bt_dev.le.rl_entries = 3;
	expected_rl_entries = bt_dev.le.rl_entries - 1;
	keys.state |= BT_KEYS_ID_ADDED;

	bt_id_del(&keys);

	expect_not_called_bt_conn_lookup_state_le();

	zassert_equal(expected_rl_entries, bt_dev.le.rl_entries, "Incorrect entries count");
	zassert_false(keys.state & BT_KEYS_ID_ADDED, "Incorrect key state");
}

/*
 *  Test deleting key from the resolving list if host side resolving isn't used.
 *  bt_conn_lookup_state_le() returns a valid connection reference.
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to a value greater than 0
 *   - 'bt_dev.le.rl_entries > bt_dev.le.rl_size + 1' condition is false
 *   - bt_conn_lookup_state_le() returns a valid connection reference.
 *
 *  Expected behaviour:
 *   - Passed key state is updated by setting 'BT_KEYS_ID_PENDING_DEL' bit
 *   - 'BT_DEV_ID_PENDING' in bt_dev.flags is set
 */
ZTEST(bt_id_del, test_conn_lookup_returns_valid_conn_ref)
{
	struct bt_keys keys = {0};
	struct bt_conn conn_ref = {0};

	/* Break the host-side resolving condition */
	bt_dev.le.rl_size = 1;
	bt_dev.le.rl_entries = 1;

	bt_conn_lookup_state_le_fake.return_val = &conn_ref;

	bt_id_del(&keys);

	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_INITIATING);
	expect_single_call_bt_conn_unref(&conn_ref);

	zassert_true((keys.state & BT_KEYS_ID_PENDING_DEL) == BT_KEYS_ID_PENDING_DEL,
		     "Incorrect key state");
	zassert_true(atomic_test_bit(bt_dev.flags, BT_DEV_ID_PENDING),
		     "Flags were not correctly set");
}

void bt_le_ext_adv_foreach_custom_fake(void (*func)(struct bt_le_ext_adv *adv, void *data),
				       void *data)
{
	struct bt_le_ext_adv adv_params = {0};

	__ASSERT_NO_MSG(func != NULL);
	__ASSERT_NO_MSG(data != NULL);

	atomic_set_bit(adv_params.flags, BT_ADV_ENABLED);
	atomic_set_bit(adv_params.flags, BT_ADV_LIMITED);

	func(&adv_params, data);
}

/*
 *  Test deleting key from the resolving list if host side resolving isn't used.
 *  bt_conn_lookup_state_le() returns a NULL connection reference and 'CONFIG_BT_BROADCASTER' and
 *  'CONFIG_BT_EXT_ADV' are enabled.
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to a value greater than 0
 *   - 'bt_dev.le.rl_entries > bt_dev.le.rl_size + 1' condition is false
 *   - bt_conn_lookup_state_le() returns NULL.
 *   - 'CONFIG_BT_BROADCASTER' and 'CONFIG_BT_EXT_ADV' are enabled.
 *   - adv_is_limited_enabled() sets advertise enable flag to true
 *
 *  Expected behaviour:
 *   - Passed key state is updated by setting 'BT_KEYS_ID_PENDING_DEL' bit
 *   - 'BT_DEV_ID_PENDING' in bt_dev.flags is set if advertising is enabled
 */
ZTEST(bt_id_del, test_conn_lookup_returns_null_broadcaster_ext_adv_enabled)
{
	struct bt_keys keys = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);

	/* Break the host-side resolving condition */
	bt_dev.le.rl_size = 1;
	bt_dev.le.rl_entries = 1;

	bt_conn_lookup_state_le_fake.return_val = NULL;

	/* When bt_le_ext_adv_foreach() is called, this callback will be triggered and causes
	 * adv_is_limited_enabled() to set the advertising enable flag to true.
	 */
	bt_le_ext_adv_foreach_fake.custom_fake = bt_le_ext_adv_foreach_custom_fake;

	bt_id_del(&keys);

	expect_single_call_bt_le_ext_adv_foreach();

	zassert_true((keys.state & BT_KEYS_ID_PENDING_DEL) == BT_KEYS_ID_PENDING_DEL,
		     "Incorrect key state");
	zassert_true(atomic_test_bit(bt_dev.flags, BT_DEV_ID_PENDING),
		     "Flags were not correctly set");
}

/*
 *  Test deleting key from the resolving list when host side resolving isn't used.
 *  bt_conn_lookup_state_le() returns a NULL connection reference.
 *  'CONFIG_BT_BROADCASTER' is enabled while 'CONFIG_BT_EXT_ADV' isn't enabled.
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to a value greater than 0
 *   - (bt_dev.le.rl_entries > bt_dev.le.rl_size ) true
 *   - (bt_dev.le.rl_entries > bt_dev.le.rl_size + 1) false
 *   - bt_conn_lookup_state_le() returns NULL.
 *   - 'CONFIG_BT_BROADCASTER' is enabled.
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled.
 *   - 'CONFIG_BT_PRIVACY' isn't enabled.
 *
 *  Expected behaviour:
 *   - Passed key state is updated by setting 'BT_KEYS_ID_ADDED' bit
 */
ZTEST(bt_id_del, test_conn_lookup_returns_null_broadcaster_no_ext_adv)
{
	struct bt_keys keys = {0};
	struct net_buf net_buff = {0};
	uint8_t expected_rl_entries;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	/* Break the host-side resolving condition */
	bt_dev.le.rl_size = 1;
	/*
	 * (bt_dev.le.rl_entries > bt_dev.le.rl_size ) true
	 * (bt_dev.le.rl_entries > bt_dev.le.rl_size + 1) false
	 */
	bt_dev.le.rl_entries = 2;
	expected_rl_entries = bt_dev.le.rl_entries - 1;

	bt_conn_lookup_state_le_fake.return_val = NULL;
	keys.state |= BT_KEYS_ID_ADDED;

	/* This makes addr_res_enable() succeeds and returns 0 */
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	bt_id_del(&keys);

	expect_single_call_bt_keys_foreach_type(BT_KEYS_IRK);

	zassert_equal(expected_rl_entries, bt_dev.le.rl_entries, "Incorrect entries count");
	zassert_false(keys.state & BT_KEYS_ID_ADDED, "Incorrect key state");
}

/*
 *  Test deleting key from the resolving list when host side resolving isn't used.
 *  bt_conn_lookup_state_le() returns a NULL connection reference.
 *  'CONFIG_BT_BROADCASTER' and 'CONFIG_BT_PRIVACY' are enabled while 'CONFIG_BT_EXT_ADV' isn't
 *  enabled.
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to a value greater than 0
 *   - (bt_dev.le.rl_entries > bt_dev.le.rl_size ) true
 *   - (bt_dev.le.rl_entries > bt_dev.le.rl_size + 1) false
 *   - bt_conn_lookup_state_le() returns NULL.
 *   - 'CONFIG_BT_BROADCASTER' is enabled.
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled.
 *   - 'CONFIG_BT_PRIVACY' is enabled.
 *
 *  Expected behaviour:
 *   - Passed key state is updated by setting 'BT_KEYS_ID_ADDED' bit
 */
ZTEST(bt_id_del, test_conn_lookup_returns_null_broadcaster_no_ext_adv_privacy_enabled)
{
	struct bt_keys keys = {0};
	struct net_buf net_buff = {0};
	uint8_t expected_rl_entries;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	/* Break the host-side resolving condition */
	bt_dev.le.rl_size = 1;
	/*
	 * (bt_dev.le.rl_entries > bt_dev.le.rl_size ) true
	 * (bt_dev.le.rl_entries > bt_dev.le.rl_size + 1) false
	 */
	bt_dev.le.rl_entries = 2;
	expected_rl_entries = bt_dev.le.rl_entries - 1;

	bt_conn_lookup_state_le_fake.return_val = NULL;
	keys.state |= BT_KEYS_ID_ADDED;

	/* This makes addr_res_enable() succeeds and returns 0 */
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	bt_id_del(&keys);

	expect_single_call_bt_keys_foreach_type(BT_KEYS_ALL);

	zassert_equal(expected_rl_entries, bt_dev.le.rl_entries, "Incorrect entries count");
	zassert_false(keys.state & BT_KEYS_ID_ADDED, "Incorrect key state");
}

/*
 *  Test deleting key from the resolving list when host side resolving isn't used.
 *  bt_conn_lookup_state_le() returns a NULL connection reference.
 *  'CONFIG_BT_BROADCASTER' and 'CONFIG_BT_PRIVACY' are enabled while 'CONFIG_BT_EXT_ADV' isn't
 *  enabled.
 *  An HCI key address delete request is sent through hci_id_del()
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to a value greater than 0
 *   - bt_dev.le.rl_entries equals bt_dev.le.rl_size
 *   - bt_conn_lookup_state_le() returns NULL.
 *   - 'CONFIG_BT_BROADCASTER' is enabled.
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled.
 *   - 'CONFIG_BT_PRIVACY' is enabled.
 *
 *  Expected behaviour:
 *   - hci_id_del() uses the correct address while creating the HCI request
 *   - Passed key state is updated by setting 'BT_KEYS_ID_ADDED' bit
 */
ZTEST(bt_id_del, test_send_hci_id_del)
{
	struct bt_keys keys = {0};
	struct net_buf net_buff = {0};
	struct bt_hci_cp_le_rem_dev_from_rl cp = {0};
	uint8_t expected_rl_entries;

	/* Break the host-side resolving condition */
	bt_dev.le.rl_size = 1;
	bt_dev.le.rl_entries = 1;
	expected_rl_entries = bt_dev.le.rl_entries - 1;

	bt_conn_lookup_state_le_fake.return_val = NULL;
	keys.state |= BT_KEYS_ID_ADDED;

	bt_addr_le_copy(&keys.addr, BT_RPA_LE_ADDR);

	/* This makes hci_id_del() succeeds and returns 0 */
	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	bt_id_del(&keys);

	/* This verifies hci_id_del() behaviour */
	expect_single_call_net_buf_simple_add(&net_buff.b, sizeof(cp));

	zassert_mem_equal(&cp.peer_id_addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_equal(expected_rl_entries, bt_dev.le.rl_entries, "Incorrect entries count");
	zassert_false(keys.state & BT_KEYS_ID_ADDED, "Incorrect key state");
}

/*
 *  Test stopping scanning procedure if it is currently active and re-enable it after updating keys.
 *  If it is active, it is disabled then re-enabled after updating the key status.
 *  bt_conn_lookup_state_le() returns a NULL connection reference.
 *  'CONFIG_BT_BROADCASTER', 'CONFIG_BT_OBSERVER' and 'CONFIG_BT_EXT_ADV' are enabled.
 *
 *  Constraints:
 *   - bt_dev.le.rl_size is set to a value greater than 0
 *   - bt_dev.le.rl_entries is set to 0
 *   - bt_conn_lookup_state_le() returns NULL.
 *   - 'CONFIG_BT_BROADCASTER' is enabled.
 *   - 'CONFIG_BT_OBSERVER' is enabled.
 *   - 'CONFIG_BT_EXT_ADV' is enabled.
 *
 *  Expected behaviour:
 *   - Passed key state is updated by setting 'BT_KEYS_ID_ADDED' bit
 */
ZTEST(bt_id_del, test_scan_re_enabled_observer_enabled_ext_adv)
{
	struct bt_keys keys = {0};
	struct net_buf net_buff = {0};
	struct bt_hci_cp_le_rem_dev_from_rl cp = {0};
	uint8_t expected_args_history[] = {BT_HCI_LE_SCAN_DISABLE, BT_HCI_LE_SCAN_ENABLE};
	uint8_t expected_rl_entries;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);

	/* Break the host-side resolving condition */
	bt_dev.le.rl_size = 1;
	bt_dev.le.rl_entries = 1;
	expected_rl_entries = bt_dev.le.rl_entries - 1;

	/* Make scan enabled flag true */
	atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);
	atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED);

	bt_conn_lookup_state_le_fake.return_val = NULL;

	/* This makes hci_id_del() succeeds and returns 0 */
	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	bt_id_del(&keys);

	expect_call_count_bt_le_scan_set_enable(2, expected_args_history);

	zassert_equal(expected_rl_entries, bt_dev.le.rl_entries, "Incorrect entries count");
	zassert_false(keys.state & BT_KEYS_ID_ADDED, "Incorrect key state");
}
