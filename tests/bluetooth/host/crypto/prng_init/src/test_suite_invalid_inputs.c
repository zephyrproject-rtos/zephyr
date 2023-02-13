/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <host/crypto.h>
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/hmac_prng.h"
#include "mocks/hmac_prng_expects.h"
#include "mocks/crypto_help_utils.h"
#include "host_mocks/assert.h"

ZTEST_SUITE(prng_init_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test prng_init() fails when bt_hci_le_rand() fails
 *
 *  Constraints:
 *   - bt_hci_le_rand() fails and returns a negative error code.
 *
 *  Expected behaviour:
 *   - prng_init() returns a negative error code (failure)
 */
ZTEST(prng_init_invalid_cases, test_bt_hci_le_rand_fails)
{
	int err;
	uint8_t expected_args_history[] = {8};

	bt_hci_le_rand_fake.return_val = -1;

	err = prng_init();

	expect_call_count_bt_hci_le_rand(1, expected_args_history);

	zassert_true(err < 0, "'%s()' returned unexpected error code %d", test_unit_name, err);
}

/*
 *  Test prng_init() fails when tc_hmac_prng_init() fails
 *
 *  Constraints:
 *   - bt_hci_le_rand() succeeds and returns 0 (success)
 *   - tc_hmac_prng_init() fails and returns 'TC_CRYPTO_FAIL'.
 *
 *  Expected behaviour:
 *   - prng_init() returns a negative error code '-EIO' (failure)
 */
ZTEST(prng_init_invalid_cases, test_tc_hmac_prng_init_fails)
{
	int err;
	uint8_t expected_args_history[] = {8};
	struct tc_hmac_prng_struct *hmac_prng = bt_crypto_get_hmac_prng_instance();

	bt_hci_le_rand_fake.return_val = 0;
	tc_hmac_prng_init_fake.return_val = TC_CRYPTO_FAIL;

	err = prng_init();

	expect_call_count_bt_hci_le_rand(1, expected_args_history);
	expect_single_call_tc_hmac_prng_init(hmac_prng, 8);

	zassert_true(err == -EIO, "'%s()' returned unexpected error code %d", test_unit_name, err);
}

/*
 *  Test prng_init() fails when prng_reseed() fails
 *
 *  Constraints:
 *   - bt_hci_le_rand() succeeds and returns 0 (success)
 *   - tc_hmac_prng_init() succeeds and returns 'TC_CRYPTO_SUCCESS'.
 *   - tc_hmac_prng_reseed() fails and returns 'TC_CRYPTO_FAIL'.
 *
 *  Expected behaviour:
 *   - prng_init() returns a negative error code '-EIO' (failure)
 */
ZTEST(prng_init_invalid_cases, test_prng_reseed_fails)
{
	int err;
	uint8_t expected_args_history[] = {8, 32};
	struct tc_hmac_prng_struct *hmac_prng = bt_crypto_get_hmac_prng_instance();

	bt_hci_le_rand_fake.return_val = 0;
	tc_hmac_prng_init_fake.return_val = TC_CRYPTO_SUCCESS;
	tc_hmac_prng_reseed_fake.return_val = TC_CRYPTO_FAIL;

	err = prng_init();

	expect_call_count_bt_hci_le_rand(2, expected_args_history);
	expect_single_call_tc_hmac_prng_init(hmac_prng, 8);
	expect_single_call_tc_hmac_prng_reseed(hmac_prng, 32, sizeof(int64_t));

	zassert_true(err == -EIO, "'%s()' returned unexpected error code %d", test_unit_name, err);
}
