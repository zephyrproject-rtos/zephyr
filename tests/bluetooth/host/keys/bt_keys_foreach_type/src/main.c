/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <host/keys.h>
#include <zephyr/fff.h>
#include "mocks/keys_help_utils.h"
#include "testing_common_defs.h"

DEFINE_FFF_GLOBALS;

/* This LUT contains different combinations of ID, Address with no key type */
static const struct id_addr_type testing_id_addr_type_no_type_lut[] = {
	{ BT_ADDR_ID_1, BT_ADDR_LE_1, 0x00 },
	{ BT_ADDR_ID_1, BT_ADDR_LE_2, 0x00 },
	{ BT_ADDR_ID_2, BT_ADDR_LE_1, 0x00 },
	{ BT_ADDR_ID_2, BT_ADDR_LE_2, 0x00 },
	{ BT_ADDR_ID_3, BT_ADDR_LE_1, 0x00 },
	{ BT_ADDR_ID_3, BT_ADDR_LE_2, 0x00 },
	{ BT_ADDR_ID_4, BT_ADDR_LE_1, 0x00 }
};

/* This LUT contains different combinations of ID, Address and key type */
const struct id_addr_type testing_id_addr_type_lut[] = {
	{ BT_ADDR_ID_1, BT_ADDR_LE_1, BT_KEYS_PERIPH_LTK },
	{ BT_ADDR_ID_1, BT_ADDR_LE_2, BT_KEYS_IRK },
	{ BT_ADDR_ID_2, BT_ADDR_LE_1, BT_KEYS_LTK },
	{ BT_ADDR_ID_2, BT_ADDR_LE_2, BT_KEYS_LOCAL_CSRK },
	{ BT_ADDR_ID_3, BT_ADDR_LE_1, BT_KEYS_REMOTE_CSRK },
	{ BT_ADDR_ID_3, BT_ADDR_LE_2, BT_KEYS_LTK_P256 },
	{ BT_ADDR_ID_4, BT_ADDR_LE_1, BT_KEYS_ALL }
};

/* This list will hold returned references while filling keys pool */
static struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_lut) == ARRAY_SIZE(returned_keys_refs));
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_no_type_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_no_type_lut) == ARRAY_SIZE(returned_keys_refs));

static void *type_not_set_ts_setup(void)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr_type(testing_id_addr_type_no_type_lut,
					       ARRAY_SIZE(testing_id_addr_type_no_type_lut),
					       returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	return NULL;
}

ZTEST_SUITE(bt_keys_foreach_type_keys_type_not_set, NULL, type_not_set_ts_setup, NULL, NULL, NULL);

/* Callback to be used when no calls are expected by bt_keys_foreach_type() */
static void bt_keys_foreach_type_unreachable_cb(struct bt_keys *keys, void *data)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test calling bt_keys_foreach_type() with a valid key type while the keys type isn't set
 *
 *  Constraints:
 *   - Keys pool has been filled
 *   - Keys type isn't set
 *
 *  Expected behaviour:
 *   - Callback should never be called
 */
ZTEST(bt_keys_foreach_type_keys_type_not_set, test_existing_id_type_is_not_set)
{
	struct id_addr_type const *current_params_vector;
	int type;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {

		current_params_vector = &testing_id_addr_type_lut[i];
		type = current_params_vector->type;

		bt_keys_foreach_type(type, bt_keys_foreach_type_unreachable_cb, NULL);
	}
}

static void *type_set_ts_setup(void)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr_type(testing_id_addr_type_lut,
					  ARRAY_SIZE(testing_id_addr_type_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	return NULL;
}

ZTEST_SUITE(bt_keys_foreach_type_keys_type_set, NULL, type_set_ts_setup, NULL, NULL, NULL);

/* Callback to be used when calls are expected by bt_keys_foreach_type() */
void bt_keys_foreach_type_expected_cb(struct bt_keys *keys, void *data)
{
	uint32_t *call_counter = (uint32_t *)data;

	zassert_true(keys != NULL, "Unexpected NULL reference pointer for parameter '%s'", "keys");
	zassert_true(data != NULL, "Unexpected NULL reference pointer for parameter '%s'", "data");

	(*call_counter)++;
}

/*
 *  Test calling bt_keys_foreach_type() with a valid key type while the keys type is set
 *
 *  Constraints:
 *   - Keys pool has been filled
 *   - Keys type is set
 *
 *  Expected behaviour:
 *   - Callback should be called for each occurrence
 */
ZTEST(bt_keys_foreach_type_keys_type_set, test_existing_id_type_is_set)
{
	uint32_t call_counter = 0;
	struct id_addr_type const *current_params_vector;
	int type;
	int expected_call_count;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {

		current_params_vector = &testing_id_addr_type_lut[i];
		type = current_params_vector->type;
		expected_call_count = (type != BT_KEYS_ALL) ? 2 : CONFIG_BT_MAX_PAIRED;

		/* Because the keys pool list contains a record that matches the argument 'type' and
		 * a record with the value 'BT_KEYS_ALL', Callback should be called twice for each
		 * type except when key type is BT_KEYS_ALL which will cause the callback to be
		 * called as many times as the list size
		 */
		bt_keys_foreach_type(type, bt_keys_foreach_type_expected_cb, (void *)&call_counter);
		zassert_true(call_counter == expected_call_count,
			     "Incorrect calls count for 'bt_keys_foreach_type_expected_cb()'");

		call_counter = 0;
	}
}
