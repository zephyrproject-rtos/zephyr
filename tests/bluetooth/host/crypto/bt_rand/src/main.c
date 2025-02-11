/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto_help_utils.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/hmac_prng.h"
#include "mocks/hmac_prng_expects.h"

#include <zephyr/bluetooth/crypto.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/crypto.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
	HMAC_PRNG_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_rand, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test bt_rand() succeeds while 'CONFIG_BT_HOST_CRYPTO_PRNG' isn't enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' isn't enabled
 *   - bt_hci_le_rand() succeeds and returns 0 (success)
 *
 *  Expected behaviour:
 *   - bt_rand() returns 0 (success)
 */
ZTEST(bt_rand, test_bt_rand_succeeds_host_crypto_prng_disabled)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	uint8_t expected_args_history[] = {16};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	bt_hci_le_rand_fake.return_val = 0;

	err = bt_rand(buf, buf_len);

	expect_call_count_bt_hci_le_rand(1, expected_args_history);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test bt_rand() succeeds when tc_hmac_prng_generate() succeeds on the first call while
 * 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - tc_hmac_prng_generate() succeeds and returns 'TC_CRYPTO_SUCCESS' on the first call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns 0 (success)
 */
ZTEST(bt_rand, test_tc_hmac_prng_generate_succeeds_on_first_call)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	struct tc_hmac_prng_struct *hmac_prng = bt_crypto_get_hmac_prng_instance();

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	tc_hmac_prng_generate_fake.return_val = TC_CRYPTO_SUCCESS;

	err = bt_rand(buf, buf_len);

	expect_call_count_tc_hmac_prng_generate(1, buf, buf_len, hmac_prng);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

static int tc_hmac_prng_generate_custom_fake(uint8_t *out, unsigned int outlen, TCHmacPrng_t prng)
{
	if (tc_hmac_prng_generate_fake.call_count == 1) {
		return TC_HMAC_PRNG_RESEED_REQ;
	}

	return TC_CRYPTO_SUCCESS;
}

/*
 *  Test bt_rand() succeeds when tc_hmac_prng_generate() succeeds on the second call after a seeding
 *  request by tc_hmac_prng_generate() while 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - tc_hmac_prng_generate() fails and returns 'TC_HMAC_PRNG_RESEED_REQ' on the first call.
 *   - tc_hmac_prng_generate() succeeds and returns 'TC_CRYPTO_SUCCESS' on the second call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns 0 (success)
 */
ZTEST(bt_rand, test_tc_hmac_prng_generate_succeeds_on_second_call)
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

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}
