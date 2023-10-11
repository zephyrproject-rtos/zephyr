/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/fff.h>
#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"
#include "mocks/buf_help_utils.h"

DEFINE_FFF_GLOBALS;

static void tc_setup(void *f)
{
	/* Register resets */
	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(test_bt_buf_get_rx_returns_null, NULL, NULL, tc_setup, NULL, NULL);
ZTEST_SUITE(test_bt_buf_get_rx_returns_not_null, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Return value from bt_buf_get_rx() should be NULL
 *
 *  This is to test the behaviour when memory allocation request fails
 *
 *  Constraints:
 *   - Use valid buffer type 'BT_BUF_EVT'
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() returns a NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_rx()
 *   - bt_buf_get_rx() returns NULL
 */
ZTEST(test_bt_buf_get_rx_returns_null, test_returns_null_type_bt_buf_evt)
{
	struct net_buf *returned_buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
		memory_pool = bt_buf_get_evt_pool();
	} else {
		memory_pool = bt_buf_get_hci_rx_pool();
	}

	net_buf_alloc_fixed_fake.return_val = NULL;

	returned_buf = bt_buf_get_rx(BT_BUF_EVT, timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_not_called_net_buf_reserve();
	expect_not_called_net_buf_ref();

	zassert_is_null(returned_buf,
			"bt_buf_get_rx() returned non-NULL value while expecting NULL");
}

/*
 *  Return value from bt_buf_get_rx() should be NULL
 *
 *  This is to test the behaviour when memory allocation request fails
 *
 *  Constraints:
 *   - Use valid buffer type 'BT_BUF_ACL_IN'
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() returns a NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_rx()
 *   - bt_buf_get_rx() returns NULL
 */
ZTEST(test_bt_buf_get_rx_returns_null, test_returns_null_type_bt_buf_acl_in)
{
	struct net_buf *returned_buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
		memory_pool = bt_buf_get_acl_in_pool();
	} else {
		memory_pool = bt_buf_get_hci_rx_pool();
	}

	net_buf_alloc_fixed_fake.return_val = NULL;

	returned_buf = bt_buf_get_rx(BT_BUF_ACL_IN, timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_not_called_net_buf_reserve();
	expect_not_called_net_buf_ref();

	zassert_is_null(returned_buf,
			"bt_buf_get_rx() returned non-NULL value while expecting NULL");
}

/*
 *  Return value from bt_buf_get_rx() should be NULL
 *
 *  This is to test the behaviour when memory allocation request fails
 *
 *  Constraints:
 *   - Use valid buffer type 'BT_BUF_ISO_IN'
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() returns a NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_rx()
 *   - bt_buf_get_rx() returns NULL
 */
ZTEST(test_bt_buf_get_rx_returns_null, test_returns_null_type_bt_buf_iso_in)
{
	struct net_buf *returned_buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_ISO_UNICAST) || IS_ENABLED(CONFIG_BT_ISO_SYNC_RECEIVER))) {
		memory_pool = bt_buf_get_iso_rx_pool();
	} else {
		if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
			memory_pool = bt_buf_get_acl_in_pool();
		} else {
			memory_pool = bt_buf_get_hci_rx_pool();
		}
	}

	net_buf_alloc_fixed_fake.return_val = NULL;

	returned_buf = bt_buf_get_rx(BT_BUF_ISO_IN, timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_not_called_net_buf_reserve();
	expect_not_called_net_buf_ref();

	zassert_is_null(returned_buf,
			"bt_buf_get_rx() returned non-NULL value while expecting NULL");
}

/*
 *  Return value from bt_buf_get_rx() shouldn't be NULL
 *
 *  Constraints:
 *   - Use valid buffer type 'BT_BUF_EVT'
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() return a not NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_rx()
 *   - bt_buf_get_rx() returns the same value returned by net_buf_alloc_fixed()
 *   - Return buffer matches the buffer type requested
 */
