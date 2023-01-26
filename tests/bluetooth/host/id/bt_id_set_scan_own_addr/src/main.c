/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto.h"
#include "mocks/hci_core.h"
#include "mocks/rpa.h"
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

	RPA_FFF_FAKES_LIST(RESET_FAKE);
	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_set_scan_own_addr, NULL, NULL, NULL, NULL, NULL);

static int bt_rand_custom_fake(void *buf, size_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len == sizeof(BT_ADDR->val));

	/* This will make set_random_address() succeeds and returns 0 */
	memcpy(buf, &BT_ADDR->val, len);
	bt_addr_copy(&bt_dev.random_addr.a, BT_ADDR);

	return 0;
}

/*
 *  Test setting scan own address while 'CONFIG_BT_PRIVACY' isn't enabled.
 *  bt_id_set_private_addr() is called to generate a NRPA and passed to set_random_address().
 *  Address type reference is updated upon success.
 *
 *  Constraints:
 *   - bt_id_set_private_addr() succeeds and returns 0
 *   - 'CONFIG_BT_SCAN_WITH_IDENTITY' isn't enabled
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_scan_own_addr() returns 0
 *   - Address type reference is updated
 */
ZTEST(bt_id_set_scan_own_addr, test_set_nrpa_scan_address_no_privacy)
{
	int err;
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_SCAN_WITH_IDENTITY);

	bt_rand_fake.custom_fake = bt_rand_custom_fake;

	err = bt_id_set_scan_own_addr(false, &own_addr_type);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_ADDR_LE_RANDOM,
		     "Address type reference was incorrectly set");
}

/*
 *  Test setting scan own address while 'CONFIG_BT_PRIVACY' isn't enabled.
 *  If 'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled and the default identity has an RPA address of type
 * 'BT_ADDR_LE_RANDOM', set_random_address() is called and address type reference is updated upon
 *  success.
 *
 *  Constraints:
 *   - Default identity has an address with the type 'BT_ADDR_LE_RANDOM'
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - 'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled
 *   - set_random_address() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_set_scan_own_addr() returns 0
 *   - Address type reference is updated
 */
ZTEST(bt_id_set_scan_own_addr, test_setting_scan_own_rpa_address_no_privacy)
{
	int err;
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SCAN_WITH_IDENTITY);

	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	/* This will make set_random_address() succeeds and returns 0 */
	bt_addr_copy(&bt_dev.random_addr.a, &BT_RPA_LE_ADDR->a);

	err = bt_id_set_scan_own_addr(false, &own_addr_type);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_ADDR_LE_RANDOM,
		     "Address type reference was incorrectly set");
}

/*
 *  Test setting scan own address while 'CONFIG_BT_PRIVACY' is enabled and privacy features
 *  'BT_LE_FEAT_BIT_PRIVACY' bit isn't set.
 *  bt_id_set_private_addr() is called with 'BT_ID_DEFAULT' as the ID and address type reference is
 *  updated upon success.
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'BT_LE_FEAT_BIT_PRIVACY' bit isn't set.
 *   - bt_id_set_private_addr() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_set_scan_own_addr() returns 0
 *   - Address type reference is updated with the value 'BT_ADDR_LE_RANDOM'
 */
ZTEST(bt_id_set_scan_own_addr, test_setting_scan_own_address_privacy_enabled)
{
	int err;
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	/* This will cause bt_id_set_private_addr() to return 0 (success) */
	atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	err = bt_id_set_scan_own_addr(true, &own_addr_type);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_ADDR_LE_RANDOM,
		     "Address type reference was incorrectly set");
}

/*
 *  Test setting scan own address while 'CONFIG_BT_PRIVACY' is enabled and privacy features
 *  'BT_LE_FEAT_BIT_PRIVACY' bit is set.
 *  bt_id_set_private_addr() is called with 'BT_ID_DEFAULT' as the ID and address type reference
 *  is updated upon success.
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'BT_LE_FEAT_BIT_PRIVACY' bit is set.
 *   - bt_id_set_private_addr() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_set_scan_own_addr() returns 0
 *   - Address type reference is updated with the value 'BT_HCI_OWN_ADDR_RPA_OR_RANDOM'
 */
ZTEST(bt_id_set_scan_own_addr, test_setting_scan_own_address_privacy_features_set)
{
	int err;
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	/* This will cause bt_id_set_private_addr() to return 0 (success) */
	atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	bt_dev.le.features[(BT_LE_FEAT_BIT_PRIVACY) >> 3] |= BIT((BT_LE_FEAT_BIT_PRIVACY)&7);

	err = bt_id_set_scan_own_addr(true, &own_addr_type);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_HCI_OWN_ADDR_RPA_OR_RANDOM,
		     "Address type reference was incorrectly set");
}
