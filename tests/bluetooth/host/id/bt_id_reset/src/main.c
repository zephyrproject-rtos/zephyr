/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/addr.h"
#include "mocks/addr_expects.h"
#include "mocks/adv.h"
#include "mocks/hci_core.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));

	ADV_FFF_FAKES_LIST(RESET_FAKE);
	ADDR_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_reset, NULL, NULL, NULL, NULL, NULL);

static int bt_addr_le_create_static_custom_fake(bt_addr_le_t *addr)
{
	const char *func_name = "bt_addr_le_create_static";

	zassert_not_null(addr, "'%s()' was called with incorrect '%s' value", func_name, "addr");

	/* This is required to test when the generated address already exists in the ID list */
	if (bt_addr_le_create_static_fake.call_count == 1) {
		bt_addr_le_copy(addr, BT_STATIC_RANDOM_LE_ADDR_1);
	} else {
		bt_addr_le_copy(addr, BT_STATIC_RANDOM_LE_ADDR_2);
	}

	return 0;
}

/*
 *  Test resetting an ID while using a NULL value for the address.
 *  As a NULL is passed to bt_id_reset() for the address and 'BT_DEV_ENABLE' is set,
 *  a new random address is generated.
 *
 *  Constraints:
 *   - Input address is NULL
 *   - Input IRK is NULL
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *   - bt_addr_le_create_static() returns a zero error code (success)
 *
 *  Expected behaviour:
 *   - A new identity is created and the address is loaded to bt_dev.id_addr[]
 *   - bt_dev.id_count isn't changed
 */
ZTEST(bt_id_reset, test_reset_id_null_address)
{
	uint8_t input_id;
	int id_count, returned_id;

	bt_dev.id_count = 2;
	input_id = bt_dev.id_count - 1; /* ID must not equal BT_ID_DEFAULT */
	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	bt_addr_le_create_static_fake.custom_fake = bt_addr_le_create_static_custom_fake;

	returned_id = bt_id_reset(input_id, NULL, NULL);

	expect_call_count_bt_addr_le_create_static(1);

	zassert_true(returned_id >= 0, "Unexpected error code '%d' was returned", returned_id);
	zassert_equal(returned_id, input_id, "Incorrect ID %d was returned", returned_id);
	zassert_true(bt_dev.id_count == id_count, "Incorrect ID count %d was set", bt_dev.id_count);
	zassert_mem_equal(&bt_dev.id_addr[returned_id], BT_STATIC_RANDOM_LE_ADDR_1,
			  sizeof(bt_addr_le_t), "Incorrect address was set");
}

/*
 *  Test resetting an identity and generating a new one while ensuring that the generated address
 *  isn't in the ID list. As a NULL is passed to bt_id_reset() for the address and 'BT_DEV_ENABLE'
 *  is set, a new random address is generated.
 *
 *  Constraints:
 *   - Input address is NULL
 *   - Input IRK is NULL
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *   - bt_addr_le_create_static() returns a zero error code (success)
 *
 *  Expected behaviour:
 *   - A new identity is created and the address is loaded to bt_dev.id_addr[]
 *   - bt_dev.id_count isn't changed
 */
ZTEST(bt_id_reset, test_reset_id_null_address_with_no_duplication)
{
	uint8_t input_id;
	int id_count, returned_id;

	bt_dev.id_count = 2;
	bt_addr_le_copy(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1);

	input_id = bt_dev.id_count - 1; /* ID must not equal BT_ID_DEFAULT */
	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	bt_addr_le_create_static_fake.custom_fake = bt_addr_le_create_static_custom_fake;

	returned_id = bt_id_reset(input_id, NULL, NULL);

	expect_call_count_bt_addr_le_create_static(2);

	zassert_true(returned_id >= 0, "Unexpected error code '%d' was returned", returned_id);
	zassert_equal(returned_id, input_id, "Incorrect ID %d was returned", returned_id);
	zassert_true(bt_dev.id_count == id_count, "Incorrect ID count %d was set", bt_dev.id_count);
	zassert_mem_equal(&bt_dev.id_addr[returned_id], BT_STATIC_RANDOM_LE_ADDR_2,
			  sizeof(bt_addr_le_t), "Incorrect address was set");
}

