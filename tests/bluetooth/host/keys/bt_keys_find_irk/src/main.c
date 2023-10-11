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
static const struct id_addr_type testing_id_addr_type_lut[] = {

	{BT_ADDR_ID_1, BT_ADDR_LE_1, BT_KEYS_PERIPH_LTK},
	{BT_ADDR_ID_1, BT_RPA_ADDR_LE_1, BT_KEYS_PERIPH_LTK},
	{BT_ADDR_ID_1, BT_RPA_ADDR_LE_2, BT_KEYS_IRK},
	{BT_ADDR_ID_1, BT_ADDR_LE_3, BT_KEYS_IRK},

	{BT_ADDR_ID_2, BT_ADDR_LE_1, BT_KEYS_LTK},
	{BT_ADDR_ID_2, BT_RPA_ADDR_LE_3, BT_KEYS_IRK},
	{BT_ADDR_ID_2, BT_RPA_ADDR_LE_4, BT_KEYS_IRK},
	{BT_ADDR_ID_2, BT_ADDR_LE_2, BT_KEYS_LOCAL_CSRK},

	{BT_ADDR_ID_3, BT_ADDR_LE_1, BT_KEYS_REMOTE_CSRK},
	{BT_ADDR_ID_3, BT_ADDR_LE_2, BT_KEYS_LTK_P256},

	{BT_ADDR_ID_4, BT_ADDR_LE_1, BT_KEYS_ALL}};

/* Global iterator to iterate over the ID, Address and type LUT */
static uint8_t params_it;

/* Pointer to the current set of testing parameters */
static struct id_addr_type const *current_params_vector;

/* This list will hold returned references while filling keys pool */
static struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_type_lut) == ARRAY_SIZE(returned_keys_refs));

/* Check if a Bluetooth LE random address is resolvable private address. */
#define BT_ADDR_IS_RPA(a) (((a)->val[5] & 0xc0) == 0x40)

static bool check_if_addr_is_rpa(const bt_addr_le_t *addr)
{
	if (addr->type != BT_ADDR_LE_RANDOM) {
		return false;
	}

	return BT_ADDR_IS_RPA(&addr->a);
}

static bool bt_rpa_irk_matches_unreachable_custom_fake(const uint8_t irk[16], const bt_addr_t *addr)
{
	zassert_unreachable("Unexpected call to 'bt_rpa_irk_matches()' occurred");
	return true;
}

static bool bt_rpa_irk_matches_custom_fake(const uint8_t irk[16], const bt_addr_t *addr)
{
	if (irk[0] != (params_it) && !bt_addr_cmp(&current_params_vector->addr->a, addr)) {
		return false;
	}

	return true;
}

static void *empty_list_ts_setup(void)
{
	clear_key_pool();

	return NULL;
}

ZTEST_SUITE(bt_keys_find_irk_initially_empty_list, NULL, empty_list_ts_setup, NULL, NULL, NULL);

/*
 *  Find a non-existing key reference for ID and Address of type 'BT_KEYS_IRK'
 *
 *  Constraints:
 *   - Empty keys pool list
 *
 *  Expected behaviour:
 *   - A NULL value is returned
 */
ZTEST(bt_keys_find_irk_initially_empty_list, test_find_non_existing_key_reference)
{
	uint8_t id;
	const bt_addr_le_t *addr;
	struct bt_keys *returned_ref;
	struct id_addr_type const *params_vector;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {

		params_vector = &testing_id_addr_type_lut[i];
		id = params_vector->id;
		addr = params_vector->addr;

		returned_ref = bt_keys_find_irk(id, addr);

		zassert_true(returned_ref == NULL,
			     "bt_keys_find_irk() returned a non-valid reference");
	}
}

static void rpa_resolving_ts_setup(void *f)
{
	clear_key_pool();
	int rv = fill_key_pool_by_id_addr_type(
		testing_id_addr_type_lut, ARRAY_SIZE(testing_id_addr_type_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	/* Set the index to the IRK value to recognize row index during bt_rpa_irk_matches()
	 * callback
	 */
	for (size_t it = 0; it < ARRAY_SIZE(returned_keys_refs); it++) {
		returned_keys_refs[it]->irk.val[0] = it;
	}

	RPA_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_keys_find_irk_rpa_resolving, NULL, NULL, rpa_resolving_ts_setup, NULL, NULL);

/*
 *  Try to resolve an RPA address using IRK by finding an existing key reference for ID and Address
 *  of type 'BT_KEYS_IRK'. Matching the address with existing IRKs fails.
 *
 *  Constraints:
 *   - Full keys pool list
 *   - IRK value and device address don't match
 *
 *  Expected behaviour:
 *   - A NULL value is returned
 */
ZTEST(bt_keys_find_irk_rpa_resolving, test_resolving_rpa_address_by_irk_but_matching_fails)
{
	uint8_t id;
	const bt_addr_le_t *addr;
	struct bt_keys *returned_ref;
	struct id_addr_type const *params_vector;

	bt_rpa_irk_matches_fake.return_val = false;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_type_lut); i++) {

		params_vector = &testing_id_addr_type_lut[i];
		id = params_vector->id;
		addr = params_vector->addr;

		returned_ref = bt_keys_find_irk(id, addr);

		zassert_true(returned_ref == NULL,
			     "bt_keys_find_irk() returned a non-valid reference");
	}
}

/*
 *  Try to resolve an RPA address using IRK by finding an existing key reference for ID and Address
 *  of type 'BT_KEYS_IRK'. Matching the address with existing IRKs fails.
 *
 *  Constraints:
 *   - Full keys pool list
 *   - IRK value and device address match
 *
 *  Expected behaviour:
 *   - A valid reference value is returned
 */
