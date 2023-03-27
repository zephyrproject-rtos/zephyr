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

ZTEST_SUITE(bt_id_adv_random_addr_check, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test checking advertising random address if observer role isn't enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_OBSERVER' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_adv_random_addr_check() returns true
 */
ZTEST(bt_id_adv_random_addr_check, test_check_returns_true_observer_role_not_supported)
{
	bool result;
	struct bt_le_adv_param adv_param = {0};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_OBSERVER);

	result = bt_id_adv_random_addr_check(&adv_param);

	zassert_true(result, "Incorrect result was returned");
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
ZTEST(bt_id_adv_random_addr_check, test_check_returns_true_ext_adv_enabled)
{
	bool result;
	struct bt_le_adv_param adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	result = bt_id_adv_random_addr_check(&adv_param);

	zassert_true(result, "Incorrect result was returned");
}

/*
 *  Test checking advertising random address when scanner roles aren't active so that
 *  'BT_DEV_INITIATING' and 'BT_DEV_SCANNING' aren't set in bt_dev.flags
 *
 *  Constraints:
 *   - 'CONFIG_BT_OBSERVER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *   - 'BT_DEV_INITIATING' and 'BT_DEV_SCANNING' aren't set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns true
 */
ZTEST(bt_id_adv_random_addr_check, test_scanner_roles_not_active)
{
	bool result;
	struct bt_le_adv_param adv_param;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	atomic_clear_bit(bt_dev.flags, BT_DEV_INITIATING);
	atomic_clear_bit(bt_dev.flags, BT_DEV_SCANNING);

	result = bt_id_adv_random_addr_check(&adv_param);

	zassert_true(result, "Incorrect result was returned");
}

/*
 *  Test that advertiser cannot start with random static identity or
 *  using an RPA generated for a different identity than scanner roles when privacy is enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'CONFIG_BT_OBSERVER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns false
 */
ZTEST(bt_id_adv_random_addr_check, test_check_returns_false_scanner_uses_random_identity)
{
	bool result;
	struct bt_le_adv_param adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);
	atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);

	adv_param.options |= BT_LE_ADV_OPT_USE_IDENTITY;
	bt_dev.id_addr[adv_param.id].type = BT_ADDR_LE_RANDOM;

	result = bt_id_adv_random_addr_check(&adv_param);

	zassert_false(result, "Incorrect result was returned");
}

/*
 *  Test that advertiser cannot start with random static identity or using an RPA generated for
 *  a different identity than scanner roles while 'CONFIG_BT_PRIVACY' isn't enabled and
 *  'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - 'CONFIG_BT_OBSERVER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns false
 */
ZTEST(bt_id_adv_random_addr_check, test_check_returns_false_advertise_with_local_identity)
{
	bool result;
	struct bt_le_adv_param adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SCAN_WITH_IDENTITY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);

	adv_param.options &= ~BT_LE_ADV_OPT_CONNECTABLE;
	adv_param.options |= BT_LE_ADV_OPT_USE_IDENTITY;
	bt_dev.id_addr[BT_ID_DEFAULT].type = BT_ADDR_LE_RANDOM;

	result = bt_id_adv_random_addr_check(&adv_param);

	zassert_false(result, "Incorrect result was returned");
}

/*
 *  Test that advertiser cannot start with random static identity or using an RPA generated for
 *  a different identity than scanner roles while 'CONFIG_BT_PRIVACY' isn't enabled and
 *  'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - 'CONFIG_BT_OBSERVER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns false
 */
ZTEST(bt_id_adv_random_addr_check, test_check_returns_false_advertise_with_different_identity)
{
	bool result;
	struct bt_le_adv_param adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SCAN_WITH_IDENTITY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);

	adv_param.id = 1;
	bt_dev.id_addr[adv_param.id].type = BT_ADDR_LE_RANDOM;
	bt_dev.id_addr[BT_ID_DEFAULT].type = BT_ADDR_LE_RANDOM;

	result = bt_id_adv_random_addr_check(&adv_param);

	zassert_false(result, "Incorrect result was returned");
}

/*
 *  Test checking advertising random address returns 'true' as a default value
 *
 *  Constraints:
 *   - 'CONFIG_BT_OBSERVER' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *   - 'BT_DEV_INITIATING' and 'BT_DEV_SCANNING' are set in bt_dev.flags
 *   - 'CONFIG_BT_SCAN_WITH_IDENTITY' isn't enabled
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_scan_random_addr_check() returns true
 */
ZTEST(bt_id_adv_random_addr_check, test_default_return_value)
{
	bool result;
	struct bt_le_adv_param adv_param;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_SCAN_WITH_IDENTITY);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);
	atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);

	result = bt_id_adv_random_addr_check(&adv_param);

	zassert_true(result, "Incorrect result was returned");
}
