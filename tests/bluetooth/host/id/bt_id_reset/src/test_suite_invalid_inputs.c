/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/adv.h"
#include "mocks/adv_expects.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_id_reset_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test resetting default ID which shouldn't be allowed
 *
 *  Constraints:
 *   - BT_ID_DEFAULT value is used for the ID
 *   - Input address is NULL
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_resetting_default_id)
{
	int err;

	err = bt_id_reset(BT_ID_DEFAULT, NULL, NULL);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test resetting ID value that is equal to bt_dev.id_count
 *
 *  Constraints:
 *   - bt_dev.id_count is greater than 0
 *   - ID value used is equal to bt_dev.id_count
 *   - Input address is NULL
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_resetting_id_value_equal_to_dev_id_count)
{
	int err;

	bt_dev.id_count = 1;

	err = bt_id_reset(bt_dev.id_count, NULL, NULL);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test using a valid IRK pointer value while privacy isn't enabled
 *
 *  Constraints:
 *   - BT_ID_DEFAULT is used for the ID
 *   - Input address is NULL
 *   - Input IRK isn't NULL
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_null_addr_valid_irk_no_privacy_enabled)
{
	int err;
	uint8_t valid_irk_ptr[16];

	err = bt_id_reset(BT_ID_DEFAULT, NULL, valid_irk_ptr);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_reset() by using a valid address of type public and using
 *  NULL value for the IRK.
 *
 *  Constraints:
 *   - BT_ID_DEFAULT is used for the ID
 *   - A valid address of type public is used
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_public_address)
{
	int err;

	err = bt_id_reset(BT_ID_DEFAULT, BT_LE_ADDR, NULL);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_reset() by using a valid address of type RPA and using
 *  NULL value for the IRK.
 *
 *  Constraints:
 *   - BT_ID_DEFAULT is used for the ID
 *   - An RPA address of type random is used
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_rpa_address)
{
	int err;

	err = bt_id_reset(BT_ID_DEFAULT, BT_RPA_LE_ADDR, NULL);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test invalid input arguments to bt_id_reset() by using an address that already exists
 *  in the identity list.
 *
 *  Constraints:
 *   - BT_ID_DEFAULT is used for the ID
 *   - A valid random static address is used
 *   - Input address already exists in the identity list
 *   - Input IRK is NULL
 *
 *  Expected behaviour:
 *   - '-EALREADY' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_pa_address_exists_in_id_list)
{
	int err;

	bt_dev.id_count = 1;
	bt_addr_le_copy(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1);

	err = bt_id_reset(BT_ID_DEFAULT, BT_STATIC_RANDOM_LE_ADDR_1, NULL);

	zassert_true(err == -EALREADY, "Unexpected error code '%d' was returned", err);
}

static void bt_le_ext_adv_foreach_custom_fake(void (*func)(struct bt_le_ext_adv *adv, void *data),
					      void *data)
{
	struct bt_le_ext_adv adv_params = {0};

	__ASSERT_NO_MSG(func != NULL);
	__ASSERT_NO_MSG(data != NULL);

	if (IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		/* Only check if the ID is in use, as the advertiser can be
		 * started and stopped without reconfiguring parameters.
		 */
		adv_params.id = bt_dev.id_count - 1;
	} else {
		atomic_set_bit(adv_params.flags, BT_ADV_ENABLED);
		adv_params.id = bt_dev.id_count - 1;
	}

	func(&adv_params, data);
}

/*
 *  Test resetting an ID if the 'CONFIG_BT_BROADCASTER' is enabled and the same ID is already
 *  in use with the advertising data.
 *
 *  Constraints:
 *   - A valid random static address is used
 *   - Input address doesn't exist in the identity list
 *   - Input IRK is NULL
 *   - 'CONFIG_BT_BROADCASTER' is enabled
 *
 *  Expected behaviour:
 *   - '-EBUSY' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_resetting_id_used_in_advertising)
{
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_BROADCASTER);

	bt_dev.id_count = 2;

	/* When bt_le_ext_adv_foreach() is called, this callback will be triggered and causes
	 * adv_id_check_func() to set the advertising enable flag to true.
	 */
	bt_le_ext_adv_foreach_fake.custom_fake = bt_le_ext_adv_foreach_custom_fake;

	err = bt_id_reset(bt_dev.id_count - 1, BT_STATIC_RANDOM_LE_ADDR_1, NULL);

	expect_single_call_bt_le_ext_adv_foreach();

	zassert_true(err == -EBUSY, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test returning error when the ID used corresponds to an empty address and bt_unpair() fails
 *
 *  Constraints:
 *   - A valid random static address is used
 *   - Input address doesn't exist in the identity list
 *   - Input IRK is NULL
 *   - 'CONFIG_BT_CONN' is enabled
 *   - bt_unpair() fails and returns a negative error code
 *
 *  Expected behaviour:
 *   - '-EALREADY' error code is returned representing invalid values were used.
 */
ZTEST(bt_id_reset_invalid_inputs, test_bt_unpair_fails)
{
	int err;
	uint8_t id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_CONN);

	bt_dev.id_count = 2;
	id = bt_dev.id_count - 1;

	bt_addr_le_copy(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1);
	bt_addr_le_copy(&bt_dev.id_addr[1], BT_STATIC_RANDOM_LE_ADDR_1);

	bt_unpair_fake.return_val = -1;

	err = bt_id_reset(id, BT_STATIC_RANDOM_LE_ADDR_2, NULL);

	expect_single_call_bt_unpair(id, NULL);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}
