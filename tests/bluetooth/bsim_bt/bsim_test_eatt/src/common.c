/* common.c - Common test code */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

struct bt_conn *default_conn;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static volatile bool is_connected;
static volatile bool is_encrypted;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		if (default_conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;
		}

		FAIL("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	if (!default_conn) {
		default_conn = bt_conn_ref(conn);
	}

	printk("Connected: %s\n", addr);
	is_connected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
	is_connected = false;
	is_encrypted = false;
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err security_err)
{
	if (security_err == BT_SECURITY_ERR_SUCCESS && level > BT_SECURITY_L1) {
		is_encrypted = true;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop LE scan failed (err %d)\n", err);
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		FAIL("Create conn failed (err %d)\n", err);
	}

	printk("Device connected\n");
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(60e6); /* 60 seconds */
	bst_result = In_progress;
}

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error_time_line("Test eatt finished.\n");
	}
}

void central_setup_and_connect(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Can't enable Bluetooth (err %d)\n", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to start encryption procedure\n");
	}

	while (!is_encrypted) {
		k_sleep(K_MSEC(100));
	}
}

void peripheral_setup_and_connect(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Can't enable Bluetooth (err %d)\n", err);
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
	}

	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}

	/* Wait for central to start encryption */
	while (!is_encrypted) {
		k_sleep(K_MSEC(100));
	}
}

void wait_for_disconnect(void)
{
	while (is_connected) {
		k_sleep(K_MSEC(100));
	}
}

void disconnect(void)
{
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		FAIL("Disconnection failed (err %d)\n", err);
	}

	while (is_connected) {
		k_sleep(K_MSEC(100));
	}
}
