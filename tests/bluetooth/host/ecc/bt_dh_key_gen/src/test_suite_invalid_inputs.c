/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "mocks/ecc_help_utils.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include <host/ecc.h>
#include <host/hci_core.h>

ZTEST_SUITE(bt_dh_key_gen_invalid_cases, NULL, NULL, NULL, NULL, NULL);

static void bt_dh_key_unreachable_cb(const uint8_t key[BT_DH_KEY_LEN])
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test passing NULL reference for the 'remote_pk' argument
 *
 *  Constraints:
 *   - NULL reference is used for the 'remote_pk' argument
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_dh_key_gen_invalid_cases, test_null_remote_pk_reference)
{
	expect_assert();
	bt_dh_key_gen(NULL, bt_dh_key_unreachable_cb);
}

/*
 *  Test passing NULL reference for the 'dh_key_cb' argument
 *
 *  Constraints:
 *   - NULL reference is used for the 'dh_key_cb' argument
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_dh_key_gen_invalid_cases, test_null_dh_key_cb_reference)
{
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	expect_assert();
	bt_dh_key_gen(remote_pk, NULL);
}

/*
 *  Test DH key generation fails if the callback is already registered
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag is set
 *   - 'BT_DEV_PUB_KEY_BUSY' flag isn't set
 *
 *  Expected behaviour:
 *   - bt_dh_key_gen() returns a negative error code (-EALREADY)
 */
ZTEST(bt_dh_key_gen_invalid_cases, test_callback_already_registered)
{
	int result;
	struct net_buf net_buff = {0};
	struct bt_hci_cp_le_generate_dhkey cp = {0};
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	/* This makes hci_generate_dhkey_v1() succeeds and returns 0 */
	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	/* Pass first call that will set dh_key_cb = cb*/
	bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);
	result = bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);

	zassert_true(result == -EALREADY, "Unexpected error code '%d' was returned", result);
}

static void bt_dh_key_unreachable_2nd_trial_cb(const uint8_t key[BT_DH_KEY_LEN])
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

/*
 *  Test DH key generation fails if a current key generation cycle hasn't been finished yet and
 *  'dh_key_cb' isn't NULL.
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag is set
 *   - 'BT_DEV_PUB_KEY_BUSY' flag isn't set
 *
 *  Expected behaviour:
 *   - bt_dh_key_gen() returns a negative error code (-EBUSY)
 */
ZTEST(bt_dh_key_gen_invalid_cases, test_generate_key_parallel_with_running_one)
{
	int result;
	struct net_buf net_buff = {0};
	struct bt_hci_cp_le_generate_dhkey cp = {0};
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	/* This makes hci_generate_dhkey_v1() succeeds and returns 0 */
	net_buf_simple_add_fake.return_val = &cp;
	bt_hci_cmd_create_fake.return_val = &net_buff;
	bt_hci_cmd_send_sync_fake.return_val = 0;

	/* Pass first call that will set dh_key_cb = cb*/
	bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);
	result = bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_2nd_trial_cb);

	zassert_true(result == -EBUSY, "Unexpected error code '%d' was returned", result);
}

/*
 *  Test DH key generation fails if a current key generation cycle hasn't been finished yet and
 *  'dh_key_cb' isn't NULL.
 *
 *  Constraints:
 *   - 'BT_DEV_PUB_KEY_BUSY' flag is set
 *
 *  Expected behaviour:
 *   - bt_dh_key_gen() returns a negative error code (-EBUSY)
 */
ZTEST(bt_dh_key_gen_invalid_cases, test_bt_dev_pub_key_busy_set)
{
	int result;
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	atomic_set_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	result = bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);

	zassert_true(result == -EBUSY, "Unexpected error code '%d' was returned", result);
}

/*
 *  Test DH key generation fails if the 'BT_DEV_HAS_PUB_KEY' flag isn't set
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag isn't set
 *   - 'BT_DEV_PUB_KEY_BUSY' flag isn't set
 *
 *  Expected behaviour:
 *   - bt_dh_key_gen() returns a negative error code (-EADDRNOTAVAIL)
 */
ZTEST(bt_dh_key_gen_invalid_cases, test_device_has_no_pub_key)
{
	int result;
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	result = bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);

	zassert_true(result == -EADDRNOTAVAIL, "Unexpected error code '%d' was returned", result);
}

/*
 *  Test DH key generation fails when hci_generate_dhkey_v1/2() fails
 *
 *  Constraints:
 *   - 'BT_DEV_HAS_PUB_KEY' flag is set
 *   - 'BT_DEV_PUB_KEY_BUSY' flag isn't set
 *   - 'hci_generate_dhkey_v1/2()' fails and returns a negative error code (-ENOBUFS)
 *
 *  Expected behaviour:
 *   - bt_dh_key_gen() returns a negative error code (-ENOBUFS)
 */
ZTEST(bt_dh_key_gen_invalid_cases, test_hci_generate_dhkey_vx_fails)
{
	int result;
	bt_dh_key_cb_t *dh_key_cb;
	const uint8_t remote_pk[BT_PUB_KEY_LEN] = {0};

	atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS)) {
		/* Set "ECC Debug Keys" command support bit */
		bt_dev.supported_commands[41] |= BIT(2);
	}

	/* This makes hci_generate_dhkey_vx() fails and returns (-ENOBUFS) */
	bt_hci_cmd_create_fake.return_val = NULL;

	result = bt_dh_key_gen(remote_pk, bt_dh_key_unreachable_cb);

	zassert_true(result == -ENOBUFS, "Unexpected error code '%d' was returned", result);

	dh_key_cb = bt_ecc_get_dh_key_cb();
	zassert_is_null(*dh_key_cb, "Unexpected callback reference was set");
}
