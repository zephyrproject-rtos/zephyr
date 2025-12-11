/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

ZTEST_SUITE(bt_keys_add_type_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test function with NULL key reference
 *
 *  Constraints:
 *   - Any key type can be used
 *   - Key reference is passed as NULL
 *
 *  Expected behaviour:
 *   - An assertion fails and execution stops
 */
ZTEST(bt_keys_add_type_invalid_inputs, test_null_key_reference)
{
	expect_assert();
	bt_keys_add_type(NULL, 0x00);
}
