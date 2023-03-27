/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/crypto_help_utils.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/hmac_prng.h"
#include "mocks/hmac_prng_expects.h"

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
 *  Test bt_rand() fails when tc_hmac_prng_generate() fails on the first call while
 * 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - tc_hmac_prng_generate() fails and returns 'TC_CRYPTO_FAIL' on the first call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns a negative error code '-EIO' (failure)
 */
ZTEST(bt_rand_invalid_cases, test_tc_hmac_prng_generate_fails_on_first_call)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	struct tc_hmac_prng_struct *hmac_prng = bt_crypto_get_hmac_prng_instance();

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	tc_hmac_prng_generate_fake.return_val = TC_CRYPTO_FAIL;

	err = bt_rand(buf, buf_len);

	expect_call_count_tc_hmac_prng_generate(1, buf, buf_len, hmac_prng);

	zassert_true(err == -EIO, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test bt_rand() fails when prng_reseed() fails on seeding request by tc_hmac_prng_generate()
 *  while 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - tc_hmac_prng_generate() fails and returns 'TC_HMAC_PRNG_RESEED_REQ' on the first call.
 *   - prng_reseed() fails and returns a negative error code
 *
 *  Expected behaviour:
 *   - bt_rand() returns a negative error code (failure)
 */
ZTEST(bt_rand_invalid_cases, test_prng_reseed_fails_on_seeding_request)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	struct tc_hmac_prng_struct *hmac_prng = bt_crypto_get_hmac_prng_instance();

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	tc_hmac_prng_generate_fake.return_val = TC_HMAC_PRNG_RESEED_REQ;

	/* This is to make prng_reseed() fails */
	bt_hci_le_rand_fake.return_val = -1;

	err = bt_rand(buf, buf_len);

	expect_call_count_tc_hmac_prng_generate(1, buf, buf_len, hmac_prng);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

static int tc_hmac_prng_generate_custom_fake(uint8_t *out, unsigned int outlen, TCHmacPrng_t prng)
{
	if (tc_hmac_prng_generate_fake.call_count == 1) {
		return TC_HMAC_PRNG_RESEED_REQ;
	}

	return TC_CRYPTO_FAIL;
}

/*
 *  Test bt_rand() fails when tc_hmac_prng_generate() fails on the second call after a seeding
 *  request by tc_hmac_prng_generate() while 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - tc_hmac_prng_generate() fails and returns 'TC_HMAC_PRNG_RESEED_REQ' on the first call.
 *   - tc_hmac_prng_generate() fails and returns 'TC_CRYPTO_FAIL' on the second call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns a negative error code '-EIO' (failure)
 */
ZTEST(bt_rand_invalid_cases, test_tc_hmac_prng_generate_fails_on_second_call)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	struct tc_hmac_prng_struct *hmac_prng = bt_crypto_get_hmac_prng_instance();

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	tc_hmac_prng_generate_fake.custom_fake = tc_hmac_prng_generate_custom_fake;

	/* This is to make prng_reseed() succeeds and return 0 */
	bt_hci_le_rand_fake.return_val = 0;
	tc_hmac_prng_reseed_fake.return_val = TC_CRYPTO_SUCCESS;

	err = bt_rand(buf, buf_len);

	expect_call_count_tc_hmac_prng_generate(2, buf, buf_len, hmac_prng);
	expect_single_call_tc_hmac_prng_reseed(hmac_prng, 32, sizeof(int64_t));

	zassert_true(err == -EIO, "Unexpected error code '%d' was returned", err);
}