ZTEST(test_bt_buf_get_rx_returns_not_null, test_returns_not_null_type_bt_buf_evt)
{
	static struct net_buf expected_buf;
	struct net_buf *returned_buf;
	uint8_t returned_buffer_type;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
		memory_pool = bt_buf_get_evt_pool();
	} else {
		memory_pool = bt_buf_get_hci_rx_pool();
	}

	net_buf_alloc_fixed_fake.return_val = &expected_buf;

	returned_buf = bt_buf_get_rx(BT_BUF_EVT, timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_single_call_net_buf_reserve(&expected_buf);
	expect_not_called_net_buf_ref();

	zassert_equal(returned_buf, &expected_buf,
		      "bt_buf_get_rx() returned incorrect buffer pointer value");

	returned_buffer_type = bt_buf_get_type(returned_buf);
	zassert_equal(returned_buffer_type, BT_BUF_EVT,
		      "bt_buf_get_rx() returned incorrect buffer type %u, expected %u (%s)",
		      returned_buffer_type, BT_BUF_EVT, STRINGIFY(BT_BUF_EVT));
}

/*
 *  Return value from bt_buf_get_rx() shouldn't be NULL
 *
 *  Constraints:
 *   - Use valid buffer type 'BT_BUF_ACL_IN'
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() return a not NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_rx()
 *   - bt_buf_get_rx() returns the same value returned by net_buf_alloc_fixed()
 *   - Return buffer matches the buffer type requested
 */
ZTEST(test_bt_buf_get_rx_returns_not_null, test_returns_not_null_type_bt_buf_acl_in)
{
	static struct net_buf expected_buf;
	struct net_buf *returned_buf;
	uint8_t returned_buffer_type;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
		memory_pool = bt_buf_get_acl_in_pool();
	} else {
		memory_pool = bt_buf_get_hci_rx_pool();
	}

	net_buf_alloc_fixed_fake.return_val = &expected_buf;

	returned_buf = bt_buf_get_rx(BT_BUF_ACL_IN, timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_single_call_net_buf_reserve(&expected_buf);
	expect_not_called_net_buf_ref();

	zassert_equal(returned_buf, &expected_buf,
		      "bt_buf_get_rx() returned incorrect buffer pointer value");

	returned_buffer_type = bt_buf_get_type(returned_buf);
	zassert_equal(returned_buffer_type, BT_BUF_ACL_IN,
		      "bt_buf_get_rx() returned incorrect buffer type %u, expected %u (%s)",
		      returned_buffer_type, BT_BUF_ACL_IN, STRINGIFY(BT_BUF_ACL_IN));
}

/*
 *  Return value from bt_buf_get_rx() shouldn't be NULL
 *
 *  Constraints:
 *   - Use valid buffer type 'BT_BUF_ISO_IN'
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() return a not NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_rx()
 *   - bt_buf_get_rx() returns the same value returned by net_buf_alloc_fixed()
 *   - Return buffer matches the buffer type requested
 */
ZTEST(test_bt_buf_get_rx_returns_not_null, test_returns_not_null_type_bt_buf_iso_in)
{
	static struct net_buf expected_buf;
	struct net_buf *returned_buf;
	uint8_t returned_buffer_type;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_ISO_UNICAST) || IS_ENABLED(CONFIG_BT_ISO_SYNC_RECEIVER))) {
		memory_pool = bt_buf_get_iso_rx_pool();
	} else {
		if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
			memory_pool = bt_buf_get_acl_in_pool();
		} else {
			memory_pool = bt_buf_get_hci_rx_pool();
		}
	}

	net_buf_alloc_fixed_fake.return_val = &expected_buf;

	returned_buf = bt_buf_get_rx(BT_BUF_ISO_IN, timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_single_call_net_buf_reserve(&expected_buf);
	expect_not_called_net_buf_ref();

	zassert_equal(returned_buf, &expected_buf,
		      "bt_buf_get_rx() returned incorrect buffer pointer value");

	returned_buffer_type = bt_buf_get_type(returned_buf);
	zassert_equal(returned_buffer_type, BT_BUF_ISO_IN,
		      "bt_buf_get_rx() returned incorrect buffer type %u, expected %u (%s)",
		      returned_buffer_type, BT_BUF_ISO_IN, STRINGIFY(BT_BUF_ISO_IN));
}
