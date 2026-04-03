/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"

BUILD_ASSERT(CONFIG_BT_MAX_PAIRED >= 2, "CONFIG_BT_MAX_PAIRED is too small.");
BUILD_ASSERT(CONFIG_BT_ID_MAX >= 3, "CONFIG_BT_ID_MAX is too small.");


DEFINE_FLAG(flag_is_connected);
struct bt_conn *g_conn;
DEFINE_FLAG(bondable);
DEFINE_FLAG(call_bt_conn_set_bondable);

void wait_connected(void)
{
	WAIT_FOR_FLAG(flag_is_connected);
}

void wait_disconnected(void)
{
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	UNSET_FLAG(flag_is_connected);
}

BUILD_ASSERT(CONFIG_BT_MAX_CONN == 1, "This test assumes a single link.");
static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT((!g_conn || (conn == g_conn)), "Unexpected new connection.");

	if (!g_conn) {
		g_conn = bt_conn_ref(conn);
	}

	if (err != 0) {
		clear_g_conn();
		return;
	}

	SET_FLAG(flag_is_connected);

	if (IS_FLAG_SET(call_bt_conn_set_bondable) &&
	    bt_conn_set_bondable(conn, IS_FLAG_SET(bondable))) {
		TEST_ASSERT(0, "Fail during setting bondable flag for given connection.");
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void clear_g_conn(void)
{
	struct bt_conn *conn;

	conn = g_conn;
	g_conn = NULL;
	TEST_ASSERT(conn, "Test error: No g_conn!");
	bt_conn_unref(conn);
}

/* The following flags are raised by events and lowered by test code. */
DEFINE_FLAG(flag_pairing_complete);
DEFINE_FLAG(flag_bonded);
DEFINE_FLAG(flag_not_bonded);

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	TEST_FAIL("Pairing failed (unexpected): reason %u", reason);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	SET_FLAG(flag_pairing_complete);

	if (bonded) {
		SET_FLAG(flag_bonded);
	} else {
		SET_FLAG(flag_not_bonded);
	}
}

static struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {
	.pairing_failed = pairing_failed,
	.pairing_complete = pairing_complete,
};

void bs_bt_utils_setup(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "bt_enable failed.");
	err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);
	TEST_ASSERT(!err, "bt_conn_auth_info_cb_register failed.");
}

static void scan_connect_to_first_result__device_found(const bt_addr_le_t *addr, int8_t rssi,
						       uint8_t type, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (g_conn != NULL) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		TEST_FAIL("Unexpected advertisement type.");
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Got scan result, connecting.. dst %s, RSSI %d\n", addr_str, rssi);

	err = bt_le_scan_stop();
	TEST_ASSERT(!err, "Err bt_le_scan_stop %d", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	TEST_ASSERT(!err, "Err bt_conn_le_create %d", err);
}

void scan_connect_to_first_result(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_connect_to_first_result__device_found);
	TEST_ASSERT(!err, "Err bt_le_scan_start %d", err);
}

void disconnect(void)
{
	int err;

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(!err, "Err bt_conn_disconnect %d", err);
}

void unpair(int id)
{
	int err;

	err = bt_unpair(id, BT_ADDR_LE_ANY);
	TEST_ASSERT(!err, "Err bt_unpair %d", err);
}

void set_security(bt_security_t sec)
{
	int err;

	err = bt_conn_set_security(g_conn, sec);
	TEST_ASSERT(!err, "Err bt_conn_set_security %d", err);
}

void advertise_connectable(int id, bt_addr_le_t *directed_dst)
{
	int err;
	struct bt_le_adv_param param = {};

	param.id = id;
	param.interval_min = 0x0020;
	param.interval_max = 0x4000;
	param.options |= BT_LE_ADV_OPT_CONN;

	if (directed_dst) {
		param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
		param.peer = directed_dst;
	}

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);
}

void set_bondable(bool enable)
{
	if (enable) {
		SET_FLAG(bondable);
	} else {
		UNSET_FLAG(bondable);
	}
}

void enable_bt_conn_set_bondable(bool enable)
{
	if (enable) {
		SET_FLAG(call_bt_conn_set_bondable);
	} else {
		UNSET_FLAG(call_bt_conn_set_bondable);
	}
}
