/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/keys_help_utils.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

DEFINE_FFF_GLOBALS;

/* This LUT contains different combinations of ID, Address and key type.
 * Item in this list will be used to fill keys pool.
 */
static const struct id_addr_type testing_id_addr_type_lut[] = {
	{BT_ADDR_ID_1, BT_ADDR_LE_1, BT_KEYS_PERIPH_LTK},
	{BT_ADDR_ID_1, BT_ADDR_LE_2, BT_KEYS_IRK},
	{BT_ADDR_ID_2, BT_ADDR_LE_1, BT_KEYS_LTK},
	{BT_ADDR_ID_2, BT_ADDR_LE_2, BT_KEYS_LOCAL_CSRK},
	{BT_ADDR_ID_3, BT_ADDR_LE_1, BT_KEYS_REMOTE_CSRK},
	{BT_ADDR_ID_3, BT_ADDR_LE_2, BT_KEYS_LTK_P256},
	{BT_ADDR_ID_4, BT_ADDR_LE_1, BT_KEYS_ALL}};

/* This list will hold returned references while filling keys pool */
static struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_lut) == ARRAY_SIZE(returned_keys_refs));

static void *empty_list_ts_setup(void)
{
	clear_key_pool();

	return NULL;
}

ZTEST_SUITE(bt_keys_find_initially_empty_list, NULL, empty_list_ts_setup, NULL, NULL, NULL);

/*
 *  Test calling bt_keys_find() with non-existing item
 *
 *  Constraints:
 *   - Valid values of non-existing items are used
 *
 *  Expected behaviour:
 *   - NULL reference is returned
 */
ZTEST(bt_keys_find_initially_empty_list, test_find_non_existing_item)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {
		struct bt_keys *returned_ref;
		struct id_addr_type const *params_vector = &testing_id_addr_type_lut[i];
		int type = params_vector->type;
		uint8_t id = params_vector->id;
		const bt_addr_le_t *addr = params_vector->addr;

		returned_ref = bt_keys_find(type, id, addr);
		zassert_true(returned_ref == NULL, "bt_keys_find() returned a non-NULL reference");
	}
}

static void *filled_list_ts_setup(void)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr_type(
		testing_id_addr_type_lut, ARRAY_SIZE(testing_id_addr_type_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	return NULL;
}

ZTEST_SUITE(bt_keys_find_initially_filled_list, NULL, filled_list_ts_setup, NULL, NULL, NULL);

/*
 *  Test calling bt_keys_find() with existing items
 *
 *  Constraints:
 *   - Keys pool list is filled
 *   - Valid values of existing items are used
 *
 *  Expected behaviour:
 *   - Valid reference pointer is returned and matches the correct reference
 */
ZTEST(bt_keys_find_initially_filled_list, test_find_existing_item)
{
	int type;
	uint8_t id;
	const bt_addr_le_t *addr;
	struct bt_keys *returned_ref, *expected_key_ref;

	for (size_t it = 0; it < ARRAY_SIZE(testing_id_addr_type_lut); it++) {

		type = testing_id_addr_type_lut[it].type;
		id = testing_id_addr_type_lut[it].id;
		addr = testing_id_addr_type_lut[it].addr;

		expected_key_ref = returned_keys_refs[it];

		returned_ref = bt_keys_find(type, id, addr);

		zassert_true(returned_ref != NULL, "bt_keys_find() returned a NULL reference");
		zassert_equal_ptr(returned_ref, expected_key_ref,
				  "bt_keys_find() returned unexpected reference");
	}
}