/*
 *  Test resetting an identity and using BT_ADDR_LE_ANY as an input.
 *  As an initialized address to BT_ADDR_LE_ANY is passed to bt_id_reset() for the address and
 * 'BT_DEV_ENABLE' is set, a new random address is generated.
 *  The generated address should be copied to the address reference passed
 *
 *  Constraints:
 *   - Input address is NULL
 *   - Input IRK is NULL
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *   - bt_addr_le_create_static() returns a zero error code (success)
 *
 *  Expected behaviour:
 *   - A new identity is created and the address is loaded to bt_dev.id_addr[]
 *   - bt_dev.id_count isn't changed
 *   - Generated address is copied to the address reference passed
 */
ZTEST(bt_id_reset, test_reset_id_bt_addr_le_any_address)
{
	uint8_t input_id;
	int id_count, returned_id;
	bt_addr_le_t addr = bt_addr_le_any;

	bt_dev.id_count = 2;
	input_id = bt_dev.id_count - 1; /* ID must not equal BT_ID_DEFAULT */
	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	bt_addr_le_create_static_fake.custom_fake = bt_addr_le_create_static_custom_fake;

	returned_id = bt_id_reset(input_id, &addr, NULL);

	expect_call_count_bt_addr_le_create_static(1);

	zassert_true(returned_id >= 0, "Unexpected error code '%d' was returned", returned_id);
	zassert_equal(returned_id, input_id, "Incorrect ID %d was returned", returned_id);
	zassert_true(bt_dev.id_count == id_count, "Incorrect ID count %d was set", bt_dev.id_count);
	zassert_mem_equal(&bt_dev.id_addr[returned_id], BT_STATIC_RANDOM_LE_ADDR_1,
			  sizeof(bt_addr_le_t), "Incorrect address was set");
	zassert_mem_equal(&addr, BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}

/*
 *  Test resetting an identity, but bt_addr_le_create_static() returns an error.
 *
 *  Constraints:
 *   - Input address is NULL
 *   - Input IRK is NULL
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *   - bt_addr_le_create_static() returns a non-zero error code (failure)
 *
 *  Expected behaviour:
 *   - No new identity is created
 *   - bt_dev.id_count is kept unchanged
 */
ZTEST(bt_id_reset, test_reset_id_null_address_fails)
{
	uint8_t input_id;
	int id_count, err;

	bt_dev.id_count = 2;
	input_id = bt_dev.id_count - 1; /* ID must not equal BT_ID_DEFAULT */
	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	bt_addr_le_create_static_fake.return_val = -1;

	err = bt_id_reset(input_id, NULL, NULL);

	expect_call_count_bt_addr_le_create_static(1);

	zassert_true(err == -1, "Unexpected error code '%d' was returned", err);
	zassert_true(bt_dev.id_count == id_count, "Incorrect ID count %d was set", bt_dev.id_count);
}

/*
 *  Test resetting an identity while a valid random static address is passed to bt_id_reset() for
 *  the address and 'BT_DEV_ENABLE' is set.
 *  The same address is used and copied to bt_dev.id_addr[].
 *
 *  Constraints:
 *   - Valid private random address is used
 *   - Input IRK is NULL
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - The same address is used and loaded to bt_dev.id_addr[]
 *   - bt_dev.id_count is kept unchanged
 */
ZTEST(bt_id_reset, test_reset_id_valid_input_address)
{
	uint8_t input_id;
	int id_count, returned_id;
	bt_addr_le_t addr = *BT_STATIC_RANDOM_LE_ADDR_1;

	bt_dev.id_count = 2;
	input_id = bt_dev.id_count - 1; /* ID must not equal BT_ID_DEFAULT */
	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	/* Calling bt_addr_le_create_static() isn't expected */
	bt_addr_le_create_static_fake.return_val = -1;

	returned_id = bt_id_reset(input_id, &addr, NULL);

	expect_not_called_bt_addr_le_create_static();

	zassert_true(returned_id >= 0, "Unexpected error code '%d' was returned", returned_id);
	zassert_equal(returned_id, input_id, "Incorrect ID %d was returned", returned_id);
	zassert_true(bt_dev.id_count == id_count, "Incorrect ID count %d was set", bt_dev.id_count);
	zassert_mem_equal(&bt_dev.id_addr[returned_id], BT_STATIC_RANDOM_LE_ADDR_1,
			  sizeof(bt_addr_le_t), "Incorrect address was set");
}
