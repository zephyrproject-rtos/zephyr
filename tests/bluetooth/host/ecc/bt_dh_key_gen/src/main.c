/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/ecc_help_utils.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/ecc.h>
#include <host/hci_core.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	bt_dh_key_cb_t *dh_key_cb = bt_ecc_get_dh_key_cb();

	*dh_key_cb = NULL;
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));

	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_dh_key_gen, NULL, NULL, NULL, NULL, NULL);

static void bt_dh_key_unreachable_cb(const uint8_t key[BT_DH_KEY_LEN])
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test DH key generation succeeds
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag is set
 *   - 'BT_DEV_PUB_KEY_BUSY' flag isn't set
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_dh_key_gen() returns 0 (success)
 */
ZTEST(bt_dh_key_gen, test_generate_dh_key_passes)
{
	int result;
	struct net_buf net_buff = {0};
	struct bt_hci_cp_le_generate_dhkey cp = {0};
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_USE_DEBUG_KEYS);

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	/* This makes hci_generate_dhkey_v1() succeeds and returns 0 */
	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	result = bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);

	expect_single_call_net_buf_simple_add(&net_buff.b, sizeof(cp));
	expect_single_call_bt_hci_cmd_create(BT_HCI_OP_LE_GENERATE_DHKEY, sizeof(cp));
	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_LE_GENERATE_DHKEY);

	zassert_ok(result, "Unexpected error code '%d' was returned", result);
}

/*
 *  Test DH key generation succeeds with 'CONFIG_BT_USE_DEBUG_KEYS' enabled
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag is set
 *   - 'BT_DEV_PUB_KEY_BUSY' flag isn't set
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' is enabled
 *
 *  Expected behaviour:
 *   - bt_dh_key_gen() returns 0 (success)
 */
ZTEST(bt_dh_key_gen, test_generate_dh_key_passes_with_debug_keys_enabled)
{
	int result;
	struct net_buf net_buff = {0};
	struct bt_hci_cp_le_generate_dhkey_v2 cp = {0};
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_USE_DEBUG_KEYS);

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	/* Set "ECC Debug Keys" command support bit */
	bt_dev.supported_commands[41] |= BIT(2);

	/* This makes hci_generate_dhkey_v2() succeeds and returns 0 */
	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	result = bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);

	expect_single_call_net_buf_simple_add(&net_buff.b, sizeof(cp));
	expect_single_call_bt_hci_cmd_create(BT_HCI_OP_LE_GENERATE_DHKEY_V2, sizeof(cp));
	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_LE_GENERATE_DHKEY_V2);

	zassert_ok(result, "Unexpected error code '%d' was returned", result);
}
