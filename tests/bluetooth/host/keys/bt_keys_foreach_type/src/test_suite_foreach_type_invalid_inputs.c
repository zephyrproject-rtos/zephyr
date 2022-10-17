/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <host/keys.h>
#include "host_mocks/assert.h"
#include "mocks/keys_help_utils.h"

/* This LUT contains different combinations of ID, Address and key type.
 * It is defined in main.c.
 */
extern const struct id_addr_type testing_id_addr_type_lut[CONFIG_BT_MAX_PAIRED];

static void test_case_setup(void *f)
{
	clear_key_pool();
}

ZTEST_SUITE(bt_keys_foreach_type_invalid_inputs, NULL, NULL, test_case_setup, NULL, NULL);

/*
 *  Test callback function is set to NULL
 *
 *  Constraints:
 *   - Any key type can be used
 *   - Callback function pointer is set to NULL
 *
 *  Expected behaviour:
 *   - An assertion fails and execution stops
 */
ZTEST(bt_keys_foreach_type_invalid_inputs, test_null_callback)
{
	expect_assert();
	bt_keys_foreach_type(0x00, NULL, NULL);
}

/* Callback to be used when no calls are expected by bt_keys_foreach_type() */
static void bt_keys_foreach_type_unreachable_cb(struct bt_keys *keys, void *data)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test empty keys pool list with no key type set and a NULL value for the user data.
 *
 *  Constraints:
 *   - Empty keys pool list
 *   - Valid value is used for the key type
 *   - NULL value is used for the user data
 *   - Valid callback is passed to bt_keys_foreach_type()
 *
 *  Expected behaviour:
 *   - Callback should never be called
 */
ZTEST(bt_keys_foreach_type_invalid_inputs, test_empty_list_no_type_set_with_null_user_data)
{
	bt_keys_foreach_type(0x00, bt_keys_foreach_type_unreachable_cb, NULL);
}

/*
 *  Test empty keys pool list with no key type set and a valid value for the user data.
 *
 *  Constraints:
 *   - Empty keys pool list
 *   - Valid value is used for the key type
 *   - Valid value is used for the user data
 *   - Valid callback is passed to bt_keys_foreach_type()
 *
 *  Expected behaviour:
 *   - Callback should never be called
 */
ZTEST(bt_keys_foreach_type_invalid_inputs, test_empty_list_no_type_set_with_valid_user_data)
{
	size_t user_data;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {
		int type = testing_id_addr_type_lut[i].type;

		bt_keys_foreach_type(type, bt_keys_foreach_type_unreachable_cb, &user_data);
	}
}
