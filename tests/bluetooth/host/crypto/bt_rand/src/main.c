/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/prng.h"
#include "mocks/prng_expects.h"

#include <zephyr/bluetooth/crypto.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/crypto.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
	PRNG_FFF_FAKES_LIST(RESET_FAKE);
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
 *  Test bt_rand() succeeds when psa_generate_random() succeeds on the first call while
 * 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - psa_generate_random() succeeds and returns 'PSA_SUCCESS' on the first call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns 0 (success)
 */
ZTEST(bt_rand, test_psa_generate_random_succeeds_on_first_call)
{
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HOST_CRYPTO_PRNG);

	psa_generate_random_fake.return_val = PSA_SUCCESS;

	err = bt_rand(buf, buf_len);

	expect_single_call_psa_generate_random(buf, buf_len);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}
