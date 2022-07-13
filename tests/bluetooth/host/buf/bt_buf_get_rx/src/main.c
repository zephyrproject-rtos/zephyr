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

static bool assetion_expected_flag;

void assert_post_action(const char *file, unsigned int line)
{
	printk("Assert error expected as part of test case.\n");
	printk("file : %s\n", file);
	printk("line : %d\n", line);

	if (assetion_expected_flag == true) {
		ztest_test_pass();
	} else {
		ztest_test_fail();
	}
	assetion_expected_flag = false;
}

void test_bt_buf_get_rx_invalid_input_type_bt_buf_cmd(void)
{
	assetion_expected_flag = true;
	bt_buf_get_rx(BT_BUF_CMD, Z_TIMEOUT_TICKS(1000));
	zassert_false(assetion_expected_flag, "Flag value is incorrect");
}

void test_bt_buf_get_rx_invalid_input_type_bt_buf_acl_out(void)
{
	assetion_expected_flag = true;
	bt_buf_get_rx(BT_BUF_ACL_OUT, Z_TIMEOUT_TICKS(1000));
	zassert_false(assetion_expected_flag, "Flag value is incorrect");
}

void test_bt_buf_get_rx_invalid_input_type_bt_buf_iso_out(void)
{
	assetion_expected_flag = true;
	bt_buf_get_rx(BT_BUF_ISO_OUT, Z_TIMEOUT_TICKS(1000));
	zassert_false(assetion_expected_flag, "Flag value is incorrect");
}

void test_bt_buf_get_rx_invalid_input_type_bt_buf_h4(void)
{
	assetion_expected_flag = true;
	bt_buf_get_rx(BT_BUF_H4, Z_TIMEOUT_TICKS(1000));
	zassert_false(assetion_expected_flag, "Flag value is incorrect");
}

void test_bt_buf_get_rx_returns_null(void)
{
	struct net_buf *buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	ztest_returns_value(net_buf_alloc_fixed, NULL);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_rx(BT_BUF_EVT, timeout);
	zassert_is_null(buf, "Return value was not NULL");

	ztest_returns_value(net_buf_alloc_fixed, NULL);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, timeout);
	zassert_is_null(buf, "Return value was not NULL");

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_SYNC_RECEIVER)
	ztest_returns_value(net_buf_alloc_fixed, NULL);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_rx(BT_BUF_ISO_IN, timeout);
	zassert_is_null(buf, "Return value was not NULL");
#endif
}

void test_bt_buf_get_rx_returns_not_null(void)
{
	static struct net_buf test_reference;
	struct net_buf *buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_rx(BT_BUF_EVT, timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");

	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_ACL_IN, "Incorrect type");

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_SYNC_RECEIVER)
	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_rx(BT_BUF_ISO_IN, timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_ISO_IN, "Incorrect type");
#endif
}

void test_main(void)
{
	ztest_test_suite(default,
			ztest_unit_test(test_bt_buf_get_rx_returns_null),
			ztest_unit_test(test_bt_buf_get_rx_returns_not_null),
			ztest_unit_test(test_bt_buf_get_rx_invalid_input_type_bt_buf_cmd),
			ztest_unit_test(test_bt_buf_get_rx_invalid_input_type_bt_buf_acl_out),
			ztest_unit_test(test_bt_buf_get_rx_invalid_input_type_bt_buf_iso_out),
			ztest_unit_test(test_bt_buf_get_rx_invalid_input_type_bt_buf_h4)
			);

	assetion_expected_flag = false;
	ztest_run_test_suite(default);
}
