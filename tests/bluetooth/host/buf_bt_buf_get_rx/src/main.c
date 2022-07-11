/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

void test_bt_buf_get_rx_returns_null(void)
{
	struct net_buf *buf;

	ztest_returns_value(net_buf_alloc_fixed, NULL);
	buf = bt_buf_get_rx(BT_BUF_EVT, Z_TIMEOUT_TICKS(1000));
	zassert_is_null(buf, "Return value was not NULL");

	ztest_returns_value(net_buf_alloc_fixed, NULL);
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, Z_TIMEOUT_TICKS(1000));
	zassert_is_null(buf, "Return value was not NULL");
}

void test_bt_buf_get_rx_returns_not_null(void)
{
	static struct net_buf test_reference;
	struct net_buf *buf;

	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	buf = bt_buf_get_rx(BT_BUF_EVT, Z_TIMEOUT_TICKS(1000));
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");

	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, Z_TIMEOUT_TICKS(1000));
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_ACL_IN, "Incorrect type");
}

void test_main(void)
{
	ztest_test_suite(public,
			ztest_unit_test(test_bt_buf_get_rx_returns_null),
			ztest_unit_test(test_bt_buf_get_rx_returns_not_null)
			);

	ztest_run_test_suite(public);
}
