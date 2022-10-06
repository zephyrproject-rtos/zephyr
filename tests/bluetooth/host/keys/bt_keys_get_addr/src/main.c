/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/addr.h>
#include <host/keys.h>
#include "mocks/conn.h"
#include "mocks/hci_core.h"
#include "mocks/keys_help_utils.h"
#include "host_mocks/print_utils.h"
#include "testing_common_defs.h"

/* This LUT contains different combinations of ID and Address pairs */
const struct id_addr_pair testing_id_addr_pair_lut[] = {
	{ BT_ADDR_ID_1, BT_ADDR_LE_1 },
	{ BT_ADDR_ID_1, BT_ADDR_LE_2 },
	{ BT_ADDR_ID_2, BT_ADDR_LE_1 },
	{ BT_ADDR_ID_2, BT_ADDR_LE_2 }
};

/* This list will hold returned references while filling keys pool */
struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

static bool all_startup_checks_executed;

BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == CONFIG_BT_MAX_PAIRED);
BUILD_ASSERT(ARRAY_SIZE(testing_id_addr_pair_lut) == ARRAY_SIZE(returned_keys_refs));

static bool startup_suite_predicate(const void *global_state)
{
	return (all_startup_checks_executed == false);
}

ZTEST_SUITE(bt_keys_get_addr_startup, startup_suite_predicate, NULL, NULL, NULL, NULL);

/*
 *  Check if the keys pool list is empty after starting up
 *
 *  Constraints:
 *   - Check is executed after starting up, prior to adding any keys
 *
 *  Expected behaviour:
 *   - Keys pool list is empty
 */
ZTEST(bt_keys_get_addr_startup, test_keys_pool_list_is_empty_at_startup)
{
	zassert_true(check_key_pool_is_empty(),
		     "List isn't empty, make sure to run this test just after a fresh start");
}

ZTEST_SUITE(bt_keys_get_addr_populate_non_existing_keys, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test filling the keys pool with (ID, Address) pairs
 *
 *  Constraints:
 *   - Keys pool list is empty after starting up
 *
 *  Expected behaviour:
 *   - A valid reference is returned by bt_keys_get_addr()
 *   - ID value matches the one passed to bt_keys_get_addr()
 *   - Address value matches the one passed to bt_keys_get_addr()
 */
ZTEST(bt_keys_get_addr_populate_non_existing_keys, test_populate_key_pool)
{
	struct id_addr_pair const *current_params_vector;
	struct bt_keys *returned_key;
	uint8_t id;
	bt_addr_le_t *addr;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {

		current_params_vector = &testing_id_addr_pair_lut[i];
		id = current_params_vector->id;
		addr = current_params_vector->addr;

		returned_key = bt_keys_get_addr(id, addr);
		returned_keys_refs[i] = returned_key;

		zassert_true(returned_key != NULL,
			     "bt_keys_get_addr() failed to add key %d to the keys pool", i);
		zassert_true(returned_key->id == id,
			     "bt_keys_get_addr() returned a reference with an incorrect ID");
		zassert_true(!bt_addr_le_cmp(&returned_key->addr, addr),
			     "bt_keys_get_addr() set incorrect address %s value, expected %s",
			     bt_addr_le_str(&returned_key->addr), bt_addr_le_str(addr));
	}
}

/*
 *  Test no equal references returned by bt_keys_get_addr()
 *
 *  Constraints:
 *   - Keys pool has been filled
 *
 *  Expected behaviour:
 *   - All returned references are different from each other
 */
ZTEST(bt_keys_get_addr_populate_non_existing_keys, test_no_equal_references)
{
	struct bt_keys *keys_pool = bt_keys_get_key_pool();

	for (uint32_t i = 0; i < ARRAY_SIZE(returned_keys_refs); i++) {
		struct bt_keys *returned_ref = returned_keys_refs[i];

		zassert_equal_ptr(keys_pool + i, returned_ref,
				  "bt_keys_get_addr() returned unexpected reference at slot %u", i);
	}
}

/* Setup test variables */
static void test_case_setup(void *f)
{
	clear_key_pool();

	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);
}

ZTEST_SUITE(bt_keys_get_addr_get_existing_keys, NULL, NULL, test_case_setup, NULL, NULL);

/*
 *  Test getting a valid key reference by a matching ID and address pair
 *
 *  Constraints:
 *   - ID and address pairs has been inserted in the list
 *
 *  Expected behaviour:
 *   - A valid reference is returned by bt_keys_get_addr() that
 *     matches the one returned after adding the ID and address pair
 *   - ID value matches the one passed to bt_keys_get_addr()
 *   - Address value matches the one passed to bt_keys_get_addr()
 */
ZTEST(bt_keys_get_addr_get_existing_keys, test_get_key_by_matched_id_and_address)
{
	struct bt_keys *returned_key, *expected_key_ref;
	struct id_addr_pair const *current_params_vector;
	uint8_t id;
	bt_addr_le_t *addr;

	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {
		current_params_vector = &testing_id_addr_pair_lut[i];
		id = current_params_vector->id;
		addr = current_params_vector->addr;

		returned_key = bt_keys_get_addr(id, addr);
		expected_key_ref = returned_keys_refs[i];

		zassert_true(returned_key != NULL,
			     "bt_keys_get_addr() failed to add key %d to the keys pool", i);
		zassert_equal_ptr(returned_key, expected_key_ref,
				  "bt_keys_get_addr() returned unexpected reference");
	}
}

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{

	/* Skip tests if not all startup suite hasn't been executed */
	if (strcmp(test->test_suite_name, "bt_keys_get_addr_startup")) {
		zassume_true(all_startup_checks_executed == true, NULL);
	}

	CONN_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

void test_main(void)
{
	/* Only startup suite will run. */
	all_startup_checks_executed = false;
	ztest_run_all(NULL);

	/* All other suites, except startup suite, will run. */
	all_startup_checks_executed = true;
	ztest_run_all(NULL);

	/* Check that all the suites in this binary ran at least once. */
	ztest_verify_all_test_suites_ran();
}
