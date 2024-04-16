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

ZTEST_SUITE(bt_le_oob_get_local_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference for the OOB reference argument
 *
 *  Constraints:
 *   - NULL reference is used as an argument for the OOB reference
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_null_oob_reference)
{
	expect_assert();
	bt_le_oob_get_local(0x00, NULL);
}

/*
 *  Test trying to get the local LE Out of Band (OOB) information while the device ready flag
 *  'BT_DEV_READY' bit isn't set
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - 'BT_DEV_READY' bit isn't set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - '-EAGAIN' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_dev_ready_flag_not_set)
{
	int err;
	struct bt_le_oob oob = {0};

	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_le_oob_get_local(0x00, &oob);

	zassert_true(err == -EAGAIN, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test trying to get the local LE Out of Band (OOB) information if the ID used is out of range.
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is out of range or exceeds the maximum value defined by 'CONFIG_BT_ID_MAX'
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_out_of_range_id_value)
{
	int err;
	struct bt_le_oob oob = {0};

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_le_oob_get_local(CONFIG_BT_ID_MAX, &oob);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test RPA can't be updated while a connection is being established
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_CENTRAL' bit is enabled
 *   - 'BT_DEV_INITIATING' bit is set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_updating_rpa_fails_while_establishing_connection)
{
	int err;
	struct bt_conn conn = {0};
	struct bt_le_oob oob = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CENTRAL);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);

	bt_conn_lookup_state_le_fake.return_val = &conn;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_CONNECTING_SCAN);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Verify that bt_conn_lookup_state_le() is used to lookup if a connection is being
 *  established if:
 *  - Advertise parameters reference is NULL
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_BROADCASTER' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_CENTRAL' bit is enabled
 *   - 'BT_DEV_INITIATING' bit is set in bt_dev.flags
 *   - bt_le_adv_lookup_legacy() returns NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_conn_state_checked_with_null_adv)
{
	int err;
	struct bt_conn conn = {0};
	struct bt_le_oob oob = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CENTRAL);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);

	/* This will set the advertise parameters reference to NULL */
	bt_le_adv_lookup_legacy_fake.return_val = NULL;
	bt_conn_lookup_state_le_fake.return_val = &conn;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_le_adv_lookup_legacy();
	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_CONNECTING_SCAN);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Verify that bt_conn_lookup_state_le() is used to lookup if a connection is being
 *  established if:
 *  - Advertise parameters reference isn't NULL
 *  - Advertise parameters reference ID doesn't match the one passed to bt_le_oob_get_local()
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_BROADCASTER' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_CENTRAL' bit is enabled
 *   - 'BT_DEV_INITIATING' bit is set in bt_dev.flags
 *   - bt_le_adv_lookup_legacy() returns a valid advertise parameters reference
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_conn_state_checked_non_matched_id)
{
	int err;
	struct bt_conn conn = {0};
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CENTRAL);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);

	adv.id = 1;
	atomic_set_bit(adv.flags, BT_ADV_ENABLED);
	atomic_set_bit(adv.flags, BT_ADV_USE_IDENTITY);
	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	bt_le_adv_lookup_legacy_fake.return_val = &adv;
	bt_conn_lookup_state_le_fake.return_val = &conn;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_le_adv_lookup_legacy();
	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_CONNECTING_SCAN);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Verify that bt_conn_lookup_state_le() is used to lookup if a connection is being
 *  established if:
 *  - Advertise parameters reference isn't NULL
 *  - Advertise parameters reference ID matches the one passed to bt_le_oob_get_local()
 *  - 'BT_ADV_ENABLED' flags isn't set in advertise parameters reference
 *  - 'BT_ADV_USE_IDENTITY' flags is set in advertise parameters reference
 *  - Address type loaded to bt_dev.id_addr[BT_ID_DEFAULT] is random
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_BROADCASTER' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_CENTRAL' bit is enabled
 *   - 'BT_DEV_INITIATING' bit is set in bt_dev.flags
 *   - bt_le_adv_lookup_legacy() returns a valid advertise parameters reference
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_conn_state_checked_adv_enable_not_set)
{
	int err;
	struct bt_conn conn = {0};
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CENTRAL);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);

	adv.id = BT_ID_DEFAULT;
	atomic_clear_bit(adv.flags, BT_ADV_ENABLED);
	atomic_set_bit(adv.flags, BT_ADV_USE_IDENTITY);
	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	bt_le_adv_lookup_legacy_fake.return_val = &adv;
	bt_conn_lookup_state_le_fake.return_val = &conn;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_le_adv_lookup_legacy();
	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_CONNECTING_SCAN);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Verify that bt_conn_lookup_state_le() is used to lookup if a connection is being
 *  established if:
 *  - Advertise parameters reference isn't NULL
 *  - Advertise parameters reference ID matches the one passed to bt_le_oob_get_local()
 *  - 'BT_ADV_ENABLED' flags is set in advertise parameters reference
 *  - 'BT_ADV_USE_IDENTITY' flags isn't set in advertise parameters reference
 *  - Address type loaded to bt_dev.id_addr[BT_ID_DEFAULT] is random
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_BROADCASTER' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_CENTRAL' bit is enabled
 *   - 'BT_DEV_INITIATING' bit is set in bt_dev.flags
 *   - bt_le_adv_lookup_legacy() returns a valid advertise parameters reference
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_conn_state_checked_adv_use_identity_not_set)
{
	int err;
	struct bt_conn conn = {0};
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CENTRAL);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);

	adv.id = BT_ID_DEFAULT;
	atomic_set_bit(adv.flags, BT_ADV_ENABLED);
	atomic_clear_bit(adv.flags, BT_ADV_USE_IDENTITY);
	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	bt_le_adv_lookup_legacy_fake.return_val = &adv;
	bt_conn_lookup_state_le_fake.return_val = &conn;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_le_adv_lookup_legacy();
	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_CONNECTING_SCAN);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Verify that bt_conn_lookup_state_le() is used to lookup if a connection is being
 *  established if:
 *  - Advertise parameters reference isn't NULL
 *  - Advertise parameters reference ID matches the one passed to bt_le_oob_get_local()
 *  - 'BT_ADV_ENABLED' flags is set in advertise parameters reference
 *  - 'BT_ADV_USE_IDENTITY' flags is set in advertise parameters reference
 *  - Address type loaded to bt_dev.id_addr[BT_ID_DEFAULT] isn't random
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_BROADCASTER' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_CENTRAL' bit is enabled
 *   - 'BT_DEV_INITIATING' bit is set in bt_dev.flags
 *   - bt_le_adv_lookup_legacy() returns a valid advertise parameters reference
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_conn_state_checked_public_dev_address)
{
	int err;
	struct bt_conn conn = {0};
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CENTRAL);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	atomic_set_bit(bt_dev.flags, BT_DEV_INITIATING);

	adv.id = BT_ID_DEFAULT;
	atomic_set_bit(adv.flags, BT_ADV_ENABLED);
	atomic_set_bit(adv.flags, BT_ADV_USE_IDENTITY);
	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_LE_ADDR);

	bt_le_adv_lookup_legacy_fake.return_val = &adv;
	bt_conn_lookup_state_le_fake.return_val = &conn;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_le_adv_lookup_legacy();
	expect_single_call_bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_CONNECTING_SCAN);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test RPA can't be updated while advertising with random static identity address for a different
 *  identity.
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid but different from the one used in advertising
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_BROADCASTER' bit is enabled
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_updating_rpa_fails_while_advertising_random_identity)
{
	int err;
	struct bt_le_oob oob = {0};
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	adv.id = 1;
	atomic_set_bit(adv.flags, BT_ADV_ENABLED);
	atomic_set_bit(adv.flags, BT_ADV_USE_IDENTITY);
	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	bt_le_adv_lookup_legacy_fake.return_val = &adv;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_le_adv_lookup_legacy();

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test RPA can't be updated if observer role is enabled and the device is scanning or initiating
 *  a connection
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the OOB information reference
 *   - ID used is valid but different from the one used in advertising
 *   - 'BT_DEV_READY' bit is set in bt_dev.flags
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - 'CONFIG_BT_OBSERVER' bit is enabled
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_updating_rpa_fails_if_observer_scanning_connecting)
{
	int err;
	struct bt_le_oob oob = {0};
	uint32_t testing_flags[] = {BT_DEV_SCANNING, BT_DEV_INITIATING};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_OBSERVER);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	for (size_t i = 0; i < ARRAY_SIZE(testing_flags); i++) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_SCANNING);
		atomic_clear_bit(bt_dev.flags, BT_DEV_INITIATING);

		atomic_set_bit(bt_dev.flags, testing_flags[i]);

		err = bt_le_oob_get_local(1, &oob);

		zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
	}
}

