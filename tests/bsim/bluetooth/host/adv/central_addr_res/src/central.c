/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_bonded);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	__ASSERT_NO_MSG(g_conn == conn);

	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	bt_conn_drop(&g_conn);
	UNSET_FLAG(flag_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	if (!bonded) {
		TEST_FAIL("Pairing did not create a bond");
		return;
	}

	SET_FLAG(flag_bonded);
}

static struct bt_conn_auth_info_cb auth_info_callbacks = {
	.pairing_complete = pairing_complete,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (g_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
	}
}

static void test_central(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_conn_auth_info_cb_register(&auth_info_callbacks);
	TEST_ASSERT(err == 0, "bt_conn_auth_info_cb_register failed (%d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (%d)", err);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	TEST_ASSERT(err == 0, "bt_le_scan_start failed (%d)", err);

	WAIT_FOR_FLAG(flag_connected);

	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "bt_conn_set_security failed (%d)", err);

	WAIT_FOR_FLAG(flag_bonded);

	/* Stay connected long enough for the peripheral to read the Central
	 * Address Resolution characteristic.
	 */
	k_sleep(K_SECONDS(5));

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "bt_conn_disconnect failed (%d)", err);

	WAIT_FOR_FLAG_UNSET(flag_connected);

	TEST_PASS("Central passed");
}

static const struct bst_test_instance test_inst[] = {
	{
		.test_id = "central",
		.test_main_f = test_central,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_inst);
}
