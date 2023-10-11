/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

DEFINE_FFF_GLOBALS;

/* This LUT contains different key types */
static const int testing_type_lut[] = {BT_KEYS_PERIPH_LTK, BT_KEYS_IRK,		BT_KEYS_LTK,
				       BT_KEYS_LOCAL_CSRK, BT_KEYS_REMOTE_CSRK, BT_KEYS_LTK_P256,
				       BT_KEYS_ALL};

ZTEST_SUITE(bt_keys_add_type_set_and_verify_type_value, NULL, NULL, NULL, NULL, NULL);

/*
 *  Set key type and verify that the value is set correctly
 *
 *  Constraints:
 *   - Valid key reference is used
 *
 *  Expected behaviour:
 *   - The key type value is set correctly
 */
ZTEST(bt_keys_add_type_set_and_verify_type_value, test_set_type_value_correctly)
{
	for (size_t it = 0; it < ARRAY_SIZE(testing_type_lut); it++) {
		struct bt_keys key_ref;
		int type = testing_type_lut[it];

		memset(&key_ref, 0x00, sizeof(struct bt_keys));
		bt_keys_add_type(&key_ref, type);

		zassert_true(key_ref.keys == type,
			     "bt_keys_add_type() set incorrect key type value");
	}
}

/*
 *  Set key type to all valid types and verify that the value is set correctly
 *
 *  Constraints:
 *   - Valid key reference is used
 *
 *  Expected behaviour:
 *   - The key type value is set correctly
 */
ZTEST(bt_keys_add_type_set_and_verify_type_value, test_set_type_value_all_valid_correctly)
{
	int expected_value = BT_KEYS_PERIPH_LTK | BT_KEYS_IRK | BT_KEYS_LTK | BT_KEYS_LOCAL_CSRK |
			     BT_KEYS_REMOTE_CSRK | BT_KEYS_LTK_P256;

	struct bt_keys key_ref;
	int type = BT_KEYS_ALL;

	memset(&key_ref, 0x00, sizeof(struct bt_keys));
	bt_keys_add_type(&key_ref, type);

	zassert_true(key_ref.keys == expected_value,
		     "bt_keys_add_type() set incorrect key type value");
}

/*
 *  Mask the key type with zero and verify that it has no effect
 *
 *  Constraints:
 *   - Valid key reference is used
 *
 *  Expected behaviour:
 *   - The key type value isn't changed
 */
ZTEST(bt_keys_add_type_set_and_verify_type_value, test_set_type_value_with_zero_mask_has_no_effect)
{
	int expected_value = BT_KEYS_PERIPH_LTK | BT_KEYS_IRK | BT_KEYS_LTK | BT_KEYS_LOCAL_CSRK |
			     BT_KEYS_REMOTE_CSRK | BT_KEYS_LTK_P256;

	struct bt_keys key_ref;
	int type = BT_KEYS_ALL;

	memset(&key_ref, 0x00, sizeof(struct bt_keys));
	bt_keys_add_type(&key_ref, type);
	bt_keys_add_type(&key_ref, 0x00);

	zassert_true(key_ref.keys == expected_value,
		     "bt_keys_add_type() set incorrect key type value");
}
