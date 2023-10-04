/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/keys_help_utils.h"
#include "mocks/settings_store_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

DEFINE_FFF_GLOBALS;

/* This LUT contains different combinations of ID, Address and key type.
 * Item in this list will be used to fill keys pool.
 */
const struct id_addr_pair testing_id_addr_pair_lut[] = {

	{BT_ADDR_ID_1, BT_ADDR_LE_1},	  {BT_ADDR_ID_1, BT_RPA_ADDR_LE_1},
	{BT_ADDR_ID_1, BT_RPA_ADDR_LE_2}, {BT_ADDR_ID_1, BT_ADDR_LE_3},

	{BT_ADDR_ID_2, BT_ADDR_LE_1},	  {BT_ADDR_ID_2, BT_RPA_ADDR_LE_2},
	{BT_ADDR_ID_2, BT_RPA_ADDR_LE_3}, {BT_ADDR_ID_2, BT_ADDR_LE_2},

	{BT_ADDR_ID_3, BT_ADDR_LE_1},	  {BT_ADDR_ID_3, BT_ADDR_LE_2},

	{BT_ADDR_ID_4, BT_ADDR_LE_1}};

/* This list will hold returned references while filling keys pool */
struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

/* Holds the last key reference updated */
static struct bt_keys *last_keys_updated;

BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == ARRAY_SIZE(returned_keys_refs));

static void tc_setup(void *f)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	last_keys_updated = returned_keys_refs[CONFIG_BT_MAX_PAIRED - 1];
}

ZTEST_SUITE(bt_keys_update_usage_overwrite_oldest_enabled, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Request updating non-existing item in the keys pool list
 *
 *  Constraints:
 *   - Keys pool list is filled with items that are different from the testing ID and address pair
 *     used
 *
 *  Expected behaviour:
 *   - Last updated key reference isn't changed
 */
ZTEST(bt_keys_update_usage_overwrite_oldest_enabled, test_update_non_existing_key)
{
	uint8_t id = BT_ADDR_ID_5;
	bt_addr_le_t *addr = BT_ADDR_LE_5;

	bt_keys_update_usage(id, addr);

	zassert_equal_ptr(bt_keys_get_last_keys_updated(), last_keys_updated,
			  "bt_keys_update_usage() changed last updated key reference unexpectedly");
}

/*
 *  Request updating the latest key reference
 *
 *  Constraints:
 *   - Keys pool list is filled with items
 *   - ID and address pair used are the last added pair to the list
 *
 *  Expected behaviour:
 *   - Last updated key reference isn't changed
 */
ZTEST(bt_keys_update_usage_overwrite_oldest_enabled, test_update_latest_reference)
{
	uint8_t id = testing_id_addr_pair_lut[CONFIG_BT_MAX_PAIRED - 1].id;
	bt_addr_le_t *addr = testing_id_addr_pair_lut[CONFIG_BT_MAX_PAIRED - 1].addr;

	bt_keys_update_usage(id, addr);

	zassert_equal_ptr(bt_keys_get_last_keys_updated(), last_keys_updated,
			  "bt_keys_update_usage() changed last updated key reference unexpectedly");
}

/*
 *  Request updating existing items aging counter
 *
 *  Constraints:
 *   - Keys pool list is filled with items
 *   - ID and address used exist in the keys pool list
 *   - CONFIG_BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING isn't enabled
 *
 *  Expected behaviour:
 *   - Last updated key reference matches the last updated key reference
 */
ZTEST(bt_keys_update_usage_overwrite_oldest_enabled, test_update_non_latest_reference)
{
	uint8_t id;
	uint32_t old_aging_counter;
	bt_addr_le_t *addr;
	struct bt_keys *expected_updated_keys;
	struct id_addr_pair const *params_vector;

	if (IS_ENABLED(CONFIG_BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING)) {
		ztest_test_skip();
	}

	for (size_t it = 0; it < ARRAY_SIZE(testing_id_addr_pair_lut); it++) {
		params_vector = &testing_id_addr_pair_lut[it];
		id = params_vector->id;
		addr = params_vector->addr;
		expected_updated_keys = returned_keys_refs[it];
		old_aging_counter = expected_updated_keys->aging_counter;

		bt_keys_update_usage(id, addr);

		zassert_true(expected_updated_keys->aging_counter > (old_aging_counter),
			     "bt_keys_update_usage() set incorrect aging counter");

		zassert_equal_ptr(
			bt_keys_get_last_keys_updated(), expected_updated_keys,
			"bt_keys_update_usage() changed last updated key reference unexpectedly");

		expect_not_called_settings_save_one();
	}
}
