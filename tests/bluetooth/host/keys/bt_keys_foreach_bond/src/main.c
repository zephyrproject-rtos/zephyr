/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <host/keys.h>
#include <zephyr/fff.h>
#include "mocks/keys_help_utils.h"
#include "testing_common_defs.h"

DEFINE_FFF_GLOBALS;

/* This LUT contains different combinations of ID and Address pairs */
const struct id_addr_pair testing_id_addr_pair_lut[] = {
	{ BT_ADDR_ID_1, BT_ADDR_LE_1 },
	{ BT_ADDR_ID_1, BT_ADDR_LE_2 },
	{ BT_ADDR_ID_2, BT_ADDR_LE_1 },
	{ BT_ADDR_ID_2, BT_ADDR_LE_2 }
};

/* This list will hold returned references while filling keys pool */
static struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == ARRAY_SIZE(returned_keys_refs));

static void *type_not_set_ts_setup(void)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	return NULL;
}

ZTEST_SUITE(bt_keys_foreach_bond_keys_type_not_set, NULL, type_not_set_ts_setup, NULL, NULL, NULL);

/* Callback to be used when no calls are expected by bt_foreach_bond() */
static void bt_foreach_bond_unreachable_cb(const struct bt_bond_info *info, void *user_data)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test calling bt_foreach_bond() with a valid ID that exists
 *  in the keys pool but the keys type isn't set
 *
 *  Constraints:
 *   - Keys pool has been filled
 *   - Keys type isn't set
 *
 *  Expected behaviour:
 *   - Callback should never be called
 */
ZTEST(bt_keys_foreach_bond_keys_type_not_set, test_existing_id_type_is_not_set)
{
	struct id_addr_pair const *current_params_vector;
	uint8_t id;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {

		current_params_vector = &testing_id_addr_pair_lut[i];
		id = current_params_vector->id;

		bt_foreach_bond(id, bt_foreach_bond_unreachable_cb, NULL);
	}
}

static void *type_set_ts_setup(void)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	for (uint32_t i = 0; i < ARRAY_SIZE(returned_keys_refs); i++) {
		returned_keys_refs[i]->keys |= BT_KEYS_ALL;
	}

	return NULL;
}

ZTEST_SUITE(bt_keys_foreach_bond_keys_type_set, NULL, type_set_ts_setup, NULL, NULL, NULL);

/* Callback to be used when calls are expected by bt_foreach_bond() */
static void bt_foreach_bond_expected_cb(const struct bt_bond_info *info, void *user_data)
{
	uint32_t *call_counter = (uint32_t *)user_data;

	zassert_true(info != NULL, "Unexpected NULL reference pointer for parameter '%s'", "info");
	zassert_true(user_data != NULL, "Unexpected NULL reference pointer for parameter '%s'",
		     "user_data");

	(*call_counter)++;
}

/*
 *  Test calling bt_foreach_bond() with a valid ID that exists
 *  in the keys pool while the keys type is set
 *
 *  Constraints:
 *   - Keys pool has been filled
 *   - Keys type is set
 *
 *  Expected behaviour:
 *   - Callback should be called for each occurrence
 */
ZTEST(bt_keys_foreach_bond_keys_type_set, test_existing_id_type_is_set)
{
	uint32_t call_counter = 0;
	struct id_addr_pair const *current_params_vector;
	uint8_t id;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {

		current_params_vector = &testing_id_addr_pair_lut[i];
		id = current_params_vector->id;

		/* Each ID was registered in the list with 2 different addresses */
		bt_foreach_bond(id, bt_foreach_bond_expected_cb, (void *)&call_counter);
		zassert_true(call_counter == 2,
			     "Incorrect call counter for 'bt_foreach_bond_expected_cb()'");

		call_counter = 0;
	}
}
