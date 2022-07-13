/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"
#include "hooks.h"

#include <zephyr/bluetooth/hci.h>
#include <host/hci_core.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

void test_bt_buf_get_cmd_complete_returns_not_null(void)
{
	static struct net_buf test_reference;
	struct net_buf *buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	bt_dev.sent_cmd = NULL;
	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_cmd_complete(timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");

	bt_dev.sent_cmd = &test_reference;
	ztest_expect_value(net_buf_ref, buf, bt_dev.sent_cmd);
	buf = bt_buf_get_cmd_complete(timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");
}

void test_main(void)
{
	ztest_test_suite(default,
			ztest_unit_test(test_bt_buf_get_cmd_complete_returns_not_null)
			);

	ztest_run_test_suite(default);
}
