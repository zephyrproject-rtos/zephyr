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

struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool, k_timeout_t timeout)
{
	static uint8_t call_counter;
	struct net_buf *buf = NULL;

	if (call_counter++ == 0) {
		buf = (struct net_buf *)ztest_get_return_value_ptr();
	} else {
		ztest_test_fail();
		buf = NULL;
	}
	return buf;
}

struct net_buf *dummy_call_function(void)
{
	return (struct net_buf *)ztest_get_return_value_ptr();
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	ztest_check_expected_value(buf);
	return buf;
}

void test_bt_buf_get_cmd_complete_returns_not_null(void)
{
	static struct net_buf test_reference;
	struct net_buf *buf;

	bt_dev.sent_cmd = NULL;
	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	buf = bt_buf_get_cmd_complete(Z_TIMEOUT_TICKS(1000));
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");

	bt_dev.sent_cmd = &test_reference;
	ztest_expect_value(net_buf_ref, buf, bt_dev.sent_cmd);
	buf = bt_buf_get_cmd_complete(Z_TIMEOUT_TICKS(1000));
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");
}

void test_main(void)
{
	ztest_test_suite(public,
			ztest_unit_test(test_bt_buf_get_cmd_complete_returns_not_null)
			);

	ztest_run_test_suite(public);
}
