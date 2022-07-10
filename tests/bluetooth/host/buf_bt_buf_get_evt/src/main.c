/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

#include <zephyr/bluetooth/hci.h>
#include <host/hci_core.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

static uint8_t net_buf_alloc_fixed_call_counter;

struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool, k_timeout_t timeout)
{
	struct net_buf *buf = NULL;

	if (net_buf_alloc_fixed_call_counter++ == 0) {
		buf = (struct net_buf *)ztest_get_return_value_ptr();
	} else {
		ztest_test_fail();
		buf = NULL;
	}
	return buf;
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	ztest_check_expected_value(buf);
	return buf;
}

static void unit_test_setup(void)
{
	net_buf_alloc_fixed_call_counter = 0;
}

void bt_buf_get_evt_returns_not_null(uint8_t evt, bool discardable)
{
	static struct net_buf test_reference;
	struct net_buf *buf;

	bt_dev.sent_cmd = NULL;
	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	buf = bt_buf_get_evt(evt, discardable, Z_TIMEOUT_TICKS(1000));
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");

	bt_dev.sent_cmd = &test_reference;
	ztest_expect_value(net_buf_ref, buf, bt_dev.sent_cmd);
	buf = bt_buf_get_evt(evt, discardable, Z_TIMEOUT_TICKS(1000));
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");
}

void test_bt_buf_get_evt_cmd_complete(void)
{
	unit_test_setup();
	bt_buf_get_evt_returns_not_null(BT_HCI_EVT_CMD_COMPLETE, true);

	unit_test_setup();
	bt_buf_get_evt_returns_not_null(BT_HCI_EVT_CMD_COMPLETE, false);
}

void test_main(void)
{
	ztest_test_suite(public,
			ztest_unit_test(test_bt_buf_get_evt_cmd_complete)
			);

	ztest_run_test_suite(public);
}
