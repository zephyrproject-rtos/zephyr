/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/prng.h"
#include "mocks/prng_expects.h"

#include <zephyr/bluetooth/crypto.h>
#include <zephyr/kernel.h>

#include <host/crypto.h>

ZTEST_SUITE(bt_rand_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference destination buffer argument
 *
 *  Constraints:
 *   - NULL reference is used as an argument for the destination buffer
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_rand_invalid_cases, test_null_dst_buf_reference)
{
	expect_assert();
	bt_rand(NULL, 1);
}

/*
 *  Test passing a valid destination buffer reference with size 0
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the destination buffer
 *   - Destination buffer size is passed as 0
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_rand_invalid_cases, test_zero_dst_buf_size_reference)
{
	uint8_t buf[16];

	expect_assert();
	bt_rand(buf, 0);
}

/*
 *  Test bt_rand() fails when bt_hci_le_rand() fails while 'CONFIG_BT_HOST_CRYPTO_PRNG'
 *  isn't enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' isn't enabled
 *   - bt_hci_le_rand() fails and returns a negative error code.
 *
 *  Expected behaviour:
 *   - bt_rand() returns a negative error code (failure)
 */
ZTEST(bt_rand_invalid_cases, test_bt_hci_le_rand_fails)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	uint8_t expected_args_history[] = {16};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	bt_hci_le_rand_fake.return_val = -1;

	err = bt_rand(buf, buf_len);

	expect_call_count_bt_hci_le_rand(1, expected_args_history);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test bt_rand() fails when psa_generate_random() fails on the first call while
 * 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - psa_generate_random() fails and returns '-EIO' on the first call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns a negative error code '-EIO' (failure)
 */
ZTEST(bt_rand_invalid_cases, test_tc_hmac_prng_generate_fails_on_first_call)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	psa_generate_random_fake.return_val = -EIO;

	err = bt_rand(buf, buf_len);

	expect_single_call_psa_generate_random(buf, buf_len);

	zassert_true(err == -EIO, "Unexpected error code '%d' was returned", err);
}
