/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/aes.h"

#include <zephyr/bluetooth/crypto.h>
#include <zephyr/kernel.h>

#include <host/crypto.h>

ZTEST_SUITE(bt_encrypt_le_invalid_cases, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference for the key argument
 *
 *  Constraints:
 *   - NULL reference is used for the key argument
 *   - Valid references are used for the other arguments
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_encrypt_le_invalid_cases, test_null_key_reference)
{
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	expect_assert();
	bt_encrypt_le(NULL, plaintext, enc_data);
}

/*
 *  Test passing NULL reference for the plain text argument
 *
 *  Constraints:
 *   - NULL reference is used for the plain text argument
 *   - Valid references are used for the other arguments
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_encrypt_le_invalid_cases, test_null_plaintext_reference)
{
	const uint8_t key[16] = {0};
	uint8_t enc_data[16] = {0};

	expect_assert();
	bt_encrypt_le(key, NULL, enc_data);
}

/*
 *  Test passing NULL reference for the encrypted data destination buffer argument
 *
 *  Constraints:
 *   - NULL reference is used for the encrypted data destination buffer argument
 *   - Valid references are used for the other arguments
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_encrypt_le_invalid_cases, test_null_enc_data_reference)
{
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};

	expect_assert();
	bt_encrypt_le(key, plaintext, NULL);
}

/*
 *  Test bt_encrypt_le() fails when tc_aes128_set_encrypt_key() fails
 *
 *  Constraints:
 *   - tc_aes128_set_encrypt_key() fails and returns 'TC_CRYPTO_FAIL'.
 *
 *  Expected behaviour:
 *   - bt_encrypt_le() returns a negative error code '-EINVAL' (failure)
 */
ZTEST(bt_encrypt_le_invalid_cases, test_tc_aes128_set_encrypt_key_fails)
{
	int err;
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	tc_aes128_set_encrypt_key_fake.return_val = TC_CRYPTO_FAIL;

	err = bt_encrypt_le(key, plaintext, enc_data);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test bt_encrypt_le() fails when tc_aes_encrypt() fails
 *
 *  Constraints:
 *   - tc_aes128_set_encrypt_key() succeeds and returns 'TC_CRYPTO_SUCCESS'.
 *   - tc_aes_encrypt() fails and returns 'TC_CRYPTO_FAIL'.
 *
 *  Expected behaviour:
 *   - bt_encrypt_le() returns a negative error code '-EINVAL' (failure)
 */
ZTEST(bt_encrypt_le_invalid_cases, test_tc_aes_encrypt_fails)
{
	int err;
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	tc_aes128_set_encrypt_key_fake.return_val = TC_CRYPTO_SUCCESS;
	tc_aes_encrypt_fake.return_val = TC_CRYPTO_FAIL;

	err = bt_encrypt_le(key, plaintext, enc_data);

	zassert_true(err == -EINVAL, "Unexpected error code '%d' was returned", err);
}
