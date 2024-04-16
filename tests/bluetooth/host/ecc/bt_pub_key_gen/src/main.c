/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/ecc_help_utils.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/ecc.h>
#include <host/hci_core.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	sys_slist_t *pub_key_cb_slist = bt_ecc_get_pub_key_cb_slist();

	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	sys_slist_init(pub_key_cb_slist);

	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_pub_key_gen, NULL, NULL, NULL, NULL, NULL);

static void bt_pub_key_gen_debug_key_callback(const uint8_t key[BT_PUB_KEY_LEN])
{
	uint8_t const *internal_dbg_key = bt_ecc_get_internal_debug_public_key();
	const char *func_name = "bt_pub_key_gen_null_key_callback";

	zassert_equal_ptr(key, internal_dbg_key, "'%s()' was called with incorrect '%s' value",
			  func_name, "key");
}

/*
 *  Test using the internal debug public key
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command is supported
 *   - "LE Generate DH Key" command is supported
 *   - "ECC Debug Keys" command is supported
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' is enabled
 *
 *  Expected behaviour:
 *   - bt_pub_key_gen() returns 0 (success)
 */
ZTEST(bt_pub_key_gen, test_using_internal_debug_public_key)
{
	int result;
	bool flags_check;
	struct bt_pub_key_cb new_cb = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_USE_DEBUG_KEYS);

	new_cb.func = bt_pub_key_gen_debug_key_callback;

	/* Set "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(1);
	/* Set "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(2);
	/* Set "ECC Debug Keys" command support bit */
	bt_dev.supported_commands[41] |= BIT(2);

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	bt_hci_cmd_send_sync_fake.return_val = 0;

	result = bt_pub_key_gen(&new_cb);

	expect_not_called_bt_hci_cmd_send_sync();

	zassert_ok(result, "Unexpected error code '%d' was returned", result);

	flags_check = atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	zassert_true(flags_check, "Flags were not correctly set");
}

static void bt_pub_key_gen_callback(const uint8_t key[BT_PUB_KEY_LEN])
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test generating a public key request
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command is supported
 *   - "LE Generate DH Key" command is supported
 *   - bt_hci_cmd_send_sync() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_pub_key_gen() returns 0 (success)
 */
ZTEST(bt_pub_key_gen, test_public_key_generation_request_passes)
{
	int result;
	bool flags_check;
	struct bt_pub_key_cb new_cb = {0};

	new_cb.func = bt_pub_key_gen_callback;

	/* Set "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(1);
	/* Set "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(2);

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	bt_hci_cmd_send_sync_fake.return_val = 0;

	result = bt_pub_key_gen(&new_cb);

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_LE_P256_PUBLIC_KEY);

	zassert_ok(result, "Unexpected error code '%d' was returned", result);

	flags_check = atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	zassert_false(flags_check, "Flags were not correctly set");

	flags_check = atomic_test_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);
	zassert_true(flags_check, "Flags were not correctly set");
}

/*
 *  Test generating a public key request while 'BT_DEV_PUB_KEY_BUSY' flag is set
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command is supported
 *   - "LE Generate DH Key" command is supported
 *   - bt_hci_cmd_send_sync() isn't called
 *
 *  Expected behaviour:
 *   - bt_pub_key_gen() returns 0 (success)
 */
ZTEST(bt_pub_key_gen, test_no_public_key_generation_request_duplication)
{
	int result;
	bool flags_check;
	struct bt_pub_key_cb new_cb = {0};

	new_cb.func = bt_pub_key_gen_callback;

	/* Set "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(1);
	/* Set "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(2);

	atomic_set_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	result = bt_pub_key_gen(&new_cb);

	expect_not_called_bt_hci_cmd_send_sync();

	zassert_ok(result, "Unexpected error code '%d' was returned", result);

	flags_check = atomic_test_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);
	zassert_true(flags_check, "Flags were not correctly set");
}
