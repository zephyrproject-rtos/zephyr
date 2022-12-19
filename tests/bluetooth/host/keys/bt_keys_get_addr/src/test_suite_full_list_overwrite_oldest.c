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
#include "mocks/hci_core_expects.h"
#include "testing_common_defs.h"

/* This LUT contains different combinations of ID and Address pairs */
extern const struct id_addr_pair testing_id_addr_pair_lut[CONFIG_BT_MAX_PAIRED];

/* This list holds returned references while filling keys pool */
extern struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

/* Pointer to the current set of oldest testing parameter */
struct id_addr_pair const *oldest_params_vector;

static int bt_unpair_custom_fake(uint8_t id, const bt_addr_le_t *addr)
{
	struct bt_keys *keys = NULL;

	/* Find the key slot with matched id and address */
	keys = bt_keys_find_addr(id, addr);
	if (keys) {
		bt_keys_clear(keys);
	}

	/* This check is used here because bt_unpair() is called with a local variable address */
	expect_single_call_bt_unpair(oldest_params_vector->id, oldest_params_vector->addr);

	return 0;
}

static int bt_unpair_unreachable_custom_fake(uint8_t id, const bt_addr_le_t *addr)
{
	ARG_UNUSED(id);
	ARG_UNUSED(addr);
	zassert_unreachable("Unexpected call to 'bt_unpair()' occurred");
	return 0;
}

static void bt_conn_foreach_key_slot_0_in_use_custom_fake(int type, bt_conn_foreach_cb func,
							  void *data)
{
	struct bt_conn conn;

	/* This will make the effect as if there is a disconnection */
	conn.state = BT_CONN_DISCONNECTED;
	conn.id = 0x9E;
	func(&conn, data);

	/* This will make the effect as if there is a connection with no key */
	conn.state = BT_CONN_CONNECTED;
	conn.id = 0xFF;
	bt_addr_le_copy(&conn.le.dst, (const bt_addr_le_t *)BT_ADDR_LE_ANY);
	bt_conn_get_dst_fake.return_val = &conn.le.dst;
	func(&conn, data);

	/* This will make the effect as if key at slot 0 is in use with a connection */
	conn.state = BT_CONN_CONNECTED;
	conn.id = testing_id_addr_pair_lut[0].id;
	bt_addr_le_copy(&conn.le.dst, testing_id_addr_pair_lut[0].addr);
	bt_conn_get_dst_fake.return_val = &conn.le.dst;
	func(&conn, data);
}

static void bt_conn_foreach_all_keys_in_use_custom_fake(int type, bt_conn_foreach_cb func,
							void *data)
{
	struct bt_conn conn;

	/* This will make the effect as if key at slot 'x' is in use with a connection */
	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {
		conn.state = BT_CONN_CONNECTED;
		conn.id = testing_id_addr_pair_lut[i].id;
		bt_addr_le_copy(&conn.le.dst, testing_id_addr_pair_lut[i].addr);
		bt_conn_get_dst_fake.return_val = &conn.le.dst;
		func(&conn, data);
	}
}

static void bt_conn_foreach_no_keys_in_use_custom_fake(int type, bt_conn_foreach_cb func,
							void *data)
{
	struct bt_conn conn;

	/* This will make the effect as if key at slot 'x' is in use with a connection */
	for (size_t i = 0; i < ARRAY_SIZE(testing_id_addr_pair_lut); i++) {
		conn.state = BT_CONN_DISCONNECTED;
		conn.id = testing_id_addr_pair_lut[i].id;
		bt_addr_le_copy(&conn.le.dst, testing_id_addr_pair_lut[i].addr);
		bt_conn_get_dst_fake.return_val = &conn.le.dst;
		func(&conn, data);
	}
}

/* Setup test variables */
static void test_case_setup(void *f)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_KEYS_OVERWRITE_OLDEST);

	clear_key_pool();

	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);
}

ZTEST_SUITE(bt_keys_get_addr_full_list_overwrite_oldest, NULL, NULL, test_case_setup, NULL, NULL);

/*
 *  Test adding extra (ID, Address) pair while the keys pool list is full while all keys are in
 *  use with connections so that no more (ID, Address) pairs can be added.
 *
 *  Constraints:
 *   - Keys pool list is full
 *   - All Keys are used with a connection
 *   - CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
 *
 *  Expected behaviour:
 *   - NULL pointer is returned as there is no room
 */
ZTEST(bt_keys_get_addr_full_list_overwrite_oldest, test_full_list_all_keys_in_use)
{
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_3;
	bt_addr_le_t *addr = BT_ADDR_LE_3;

	bt_unpair_fake.custom_fake = bt_unpair_unreachable_custom_fake;
	bt_conn_foreach_fake.custom_fake = bt_conn_foreach_all_keys_in_use_custom_fake;

	returned_key = bt_keys_get_addr(id, addr);

	zassert_true(returned_key == NULL, "bt_keys_get_addr() returned a non-NULL reference");
}

