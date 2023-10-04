/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto.h"
#include "mocks/crypto_expects.h"
#include "mocks/hci_core.h"
#include "mocks/rpa.h"
#include "mocks/rpa_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	bt_addr_le_copy(&bt_dev.random_addr, &bt_addr_le_none);
#if defined(CONFIG_BT_PRIVACY)
	bt_addr_copy(&bt_dev.rpa[BT_ID_DEFAULT], BT_ADDR_NONE);
#endif

	RPA_FFF_FAKES_LIST(RESET_FAKE);
	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_set_adv_private_addr, NULL, NULL, NULL, NULL, NULL);

static int bt_rand_custom_fake(void *buf, size_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len == sizeof(BT_ADDR->val));

	/* This will make set_random_address() succeeds and returns 0 */
	memcpy(buf, &BT_ADDR->val, len);
	bt_addr_copy(&bt_dev.random_addr.a, BT_ADDR);

	return 0;
}

static int bt_rpa_create_custom_fake(const uint8_t irk[16], bt_addr_t *rpa)
{
	__ASSERT_NO_MSG(irk != NULL);
	__ASSERT_NO_MSG(rpa != NULL);

	/* This will make set_random_address() succeeds and returns 0 */
	bt_addr_copy(rpa, &BT_RPA_LE_ADDR->a);
	bt_addr_copy(&bt_dev.random_addr.a, &BT_RPA_LE_ADDR->a);

	return 0;
}

/*
 *  Test setting advertising private address with a valid advertise parameters reference while
 *  'CONFIG_BT_PRIVACY' is enabled and 'CONFIG_BT_EXT_ADV' isn't enabled
 *
 *  Constraints:
 *   - A valid advertise parameters ID is used (<= CONFIG_BT_ID_MAX)
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'CONFIG_BT_EXT_ADV' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns 0 (success)
 */
ZTEST(bt_id_set_adv_private_addr, test_set_adv_private_address_with_valid_ref_privacy_succeeds)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	bt_rpa_create_fake.custom_fake = bt_rpa_create_custom_fake;

	err = bt_id_set_adv_private_addr(&adv_param);

#if defined(CONFIG_BT_PRIVACY)
	expect_single_call_bt_rpa_create(bt_dev.irk[adv_param.id]);
#endif

	zassert_true(atomic_test_bit(bt_dev.flags, BT_DEV_RPA_VALID),
		     "Flags were not correctly set");

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting advertising private address with a valid advertise parameters reference while
 *  'CONFIG_BT_PRIVACY' and 'CONFIG_BT_EXT_ADV' are enabled
 *
 *  Constraints:
 *   - A valid advertise parameters ID is used (<= CONFIG_BT_ID_MAX)
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns 0 (success)
 */
ZTEST(bt_id_set_adv_private_addr, test_set_adv_private_address_with_valid_ref_priv_ext_adv_succeeds)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	bt_rpa_create_fake.custom_fake = bt_rpa_create_custom_fake;

	err = bt_id_set_adv_private_addr(&adv_param);

#if defined(CONFIG_BT_PRIVACY)
	expect_single_call_bt_rpa_create(bt_dev.irk[adv_param.id]);
#endif

#if defined(CONFIG_BT_EXT_ADV)
	zassert_true(atomic_test_bit(adv_param.flags, BT_ADV_RANDOM_ADDR_PENDING),
		     "Flags were not correctly set");

	zassert_mem_equal(&adv_param.random_addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
#endif

	zassert_true(atomic_test_bit(adv_param.flags, BT_ADV_RPA_VALID),
		     "Flags were not correctly set");

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting advertising private address with a valid advertise parameters reference after it
 *  was set previously while 'CONFIG_BT_PRIVACY' and 'CONFIG_BT_EXT_ADV' are enabled
 *
 *  Constraints:
 *   - A valid advertise parameters ID is used (<= CONFIG_BT_ID_MAX)
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *   - 'CONFIG_BT_EXT_ADV' is enabled
 *   - 'BT_ADV_RPA_VALID' flag in advertise parameters reference is set
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns 0 (success)
 */
ZTEST(bt_id_set_adv_private_addr, test_set_adv_private_address_with_valid_ref_priv_ext_adv_again)
{
	int err;
	struct bt_le_ext_adv adv_param = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	atomic_set_bit(adv_param.flags, BT_ADV_RPA_VALID);

	err = bt_id_set_adv_private_addr(&adv_param);

	expect_not_called_bt_rpa_create();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test setting advertising private address with a valid advertise parameters reference while
 *  'CONFIG_BT_PRIVACY' isn't enabled
 *
 *  Constraints:
 *   - 'CONFIG_BT_PRIVACY' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_id_set_adv_private_addr() returns 0 (success)
 */
ZTEST(bt_id_set_adv_private_addr, test_set_adv_private_address_with_valid_ref_succeeds)
{
	int err;
	struct bt_le_ext_adv adv_param;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_PRIVACY);

	bt_rand_fake.custom_fake = bt_rand_custom_fake;

	err = bt_id_set_adv_private_addr(&adv_param);

	expect_not_called_bt_rpa_create();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
}
