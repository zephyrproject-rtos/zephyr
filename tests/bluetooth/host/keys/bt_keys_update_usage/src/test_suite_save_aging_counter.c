/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/keys_help_utils.h"
#include "mocks/settings_store.h"
#include "mocks/settings_store_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

/* This LUT contains different combinations of ID and Address pairs.
 * It is defined in main.c.
 */
extern const struct id_addr_pair testing_id_addr_pair_lut[CONFIG_BT_MAX_PAIRED];

/* This list holds returned references while filling keys pool.
 * It is defined in main.c.
 */
extern struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

static void tc_setup(void *f)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING);

	clear_key_pool();
	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);
}

ZTEST_SUITE(bt_keys_update_usage_save_aging_counter, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Request updating existing items aging counter
 *
 *  Constraints:
 *   - Keys pool list is filled with items
 *   - ID and address used exist in the keys pool list
 *   - CONFIG_BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING is enabled
 *
 *  Expected behaviour:
 *   - Last updated key reference matches the last updated key reference
 *   - bt_keys_store() is called once with the correct parameters
 */
ZTEST(bt_keys_update_usage_save_aging_counter, test_update_usage_and_save_aging_counter)
{
	uint8_t id;
	uint32_t old_aging_counter;
	bt_addr_le_t *addr;
	struct bt_keys *expected_updated_keys;
	struct id_addr_pair const *params_vector;

	for (size_t it = 0; it < ARRAY_SIZE(testing_id_addr_pair_lut); it++) {
		params_vector = &testing_id_addr_pair_lut[it];
		id = params_vector->id;
		addr = params_vector->addr;
		expected_updated_keys = returned_keys_refs[it];
		old_aging_counter = expected_updated_keys->aging_counter;

		/* Reset fake functions call counter */
		SETTINGS_STORE_FFF_FAKES_LIST(RESET_FAKE);

		bt_keys_update_usage(id, addr);

		zassert_true(expected_updated_keys->aging_counter > (old_aging_counter),
			     "bt_keys_update_usage() set incorrect aging counter");

		zassert_equal_ptr(
			bt_keys_get_last_keys_updated(), expected_updated_keys,
			"bt_keys_update_usage() changed last updated key reference unexpectedly");

		/* Check if bt_keys_store() was called */
		expect_single_call_settings_save_one(expected_updated_keys->storage_start);
	}
}
