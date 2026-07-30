/*
 * Copyright (c) 2026 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/adv.h"
#include "mocks/crypto.h"
#include "mocks/crypto_expects.h"
#include "mocks/hci_core.h"
#include "mocks/keys.h"
#include "mocks/settings.h"
#include "mocks/settings_expects.h"
#include "testing_common_defs.h"

#include <string.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

static uint8_t testing_irk_value[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
					0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	(void)memset(&bt_dev, 0x00, sizeof(struct bt_dev));

	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
	KEYS_FFF_FAKES_LIST(RESET_FAKE);
	SETTINGS_FFF_FAKES_LIST(RESET_FAKE);
	ADV_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

static void bt_id_reset_irk_before(void *fixture)
{
	bt_dev.id_count = 1;
	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
}

ZTEST_SUITE(bt_id_reset_irk, NULL, NULL, bt_id_reset_irk_before, NULL, NULL);

static int bt_rand_custom_fake(void *buf, size_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len == sizeof(testing_irk_value));

	(void)memcpy(buf, testing_irk_value, len);

	return 0;
}

/*
 *  Test successfully resetting the IRK when CONFIG_BT_SETTINGS is not enabled.
 *  Settings should not be stored.
 *
 *  Constraints:
 *   - CONFIG_BT_PRIVACY is enabled
 *   - bt_dev.id_count is 1, id = BT_ID_DEFAULT (0)
 *   - 'BT_DEV_READY' flag IS set in bt_dev.flags
 *   - CONFIG_BT_SETTINGS is not enabled
 *   - bt_rand() returns zero (success)
 *
 *  Expected behaviour:
 *   - bt_dev.irk[0] is updated with the new random IRK
 *   - 'BT_DEV_RPA_VALID' flag is cleared
 *   - bt_settings_store_irk() is NOT called
 *   - Return value is 0
 */
static ZTEST(bt_id_reset_irk, test_success_irk_updated_no_settings_persist)
{
	int err;

	atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);
	bt_rand_fake.custom_fake = bt_rand_custom_fake;

	err = bt_id_reset_irk(BT_ID_DEFAULT);

	expect_single_call_bt_rand(&bt_dev.irk[BT_ID_DEFAULT], 16);
	expect_not_called_bt_settings_store_irk();

	zassert_equal(err, 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(bt_dev.irk[BT_ID_DEFAULT], testing_irk_value, 16,
			  "IRK was not updated correctly");
	zassert_false(atomic_test_bit(bt_dev.flags, BT_DEV_RPA_VALID),
		      "BT_DEV_RPA_VALID should have been cleared");
}

/*
 *  Test successfully resetting the IRK when BT_DEV_READY is set and
 *  CONFIG_BT_SETTINGS is enabled — settings must be persisted.
 *
 *  Constraints:
 *   - CONFIG_BT_PRIVACY and CONFIG_BT_SETTINGS are enabled
 *   - bt_dev.id_count is 1, id = BT_ID_DEFAULT (0)
 *   - 'BT_DEV_READY' flag IS set in bt_dev.flags
 *   - bt_rand() returns zero (success)
 *
 *  Expected behaviour:
 *   - bt_dev.irk[0] is updated with the new random IRK
 *   - bt_settings_store_irk() is called once
 *   - Return value is 0
 */
static ZTEST(bt_id_reset_irk, test_success_settings_stored_when_ready)
{
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SETTINGS);

	bt_rand_fake.custom_fake = bt_rand_custom_fake;

	err = bt_id_reset_irk(BT_ID_DEFAULT);

	expect_single_call_bt_rand(&bt_dev.irk[BT_ID_DEFAULT], 16);
	expect_single_call_bt_settings_store_irk();

	zassert_equal(err, 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(bt_dev.irk[BT_ID_DEFAULT], testing_irk_value, 16,
			  "IRK was not updated correctly");
}

/*
 *  Test that when bt_rand() fails the error is forwarded and the IRK
 *  and settings are left untouched.
 *
 *  Constraints:
 *   - CONFIG_BT_PRIVACY is enabled
 *   - bt_dev.id_count is 1, id = BT_ID_DEFAULT (0)
 *   - bt_rand() returns a non-zero error code (failure)
 *
 *  Expected behaviour:
 *   - bt_settings_store_irk() is NOT called
 *   - The negative error code from bt_rand() is returned
 */
static ZTEST(bt_id_reset_irk, test_bt_rand_failure_error_forwarded)
{
	int err;

	bt_rand_fake.return_val = -EIO;

	err = bt_id_reset_irk(BT_ID_DEFAULT);

	expect_not_called_bt_settings_store_irk();

	zassert_equal(err, -EIO, "Unexpected error code '%d' was returned", err);
}
