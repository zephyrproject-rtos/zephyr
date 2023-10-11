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

ZTEST_SUITE(bt_id_set_private_addr_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test setting private address with invalid id
 *
 *  Constraints:
 *   - Non-valid 'id' should be used (>= CONFIG_BT_ID_MAX)
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_set_private_addr_invalid_cases, test_setting_address_with_invalid_id)
{
	expect_assert();
	bt_id_set_private_addr(0xff);
}

/*
 *  Test setting private address, while CONFIG_BT_PRIVACY isn't enabled, but bt_rand() fails
 *
 *  Constraints:
 *   - Any ID value can be used
 *   - bt_rand() fails and returns a negative error code (failure)
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_private_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_private_addr_invalid_cases, test_setting_address_bt_rand_fails)
{
	int err;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	bt_rand_fake.return_val = -1;

	err = bt_id_set_private_addr(BT_ID_DEFAULT);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting private address, while CONFIG_BT_PRIVACY is enabled, but bt_rpa_create() fails
 *
 *  Constraints:
 *   - Any ID value can be used
 *   - bt_rpa_create() fails and returns a negative error code (failure)
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_private_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_private_addr_invalid_cases, test_setting_address_bt_rpa_create_fails)
{
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	bt_rpa_create_fake.return_val = -1;

	err = bt_id_set_private_addr(BT_ID_DEFAULT);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

static int bt_rand_custom_fake(void *buf, size_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len == sizeof(BT_ADDR->val));

	return 0;
}

/*
 *  Test setting private address, but set_random_address() fails
 *
 *  Constraints:
 *   - Any ID value can be used
 *   - bt_rand() returns 0 (success)
 *   - set_random_address() fails and returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_set_private_addr() returns a negative error code (failure)
 */
ZTEST(bt_id_set_private_addr_invalid_cases, test_setting_address_set_random_address_fails)
{
	int err;

	if (!IS_ENABLED(CONFIG_BT_PRIVACY)) {
		bt_rand_fake.custom_fake = bt_rand_custom_fake;
	}

	/* This will make set_random_address() returns a negative number error code */
	bt_hci_cmd_create_fake.return_val = NULL;

	err = bt_id_set_private_addr(BT_ID_DEFAULT);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}
