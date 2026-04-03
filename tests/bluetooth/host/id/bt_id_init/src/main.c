/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto.h"
#include "mocks/hci_core.h"
#include "mocks/kernel.h"
#include "mocks/kernel_expects.h"
#include "mocks/smp.h"
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

	KERNEL_FFF_FAKES_LIST(RESET_FAKE);
	SMP_FFF_FAKES_LIST(RESET_FAKE);
	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_init, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test initializing the device identity by calling bt_id_init() while the device identity count
 *  bt_dev.id_count isn't 0.
 *
 *  Constraints:
 *   - bt_dev.id_count is set to value greater than 0
 *
 *  Expected behaviour:
 *   - bt_id_init() returns 0 and identity count isn't changed
 */
ZTEST(bt_id_init, test_init_dev_identity_while_valid_identities_exist)
{
	int err;

	bt_dev.id_count = 1;

	err = bt_id_init();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(bt_dev.id_count == 1, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);

#if defined(CONFIG_BT_PRIVACY)
	expect_single_call_k_work_init_delayable(&bt_dev.rpa_update);
#endif
}

/*
 *  Test initializing the device identity by calling bt_id_init() while the device identity count
 *  bt_dev.id_count is set to 0 and 'CONFIG_BT_SETTINGS' is enabled.
 *
 *  Constraints:
 *   - bt_dev.id_count is set 0
 *   - 'CONFIG_BT_SETTINGS' is enabled
 *
 *  Expected behaviour:
 *   - bt_id_init() returns 0 and identity count isn't changed
 */
ZTEST(bt_id_init, test_init_dev_identity_while_bt_settings_enabled)
{
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SETTINGS);

	err = bt_id_init();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_true(bt_dev.id_count == 0, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);

#if defined(CONFIG_BT_PRIVACY)
	expect_single_call_k_work_init_delayable(&bt_dev.rpa_update);
#endif
}
