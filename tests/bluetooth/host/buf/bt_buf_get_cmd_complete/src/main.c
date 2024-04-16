/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <host/hci_core.h>
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

ZTEST_SUITE(bt_buf_get_cmd_complete_returns_not_null, NULL, NULL, tc_setup, NULL, NULL);
ZTEST_SUITE(bt_buf_get_cmd_complete_returns_null, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Return value from bt_buf_get_cmd_complete() should be NULL
 *
 *  This is to test the behaviour when memory allocation request fails
 *
 *  Constraints:
 *   - bt_dev.sent_cmd value is NULL
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() returns a NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_cmd_complete()
 *   - bt_dev.sent_cmd value is cleared after calling bt_buf_get_cmd_complete()
 *   - bt_buf_get_cmd_complete() returns NULL
 */
ZTEST(bt_buf_get_cmd_complete_returns_null, test_returns_null_sent_cmd_is_null)
{
	struct net_buf *returned_buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	bt_dev.sent_cmd = NULL;

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
		memory_pool = bt_buf_get_evt_pool();
	} else {
		memory_pool = bt_buf_get_hci_rx_pool();
	}

	net_buf_alloc_fixed_fake.return_val = NULL;

	returned_buf = bt_buf_get_cmd_complete(timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_not_called_net_buf_reserve();
	expect_not_called_net_buf_ref();

	zassert_is_null(returned_buf,
			"bt_buf_get_cmd_complete() returned non-NULL value while expecting NULL");

	zassert_equal(bt_dev.sent_cmd, NULL,
		     "bt_buf_get_cmd_complete() didn't clear bt_dev.sent_cmd");
}

/*
 *  Return value from bt_buf_get_cmd_complete() shouldn't be NULL
 *
 *  Constraints:
 *   - bt_dev.sent_cmd value is NULL
 *   - Timeout value is a positive non-zero value
 *   - net_buf_alloc() return a not NULL value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called with the correct memory allocation pool
 *     and the same timeout value passed to bt_buf_get_cmd_complete()
 *   - bt_dev.sent_cmd value is cleared after calling bt_buf_get_cmd_complete()
 *   - bt_buf_get_cmd_complete() returns the same value returned by net_buf_alloc_fixed()
 *   - Return buffer type is set to BT_BUF_EVT
 */
ZTEST(bt_buf_get_cmd_complete_returns_not_null, test_returns_not_null_sent_cmd_is_null)
{
	static struct net_buf expected_buf;
	struct net_buf *returned_buf;
	uint8_t returned_buffer_type;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	bt_dev.sent_cmd = NULL;

	struct net_buf_pool *memory_pool;

	if ((IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL))) {
		memory_pool = bt_buf_get_evt_pool();
	} else {
		memory_pool = bt_buf_get_hci_rx_pool();
	}

	net_buf_alloc_fixed_fake.return_val = &expected_buf;

	returned_buf = bt_buf_get_cmd_complete(timeout);

	expect_single_call_net_buf_alloc(memory_pool, &timeout);
	expect_single_call_net_buf_reserve(&expected_buf);
	expect_not_called_net_buf_ref();

	zassert_equal(returned_buf, &expected_buf,
		      "bt_buf_get_cmd_complete() returned incorrect buffer pointer value");

	returned_buffer_type = bt_buf_get_type(returned_buf);
	zassert_equal(returned_buffer_type, BT_BUF_EVT,
		      "bt_buf_get_cmd_complete() returned incorrect buffer type %u, expected %u (%s)",
		      returned_buffer_type, BT_BUF_EVT, STRINGIFY(BT_BUF_EVT));

	zassert_equal(bt_dev.sent_cmd, NULL,
		     "bt_buf_get_cmd_complete() didn't clear bt_dev.sent_cmd");
}

/*
 *  Return value from bt_buf_get_cmd_complete() shouldn't be NULL
 *
 *  Constraints:
 *   - bt_dev.sent_cmd value isn't NULL
 *   - Timeout value is a positive non-zero value
 *
 *  Expected behaviour:
 *   - net_buf_alloc() isn't called
 *   - bt_dev.sent_cmd value is cleared after calling bt_buf_get_cmd_complete()
 *   - bt_buf_get_cmd_complete() returns the same value set to bt_dev.sent_cmd
 *   - Return buffer type is set to BT_BUF_EVT
 */
ZTEST(bt_buf_get_cmd_complete_returns_not_null, test_returns_not_null_sent_cmd_is_not_null)
{
	static struct net_buf expected_buf;
	struct net_buf *returned_buf;
	uint8_t returned_buffer_type;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	bt_dev.sent_cmd = &expected_buf;

	net_buf_ref_fake.return_val = &expected_buf;

	returned_buf = bt_buf_get_cmd_complete(timeout);

	expect_single_call_net_buf_reserve(&expected_buf);
	expect_not_called_net_buf_alloc();

	zassert_equal(returned_buf, &expected_buf,
		      "bt_buf_get_cmd_complete() returned incorrect buffer pointer value");

	returned_buffer_type = bt_buf_get_type(returned_buf);
	zassert_equal(returned_buffer_type, BT_BUF_EVT,
		      "bt_buf_get_cmd_complete() returned incorrect buffer type %u, expected %u (%s)",
		      returned_buffer_type, BT_BUF_EVT, STRINGIFY(BT_BUF_EVT));

	zassert_equal(bt_dev.sent_cmd, NULL,
		     "bt_buf_get_cmd_complete() didn't clear bt_dev.sent_cmd");
}
