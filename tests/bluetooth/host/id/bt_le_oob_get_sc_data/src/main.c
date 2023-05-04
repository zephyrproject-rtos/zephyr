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

ZTEST_SUITE(bt_le_oob_get_sc_data, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test getting OOB information and verify that input arguments are passed correctly to
 *  bt_smp_le_oob_get_sc_data().
 *
 *  Constraints:
 *   - Valid references are used for input parameters
 *   - 'BT_DEV_READY' bit isn't set in bt_dev.flags
 *   - bt_smp_le_oob_get_sc_data() returns 0 (success)
 *
 *  Expected behaviour:
 *   - bt_le_oob_get_sc_data() returns 0 (success)
 */
ZTEST(bt_le_oob_get_sc_data, test_passing_arguments_correctly)
{
	int err;
	struct bt_conn conn = {0};
	const struct bt_le_oob_sc_data *oobd_local = {0};
	const struct bt_le_oob_sc_data *oobd_remote = {0};

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	bt_smp_le_oob_get_sc_data_fake.return_val = 0;

	err = bt_le_oob_get_sc_data(&conn, &oobd_local, &oobd_remote);

	expect_single_call_bt_smp_le_oob_get_sc_data(&conn, &oobd_local, &oobd_remote);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test getting OOB information and verify it fails when bt_smp_le_oob_get_sc_data() fails.
 *
 *  Constraints:
 *   - Valid references are used for input parameters
 *   - 'BT_DEV_READY' bit isn't set in bt_dev.flags
 *   - bt_smp_le_oob_get_sc_data() returns '-EINVAL' (failure)
 *
 *  Expected behaviour:
 *   - bt_le_oob_get_sc_data() returns '-EINVAL' (failure)
 */
ZTEST(bt_le_oob_get_sc_data, test_bt_smp_le_oob_get_sc_data_fails)
{
	int err;
	struct bt_conn conn = {0};
	const struct bt_le_oob_sc_data *oobd_local = {0};
	const struct bt_le_oob_sc_data *oobd_remote = {0};

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	bt_smp_le_oob_get_sc_data_fake.return_val = -EINVAL;

	err = bt_le_oob_get_sc_data(&conn, &oobd_local, &oobd_remote);

	expect_single_call_bt_smp_le_oob_get_sc_data(&conn, &oobd_local, &oobd_remote);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}