/*
 *  Get LE local Out Of Band information returns an error if bt_smp_le_oob_generate_sc_data()
 *  failed while privacy isn't enabled
 *
 *  Constraints:
 *   - Use a valid reference
 *   - 'CONFIG_BT_SMP' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit isn't enabled
 *   - bt_smp_le_oob_generate_sc_data() returns a negative error code other than (-ENOTSUP)
 *
 *  Expected behaviour:
 *   - bt_le_oob_get_local() returns a negative error code (failure)
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_get_local_out_of_band_information_no_privacy)
{
	int err;
	struct bt_le_oob oob;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SMP);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	bt_smp_le_oob_generate_sc_data_fake.return_val = -1;

	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

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
 *   - Use a valid reference
 *   - 'CONFIG_BT_SMP' bit is enabled
 *   - 'CONFIG_BT_PRIVACY' bit is enabled
 *   - bt_smp_le_oob_generate_sc_data() returns a negative error code other than (-ENOTSUP)
 *
 *  Expected behaviour:
 *   - bt_le_oob_get_local() returns a negative error code (failure)
 */
ZTEST(bt_le_oob_get_local_invalid_inputs, test_get_local_out_of_band_information_privacy_enabled)
{
	int err;
	struct bt_le_oob oob;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SMP);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	bt_smp_le_oob_generate_sc_data_fake.return_val = -1;

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	bt_addr_le_copy(&bt_dev.random_addr, BT_RPA_LE_ADDR);

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	expect_single_call_bt_smp_le_oob_generate_sc_data(&oob.le_sc_data);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&oob.addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}
