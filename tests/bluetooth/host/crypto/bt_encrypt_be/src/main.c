/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/aes.h"
#include "mocks/aes_expects.h"

#include <zephyr/bluetooth/crypto.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/crypto.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	AES_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_encrypt_be, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test bt_encrypt_be() succeeds
 *
 *  Constraints:
 *   - tc_aes128_set_encrypt_key() succeeds and returns 'TC_CRYPTO_SUCCESS'.
 *   - tc_aes_encrypt() succeeds and returns 'TC_CRYPTO_SUCCESS'.
 *
 *  Expected behaviour:
 *   - bt_encrypt_be() returns 0 (success)
 */
ZTEST(bt_encrypt_be, test_bt_encrypt_be_succeeds)
{
	int err;
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	tc_aes128_set_encrypt_key_fake.return_val = TC_CRYPTO_SUCCESS;
	tc_aes_encrypt_fake.return_val = TC_CRYPTO_SUCCESS;

	err = bt_encrypt_be(key, plaintext, enc_data);

	expect_single_call_tc_aes_encrypt(enc_data);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}
