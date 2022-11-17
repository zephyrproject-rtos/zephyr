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
#include "host_mocks/assert.h"
#include "testing_common_defs.h"

/* This LUT contains different combinations of ID and Address pairs */
extern const struct id_addr_pair testing_id_addr_pair_lut[CONFIG_BT_MAX_PAIRED];

/* This list holds returned references while filling keys pool */
extern struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

static int bt_unpair_unreachable_custom_fake(uint8_t id, const bt_addr_le_t *addr)
{
	ARG_UNUSED(id);
	ARG_UNUSED(addr);
	zassert_unreachable("Unexpected call to 'bt_unpair()' occurred");
	return 0;
}

static void bt_conn_foreach_conn_ref_null_custom_fake(int type, bt_conn_foreach_cb func,
							void *data)
{
	func(NULL, data);
}

static void bt_conn_foreach_data_ref_null_custom_fake(int type, bt_conn_foreach_cb func,
							void *data)
{
	struct bt_conn conn;

	func(&conn, NULL);
}

/* Setup test variables */
static void test_case_setup(void *f)
{
	clear_key_pool();

	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);
}

ZTEST_SUITE(bt_keys_find_key_in_use_invalid_cases, NULL, NULL, test_case_setup, NULL, NULL);

/*
 *  Test adding extra (ID, Address) pair while the keys pool list is full, but while looking
 *  for the keys in use, find_key_in_use() receives an invalid NULL connection reference.
 *
 *  Constraints:
 *   - Keys pool list is full
 *   - CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
 *
 *  Expected behaviour:
 *   - Internal function find_key_in_use() receives a NULL connection reference
 *   - An assertion fails at find_key_in_use() and execution stops
 */
ZTEST(bt_keys_find_key_in_use_invalid_cases, test_find_key_in_use_receives_null_conn_ref)
{
	uint8_t id = BT_ADDR_ID_5;
	bt_addr_le_t *addr = BT_ADDR_LE_5;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_KEYS_OVERWRITE_OLDEST);

	bt_unpair_fake.custom_fake = bt_unpair_unreachable_custom_fake;
	bt_conn_foreach_fake.custom_fake = bt_conn_foreach_conn_ref_null_custom_fake;

	expect_assert();
	bt_keys_get_addr(id, addr);

	/* Should not reach this point */
	zassert_unreachable(NULL);
}

/*
 *  Test adding extra (ID, Address) pair while the keys pool list is full, but while looking
 *  for the keys in use, find_key_in_use() receives an invalid NULL data reference.
 *
 *  Constraints:
 *   - Keys pool list is full
 *   - CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
 *
 *  Expected behaviour:
 *   - Internal function find_key_in_use() receives a NULL data reference
 *   - An assertion fails at find_key_in_use() and execution stops
 */
ZTEST(bt_keys_find_key_in_use_invalid_cases, test_find_key_in_use_receives_null_data_ref)
{
	uint8_t id = BT_ADDR_ID_5;
	bt_addr_le_t *addr = BT_ADDR_LE_5;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_KEYS_OVERWRITE_OLDEST);

	bt_unpair_fake.custom_fake = bt_unpair_unreachable_custom_fake;
	bt_conn_foreach_fake.custom_fake = bt_conn_foreach_data_ref_null_custom_fake;

	expect_assert();
	bt_keys_get_addr(id, addr);

	/* Should not reach this point */
	zassert_unreachable(NULL);
}

ZTEST_SUITE(bt_keys_get_addr_null_reference, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test invalid (NULL) BT address reference
 *
 *  Constraints:
 *   - Address value is NULL
 *
 *  Expected behaviour:
 *   - An assertion fails and execution stops
 */
ZTEST(bt_keys_get_addr_null_reference, test_null_address_reference)
{
	expect_assert();
	bt_keys_get_addr(0x00, NULL);
}
