/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "utils.h"

BUILD_ASSERT(CONFIG_BT_MAX_PAIRED >= 2, "CONFIG_BT_MAX_PAIRED is too small.");
BUILD_ASSERT(CONFIG_BT_ID_MAX >= 3, "CONFIG_BT_ID_MAX is too small.");
BUILD_ASSERT(CONFIG_BT_MAX_CONN == 2, "CONFIG_BT_MAX_CONN should be equal to two.");
BUILD_ASSERT(CONFIG_BT_GATT_CLIENT, "CONFIG_BT_GATT_CLIENT is disabled.");

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

DEFINE_FLAG(flag_has_new_conn);
struct bt_conn *new_conn;

DEFINE_FLAG(flag_has_disconnected);

void clear_conn(struct bt_conn *conn)
{
	if (new_conn == conn) {
		new_conn = NULL;
	}

	ASSERT(conn, "Test error: No new_conn!\n");
	bt_conn_unref(conn);
}

void wait_connected(struct bt_conn **conn)
{
	WAIT_FOR_FLAG(flag_has_new_conn);
	UNSET_FLAG(flag_has_new_conn);

	ASSERT(new_conn, "connection unpopulated.");
	*conn = new_conn;
	new_conn = NULL;
}

void wait_disconnected(void)
{
	WAIT_FOR_FLAG(flag_has_disconnected);
	UNSET_FLAG(flag_has_disconnected);
}

static void print_conn_state_transition(const char *prefix, struct bt_conn *conn)
{
	int err;
	struct bt_conn_info info;
	char addr_str[BT_ADDR_LE_STR_LEN];

	err = bt_conn_get_info(conn, &info);
	ASSERT(!err, "Unexpected conn info result.");

	bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));
	printk("%s: %s\n", prefix, addr_str);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	print_conn_state_transition("Disonnected", conn);
	SET_FLAG(flag_has_disconnected);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ASSERT((!new_conn || (conn == new_conn)), "Unexpected new connection.");

	if (!new_conn) {
		new_conn = bt_conn_ref(conn);
	}

	if (err != 0) {
		clear_conn(conn);
		return;
	}

	print_conn_state_transition("Connected", conn);
	SET_FLAG(flag_has_new_conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

DEFINE_FLAG(flag_pairing_completed);

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	print_conn_state_transition("Paired", conn);
	SET_FLAG(flag_pairing_completed);
}

static struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {
	.pairing_complete = pairing_complete,
};

void set_security(struct bt_conn *conn, bt_security_t sec)
{
	int err;

	err = bt_conn_set_security(conn, sec);
	ASSERT(!err, "Err bt_conn_set_security %d", err);
}

void wait_pairing_completed(void)
{
	WAIT_FOR_FLAG(flag_pairing_completed);
	UNSET_FLAG(flag_pairing_completed);
}

void bs_bt_utils_setup(void)
{
	int err;

	err = bt_enable(NULL);
	ASSERT(!err, "bt_enable failed.\n");
	err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);
	ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	err = settings_load();
	ASSERT(!err, "settings_load failed.\n");
}

static void scan_connect_to_first_result__device_found(const bt_addr_le_t *addr, int8_t rssi,
						       uint8_t type, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		FAIL("Unexpected advertisement type.");
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Got scan result, connecting.. dst %s, RSSI %d\n", addr_str, rssi);

	err = bt_le_scan_stop();
	ASSERT(!err, "Err bt_le_scan_stop %d", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &new_conn);
	ASSERT(!err, "Err bt_conn_le_create %d", err);
}

void scan_connect_to_first_result(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_connect_to_first_result__device_found);
	ASSERT(!err, "Err bt_le_scan_start %d", err);
}

void disconnect(struct bt_conn *conn)
{
	int err;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	ASSERT(!err, "Err bt_conn_disconnect %d", err);
}

DEFINE_FLAG(flag_bas_has_notification);

static uint8_t bas_level = 50;

static uint8_t bas_notify_func(struct bt_conn *conn,
			       struct bt_gatt_subscribe_params *params,
			       const void *data, uint16_t length)
{
	const uint8_t *lvl8 = data;

	if ((length == 1) && (*lvl8 == bas_level)) {
		printk("BAS notification\n");
		SET_FLAG(flag_bas_has_notification);
	}

	return BT_GATT_ITER_CONTINUE;
}

void wait_bas_notification(void)
{
	WAIT_FOR_FLAG(flag_bas_has_notification);
	UNSET_FLAG(flag_bas_has_notification);
}

/* Not actually used, see below why we also have this on the central */
BT_GATT_SERVICE_DEFINE(bas,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, NULL, NULL, &bas_level),
	BT_GATT_CCC(NULL,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void bas_subscribe(struct bt_conn *conn)
{
	int err;
	static struct bt_gatt_subscribe_params subscribe_params = {0};

	/* This is a bit of a shortcut: to skip discovery, we assume the handles
	 * will be the same on the central & peripheral images.
	 */
	subscribe_params.ccc_handle = bt_gatt_attr_get_handle(&bas.attrs[3]);
	subscribe_params.value_handle = bt_gatt_attr_get_handle(&bas.attrs[2]);
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.notify = bas_notify_func;

	err = bt_gatt_subscribe(conn, &subscribe_params);
	ASSERT(!err, "bt_gatt_subscribe failed (err %d)\n", err);
}
