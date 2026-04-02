/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_PRIVACY)

#include "mocks/crypto.h"
#include "mocks/crypto_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>
#include <mocks/addr.h>
#include <mocks/addr_expects.h>

static uint8_t testing_irk_value[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
					0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15};

static void tc_setup(void *f)
{
	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_id_create_privacy_enabled, NULL, NULL, tc_setup, NULL, NULL);

static int bt_rand_custom_fake(void *buf, size_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len == 16);

	memcpy(buf, testing_irk_value, len);

	return 0;
}

/*
 *  Test creating a new identity.
 *  A valid random static address is passed to bt_id_create() for the address and 'BT_DEV_ENABLE' is
 *  set, the same address is used and copied to bt_dev.id_addr[].
 *
 *  Constraints:
 *   - Valid private random address is used
 *   - Input IRK is NULL
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - The same address is used and loaded to bt_dev.id_addr[]
 *   - IRK is loaded to bt_dev.irk[]
 *   - bt_dev.id_count is incremented
 */
ZTEST(bt_id_create_privacy_enabled, test_create_id_valid_input_address_null_irk)
{
	int id_count, new_id;
	bt_addr_le_t addr = *BT_STATIC_RANDOM_LE_ADDR_1;

	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	bt_rand_fake.custom_fake = bt_rand_custom_fake;
	/* Calling bt_addr_le_create_static() isn't expected */
	bt_addr_le_create_static_fake.return_val = -1;

	new_id = bt_id_create(&addr, NULL);

	expect_not_called_bt_addr_le_create_static();
	expect_single_call_bt_rand(&bt_dev.irk[new_id], 16);

	zassert_true(new_id >= 0, "Unexpected error code '%d' was returned", new_id);
	zassert_true(bt_dev.id_count == (id_count + 1), "Incorrect ID count %d was set",
		     bt_dev.id_count);
	zassert_mem_equal(&bt_dev.id_addr[new_id], BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_mem_equal(&bt_dev.irk[new_id], testing_irk_value, sizeof(testing_irk_value),
			  "Incorrect address was set");
}

/*
 *  Test creating a new identity.
 *  A valid random static address is passed to bt_id_create() for the address and 'BT_DEV_ENABLE' is
 *  set, the same address is used and copied to bt_dev.id_addr[].
 *
 *  Constraints:
 *   - Valid private random address is used
 *   - Input IRK is cleared (zero filled)
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - The same address is used and loaded to bt_dev.id_addr[]
 *   - IRK is loaded to bt_dev.irk[]
 *   - IRK is loaded to input IRK buffer
 *   - bt_dev.id_count is incremented
 */
ZTEST(bt_id_create_privacy_enabled, test_create_id_valid_input_address_cleared_irk)
{
	int id_count, new_id;
	bt_addr_le_t addr = *BT_STATIC_RANDOM_LE_ADDR_1;
	uint8_t zero_irk[16] = {0};

	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	bt_rand_fake.custom_fake = bt_rand_custom_fake;
	/* Calling bt_addr_le_create_static() isn't expected */
	bt_addr_le_create_static_fake.return_val = -1;

	new_id = bt_id_create(&addr, zero_irk);

	expect_not_called_bt_addr_le_create_static();
	expect_single_call_bt_rand(&bt_dev.irk[new_id], 16);

	zassert_true(new_id >= 0, "Unexpected error code '%d' was returned", new_id);
	zassert_true(bt_dev.id_count == (id_count + 1), "Incorrect ID count %d was set",
		     bt_dev.id_count);
	zassert_mem_equal(&bt_dev.id_addr[new_id], BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_mem_equal(&bt_dev.irk[new_id], testing_irk_value, sizeof(testing_irk_value),
			  "Incorrect address was set");
	zassert_mem_equal(zero_irk, testing_irk_value, sizeof(testing_irk_value),
			  "Incorrect address was set");
}

/*
 *  Test creating a new identity.
 *  A valid random static address is passed to bt_id_create() for the address and 'BT_DEV_ENABLE' is
 *  set, the same address is used and copied to bt_dev.id_addr[].
 *
 *  Constraints:
 *   - Valid private random address is used
 *   - Input IRK is filled with non-zero values
 *   - 'BT_DEV_ENABLE' flag is set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - The same address is used and loaded to bt_dev.id_addr[]
 *   - Input IRK is loaded to bt_dev.irk[]
 *   - bt_dev.id_count is incremented
 */
ZTEST(bt_id_create_privacy_enabled, test_create_id_valid_input_address_filled_irk)
{
	int id_count, new_id;
	bt_addr_le_t addr = *BT_STATIC_RANDOM_LE_ADDR_1;

	id_count = bt_dev.id_count;
	atomic_set_bit(bt_dev.flags, BT_DEV_ENABLE);
	bt_rand_fake.custom_fake = bt_rand_custom_fake;
	/* Calling bt_addr_le_create_static() isn't expected */
	bt_addr_le_create_static_fake.return_val = -1;

	new_id = bt_id_create(&addr, testing_irk_value);

	expect_not_called_bt_addr_le_create_static();
	expect_not_called_bt_rand();

	zassert_true(new_id >= 0, "Unexpected error code '%d' was returned", new_id);
	zassert_true(bt_dev.id_count == (id_count + 1), "Incorrect ID count %d was set",
		     bt_dev.id_count);
	zassert_mem_equal(&bt_dev.id_addr[new_id], BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_mem_equal(&bt_dev.irk[new_id], testing_irk_value, sizeof(testing_irk_value),
			  "Incorrect address was set");
}
#endif
