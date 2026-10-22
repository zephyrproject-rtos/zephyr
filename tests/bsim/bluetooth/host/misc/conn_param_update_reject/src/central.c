/*
 * Central (tester) side of the conn_param_update_reject bsim test.
 *
 * Copyright (c) 2026 Sharon Lin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>

#include <testlib/conn.h>
#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_param_req_received);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	printk("Connected to %s\n", bt_conn_dst_str(conn));

	__ASSERT_NO_MSG(g_conn == NULL);
	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	printk("Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	bt_conn_unref(g_conn);
	g_conn = NULL;
	UNSET_FLAG(flag_is_connected);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	printk("Rejecting connection parameter update request from %s\n", bt_conn_dst_str(conn));

	SET_FLAG(flag_param_req_received);

	return false;
}

static struct bt_conn_cb tester_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;
	struct bt_conn *conn;

	if (g_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	printk("Device found: %s (RSSI %d)\n", bt_addr_le_str(addr), rssi);

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
		return;
	}

	bt_conn_unref(conn);
}

void test_central_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	err = bk_sync_init();
	if (err != 0) {
		TEST_FAIL("Failed to initialize backchannel (err %d)", err);
		return;
	}

	err = bt_conn_cb_register(&tester_callbacks);
	__ASSERT_NO_MSG(err == 0);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_is_connected);

	/* Wait for the peripheral to finish its test */
	bk_sync_wait();

	if (!IS_FLAG_SET(flag_param_req_received)) {
		TEST_FAIL("Central never received a parameter update request");
		return;
	}

	TEST_PASS("Central rejected the connection parameter update");
}
