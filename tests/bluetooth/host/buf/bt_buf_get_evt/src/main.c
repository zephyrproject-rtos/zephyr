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

void bt_buf_get_evt_returns_not_null_cmd_complete(uint8_t evt)
{
	static struct net_buf test_reference;
	struct net_buf *buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	bt_dev.sent_cmd = NULL;
	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_evt(evt, true, timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");

	bt_dev.sent_cmd = &test_reference;
	ztest_expect_value(net_buf_ref, buf, bt_dev.sent_cmd);
	buf = bt_buf_get_evt(evt, false, timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");
}

void bt_buf_get_evt_returns_null_default(uint8_t evt)
{
	struct net_buf *buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	ztest_returns_value(net_buf_alloc_fixed, NULL);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_evt(evt, true, timeout);
	zassert_is_null(buf, "Return value was NULL");

	ztest_returns_value(net_buf_alloc_fixed, NULL);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_evt(evt, false, timeout);
	zassert_is_null(buf, "Return value was NULL");
}

void bt_buf_get_evt_returns_not_null_default(uint8_t evt)
{
	static struct net_buf test_reference;
	struct net_buf *buf;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(1000);

	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_evt(evt, true, timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");

	ztest_returns_value(net_buf_alloc_fixed, &test_reference);
	ztest_expect_value(hooks_net_buf_alloc_fixed_timeout_validation_hook, value, timeout.ticks);
	buf = bt_buf_get_evt(evt, false, timeout);
	zassert_not_null(buf, "Return value was NULL");
	zassert_equal(buf, &test_reference, "Incorrect value");
	zassert_equal(bt_buf_get_type(buf), BT_BUF_EVT, "Incorrect type");
}

void test_bt_buf_get_evt_cmd_complete(void)
{
	bt_buf_get_evt_returns_not_null_cmd_complete(BT_HCI_EVT_CMD_COMPLETE);
	bt_buf_get_evt_returns_not_null_cmd_complete(BT_HCI_EVT_CMD_STATUS);
}

void test_bt_buf_get_evt_default(void)
{
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_UNKNOWN);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_VENDOR);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_INQUIRY_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_CONN_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_CONN_REQUEST);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_DISCONN_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_AUTH_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_ENCRYPT_CHANGE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_REMOTE_FEATURES);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_REMOTE_VERSION_INFO);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_HARDWARE_ERROR);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_ROLE_CHANGE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_NUM_COMPLETED_PACKETS);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_PIN_CODE_REQ);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_LINK_KEY_REQ);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_LINK_KEY_NOTIFY);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_DATA_BUF_OVERFLOW);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_REMOTE_EXT_FEATURES);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_SYNC_CONN_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_EXTENDED_INQUIRY_RESULT);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_IO_CAPA_REQ);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_IO_CAPA_RESP);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_USER_CONFIRM_REQ);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_USER_PASSKEY_REQ);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_SSP_COMPLETE);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_USER_PASSKEY_NOTIFY);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_LE_META_EVENT);
	bt_buf_get_evt_returns_not_null_default(BT_HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXP);

	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_UNKNOWN);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_VENDOR);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_INQUIRY_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_CONN_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_CONN_REQUEST);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_DISCONN_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_AUTH_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_ENCRYPT_CHANGE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_REMOTE_FEATURES);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_REMOTE_VERSION_INFO);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_HARDWARE_ERROR);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_ROLE_CHANGE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_NUM_COMPLETED_PACKETS);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_PIN_CODE_REQ);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_LINK_KEY_REQ);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_LINK_KEY_NOTIFY);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_DATA_BUF_OVERFLOW);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_REMOTE_EXT_FEATURES);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_SYNC_CONN_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_EXTENDED_INQUIRY_RESULT);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_IO_CAPA_REQ);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_IO_CAPA_RESP);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_USER_CONFIRM_REQ);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_USER_PASSKEY_REQ);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_SSP_COMPLETE);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_USER_PASSKEY_NOTIFY);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_LE_META_EVENT);
	bt_buf_get_evt_returns_null_default(BT_HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXP);
}

void test_main(void)
{
	ztest_test_suite(default,
			ztest_unit_test(test_bt_buf_get_evt_default),
			ztest_unit_test(test_bt_buf_get_evt_cmd_complete)
			);

	ztest_run_test_suite(default);
}
