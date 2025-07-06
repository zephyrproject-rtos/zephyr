/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>
#include <mocks/adv.h>
#include <mocks/adv_expects.h>
#include <mocks/conn.h>
#include <mocks/conn_expects.h>
#include <mocks/smp.h>
#include <mocks/smp_expects.h>

ZTEST_SUITE(bt_le_ext_adv_oob_get_local_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference for the OOB reference argument
 *
 *  Constraints:
 *   - NULL reference is used for the advertise parameters reference
 *   - Valid reference is used for OOB reference
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_le_ext_adv_oob_get_local_invalid_inputs, test_null_adv_reference)
{
	struct bt_le_oob oob = {0};

	expect_assert();
	bt_le_ext_adv_oob_get_local(NULL, &oob);
}

/*
 *  Test passing NULL reference for the OOB reference argument
 *
 *  Constraints:
 *   - Valid reference is used for the advertise parameters reference
 *   - NULL reference is used as an argument for the OOB reference
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_le_ext_adv_oob_get_local_invalid_inputs, test_null_oob_reference)
{
	struct bt_le_ext_adv adv = {0};

	expect_assert();
	bt_le_ext_adv_oob_get_local(&adv, NULL);
}

/*
 *  Test trying to get the local LE Out of Band (OOB) information while the device ready flag
 *  'BT_DEV_READY' bit isn't set
 *
 *  Constraints:
 *   - Valid references are used for the advertise parameters and the OOB references
 *   - 'BT_DEV_READY' bit isn't set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - '-EAGAIN' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_ext_adv_oob_get_local_invalid_inputs, test_dev_ready_flag_not_set)
{
	int err;
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_le_ext_adv_oob_get_local(&adv, &oob);

	zassert_true(err == -EAGAIN, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test RPA can't be updated while a connection is being established
 *
 *  Constraints:
 *   - Valid references are used for the advertise parameters and the OOB references
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_CENTRAL' bit is enabled
 *   - 'BT_DEV_INITIATING' bit is set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_ext_adv_oob_get_local_invalid_inputs, test_updating_rpa_fails_while_connecting)
{
	int err;
	struct bt_conn conn = {0};
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CENTRAL);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);

	atomic_clear_bit(adv.flags, BT_ADV_LIMITED);
	atomic_clear_bit(adv.flags, BT_ADV_USE_IDENTITY);

	bt_conn_lookup_state_le_fake.return_val = &conn;

	err = bt_le_ext_adv_oob_get_local(&adv, &oob);

	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL,
						   BT_CONN_SCAN_BEFORE_INITIATING);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Get LE local Out Of Band information returns an error if bt_smp_le_oob_generate_sc_data()
 *  failed while privacy isn't enabled
 *
 *  Constraints:
 *   - Valid references are used for the advertise parameters and the OOB references
 *   - 'CONFIG_BT_SMP' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit isn't enabled
 *   - bt_smp_le_oob_generate_sc_data() returns a negative error code other than (-ENOTSUP)
 *
 *  Expected behaviour:
 *   - bt_le_ext_adv_oob_get_local() returns a negative error code (failure)
 */
ZTEST(bt_le_ext_adv_oob_get_local_invalid_inputs, test_get_local_oob_information_no_privacy)
{
	int err;
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SMP);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	bt_smp_le_oob_generate_sc_data_fake.return_val = -1;

	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	err = bt_le_ext_adv_oob_get_local(&adv, &oob);

	expect_single_call_bt_smp_le_oob_generate_sc_data(&oob.le_sc_data);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&oob.addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}

/*
 *  Get LE local Out Of Band information returns an error if bt_smp_le_oob_generate_sc_data()
 *  failed while privacy is enabled
 *
 *  Constraints:
 *   - Valid references are used for the advertise parameters and the OOB references
 *   - 'CONFIG_BT_SMP' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - bt_smp_le_oob_generate_sc_data() returns a negative error code other than (-ENOTSUP)
 *
 *  Expected behaviour:
 *   - bt_le_ext_adv_oob_get_local() returns a negative error code (failure)
 */
ZTEST(bt_le_ext_adv_oob_get_local_invalid_inputs, test_get_local_oob_information_privacy_enabled)
{
	int err;
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SMP);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	bt_smp_le_oob_generate_sc_data_fake.return_val = -1;

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_clear_bit(adv.flags, BT_ADV_USE_IDENTITY);
	bt_addr_le_copy(&adv.random_addr, BT_RPA_LE_ADDR);

	err = bt_le_ext_adv_oob_get_local(&adv, &oob);

	expect_single_call_bt_smp_le_oob_generate_sc_data(&oob.le_sc_data);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&oob.addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}
