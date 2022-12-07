/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include "host_mocks/assert.h"
#include "mocks/keys_help_utils.h"

/* This LUT contains different combinations of ID and Address pairs */
extern const struct id_addr_pair testing_id_addr_pair_lut[CONFIG_BT_MAX_PAIRED];

static void test_case_setup(void *f)
{
	clear_key_pool();
}

ZTEST_SUITE(bt_keys_foreach_bond_invalid_inputs, NULL, NULL, test_case_setup, NULL, NULL);

/*
 *  Test callback function is set to NULL
 *
 *  Constraints:
 *   - Any ID value can be used
 *   - Callback function pointer is set to NULL
 *
 *  Expected behaviour:
 *   - An assertion fails and execution stops
 */
ZTEST(bt_keys_foreach_bond_invalid_inputs, test_null_callback)
{
	expect_assert();
	bt_foreach_bond(0x00, NULL, NULL);
}

/* Callback to be used when no calls are expected by bt_foreach_bond() */
static void bt_foreach_bond_unreachable_cb(const struct bt_bond_info *info, void *user_data)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test if the callback will be called if the ID doesn't exist with a NULL value for the user data.
 *
 *  Constraints:
 *   - Empty keys pool list
 *   - ID doesn't exist in the list
 *   - NULL value is used for the user data
 *   - Valid callback is passed to bt_keys_foreach_bond()
 *
 *  Expected behaviour:
 *   - Callback should never be called
 */
ZTEST(bt_keys_foreach_bond_invalid_inputs, test_callback_non_existing_id_with_null_user_data)
{
	bt_foreach_bond(0x00, bt_foreach_bond_unreachable_cb, NULL);
}

/*
 *  Test if the callback will be called if the ID doesn't exist with a valid value for the user
 *  data.
 *
 *  Constraints:
 *   - Empty keys pool list
 *   - ID doesn't exist in the list
 *   - Valid value is used for the user data
 *   - Valid callback is passed to bt_keys_foreach_bond()
 *
 *  Expected behaviour:
 *   - Callback should never be called
 */
ZTEST(bt_keys_foreach_bond_invalid_inputs, test_callback_non_existing_id_with_valid_user_data)
{
	size_t user_data;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {
		uint8_t id = testing_id_addr_pair_lut[i].id;

		bt_foreach_bond(id, bt_foreach_bond_unreachable_cb, &user_data);
	}
}
