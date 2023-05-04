/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/hci_core.h"
#include "mocks/net_buf.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_id_set_adv_random_addr_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test setting advertise random address while passing a NULL value as a reference to
 *  the advertise parameters.
 *
 *  Constraints:
 *   - A NULL value is passed to the function as a reference
 *   - A valid address pointer is used
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_adv_random_addr_invalid_cases, test_null_adv_params_reference)
{
	expect_assert();
	bt_id_set_adv_random_addr(NULL, &BT_RPA_LE_ADDR->a);
}

/*
 *  Test setting advertise random address while passing a NULL value as an address reference
 *
 *  Constraints:
 *   - A valid value is passed to the function as a reference for advertise parameters
 *   - A NULL address pointer is used
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_adv_random_addr_invalid_cases, test_null_address_reference)
{
	struct bt_le_ext_adv adv_param = {0};

	expect_assert();
	bt_id_set_adv_random_addr(&adv_param, NULL);
}

/*
 *  Test setting advertise random address while passing a NULL value for the advertise parameters
 *  and the address
 *
 *  Constraints:
 *   - A NULL value is passed to the function as a reference
 *   - A NULL address pointer is used
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_adv_random_addr_invalid_cases, test_null_arguments)
{
	expect_assert();
	bt_id_set_adv_random_addr(NULL, NULL);
}

/*
 *  Test setting advertising random address while 'CONFIG_BT_EXT_ADV' is enabled
 *  and 'BT_ADV_PARAMS_SET' flag in advertising parameters reference is set.
 *  bt_hci_cmd_create() fails to allocate buffers and returns NULL.
 *
 *  Constraints:
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *   - 'BT_ADV_PARAMS_SET' flag in advertising parameters reference is set
 *   - bt_hci_cmd_create() returns null
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_random_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_random_addr_invalid_cases, test_bt_hci_cmd_create_returns_null)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(adv_param.flags, BT_ADV_PARAMS_SET);

	bt_hci_cmd_create_fake.return_val = NULL;

	err = bt_id_set_adv_random_addr(&adv_param, &BT_RPA_LE_ADDR->a);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting advertising random address while 'CONFIG_BT_EXT_ADV' is enabled
 *  and 'BT_ADV_PARAMS_SET' flag in advertising parameters reference is set.
 *  bt_hci_cmd_send_sync() fails and returns a negative error code.
 *
 *  Constraints:
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *   - 'BT_ADV_PARAMS_SET' flag in advertising parameters reference is set
 *   - bt_hci_cmd_create() returns a valid buffer pointer
 *   - bt_hci_cmd_send_sync() fails and returns a negative error code.
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_random_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_random_addr_invalid_cases, test_bt_hci_cmd_send_sync_fails)
{
	int err;
	struct net_buf net_buff;
	struct bt_hci_cp_le_set_adv_set_random_addr cp;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(adv_param.flags, BT_ADV_PARAMS_SET);

	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = -1;

	err = bt_id_set_adv_random_addr(&adv_param, &BT_RPA_LE_ADDR->a);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}
