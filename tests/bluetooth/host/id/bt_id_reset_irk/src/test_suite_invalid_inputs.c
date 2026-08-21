/*
 * Copyright (c) 2026 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

static void invalid_inputs_before(void *fixture)
{
	bt_dev.id_count = 1;
}

ZTEST_SUITE(bt_id_reset_irk_invalid_inputs, NULL, NULL, invalid_inputs_before, NULL, NULL);

/*
 *  Test calling bt_id_reset_irk() with an ID equal to bt_dev.id_count (out of range).
 *
 *  Constraints:
 *   - CONFIG_BT_PRIVACY is enabled
 *   - bt_dev.id_count is 1
 *   - id value used equals bt_dev.id_count (invalid)
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned.
 */
static ZTEST(bt_id_reset_irk_invalid_inputs, test_id_out_of_range_equal_to_id_count)
{
	int err;

	err = bt_id_reset_irk(bt_dev.id_count);

	zassert_equal(err, -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test calling bt_id_reset_irk() with an ID greater than bt_dev.id_count.
 *
 *  Constraints:
 *   - CONFIG_BT_PRIVACY is enabled
 *   - bt_dev.id_count is 1
 *   - id value used is greater than bt_dev.id_count (invalid)
 *
 *  Expected behaviour:
 *   - '-EINVAL' error code is returned.
 */
static ZTEST(bt_id_reset_irk_invalid_inputs, test_id_out_of_range_greater_than_id_count)
{
	int err;

	err = bt_id_reset_irk(bt_dev.id_count + 1);

	zassert_equal(err, -EINVAL, "Unexpected error code '%d' was returned", err);
}
