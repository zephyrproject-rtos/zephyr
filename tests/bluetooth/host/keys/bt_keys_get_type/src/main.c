/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/bt_str.h"

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

ZTEST_SUITE(bt_keys_get_type_initially_empty_list, NULL, empty_list_ts_setup, NULL, NULL, NULL);

static void *full_list_ts_setup(void)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr_type(
		testing_id_addr_type_lut, ARRAY_SIZE(testing_id_addr_type_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	return NULL;
}

ZTEST_SUITE(bt_keys_get_type_initially_filled_list, NULL, full_list_ts_setup, NULL, NULL, NULL);

/*
 *  Test getting a non-existing key reference with type, ID and Address while the list isn't full.
 *
 *  Constraints:
 *   - Keys pool list isn't full
 *   - ID and address pair used doesn't exist in the keys pool list
 *
 *  Expected behaviour:
 *   - A key slot is reserved and data type, ID and Address are stored
 *   - A valid reference is returned by bt_keys_get_type()
 *   - ID value matches the one passed to bt_keys_get_type()
 *   - Address value matches the one passed to bt_keys_get_type()
 *   - Key type value matches the one passed to bt_keys_get_type()
 */
ZTEST(bt_keys_get_type_initially_empty_list, test_get_non_existing_key_reference)
{
	struct bt_keys *returned_key;
	int type;
	uint8_t id;
	bt_addr_le_t *addr;
	struct id_addr_type const *params_vector;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {

		params_vector = &testing_id_addr_type_lut[i];
		type = params_vector->type;
		id = params_vector->id;
		addr = params_vector->addr;

		returned_key = bt_keys_get_type(type, id, addr);
		returned_keys_refs[i] = returned_key;

		zassert_true(returned_key != NULL,
			     "bt_keys_get_type() failed to add key %d to the keys pool", i);
		zassert_true(returned_key->id == id,
			     "bt_keys_get_type() returned a reference with an incorrect ID");
		zassert_true(returned_key->keys == type,
			     "bt_keys_get_type() returned a reference with an incorrect key type");
		zassert_true(!bt_addr_le_cmp(&returned_key->addr, addr),
			     "bt_keys_get_type() returned incorrect address %s value, expected %s",
			     bt_addr_le_str(&returned_key->addr), bt_addr_le_str(addr));
	}
}

/*
 *  Test getting a non-existing key reference with type, ID and Address while the list is full.
 *
 *  Constraints:
 *   - Keys pool list is filled with items different from the ones used for testing
 *
 *  Expected behaviour:
 *   - A NULL value is returned by bt_keys_get_type()
 */
ZTEST(bt_keys_get_type_initially_filled_list, test_get_non_existing_key_reference_full_list)
{
	struct bt_keys *returned_key;
	int type = BT_KEYS_IRK;
	uint8_t id = BT_ADDR_ID_5;
	const bt_addr_le_t *addr = BT_ADDR_LE_5;

	returned_key = bt_keys_get_type(type, id, addr);

	zassert_true(returned_key == NULL, "bt_keys_get_type() returned a non-NULL reference");
}

/*
 *  Test getting an existing key reference with type, ID and Address while the list is full.
 *
 *  Constraints:
 *   - Keys pool list is filled with the ID and address pairs used
 *
 *  Expected behaviour:
 *   - A valid reference is returned by bt_keys_get_type()
 *   - Key reference returned matches the previously returned one
 *     when it was firstly inserted in the list
 */
ZTEST(bt_keys_get_type_initially_filled_list, test_get_existing_key_reference)
{
	struct bt_keys *returned_key, *expected_key_ref;
	int type;
	uint8_t id;
	bt_addr_le_t *addr;
	struct id_addr_type const *params_vector;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {

		params_vector = &testing_id_addr_type_lut[i];
		type = params_vector->type;
		id = params_vector->id;
		addr = params_vector->addr;

		returned_key = bt_keys_get_type(type, id, addr);
		expected_key_ref = returned_keys_refs[i];

		zassert_true(returned_key != NULL,
			     "bt_keys_get_type() failed to add key %d to the keys pool", i);
		zassert_equal_ptr(returned_key, expected_key_ref,
				  "bt_keys_get_type() returned unexpected reference");
	}
}
