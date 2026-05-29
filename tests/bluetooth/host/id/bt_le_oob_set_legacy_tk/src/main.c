/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>
#include <mocks/conn.h>
#include <mocks/smp.h>
#include <mocks/smp_expects.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));

	SMP_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_le_oob_set_legacy_tk, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test setting OOB Temporary Key to be used for pairing and verify that input arguments are
 *  passed correctly to bt_smp_le_oob_set_tk().
 *
 *  Constraints:
 *   - Valid references are used for the connection and TK references
 *   - bt_smp_le_oob_set_tk() returns 0 (success)
 *
 *  Expected behaviour:
 *   - bt_le_oob_set_legacy_tk() returns 0 (success)
 */
ZTEST(bt_le_oob_set_legacy_tk, test_passing_arguments_correctly)
{
	int err;
	struct bt_conn conn = {0};
	const uint8_t tk[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	bt_smp_le_oob_set_tk_fake.return_val = 0;

	err = bt_le_oob_set_legacy_tk(&conn, tk);

	expect_single_call_bt_smp_le_oob_set_tk(&conn, tk);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting OOB Temporary Key to be used for pairing and verify it fails when
 *  bt_smp_le_oob_set_tk() fails.
 *
 *  Constraints:
 *   - Valid references are used for the connection and TK references
 *   - bt_smp_le_oob_set_tk() returns '-EINVAL' (failure)
 *
 *  Expected behaviour:
 *   - bt_le_oob_set_legacy_tk() returns '-EINVAL' (failure)
 */
ZTEST(bt_le_oob_set_legacy_tk, test_bt_smp_le_oob_set_tk_fails)
{
	int err;
	struct bt_conn conn = {0};
	const uint8_t tk[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	bt_smp_le_oob_set_tk_fake.return_val = -EINVAL;

	err = bt_le_oob_set_legacy_tk(&conn, tk);

	expect_single_call_bt_smp_le_oob_set_tk(&conn, tk);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}
