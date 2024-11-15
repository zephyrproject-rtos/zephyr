/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/crypto.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_id_set_adv_own_addr_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL value for advertise parameters reference
 *
 *  Constraints:
 *   - Advertise parameters reference is passed as NULL
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_adv_own_addr_invalid_inputs, test_null_advertise_parameters_reference)
{
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	expect_assert();
	bt_id_set_adv_own_addr(NULL, 0x00, false, &own_addr_type);
}

/*
 *  Test passing NULL value for address type reference
 *
 *  Constraints:
 *   - Address type reference is passed as NULL
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_adv_own_addr_invalid_inputs, test_null_address_type_reference)
{
	struct bt_le_ext_adv adv;

	expect_assert();
	bt_id_set_adv_own_addr(&adv, 0x00, false, NULL);
}

/*
 *  Test that operation not supported to use RPA with directed advertisement with connectable
 *  advertisement if privacy feature bit 'BT_LE_FEAT_BIT_PRIVACY' isn't enabled
 *
 *  Constraints:
 *   - Directed advertising flag is set
 *   - 'BT_LE_FEAT_BIT_PRIVACY' bit isn't set
 *   - Options 'BT_LE_ADV_OPT_CONN' bit is set
 *   - Options 'BT_LE_ADV_OPT_DIR_ADDR_RPA' bit is set
 *
 *  Expected behaviour:
 *   - 'ENOTSUP' error is returned
 */
ZTEST(bt_id_set_adv_own_addr_invalid_inputs, test_dir_adv_with_rpa_no_privacy)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	options |= BT_LE_ADV_OPT_CONN;
	options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

	bt_dev.le.features[(BT_LE_FEAT_BIT_PRIVACY) >> 3] &= ~BIT((BT_LE_FEAT_BIT_PRIVACY)&7);

	err = bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	zassert_true(err == -ENOTSUP, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting the advertising private address through bt_id_set_adv_private_addr() if
 *  privacy is enabled and 'BT_LE_ADV_OPT_USE_IDENTITY' options bit isn't set.
 *  Operation fails if bt_id_set_adv_private_addr() failed and a negative error code is returned.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_CONN' bit is set
 *   - Options 'BT_LE_ADV_OPT_USE_IDENTITY' bit isn't set
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - bt_id_set_adv_private_addr() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_own_addr_invalid_inputs, test_bt_id_set_adv_private_addr_fails)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	options |= BT_LE_ADV_OPT_CONN;

	err = bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_ADDR_LE_ANONYMOUS,
		     "Address type reference was unexpectedly modified");
}

/*
 *  Test setting the advertising private address with a static random address through
 *  bt_id_set_adv_random_addr() if privacy isn't enabled.
 *  Operation fails if bt_id_set_adv_random_addr() failed and a negative error code is returned.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_CONN' bit is set
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *   - bt_id_set_adv_random_addr() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_own_addr_invalid_inputs, test_bt_id_set_adv_random_addr_fails_adv_connectable)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);
	/* If 'CONFIG_BT_EXT_ADV' is defined, it changes bt_id_set_adv_random_addr() behaviour */
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	options |= BT_LE_ADV_OPT_CONN;

	adv.id = 0;
	bt_addr_le_copy(&bt_dev.id_addr[adv.id], BT_RPA_LE_ADDR);

	err = bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
	zassert_true(own_addr_type == BT_ADDR_LE_ANONYMOUS,
		     "Address type reference was unexpectedly modified");
}

/*
 *  Test setting the advertising private address with a static random address through
 *  bt_id_set_adv_random_addr() when device isn't advertising as a connectable device (i.e.
 *  BT_LE_ADV_OPT_CONN bit in options isn't set) and the advertisement is using the device
 *  identity (i.e. BT_LE_ADV_OPT_USE_IDENTITY bit is set in options).
 *
 *  Operation fails if bt_id_set_adv_random_addr() failed and a negative error code is returned.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_USE_IDENTITY' bit is set
 *   - Options 'BT_LE_ADV_OPT_CONN' bit isn't set
 *   - bt_id_set_adv_random_addr() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_own_addr_invalid_inputs, test_bt_id_set_adv_random_addr_fails_not_connectable)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	/* If 'CONFIG_BT_EXT_ADV' is defined, it changes bt_id_set_adv_random_addr() behaviour */
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	options |= BT_LE_ADV_OPT_USE_IDENTITY;
	options &= ~BT_LE_ADV_OPT_CONN;

	adv.id = 0;
	bt_addr_le_copy(&bt_dev.id_addr[adv.id], BT_RPA_LE_ADDR);

	err = bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting the advertising private address through bt_id_set_adv_private_addr() if
 *  'BT_LE_ADV_OPT_USE_IDENTITY' and 'BT_LE_ADV_OPT_USE_IDENTITY' options bits aren't set.
 *  Operation fails if bt_id_set_adv_private_addr() failed and a negative error code is returned.
 *
 *  Constraints:
 *   - Options 'BT_LE_ADV_OPT_CONN' bit isn't set
 *   - Options 'BT_LE_ADV_OPT_USE_IDENTITY' bit isn't set
 *   - bt_id_set_adv_private_addr() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_own_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_own_addr_invalid_inputs, test_bt_id_set_adv_private_addr_fails_not_connectable)
{
	int err;
	uint32_t options = 0;
	struct bt_le_ext_adv adv = {0};
	uint8_t own_addr_type = BT_ADDR_LE_ANONYMOUS;

	options &= ~BT_LE_ADV_OPT_CONN;
	options &= ~BT_LE_ADV_OPT_USE_IDENTITY;

	/* This will cause bt_id_set_adv_private_addr() to return a negative error code */
	bt_rand_fake.return_val = -1;

	err = bt_id_set_adv_own_addr(&adv, options, true, &own_addr_type);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}
