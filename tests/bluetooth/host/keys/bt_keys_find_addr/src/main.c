/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/keys_help_utils.h"
#include "mocks/rpa.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

DEFINE_FFF_GLOBALS;

/* This LUT contains different combinations of ID, Address and key type.
 * Item in this list will be used to fill keys pool.
 */
static const struct id_addr_pair testing_id_addr_pair_lut[] = {

	{BT_ADDR_ID_1, BT_ADDR_LE_1},	  {BT_ADDR_ID_1, BT_RPA_ADDR_LE_1},
	{BT_ADDR_ID_1, BT_RPA_ADDR_LE_2}, {BT_ADDR_ID_1, BT_ADDR_LE_3},

	{BT_ADDR_ID_2, BT_ADDR_LE_1},	  {BT_ADDR_ID_2, BT_RPA_ADDR_LE_2},
	{BT_ADDR_ID_2, BT_RPA_ADDR_LE_3}, {BT_ADDR_ID_2, BT_ADDR_LE_2},

	{BT_ADDR_ID_3, BT_ADDR_LE_1},	  {BT_ADDR_ID_3, BT_ADDR_LE_2},

	{BT_ADDR_ID_4, BT_ADDR_LE_1}};

/* This list will hold returned references while filling keys pool */
static struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == ARRAY_SIZE(returned_keys_refs));

static void *empty_list_ts_setup(void)
{
	clear_key_pool();

	return NULL;
}

ZTEST_SUITE(bt_keys_find_addr_initially_empty_list, NULL, empty_list_ts_setup, NULL, NULL, NULL);

/*
 *  Find a non-existing key reference for ID and Address pair
 *
 *  Constraints:
 *   - Empty keys pool list
 *
 *  Expected behaviour:
 *   - A NULL value is returned
 */
ZTEST(bt_keys_find_addr_initially_empty_list, test_find_non_existing_key)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {
		struct bt_keys *returned_ref;
		struct id_addr_pair const *params_vector = &testing_id_addr_pair_lut[i];
		uint8_t id = params_vector->id;
		const bt_addr_le_t *addr = params_vector->addr;

		returned_ref = bt_keys_find_addr(id, addr);

		zassert_true(returned_ref == NULL, "bt_keys_find() returned a non-NULL reference");
	}
}

static void *filled_list_ts_setup(void)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	return NULL;
}

ZTEST_SUITE(bt_keys_find_addr_initially_filled_list, NULL, filled_list_ts_setup, NULL, NULL, NULL);

/*
 *  Find an existing key reference by ID and Address
 *
 *  Constraints:
 *   - ID and address pair does exist in keys pool
 *
 *  Expected behaviour:
 *   - A valid reference value is returned
 */
ZTEST(bt_keys_find_addr_initially_filled_list, test_find_existing_key_by_id_and_address)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {
		struct bt_keys *returned_ref, *expected_key_ref;
		struct id_addr_pair const *params_vector = &testing_id_addr_pair_lut[i];
		uint8_t id = params_vector->id;
		const bt_addr_le_t *addr = params_vector->addr;

		expected_key_ref = returned_keys_refs[i];

		returned_ref = bt_keys_find_addr(id, addr);

		zassert_true(returned_ref != NULL, "bt_keys_find_addr() returned a NULL reference");
		zassert_equal_ptr(returned_ref, expected_key_ref,
				  "bt_keys_find_addr() returned unexpected reference");
	}
}
