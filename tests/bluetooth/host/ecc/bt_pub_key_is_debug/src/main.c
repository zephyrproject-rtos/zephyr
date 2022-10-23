/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/ecc_help_utils.h"

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/ecc.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(bt_pub_key_is_debug, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test bt_pub_key_is_debug() returns 'true' if key passed matches the internal debug key
 *
 *  Constraints:
 *   - The key passed matches the internal debug key
 *
 *  Expected behaviour:
 *   - bt_pub_key_is_debug() returns 'true'
 */
ZTEST(bt_pub_key_is_debug, test_key_matches_internal_key)
{
	bool result;
	uint8_t const *internal_dbg_public_key = bt_ecc_get_internal_debug_public_key();
	uint8_t testing_public_key[BT_PUB_KEY_LEN] = {0};

	memcpy(testing_public_key, internal_dbg_public_key, BT_PUB_KEY_LEN);

	result = bt_pub_key_is_debug(testing_public_key);

	zassert_true(result, "Unexpected error code '%d' was returned", result);
}

/*
 *  Test bt_pub_key_is_debug() returns 'false' if key passed doesn't match the internal debug key
 *
 *  Constraints:
 *   - The key passed doesn't match the internal debug key
 *
 *  Expected behaviour:
 *   - bt_pub_key_is_debug() returns 'false'
 */
ZTEST(bt_pub_key_is_debug, test_key_mismatches_internal_key)
{
	bool result;
	uint8_t testing_public_key[BT_PUB_KEY_LEN] = {0};

	result = bt_pub_key_is_debug(testing_public_key);

	zassert_false(result, "Unexpected error code '%d' was returned", result);
}