ZTEST(bt_keys_find_irk_rpa_resolving, test_resolving_rpa_address_by_irk_and_matching_succeeds)
{
	int type;
	uint8_t id;
	const bt_addr_le_t *addr;
	struct id_addr_type const *params_vector;
	struct bt_keys *returned_ref, *expected_key_ref;

	bt_rpa_irk_matches_fake.custom_fake = bt_rpa_irk_matches_custom_fake;

	for (size_t it = 0; it < ARRAY_SIZE(testing_id_addr_type_lut); it++) {

		params_vector = &testing_id_addr_type_lut[it];
		type = params_vector->type;
		id = params_vector->id;
		addr = params_vector->addr;

		/* Set global variables to be used within bt_rpa_irk_matches_custom_fake() */
		params_it = it;
		current_params_vector = params_vector;

		expected_key_ref = returned_keys_refs[it];

		/*
		 * Try to resolve the current testing vector address.
		 * Address will be considered resolvable if:
		 *  - The current testing vector address is an RPA
		 *  - The current testing vector key type is an IRK
		 */
		returned_ref = bt_keys_find_irk(id, addr);

		if (check_if_addr_is_rpa(addr) && (type & BT_KEYS_IRK)) {

			zassert_true(returned_ref != NULL,
				     "bt_keys_find_irk() returned a NULL reference %d", it);

			zassert_equal_ptr(returned_ref, expected_key_ref,
					  "bt_keys_find_irk() returned unexpected reference");

			/* Check if address has been stored */
			zassert_mem_equal(&returned_ref->irk.rpa, &addr->a, sizeof(bt_addr_t),
					  "Incorrect address was stored by 'bt_keys_find_irk()'");
		} else {
			zassert_true(returned_ref == NULL,
				     "bt_keys_find_irk() returned a non-valid reference %d", it);
		}
	}
}

static void *no_resolving_ts_setup(void)
{
	int type;
	const bt_addr_le_t *addr;
	struct id_addr_type const *params_vector;

	clear_key_pool();
	int rv = fill_key_pool_by_id_addr_type(
		testing_id_addr_type_lut, ARRAY_SIZE(testing_id_addr_type_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);

	/* Set the index to the IRK value to recognize row index during bt_rpa_irk_matches()
	 * callback
	 */
	for (size_t it = 0; it < ARRAY_SIZE(returned_keys_refs); it++) {
		returned_keys_refs[it]->irk.val[0] = it;
	}

	/* Copy the address as if was previously resolved using IRK */
	for (size_t it = 0; it < ARRAY_SIZE(returned_keys_refs); it++) {

		params_vector = &testing_id_addr_type_lut[it];
		type = params_vector->type;
		addr = params_vector->addr;

		if (check_if_addr_is_rpa(addr) && (type & BT_KEYS_IRK)) {
			bt_addr_copy(&returned_keys_refs[it]->irk.rpa, &addr->a);
		}
	}

	return NULL;
}

ZTEST_SUITE(bt_keys_find_irk_no_resolving, NULL, no_resolving_ts_setup, NULL, NULL, NULL);

/*
 *  Find an existing key reference for ID and Address of type 'BT_KEYS_IRK'
 *  while the address has been resolved previously using the IRK
 *
 *  Constraints:
 *   - Full keys pool list
 *   - IRK address and device address match
 *
 *  Expected behaviour:
 *   - A valid reference value is returned
 */
ZTEST(bt_keys_find_irk_no_resolving, test_find_key_of_previously_resolved_address)
{
	int type;
	uint8_t id;
	const bt_addr_le_t *addr;
	struct id_addr_type const *params_vector;
	struct bt_keys *returned_ref, *expected_key_ref;

	for (size_t it = 0; it < ARRAY_SIZE(testing_id_addr_type_lut); it++) {

		params_vector = &testing_id_addr_type_lut[it];
		type = params_vector->type;
		id = params_vector->id;
		addr = params_vector->addr;

		/* Set global variables to be used within bt_rpa_irk_matches_custom_fake() */
		params_it = it;
		current_params_vector = params_vector;

		/*
		 * Now, as the address under test should have been resolved before,
		 * bt_rpa_irk_matches() isn't expected to be called for an RPA.
		 *
		 * But, for other records, which won't be resolved, a call to bt_rpa_irk_matches()
		 * is expected simulating the attempt to resolve it
		 */
		if (check_if_addr_is_rpa(addr) && (type & BT_KEYS_IRK)) {
			bt_rpa_irk_matches_fake.custom_fake =
				bt_rpa_irk_matches_unreachable_custom_fake;
		} else {
			bt_rpa_irk_matches_fake.custom_fake = bt_rpa_irk_matches_custom_fake;
		}

		expected_key_ref = returned_keys_refs[it];

		returned_ref = bt_keys_find_irk(id, addr);

		if (check_if_addr_is_rpa(addr) && (type & BT_KEYS_IRK)) {

			zassert_true(returned_ref != NULL,
				     "bt_keys_find_irk() returned a NULL reference");
			zassert_equal_ptr(returned_ref, expected_key_ref,
					  "bt_keys_find_irk() returned unexpected reference");

			/* Check if address has been stored */
			zassert_mem_equal(&returned_ref->irk.rpa, &addr->a, sizeof(bt_addr_t),
					  "Incorrect address was stored by 'bt_keys_find_irk()'");
		} else {
			zassert_true(returned_ref == NULL,
				     "bt_keys_find_irk() returned a non-valid reference");
		}
	}
}
