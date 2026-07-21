/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/addr.h>
#include <host/keys.h>
#include "mocks/keys_help_utils.h"
#include "mocks/hci_core_expects.h"
#include "testing_common_defs.h"

/* This LUT contains different combinations of ID and Address pairs */
extern const struct id_addr_pair testing_id_addr_pair_lut[CONFIG_BT_MAX_PAIRED];

/* This list holds returned references while filling keys pool */
extern struct bt_keys *returned_keys_refs[CONFIG_BT_MAX_PAIRED];

/* Setup test variables */
static void test_case_setup(void *f)
{
	Z_TEST_SKIP_IFDEF(CONFIG_BT_KEYS_OVERWRITE_OLDEST);

	clear_key_pool();

	int rv = fill_key_pool_by_id_addr(testing_id_addr_pair_lut,
					  ARRAY_SIZE(testing_id_addr_pair_lut), returned_keys_refs);

	zassert_true(rv == 0, "Failed to fill keys pool list, error code %d", -rv);
}

ZTEST_SUITE(bt_keys_get_addr_full_list_no_overwrite, NULL, NULL, test_case_setup, NULL, NULL);

/*
 *  Test adding extra (ID, Address) pair while the keys pool list is full.
 *  As 'CONFIG_BT_KEYS_OVERWRITE_OLDEST' isn't enabled, no (ID, Address) pairs can be added while
 *  the list is full.
 *
 *  Constraints:
 *   - Keys pool list is full
 *   - CONFIG_BT_KEYS_OVERWRITE_OLDEST isn't enabled
 *
 *  Expected behaviour:
 *   - NULL reference pointer is returned
 */
ZTEST(bt_keys_get_addr_full_list_no_overwrite, test_adding_new_pair_to_full_list)
{
	struct bt_keys *returned_key;
	uint8_t id = BT_ADDR_ID_3;
	bt_addr_le_t *addr = BT_ADDR_LE_3;

	returned_key = bt_keys_get_addr(id, addr);

	expect_not_called_bt_unpair();

	zassert_true(returned_key == NULL, "bt_keys_get_addr() returned a non-NULL reference");
}
