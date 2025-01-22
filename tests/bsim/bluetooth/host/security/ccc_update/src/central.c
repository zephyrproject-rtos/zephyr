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

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "common.h"
#include "settings.h"

#include "argparse.h"
#include "bs_pc_backchannel.h"

#define CLIENT_CLIENT_CHAN 0
#define SERVER_CLIENT_CHAN 1

static DEFINE_FLAG(connected_flag);
static DEFINE_FLAG(disconnected_flag);
static DEFINE_FLAG(security_updated_flag);

#define BT_UUID_DUMMY_SERVICE BT_UUID_DECLARE_128(DUMMY_SERVICE_TYPE)
#define BT_UUID_DUMMY_SERVICE_NOTIFY BT_UUID_DECLARE_128(DUMMY_SERVICE_NOTIFY_TYPE)

static struct bt_conn *default_conn;

static struct bt_conn_cb central_cb;

static DEFINE_FLAG(gatt_write_flag);
static uint8_t gatt_write_att_err;

static void gatt_write_cb(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_write_params *params)
{
	gatt_write_att_err = att_err;

	if (att_err) {
		TEST_FAIL("GATT write ATT error (err %d)", att_err);
	}

	SET_FLAG(gatt_write_flag);
}

static int gatt_write(struct bt_conn *conn, uint16_t handle, const uint8_t *write_buf,
		      size_t write_size)
{
	int err;
	struct bt_gatt_write_params params;

	params.func = gatt_write_cb;
	params.handle = handle;
	params.offset = 0;
	params.data = write_buf;
	params.length = write_size;

	UNSET_FLAG(gatt_write_flag);

	/* `bt_gatt_write` is used instead of `bt_gatt_subscribe` and
	 * `bt_gatt_unsubscribe` to bypass subscribtion checks of GATT client
	 */
	err = bt_gatt_write(conn, &params);
	if (err) {
		TEST_FAIL("GATT write failed (err %d)", err);
	}

	WAIT_FOR_FLAG(gatt_write_flag);

	return gatt_write_att_err;
}

static void ccc_subscribe(void)
{
	int err;
	uint8_t buf = 1;

	err = gatt_write(default_conn, CCC_HANDLE, &buf, sizeof(buf));
	if (err) {
		TEST_FAIL("Failed to subscribe (att err %d)", err);
	}
}

static void ccc_unsubscribe(void)
{
	int err;
	uint8_t buf = 0;

	err = gatt_write(default_conn, CCC_HANDLE, &buf, sizeof(buf));
	if (err) {
		TEST_FAIL("Failed to unsubscribe (att err %d)", err);
	}
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
	backchannel_sync_send(SERVER_CLIENT_CHAN, SERVER_ID);
	/* wait for server to check that the subscribtion is well registered */
	backchannel_sync_wait(SERVER_CLIENT_CHAN, SERVER_ID);
}

static void connect_unsubscribe(void)
{
	start_scan();

	WAIT_FOR_FLAG(connected_flag);
	UNSET_FLAG(connected_flag);

	/* wait for server to check that the subscribtion has not been restored */
	backchannel_sync_wait(SERVER_CLIENT_CHAN, SERVER_ID);

	LOG_DBG("Trying to unsubscribe without being paired...");
	/* try to unsubscribe */
	ccc_unsubscribe();

	/* confirm to server that we send unsubscribtion request */
	backchannel_sync_send(SERVER_CLIENT_CHAN, SERVER_ID);
	/* wait for server to check that the unsubscribtion is ignored */
	backchannel_sync_wait(SERVER_CLIENT_CHAN, SERVER_ID);
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

	/* notify the end of security update to server */
	backchannel_sync_send(SERVER_CLIENT_CHAN, SERVER_ID);
	/* wait for server to check that the subscribtion has been restored */
	backchannel_sync_wait(SERVER_CLIENT_CHAN, SERVER_ID);

	/* wait for server to check that the subscription no longer exist */
	backchannel_sync_send(SERVER_CLIENT_CHAN, SERVER_ID);
}

/* Util functions */

void central_backchannel_init(void)
{
	uint device_number = get_device_nbr();
	uint channel_numbers[2] = {
		0,
		0,
	};
	uint device_numbers[2];
	uint num_ch = 2;
	uint *ch;

	device_numbers[0] = (device_number == GOOD_CLIENT_ID) ? BAD_CLIENT_ID : GOOD_CLIENT_ID;
	device_numbers[1] = SERVER_ID;

	LOG_DBG("Opening back channels for device %d", device_number);
	ch = bs_open_back_channel(device_number, device_numbers, channel_numbers, num_ch);
	if (!ch) {
		TEST_FAIL("Unable to open backchannel");
	}
	LOG_DBG("Back channels for device %d opened", device_number);
}

static void set_public_addr(void)
{
	bt_addr_le_t addr = {BT_ADDR_LE_RANDOM, {{0x0A, 0x89, 0x67, 0x45, 0x23, 0xC1}}};

	bt_id_create(&addr, NULL);
}

/* Main functions */

void run_central(void)
{
	int err;

	central_cb.connected = connected;
	central_cb.disconnected = disconnected;
	central_cb.security_changed = security_changed;

	central_backchannel_init();
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

	/* tell the bad client that we disconnected and wait for him to disconnect */
	backchannel_sync_send(CLIENT_CLIENT_CHAN, BAD_CLIENT_ID);
	backchannel_sync_wait(CLIENT_CLIENT_CHAN, BAD_CLIENT_ID);

	connect_restore_sec();
	disconnect();

	TEST_PASS("Central test passed");
}

void run_bad_central(void)
{
	int err;

	central_cb.connected = connected;
	central_cb.disconnected = disconnected;
	central_cb.security_changed = security_changed;

	central_backchannel_init();
	set_public_addr();

	/* wait for good central to disconnect from server */
	backchannel_sync_wait(CLIENT_CLIENT_CHAN, GOOD_CLIENT_ID);

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)");
	}

	LOG_DBG("Bluetooth initialized");

	bt_conn_cb_register(&central_cb);

	err = settings_load();
	if (err) {
		TEST_FAIL("Settings load failed (err %d)");
	}

	connect_unsubscribe();
	disconnect();

	TEST_PASS("Bad Central test passed");

	/* tell the good client that we disconnected from the server */
	backchannel_sync_send(CLIENT_CLIENT_CHAN, GOOD_CLIENT_ID);
}
