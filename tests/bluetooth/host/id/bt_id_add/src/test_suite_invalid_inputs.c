/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>
#include <host/keys.h>

ZTEST_SUITE(bt_id_add_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL value for keys reference
 *
 *  Constraints:
 *   - Keys reference is passed as NULL
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_add_invalid_inputs, test_null_keys_ref)
{
	expect_assert();
	bt_id_add(NULL);
}
