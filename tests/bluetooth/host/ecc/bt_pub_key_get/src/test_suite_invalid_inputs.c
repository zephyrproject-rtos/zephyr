/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include <host/ecc.h>
#include <host/hci_core.h>

ZTEST_SUITE(bt_pub_key_get_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test getting currently used public key if 'BT_DEV_HAS_PUB_KEY' isn't set and
 *  'CONFIG_BT_USE_DEBUG_KEYS' isn't enabled
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag isn't set
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' isn't enabled
 *
 *  Expected behaviour:
 *   - NULL value is returned
 */
ZTEST(bt_pub_key_get_invalid_cases, test_bt_dev_has_pub_key_not_set)
{
	const uint8_t *pub_key;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_USE_DEBUG_KEYS);

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	pub_key = bt_pub_key_get();

	zassert_is_null(pub_key, "Incorrect reference was returned");
}

/*
 *  Test getting currently used debug public key if 'CONFIG_BT_USE_DEBUG_KEYS' is enabled, but
 *  "ECC Debug Keys" command isn't supported
 *
 *  Constraints:
 *   - 'CONFIG_BT_USE_DEBUG_KEYS' is enabled
 *   - "ECC Debug Keys" command isn't supported
 *   - 'BT_DEV_HAS_PUB_KEY' flag isn't set
 *
 *  Expected behaviour:
 *   - NULL value is returned
 */
ZTEST(bt_pub_key_get_invalid_cases, test_get_debug_pub_key)
{
	const uint8_t *pub_key;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_USE_DEBUG_KEYS);

	/* Clear "ECC Debug Keys" command support bit */
	bt_dev.supported_commands[41] &= ~BIT(2);
	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	pub_key = bt_pub_key_get();

	zassert_is_null(pub_key, "Incorrect reference was returned");
}
