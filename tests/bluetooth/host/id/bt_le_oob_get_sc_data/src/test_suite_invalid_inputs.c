/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>
#include <mocks/conn.h>

ZTEST_SUITE(bt_le_oob_get_sc_data_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference for the connection reference argument
 *
 *  Constraints:
 *   - NULL reference is used for the connection reference
 *   - Valid references are used for other parameters
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_le_oob_get_sc_data_invalid_inputs, test_null_conn_reference)
{
	const struct bt_le_oob_sc_data *oobd_local = {0};
	const struct bt_le_oob_sc_data *oobd_remote = {0};

	expect_assert();
	bt_le_oob_get_sc_data(NULL, &oobd_local, &oobd_remote);
}

/*
 *  Test trying to set OOB data while the device ready flag 'BT_DEV_READY' bit isn't set
 *
 *  Constraints:
 *   - Valid references are used for input parameters
 *   - 'BT_DEV_READY' bit isn't set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - '-EAGAIN' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_sc_data_invalid_inputs, test_dev_ready_flag_not_set)
{
	int err;
	struct bt_conn conn = {0};
	const struct bt_le_oob_sc_data *oobd_local = {0};
	const struct bt_le_oob_sc_data *oobd_remote = {0};

	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_le_oob_get_sc_data(&conn, &oobd_local, &oobd_remote);

	zassert_true(err == -EAGAIN, "Unexpected error code '%d' was returned", err);
}
