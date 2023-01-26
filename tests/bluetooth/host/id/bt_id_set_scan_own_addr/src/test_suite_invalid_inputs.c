/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/crypto.h"
#include "mocks/hci_core.h"
#include "mocks/rpa.h"
#include "mocks/rpa_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_id_set_scan_own_addr_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL value for address type reference
 *
 *  Constraints:
 *   - Address type reference is passed as NULL
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_scan_own_addr_invalid_inputs, test_null_address_type_reference)
{
	expect_assert();
	bt_id_set_scan_own_addr(false, NULL);
}

/*
 *  Test setting scan own address while 'CONFIG_BT_PRIVACY' isn't enabled.
 *  bt_id_set_private_addr() is called to generate a NRPA, but execution fails
 *  and it returns an error.
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - 'CONFIG_BT_SCAN_WITH_IDENTITY' isn't enabled
 *   - bt_id_set_private_addr() fails and returns a negative error
 *
 *  Expected behaviour:
 *   - bt_id_set_scan_own_addr() fails and returns the same error code returned by
 *     bt_id_set_private_addr()
 */
ZTEST(bt_id_set_scan_own_addr_invalid_inputs, test_bt_id_set_private_addr_fails_no_privacy)
{
	int err;
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_SCAN_WITH_IDENTITY);

	bt_rand_fake.return_val = -1;

	err = bt_id_set_scan_own_addr(false, &own_addr_type);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting scan own address while 'CONFIG_BT_PRIVACY' isn't enabled.
 *  If 'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled and the default identity has an RPA address of type
 * 'BT_ADDR_LE_RANDOM', set_random_address() is called, but execution fails and a negative error
 *  code is returned.
 *
 *  Constraints:
 *   - Default identity has an address with the type 'BT_ADDR_LE_RANDOM'
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - 'CONFIG_BT_SCAN_WITH_IDENTITY' is enabled
 *   - set_random_address() fails and returns a negative error
 *
 *  Expected behaviour:
 *   - bt_id_set_scan_own_addr() fails and returns the same error code returned by
 *     set_random_address()
 */
ZTEST(bt_id_set_scan_own_addr_invalid_inputs, test_set_random_address_fails)
{
	int err;
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SCAN_WITH_IDENTITY);

	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_RPA_LE_ADDR);

	/* This will cause set_random_address() to return (-ENOBUFS) */
	bt_hci_cmd_create_fake.return_val = NULL;

	err = bt_id_set_scan_own_addr(false, &own_addr_type);

	zassert_true(err == -ENOBUFS, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting scan own address while 'CONFIG_BT_PRIVACY' is enabled.
 *  bt_id_set_private_addr() is called with 'BT_ID_DEFAULT' as the ID, but it fails and returns
 *  a negative error code.
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - bt_id_set_private_addr() fails and returns a negative error
 *
 *  Expected behaviour:
 *   - bt_id_set_scan_own_addr() fails and returns the same error code returned by
 *     bt_id_set_private_addr()
 *   - Address type reference isn't set
 */
ZTEST(bt_id_set_scan_own_addr_invalid_inputs, test_bt_id_set_private_addr_fails_privacy_enabled)
{
	int err;
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	/* This will cause bt_id_set_private_addr() to fail */
	bt_rpa_create_fake.return_val = -1;

	err = bt_id_set_scan_own_addr(true, &own_addr_type);

#if defined(CONFIG_BT_PRIVACY)
	expect_single_call_bt_rpa_create(bt_dev.irk[BT_ID_DEFAULT]);
#endif

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_ADDR_LE_ANONYMOUS,
		     "Address type reference was unexpectedly modified");
}
