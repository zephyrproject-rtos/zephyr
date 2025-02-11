/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/ecc_help_utils.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/ecc.h>
#include <host/hci_core.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_pub_key_get, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test getting currently used public key if 'BT_DEV_HAS_PUB_KEY' is set and
 *  'CONFIG_BT_USE_DEBUG_KEYS' isn't enabled
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag is set
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' isn't enabled
 *
 *  Expected behaviour:
 *   - A valid reference value is returned
 */
ZTEST(bt_pub_key_get, test_bt_dev_has_pub_key_set)
{
	const uint8_t *pub_key;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_USE_DEBUG_KEYS);

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	pub_key = bt_pub_key_get();

	zassert_equal(pub_key, bt_ecc_get_public_key(), "Incorrect reference was returned");
}

/*
 *  Test getting currently used debug public key if 'CONFIG_BT_USE_DEBUG_KEYS' is enabled and
 *  "ECC Debug Keys" command is supported
 *
 *  Constraints:
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' is enabled
 *   - "ECC Debug Keys" command is supported
 *   - 'BT_DEV_HAS_PUB_KEY' flag is set (just for testing and it shouldn't affect the result)
 *
 *  Expected behaviour:
 *   - A valid reference value is returned
 */
ZTEST(bt_pub_key_get, test_get_debug_pub_key1)
{
	const uint8_t *pub_key;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_USE_DEBUG_KEYS);

	/* Set "ECC Debug Keys" command support bit */
	bt_dev.supported_commands[41] |= BIT(2);
	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	pub_key = bt_pub_key_get();

	zassert_equal(pub_key, bt_ecc_get_internal_debug_public_key(),
		      "Incorrect reference was returned");
}
