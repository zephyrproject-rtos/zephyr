/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>

#include <zephyr/settings/settings.h>

#include "common.h"
#include "settings.h"

#include "argparse.h"

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"

#define SERVER_CHAN 0

DEFINE_FLAG_STATIC(connected_flag);
DEFINE_FLAG_STATIC(disconnected_flag);
DEFINE_FLAG_STATIC(security_updated_flag);
DEFINE_FLAG_STATIC(notification_received_flag);

#define BT_UUID_DUMMY_SERVICE BT_UUID_DECLARE_128(DUMMY_SERVICE_TYPE)
#define BT_UUID_DUMMY_SERVICE_NOTIFY BT_UUID_DECLARE_128(DUMMY_SERVICE_NOTIFY_TYPE)

static struct bt_conn *default_conn;

static struct bt_conn_cb central_cb;

DEFINE_FLAG_STATIC(gatt_subscribed_flag);

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	uint8_t value;

	if (conn == NULL || data == NULL) {
		/* Peer unpaired or subscription was removed */
		UNSET_FLAG(gatt_subscribed_flag);

		return BT_GATT_ITER_STOP;
	}

	__ASSERT_NO_MSG(length == sizeof(value));

	value = *(uint8_t *)data;

	LOG_DBG("#%d notification received", value);

	SET_FLAG(notification_received_flag);

	return BT_GATT_ITER_CONTINUE;
}

static void subscribe_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	if (err) {
		return;
	}

	SET_FLAG(gatt_subscribed_flag);
}

static struct bt_gatt_subscribe_params subscribe_params;

static void ccc_subscribe(void)
{
	int err;

	UNSET_FLAG(gatt_subscribed_flag);

	subscribe_params.notify = notify_cb;
	subscribe_params.subscribe = subscribe_cb;
	subscribe_params.ccc_handle = CCC_HANDLE;
	subscribe_params.value_handle = VAL_HANDLE;
	subscribe_params.value = BT_GATT_CCC_NOTIFY;

	err = bt_gatt_subscribe(default_conn, &subscribe_params);
	if (err) {
		TEST_FAIL("Failed to subscribe (att err %d)", err);
	}

	WAIT_FOR_FLAG(gatt_subscribed_flag);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	LOG_DBG("Device found: %s (RSSI %d)", addr_str, rssi);

	err = bt_le_scan_stop();
	if (err) {
		TEST_FAIL("Failed to stop scanner (err %d)", err);
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		TEST_FAIL("Could not connect to peer: %s (err %d)", addr_str, err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	const bt_addr_le_t *addr;
	char addr_str[BT_ADDR_LE_STR_LEN];

	addr = bt_conn_get_dst(conn);
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if (err) {
		TEST_FAIL("Failed to connect to %s (err %d)", addr_str, err);
	}

	LOG_DBG("Connected: %s", addr_str);

	if (conn == default_conn) {
		SET_FLAG(connected_flag);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr_str, reason);

	SET_FLAG(disconnected_flag);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	if (!err) {
		LOG_DBG("Security changed: %s level %u", addr_str, level);
		SET_FLAG(security_updated_flag);
	} else {
		LOG_DBG("Security failed: %s level %u err %d", addr_str, level, err);
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	LOG_DBG("Scanning successfully started");
}

static void disconnect(void)
{
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		TEST_FAIL("Disconnection failed (err %d)", err);
	}

	WAIT_FOR_FLAG(disconnected_flag);
	UNSET_FLAG(disconnected_flag);
}

/* Test steps */

static void connect_pair_subscribe(void)
{
	int err;

	start_scan();

	WAIT_FOR_FLAG(connected_flag);
	UNSET_FLAG(connected_flag);

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err != 0) {
		TEST_FAIL("Failed to set security (err %d)", err);
	}

	WAIT_FOR_FLAG(security_updated_flag);
	UNSET_FLAG(security_updated_flag);

	/* subscribe while being paired */
	ccc_subscribe();

	/* confirm to server that we subscribed */
	bk_sync_send();
	/* wait for server to check that the subscribtion is well registered */
	bk_sync_wait();

	WAIT_FOR_FLAG(notification_received_flag);
	UNSET_FLAG(notification_received_flag);
}

static void connect_restore_sec(void)
{
	int err;

	start_scan();

	WAIT_FOR_FLAG(connected_flag);
	UNSET_FLAG(connected_flag);

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err != 0) {
		TEST_FAIL("Failed to set security (err %d)", err);
	}

	WAIT_FOR_FLAG(security_updated_flag);
	UNSET_FLAG(security_updated_flag);

	/* check local subscription state */
	if (!IS_FLAG_SET(gatt_subscribed_flag)) {
		TEST_FAIL("Not subscribed");
	}

	/* notify the end of security update to server */
	bk_sync_send();
	/* wait for server to check that the subscribtion has been restored */
	bk_sync_wait();

	WAIT_FOR_FLAG(notification_received_flag);
	UNSET_FLAG(notification_received_flag);
}

/* Util functions */

static void set_public_addr(void)
{
	bt_addr_le_t addr = {BT_ADDR_LE_RANDOM, {{0x0A, 0x89, 0x67, 0x45, 0x23, 0xC1}}};

	bt_id_create(&addr, NULL);
}

/* Main functions */

void run_central(int times)
{
	int err;

	central_cb.connected = connected;
	central_cb.disconnected = disconnected;
	central_cb.security_changed = security_changed;

	TEST_ASSERT(bk_sync_init() == 0);
	set_public_addr();

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	LOG_DBG("Bluetooth initialized");

	bt_conn_cb_register(&central_cb);

	err = settings_load();
	if (err) {
		TEST_FAIL("Settings load failed (err %d)", err);
	}

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err) {
		TEST_FAIL("Unpairing failed (err %d)", err);
	}

	connect_pair_subscribe();
	disconnect();

	for (int i = 0; i < times; i++) {
		connect_restore_sec();
		disconnect();
	}

	TEST_PASS("Central test passed");
}
