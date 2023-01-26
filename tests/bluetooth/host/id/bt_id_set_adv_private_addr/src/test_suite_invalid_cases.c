/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/crypto.h"
#include "mocks/hci_core.h"
#include "mocks/rpa.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_id_set_adv_private_addr_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test setting advertise private random address while passing a NULL value as a reference to
 *  the advertise parameters.
 *
 *  Constraints:
 *   - A NULL value is passed to the function as a reference
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_adv_private_addr_invalid_cases, test_set_adv_address_with_null_reference)
{
	expect_assert();
	bt_id_set_adv_private_addr(NULL);
}

/*
 *  Test setting advertise private random address with a valid reference, but bt_rand() fails
 *
 *  Constraints:
 *   - A valid advertise parameters reference is used
 *   - bt_rand() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_private_addr_invalid_cases, test_set_adv_address_bt_rand_fails)
{
	int err;
	struct bt_le_ext_adv adv_param;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	bt_rand_fake.return_val = -1;

	err = bt_id_set_adv_private_addr(&adv_param);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

static int bt_rand_custom_fake(void *buf, size_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len == sizeof(BT_ADDR->val));

	return 0;
}

/*
 *  Test setting advertise private random address with a valid reference, but bt_rand() fails
 *
 *  Constraints:
 *   - A valid advertise parameters reference is used
 *   - bt_id_set_adv_random_addr() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_private_addr_invalid_cases, test_set_adv_address_set_adv_random_addr_fails)
{
	int err;
	struct bt_le_ext_adv adv_param;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	bt_rand_fake.custom_fake = bt_rand_custom_fake;
	/* This will make set_random_address() returns a negative number error code */
	bt_hci_cmd_create_fake.return_val = NULL;

	err = bt_id_set_adv_private_addr(&adv_param);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting advertising private address with a valid advertise parameters reference while
 *  'CONFIG_BT_PRIVACY' and 'CONFIG_BT_EXT_ADV' are enabled, but bt_rpa_create() fails
 *
 *  Constraints:
 *   - A valid advertise parameters ID is used (<= CONFIG_BT_ID_MAX)
 *   - bt_rpa_create() fails and returns a negative error code (failure)
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_private_addr_invalid_cases, test_set_adv_address_bt_rpa_create_fails)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	bt_rpa_create_fake.return_val = -1;

	err = bt_id_set_adv_private_addr(&adv_param);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

static int bt_rpa_create_custom_fake(const uint8_t irk[16], bt_addr_t *rpa)
{
	__ASSERT_NO_MSG(irk != NULL);
	__ASSERT_NO_MSG(rpa != NULL);

	/* This will make set_random_address() succeeds and returns 0 */
	bt_addr_copy(rpa, &BT_RPA_LE_ADDR->a);
	bt_addr_copy(&bt_dev.random_addr.a, &BT_RPA_LE_ADDR->a);

	return 0;
}

/*
 *  Test setting advertising private address with a valid advertise parameters reference while
 *  'CONFIG_BT_PRIVACY' and 'CONFIG_BT_EXT_ADV' are enabled, but bt_id_set_adv_random_addr() fails
 *
 *  Constraints:
 *   - A valid advertise parameters ID is used (<= CONFIG_BT_ID_MAX)
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *   - bt_id_set_adv_random_addr() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_adv_private_addr_invalid_cases, test_set_adv_address_if_set_adv_random_addr_fails)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	bt_rpa_create_fake.custom_fake = bt_rpa_create_custom_fake;

	/* This will make bt_id_set_adv_random_addr() returns a negative number error code */
	atomic_set_bit(adv_param.flags, BT_ADV_PARAMS_SET);
	bt_hci_cmd_create_fake.return_val = NULL;

	err = bt_id_set_adv_private_addr(&adv_param);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}
