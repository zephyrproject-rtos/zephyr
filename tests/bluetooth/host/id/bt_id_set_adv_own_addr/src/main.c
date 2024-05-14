/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto.h"
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

	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_set_adv_own_addr, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test setting the advertising private address through bt_id_set_adv_private_addr() if
 *  privacy is enabled and 'BT_LE_ADV_OPT_USE_IDENTITY' options bit isn't set.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_CONN' bit is set
 *   - Options 'BT_LE_ADV_OPT_USE_IDENTITY' bit isn't set
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - bt_id_set_adv_private_addr() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() succeeds and returns 0
 *   - Address type reference is updated
 */
ZTEST(bt_id_set_adv_own_addr, test_bt_id_set_adv_private_addr_succeeds_adv_connectable)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;
	bool dir_adv_test_lut[] = {true, false};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	options |= BT_LE_ADV_OPT_CONN;

	/* This will cause bt_id_set_adv_private_addr() to return 0 */
	atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	for (size_t i = 0; i < ARRAY_SIZE(dir_adv_test_lut); i++) {
		err = bt_id_set_adv_own_addr(&adv, options, dir_adv_test_lut[i], &own_addr_type);

		zassert_ok(err, "Unexpected error code '%d' was returned", err);
		zassert_true(own_addr_type == BT_ADDR_LE_RANDOM,
			     "Address type reference was incorrectly set");
	}

	options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

	bt_dev.le.features[(BT_LE_FEAT_BIT_PRIVACY) >> 3] |= BIT((BT_LE_FEAT_BIT_PRIVACY)&7);

	err = bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_HCI_OWN_ADDR_RPA_OR_RANDOM,
		     "Address type reference was incorrectly set");
}

/*
 *  Test setting the advertising private address with a static random address through
 *  bt_id_set_adv_random_addr() if privacy isn't enabled.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_CONN' bit is set
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - bt_id_set_adv_random_addr() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() succeeds and returns 0
 *   - Address type reference is updated
 */
ZTEST(bt_id_set_adv_own_addr, test_bt_id_set_adv_random_addr_succeeds_adv_connectable)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;
	bool dir_adv_test_lut[] = {true, false};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	/* If 'CONFIG_BT_EXT_ADV' is defined, it changes bt_id_set_adv_random_addr() behaviour */
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	options |= BT_LE_ADV_OPT_CONN;

	adv.id = 0;
	bt_addr_le_copy(&bt_dev.id_addr[adv.id], BT_RPA_LE_ADDR);

	/* This will cause bt_id_set_adv_random_addr() to return 0 */
	bt_addr_copy(&bt_dev.random_addr.a, &BT_RPA_LE_ADDR->a);

	for (size_t i = 0; i < ARRAY_SIZE(dir_adv_test_lut); i++) {
		err = bt_id_set_adv_own_addr(&adv, options, dir_adv_test_lut[i], &own_addr_type);

		zassert_ok(err, "Unexpected error code '%d' was returned", err);
		zassert_true(own_addr_type == BT_ADDR_LE_RANDOM,
			     "Address type reference was incorrectly set");
	}

	options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

	bt_dev.le.features[(BT_LE_FEAT_BIT_PRIVACY) >> 3] |= BIT((BT_LE_FEAT_BIT_PRIVACY)&7);

	err = bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_HCI_OWN_ADDR_RPA_OR_RANDOM,
		     "Address type reference was incorrectly set");
}

/*
 *  Test setting the advertising private address with a static random address through
 *  bt_id_set_adv_random_addr() when device isn't advertising as a connectable device (i.e.
 *  BT_LE_ADV_OPT_CONN bit in options isn't set) and the advertisement is using the device
 *  identity (i.e. BT_LE_ADV_OPT_USE_IDENTITY bit is set in options).
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_USE_IDENTITY' bit is set
 *   - Options 'BT_LE_ADV_OPT_CONN' bit isn't set
 *   - bt_id_set_adv_random_addr() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() succeeds and returns 0
 *   - Address type reference is updated
 */
ZTEST(bt_id_set_adv_own_addr, test_bt_id_set_adv_random_addr_succeeds_not_connectable)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;
	bool dir_adv_test_lut[] = {true, false};

	/* If 'CONFIG_BT_EXT_ADV' is defined, it changes bt_id_set_adv_random_addr() behaviour */
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	options |= BT_LE_ADV_OPT_USE_IDENTITY;
	options &= ~BT_LE_ADV_OPT_CONN;

	adv.id = 0;
	bt_addr_le_copy(&bt_dev.id_addr[adv.id], BT_RPA_LE_ADDR);

	/* This will cause bt_id_set_adv_random_addr() to return 0 */
	bt_addr_copy(&bt_dev.random_addr.a, &BT_RPA_LE_ADDR->a);

	for (size_t i = 0; i < ARRAY_SIZE(dir_adv_test_lut); i++) {
		err = bt_id_set_adv_own_addr(&adv, options, dir_adv_test_lut[i], &own_addr_type);

		zassert_ok(err, "Unexpected error code '%d' was returned", err);
		zassert_true(own_addr_type == BT_ADDR_LE_RANDOM,
			     "Address type reference was incorrectly set");
	}
}

/*
 *  Test setting the advertising private address through bt_id_set_adv_private_addr() if
 *  'BT_LE_ADV_OPT_USE_IDENTITY' and 'BT_LE_ADV_OPT_USE_IDENTITY' options bits aren't set.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_CONN' bit isn't set
 *   - Options 'BT_LE_ADV_OPT_USE_IDENTITY' bit isn't set
 *   - bt_id_set_adv_random_addr() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() succeeds and returns 0
 *   - Address type reference is updated
 */
ZTEST(bt_id_set_adv_own_addr, test_bt_id_set_adv_private_addr_succeeds_not_connectable)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;
	bool dir_adv_test_lut[] = {true, false};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	options &= ~BT_LE_ADV_OPT_CONN;
	options &= ~BT_LE_ADV_OPT_USE_IDENTITY;

	/* This will cause bt_id_set_adv_private_addr() to return 0 */
	atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	for (size_t i = 0; i < ARRAY_SIZE(dir_adv_test_lut); i++) {
		err = bt_id_set_adv_own_addr(&adv, options, dir_adv_test_lut[i], &own_addr_type);

		zassert_ok(err, "Unexpected error code '%d' was returned", err);
		zassert_true(own_addr_type == BT_ADDR_LE_RANDOM,
			     "Address type reference was incorrectly set");
	}
}

/*
 *  Test stopping scanning if it is supported through enabling 'CONFIG_BT_OBSERVER' and active
 *  before updating the device advertising address and then re-enable it after the update is done.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_CONN' bit isn't set
 *   - Options 'BT_LE_ADV_OPT_USE_IDENTITY' bit isn't set
 *
 *  Expected behaviour:
 *   - Scanning is disabled and then re-enabled again after updating the address
 */
ZTEST(bt_id_set_adv_own_addr, test_observer_scanning_re_enabled_after_updating_address)
{
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;
	uint8_t expected_args_history[] = {BT_HCI_LE_SCAN_DISABLE, BT_HCI_LE_SCAN_ENABLE};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);

	options &= ~BT_LE_ADV_OPT_CONN;
	options &= ~BT_LE_ADV_OPT_USE_IDENTITY;

	/* Set device scanning active flag */
	atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);

	bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	expect_call_count_bt_le_scan_set_enable(2, expected_args_history);
}
