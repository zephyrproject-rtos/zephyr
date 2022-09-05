/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/id.h"
#include "mocks/id_expects.h"
#include "mocks/keys_help_utils.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

DEFINE_FFF_GLOBALS;

static void tc_setup(void *f)
{
	/* Clear keys pool */
	clear_key_pool();

	/* Register resets */
	ID_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_keys_clear_keys_with_state_not_set, NULL, NULL, tc_setup, NULL, NULL);
ZTEST_SUITE(bt_keys_clear_keys_with_state_set, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Clear an existing key and verify the result while 'BT_KEYS_ID_ADDED' state isn't set.
 *  As 'BT_KEYS_ID_ADDED' isn't set, bt_id_del() shouldn't be called.
 *
 *  Constraints:
 *   - Key reference points to a valid item
 *
 *  Expected behaviour:
 *   - The key content is cleared
 *   - bt_id_del() isn't called
 */
ZTEST(bt_keys_clear_keys_with_state_not_set, test_key_cleared_bt_id_del_not_called)
{
	struct bt_keys empty_key;
	struct bt_keys *key_ref_to_clear, *find_returned_ref;
	uint8_t id = BT_ADDR_ID_0;
	bt_addr_le_t *addr = BT_ADDR_LE_1;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		ztest_test_skip();
	}

	memset(&empty_key, 0x00, sizeof(struct bt_keys));

	/* Add custom item to the keys pool */
	key_ref_to_clear = bt_keys_get_addr(id, addr);
	zassert_true(key_ref_to_clear != NULL, "bt_keys_get_addr() returned a non-valid reference");

	/* Ensure that item exists in the keys pool */
	find_returned_ref = bt_keys_find_addr(id, addr);
	zassert_true(find_returned_ref != NULL, "bt_keys_find_addr() returned a NULL reference");

	bt_keys_clear(key_ref_to_clear);

	expect_not_called_bt_id_del();

	/* Verify that memory was cleared */
	zassert_mem_equal(key_ref_to_clear, &empty_key, sizeof(struct bt_keys),
			  "Key content wasn't cleared by 'bt_keys_clear()'");

	/* Ensure that item doesn't exist in the keys pool after calling bt_keys_clear() */
	find_returned_ref = bt_keys_find_addr(id, addr);
	zassert_true(find_returned_ref == NULL,
		     "bt_keys_find_addr() returned a non-NULL reference");
}

/*
 *  Clear an existing key and verify the result while 'BT_KEYS_ID_ADDED' state is set.
 *  As 'BT_KEYS_ID_ADDED' is set, bt_id_del() should be called.
 *
 *  Constraints:
 *   - Key reference points to a valid item
 *
 *  Expected behaviour:
 *   - The key content is cleared
 *   - bt_id_del() is called with correct key reference
 */
ZTEST(bt_keys_clear_keys_with_state_set, test_key_cleared_bt_id_del_called)
{
	struct bt_keys empty_key;
	struct bt_keys *key_ref_to_clear, *find_returned_ref;
	uint8_t id = BT_ADDR_ID_0;
	bt_addr_le_t *addr = BT_ADDR_LE_1;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		ztest_test_skip();
	}

	memset(&empty_key, 0x00, sizeof(struct bt_keys));

	/* Add custom item to the keys pool */
	key_ref_to_clear = bt_keys_get_addr(id, addr);
	zassert_true(key_ref_to_clear != NULL, "bt_keys_get_addr() returned a non-valid reference");

	/* Ensure that item exists in the keys pool */
	find_returned_ref = bt_keys_find_addr(id, addr);
	zassert_true(find_returned_ref != NULL, "bt_keys_find_addr() returned a NULL reference");

	key_ref_to_clear->state = BT_KEYS_ID_ADDED;

	bt_keys_clear(key_ref_to_clear);

	expect_single_call_bt_id_del(key_ref_to_clear);

	/* Verify that memory was cleared */
	zassert_mem_equal(key_ref_to_clear, &empty_key, sizeof(struct bt_keys),
			  "Key content wasn't cleared by 'bt_keys_clear()'");

	/* Ensure that item doesn't exist in the keys pool after calling bt_keys_clear() */
	find_returned_ref = bt_keys_find_addr(id, addr);
	zassert_true(find_returned_ref == NULL,
		     "bt_keys_find_addr() returned a non-NULL reference");
}
