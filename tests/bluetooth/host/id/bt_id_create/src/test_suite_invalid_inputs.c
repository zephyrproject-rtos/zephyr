/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_id_create_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test invalid input arguments to bt_id_create() using NULLs for address and IRK parameters.
 *
 *  Constraints:
 *   - Input address is NULL
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_create_invalid_inputs, test_null_addr_null_irk)
{
	int err;

	err = bt_id_create(NULL, NULL);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_create() using NULL for the address parameter
 *  while the IRK parameter is a valid pointer.
 *
 *  Constraints:
 *   - Input address is NULL
 *   - Input IRK isn't NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_create_invalid_inputs, test_null_addr_valid_irk_no_privacy_enabled)
{
	int err;
	uint8_t valid_irk_ptr[16];

	err = bt_id_create(NULL, valid_irk_ptr);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_create() using NULLs for address and IRK parameters
 *  while the identity list is full.
 *
 *  Constraints:
 *   - Input address is NULL
 *   - Input IRK is NULL
 *   - Identity list is full
 *
 *  Expected behaviour:
 *   - '-ENOMEM' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_create_invalid_inputs, test_id_list_is_full)
{
	int err;

	bt_dev.id_count = ARRAY_SIZE(bt_dev.id_addr);

	err = bt_id_create(NULL, NULL);

	zassert_true(err == -ENOMEM, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_create() by using a valid address of type public and using
 *  NULL value for the IRK.
 *
 *  Constraints:
 *   - A valid address of type public is used
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_create_invalid_inputs, test_public_address)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_HCI_SET_PUBLIC_ADDR)) {
		ztest_test_skip();
	}

	err = bt_id_create(BT_LE_ADDR, NULL);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_create() by using a valid address of type RPA and using
 *  NULL value for the IRK.
 *
 *  Constraints:
 *   - An RPA address of type random is used
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_create_invalid_inputs, test_rpa_address)
{
	int err;

	err = bt_id_create(BT_RPA_LE_ADDR, NULL);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_create() by using an address that already exists
 *  in the identity list.
 *
 *  Constraints:
 *   - A valid random static address is used
 *   - Input address already exists in the identity list
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EALREADY' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_create_invalid_inputs, test_pa_address_exists_in_id_list)
{
	int err;

	bt_dev.id_count = 1;
	bt_addr_le_copy(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1);

	err = bt_id_create(BT_STATIC_RANDOM_LE_ADDR_1, NULL);

	zassert_true(err == -EALREADY, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_create() by using a valid static random address and
 *  a valid pointer to an IRK that's filled with zeros.
 *
 *  Constraints:
 *   - A static random address is used
 *   - Input IRK is filled with zeros
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_create_invalid_inputs, test_zero_irk_with_privacy)
{
	int err;
	uint8_t zero_irk[16] = {0};

	err = bt_id_create(BT_STATIC_RANDOM_LE_ADDR_1, zero_irk);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}
