/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <host/crypto.h>
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/hmac_prng.h"
#include "mocks/hmac_prng_expects.h"
#include "mocks/crypto_help_utils.h"

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
	HMAC_PRNG_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(prng_init, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test prng_init() succeeds
 *
 *  Constraints:
 *   - bt_hci_le_rand() succeeds and returns 0 (success)
 *   - tc_hmac_prng_init() succeeds and returns 'TC_CRYPTO_SUCCESS'.
 *   - tc_hmac_prng_reseed() succeeds and returns 'TC_CRYPTO_SUCCESS'.
 *
 *  Expected behaviour:
 *   - prng_init() returns 0 (success)
 */
ZTEST(prng_init, test_prng_init_succeeds)
{
	int err;
	uint8_t expected_args_history[] = {8, 32};
	struct tc_hmac_prng_struct *hmac_prng = bt_crypto_get_hmac_prng_instance();

	bt_hci_le_rand_fake.return_val = 0;
	tc_hmac_prng_init_fake.return_val = TC_CRYPTO_SUCCESS;
	tc_hmac_prng_reseed_fake.return_val = TC_CRYPTO_SUCCESS;

	err = prng_init();

	expect_call_count_bt_hci_le_rand(2, expected_args_history);
	expect_single_call_tc_hmac_prng_init(hmac_prng, 8);
	expect_single_call_tc_hmac_prng_reseed(hmac_prng, 32, sizeof(int64_t));

	zassert_ok(err, "'%s()' returned unexpected error code %d", test_unit_name, err);
}
