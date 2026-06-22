/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/adv.h"
#include "mocks/adv_expects.h"

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
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_scan_random_addr_check, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test checking scan random address if broadcaster role isn't enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_BROADCASTER' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns true
 */
ZTEST(bt_id_scan_random_addr_check, test_scan_returns_true_broadcaster_role_not_supported)
{
	bool result;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_BROADCASTER);

	result = bt_id_scan_random_addr_check();

	zassert_true(result, "Incorrect result was returned");
	expect_not_called_bt_le_adv_lookup_legacy();
}

/*
 *  Test checking scan random address if extended advertising is enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns true
 */
ZTEST(bt_id_scan_random_addr_check, test_scan_returns_true_ext_adv_enabled)
{
	bool result;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	result = bt_id_scan_random_addr_check();

	zassert_true(result, "Incorrect result was returned");
	expect_not_called_bt_le_adv_lookup_legacy();
}

/*
 *  Test checking scan random address when broadcaster role is enabled, but
 *  bt_le_adv_lookup_legacy() fails
 *
 *  Constraints:
 *   - 'CONFIG_BT_BROADCASTER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *   - bt_le_adv_lookup_legacy() returns NULL
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns true
 */
ZTEST(bt_id_scan_random_addr_check, test_scan_returns_true_bt_le_adv_lookup_legacy_fails)
{
	bool result;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	bt_le_adv_lookup_legacy_fake.return_val = NULL;

	result = bt_id_scan_random_addr_check();

	zassert_true(result, "Incorrect result was returned");
	expect_single_call_bt_le_adv_lookup_legacy();
}

/*
 *  Test checking scan random address when broadcaster role is enabled.
 *  bt_le_adv_lookup_legacy() returns a valid reference with 'BT_ADV_ENABLED' flag not set
 *
 *  Constraints:
 *   - 'CONFIG_BT_BROADCASTER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *   - bt_le_adv_lookup_legacy() returns a valid reference with 'BT_ADV_ENABLED' flag not set
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns true
 */
ZTEST(bt_id_scan_random_addr_check, test_scan_returns_true_advertiser_not_active)
{
	bool result;
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	atomic_clear_bit(adv.flags, BT_ADV_ENABLED);
	bt_le_adv_lookup_legacy_fake.return_val = &adv;

	result = bt_id_scan_random_addr_check();

	zassert_true(result, "Incorrect result was returned");
	expect_single_call_bt_le_adv_lookup_legacy();
}

/*
 *  Test when privacy is not enabled then the random address will be attempted to be set.
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - 'CONFIG_BT_BROADCASTER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *   - bt_le_adv_lookup_legacy() returns a valid reference with 'BT_ADV_ENABLED' flag not set
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns false
 */
ZTEST(bt_id_scan_random_addr_check, test_scan_returns_true_advertiser_active_no_privacy)
{
	bool result;
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(adv.flags, BT_ADV_ENABLED);
	bt_le_adv_lookup_legacy_fake.return_val = &adv;

	result = bt_id_scan_random_addr_check();

	zassert_true(result, "Incorrect result was returned");
	expect_single_call_bt_le_adv_lookup_legacy();
}

/*
 *  Test that scanner or initiator cannot start if the random address is used by the advertiser for
 *  an RPA with a different identity or for a random static identity address.
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'CONFIG_BT_BROADCASTER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *   - bt_le_adv_lookup_legacy() returns a valid reference with 'BT_ADV_ENABLED' and
 *     'BT_ADV_USE_IDENTITY' flags set
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns false
 */
ZTEST(bt_id_scan_random_addr_check, test_scan_returns_false_advertiser_active_privacy_enabled)
{
	bool result;
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(adv.flags, BT_ADV_ENABLED);
	atomic_set_bit(adv.flags, BT_ADV_USE_IDENTITY);
	bt_dev.id_addr[adv.id].type = BT_ADDR_LE_RANDOM;
	bt_le_adv_lookup_legacy_fake.return_val = &adv;

	result = bt_id_scan_random_addr_check();

	zassert_false(result, "Incorrect result was returned");
	expect_single_call_bt_le_adv_lookup_legacy();
}
