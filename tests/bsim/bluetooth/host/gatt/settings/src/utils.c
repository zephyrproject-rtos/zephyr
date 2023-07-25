/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"
#include "gatt_utils.h"
#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/hci.h>

DEFINE_FLAG(flag_is_connected);
DEFINE_FLAG(flag_test_end);

void wait_connected(void)
{
	UNSET_FLAG(flag_is_connected);
	WAIT_FOR_FLAG(flag_is_connected);
	printk("connected\n");

}

void wait_disconnected(void)
{
	SET_FLAG(flag_is_connected);
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
	printk("disconnected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_unref(conn);
	UNSET_FLAG(flag_is_connected);
	gatt_clear_flags();
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		return;
	}

	bt_conn_ref(conn);
	SET_FLAG(flag_is_connected);
}

DEFINE_FLAG(flag_encrypted);

void security_changed(struct bt_conn *conn, bt_security_t level,
		      enum bt_security_err err)
{
	__ASSERT(err == 0, "Error setting security (err %u)\n", err);

	printk("Encrypted\n");
	SET_FLAG(flag_encrypted);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void scan_connect_to_first_result_device_found(const bt_addr_le_t *addr, int8_t rssi,
						      uint8_t type, struct net_buf_simple *ad)
{
	struct bt_conn *conn;
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		FAIL("Unexpected advertisement type.");
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Got scan result, connecting.. dst %s, RSSI %d\n",
	       addr_str, rssi);

	err = bt_le_scan_stop();
	__ASSERT(!err, "Err bt_le_scan_stop %d", err);

	err = bt_conn_le_create(addr,
				BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&conn);
	__ASSERT(!err, "Err bt_conn_le_create %d", err);
}

void scan_connect_to_first_result(void)
{
	int err;

	printk("start scanner\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE,
			       scan_connect_to_first_result_device_found);
	__ASSERT(!err, "Err bt_le_scan_start %d", err);
}

void advertise_connectable(void)
{
	printk("start advertiser\n");
	int err;
	struct bt_le_adv_param param = {};

	param.interval_min = 0x0020;
	param.interval_max = 0x4000;
	param.options |= BT_LE_ADV_OPT_ONE_TIME;
	param.options |= BT_LE_ADV_OPT_CONNECTABLE;

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	__ASSERT(err == 0, "Advertising failed to start (err %d)\n", err);
}

void disconnect(struct bt_conn *conn)
{
	int err;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT(!err, "Failed to initate disconnection (err %d)", err);

	printk("Waiting for disconnection...\n");
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
}

static void get_active_conn_cb(struct bt_conn *src, void *dst)
{
	*(struct bt_conn **)dst = src;
}

struct bt_conn *get_conn(void)
{
	struct bt_conn *ret;

	bt_conn_foreach(BT_CONN_TYPE_LE, get_active_conn_cb, &ret);

	return ret;
}

DEFINE_FLAG(flag_pairing_complete);

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	FAIL("Pairing failed (unexpected): reason %u", reason);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	__ASSERT(bonded, "Bonding failed\n");

	printk("Paired\n");
	SET_FLAG(flag_pairing_complete);
}

static struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {
	.pairing_failed = pairing_failed,
	.pairing_complete = pairing_complete,
};

void set_security(struct bt_conn *conn, bt_security_t sec)
{
	int err;

	UNSET_FLAG(flag_encrypted);

	err = bt_conn_set_security(conn, sec);
	__ASSERT(!err, "Err bt_conn_set_security %d", err);

	WAIT_FOR_FLAG(flag_encrypted);
}

void wait_secured(void)
{
	UNSET_FLAG(flag_encrypted);
	WAIT_FOR_FLAG(flag_encrypted);
}

void bond(struct bt_conn *conn)
{
	UNSET_FLAG(flag_pairing_complete);

	int err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);

	__ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	set_security(conn, BT_SECURITY_L2);

	WAIT_FOR_FLAG(flag_pairing_complete);
}

void wait_bonded(void)
{
	UNSET_FLAG(flag_encrypted);
	UNSET_FLAG(flag_pairing_complete);

	int err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);

	__ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	WAIT_FOR_FLAG(flag_encrypted);
	WAIT_FOR_FLAG(flag_pairing_complete);
}

struct bt_conn *connect_as_central(void)
{
	struct bt_conn *conn;

	scan_connect_to_first_result();
	wait_connected();
	conn = get_conn();

	return conn;
}

struct bt_conn *connect_as_peripheral(void)
{
	struct bt_conn *conn;

	advertise_connectable();
	wait_connected();
	conn = get_conn();

	return conn;
}