/*
 *  Test adding extra (ID, Address) pair while the keys pool list is full, but no keys are used with
 *  connections. New (ID, Address) pairs can be added by replacing the oldest pair.
 *
 *  Constraints:
 *   - Keys pool list is full
 *   - All Keys are not used with a connection
 *   - CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
 *
 *  Expected behaviour:
 *   - A valid pointer in the keys pool is returned, matching the one previously assigned to the
 *     oldest key (index 0).
 */
ZTEST(bt_keys_get_addr_full_list_overwrite_oldest, test_full_list_no_keys_in_use)
{
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_3;
	bt_addr_le_t *addr = BT_ADDR_LE_3;
	uint32_t expected_oldest_params_ref_idx;

	expected_oldest_params_ref_idx = 0;
	oldest_params_vector = &testing_id_addr_pair_lut[expected_oldest_params_ref_idx];
	bt_unpair_fake.custom_fake = bt_unpair_custom_fake;
	bt_conn_foreach_fake.custom_fake = bt_conn_foreach_no_keys_in_use_custom_fake;

	returned_key = bt_keys_get_addr(id, addr);

	zassert_true(returned_key != NULL, "bt_keys_get_addr() returned a NULL reference");
	zassert_true(returned_key == returned_keys_refs[expected_oldest_params_ref_idx],
		     "bt_keys_get_addr() returned reference doesn't match expected one");
}

/*
 *  Test adding extra (ID, Address) pair while the keys pool list is full when the oldest key slot
 *  is in use with a connection but others keys aren't.
 *  New (ID, address) pair should replace the oldest one that's not in use.
 *
 *  Constraints:
 *   - Keys pool list is full
 *   - Oldest key at slot 0 is used with a connection
 *   - Next oldest key (slot 1) isn't used with a connection
 *   - CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
 *
 *  Expected behaviour:
 *   - A valid pointer in the keys pool is returned, matching the one previously assigned to the
 *     oldest key (index 1).
 */
ZTEST(bt_keys_get_addr_full_list_overwrite_oldest, test_full_list_key_0_in_use_key_1_oldest)
{
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_4;
	bt_addr_le_t *addr = BT_ADDR_LE_4;
	uint32_t expected_oldest_params_ref_idx;

	expected_oldest_params_ref_idx = 1;
	oldest_params_vector = &testing_id_addr_pair_lut[expected_oldest_params_ref_idx];
	bt_unpair_fake.custom_fake = bt_unpair_custom_fake;
	bt_conn_foreach_fake.custom_fake = bt_conn_foreach_key_slot_0_in_use_custom_fake;

	returned_key = bt_keys_get_addr(id, addr);

	zassert_true(returned_key != NULL, "bt_keys_get_addr() returned a NULL reference");
	zassert_true(returned_key == returned_keys_refs[expected_oldest_params_ref_idx],
		     "bt_keys_get_addr() returned reference doesn't match expected one");
}

/*
 *  Test adding extra (ID, Address) pair while the keys pool list is full when the oldest key slot
 *  is in use with a connection but others keys aren't.
 *  New (ID, address) pair should replace the oldest one that's not in use.
 *
 *  Constraints:
 *   - Keys pool list is full
 *   - Key at slot 0 is used with a connection
 *   - oldest key (slot 2) isn't used with a connection
 *   - CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
 *
 *  Expected behaviour:
 *   - A valid pointer in the keys pool is returned, matching the one previously assigned to the
 *     oldest key (index 2).
 */
ZTEST(bt_keys_get_addr_full_list_overwrite_oldest, test_full_list_key_0_in_use_key_2_oldest)
{
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_5;
	bt_addr_le_t *addr = BT_ADDR_LE_5;
	uint32_t expected_oldest_params_ref_idx;

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	/* Normally first items inserted in the list are the oldest.
	 * For this particular test, we need to override that by setting
	 * the 'aging_counter'
	 */
	returned_keys_refs[1]->aging_counter = bt_keys_get_aging_counter_val();
#endif

	expected_oldest_params_ref_idx = 2;
	oldest_params_vector = &testing_id_addr_pair_lut[expected_oldest_params_ref_idx];
	bt_unpair_fake.custom_fake = bt_unpair_custom_fake;
	bt_conn_foreach_fake.custom_fake = bt_conn_foreach_key_slot_0_in_use_custom_fake;

	returned_key = bt_keys_get_addr(id, addr);

	zassert_true(returned_key != NULL, "bt_keys_get_addr() returned a NULL reference");
	zassert_true(returned_key == returned_keys_refs[expected_oldest_params_ref_idx],
		     "bt_keys_get_addr() returned reference doesn't match expected one");
}
