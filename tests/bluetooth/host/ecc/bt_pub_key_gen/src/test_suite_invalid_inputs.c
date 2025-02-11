/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/ecc_help_utils.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include <host/ecc.h>
#include <host/hci_core.h>

ZTEST_SUITE(bt_pub_key_gen_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference for the callback function pointer argument
 *
 *  Constraints:
 *   - NULL reference is used for the callback function pointer argument
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_pub_key_gen_invalid_cases, test_null_key_reference)
{
	expect_assert();
	bt_pub_key_gen(NULL);
}

/*
 *  Test using the internal debug public key, but the callback is set to NULL
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command is supported
 *   - "LE Generate DH Key" command is supported
 *   - "ECC Debug Keys" command is supported
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' is enabled
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_pub_key_gen_invalid_cases, test_using_internal_debug_public_key)
{
	struct bt_pub_key_cb new_cb = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_USE_DEBUG_KEYS);

	expect_assert();
	new_cb.func = NULL;

	/* Set "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(1);
	/* Set "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(2);
	/* Set "ECC Debug Keys" command support bit */
	bt_dev.supported_commands[41] |= BIT(2);

	bt_pub_key_gen(&new_cb);
}

/*
 *  Test public key generation isn't supported if "LE Read Local P-256 Public Key" command
 *  isn't supported
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command isn't supported
 *   - "LE Generate DH Key" command is supported
 *
 *  Expected behaviour:
 *   - bt_pub_key_gen() returns a negative error code (-ENOTSUP)
 */
ZTEST(bt_pub_key_gen_invalid_cases, test_le_read_local_p_256_pub_key_cmd_not_supported)
{
	int result;
	struct bt_pub_key_cb new_cb = {0};

	/* Clear "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] &= ~BIT(1);
	/* Set "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(2);

	result = bt_pub_key_gen(&new_cb);

	zassert_true(result == -ENOTSUP, "Unexpected error code '%d' was returned", result);
}

/*
 *  Test public key generation isn't supported if "LE Generate DH Key command isn't supported
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command is supported
 *   - "LE Generate DH Key" command isn't supported
 *
 *  Expected behaviour:
 *   - bt_pub_key_gen() returns a negative error code (-ENOTSUP)
 */
ZTEST(bt_pub_key_gen_invalid_cases, test_le_generate_dh_key_cmd_not_supported)
{
	int result;
	struct bt_pub_key_cb new_cb = {0};

	/* Set "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(1);
	/* Clear "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] &= ~BIT(2);

	result = bt_pub_key_gen(&new_cb);

	zassert_true(result == -ENOTSUP, "Unexpected error code '%d' was returned", result);
}

/*
 *  Test public key generation fails if the callback is already registered
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command is supported
 *   - "LE Generate DH Key" command is supported
 *   - Callback passed is already registered
 *
 *  Expected behaviour:
 *   - bt_pub_key_gen() returns a negative error code (-EALREADY)
 */
ZTEST(bt_pub_key_gen_invalid_cases, test_callback_already_registered)
{
	int result;
	struct bt_pub_key_cb new_cb = {0};

	/* Set "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(1);
	/* Set "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(2);

	bt_pub_key_gen(&new_cb);
	result = bt_pub_key_gen(&new_cb);

	zassert_true(result == -EALREADY, "Unexpected error code '%d' was returned", result);
}

static void bt_pub_key_gen_null_key_callback(const uint8_t key[BT_PUB_KEY_LEN])
{
	const char *func_name = "bt_pub_key_gen_null_key_callback";

	zassert_is_null(key, "'%s()' was called with incorrect '%s' value", func_name, "key");
}

/*
 *  Test public key generation fails when reading public key fails
 *
 *  Constraints:
 *   - "LE Read Local P-256 Public Key" command is supported
 *   - "LE Generate DH Key" command is supported
 *   - bt_hci_cmd_send_sync() fails and returns a negative error code
 *
 *  Expected behaviour:
 *   - bt_pub_key_gen() returns a negative error code
 */
ZTEST(bt_pub_key_gen_invalid_cases, test_reading_le_read_local_p_256_pub_key_fails)
{
	int result;
	bool flags_check;
	struct bt_pub_key_cb new_cb = {0};
	sys_slist_t *pub_key_cb_slist = bt_ecc_get_pub_key_cb_slist();

	new_cb.func = bt_pub_key_gen_null_key_callback;

	/* Set "LE Read Local P-256 Public Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(1);
	/* Set "LE Generate DH Key" command support bit */
	bt_dev.supported_commands[34] |= BIT(2);

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	bt_hci_cmd_send_sync_fake.return_val = -1;

	result = bt_pub_key_gen(&new_cb);

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_LE_P256_PUBLIC_KEY);

	zassert_true(result < 0, "Unexpected error code '%d' was returned", result);

	flags_check = atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	zassert_false(flags_check, "Flags were not correctly set");

	flags_check = atomic_test_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);
	zassert_false(flags_check, "Flags were not correctly set");

	zassert_is_null(pub_key_cb_slist->head, "Incorrect value was set to list head");
	zassert_is_null(pub_key_cb_slist->tail, "Incorrect value was set to list tail");
}
