/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

#include "testlib/conn.h"
#include "testlib/scan.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"
#include "bt_settings_hook.h"

LOG_MODULE_REGISTER(server, LOG_LEVEL_DBG);

static DEFINE_FLAG(ccc_cfg_changed_flag);
static DEFINE_FLAG(disconnected_flag);
static DEFINE_FLAG(security_changed_flag);

static struct bt_conn_cb server_conn_cb;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("CCC Update: notification %s", notif_enabled ? "enabled" : "disabled");

	SET_FLAG(ccc_cfg_changed_flag);
}

BT_GATT_SERVICE_DEFINE(test_gatt_service, BT_GATT_PRIMARY_SERVICE(test_service_uuid),
		       BT_GATT_CHARACTERISTIC(test_characteristic_uuid, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr_str, reason);

	SET_FLAG(disconnected_flag);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	TEST_ASSERT(err == 0, "Security update failed: %s level %u err %d", addr_str, level, err);

	LOG_DBG("Security changed: %s level %u", addr_str, level);
	SET_FLAG(security_changed_flag);
}

static void init_server_conn_callbacks(void)
{
	int err;

	server_conn_cb.connected = NULL;
	server_conn_cb.disconnected = disconnected;
	server_conn_cb.security_changed = security_changed;
	server_conn_cb.identity_resolved = NULL;

	err = bt_conn_cb_register(&server_conn_cb);
	TEST_ASSERT(err == 0, "Failed to set server conn callbacks (err %d)", err);
}

static void connect_and_set_security(struct bt_conn **conn)
{
	int err;
	bt_addr_le_t client = {};

	err = bt_testlib_scan_find_name(&client, ADVERTISER_NAME);
	TEST_ASSERT(err == 0, "Failed to start scan (err %d)", err);

	err = bt_testlib_connect(&client, conn);
	TEST_ASSERT(err == 0, "Failed to initiate connection (err %d)", err);

	err = bt_conn_set_security(*conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "Failed to set security (err %d)", err);

	WAIT_FOR_FLAG(security_changed_flag);
}

void server_procedure(void)
{
	/* Test purpose:
	 *
	 * Verifies that we are deleting GATT settings linked to a peer that we
	 * bonded with.
	 *
	 * Two devices:
	 * - `server`: GATT server, connect and elevate security
	 * - `client`: GATT client, when connected, will subscribe to CCC
	 *
	 * [verdict]
	 * - the server doesn't have settings leftover
	 */

	int err;
	struct bt_conn *conn = NULL;

	int number_of_settings_left = 0;

	TEST_START("server");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Cannot enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	err = settings_load();
	TEST_ASSERT(err == 0, "Failed to load settings (err %d)", err);

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	TEST_ASSERT(err == 0, "(1) Failed to unpair (err %d)", err);

	start_settings_record();

	init_server_conn_callbacks();

	connect_and_set_security(&conn);

	WAIT_FOR_FLAG(ccc_cfg_changed_flag);

	/* Settings may be written to flash on disconnection. */
	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect (err %d)", err);

	WAIT_FOR_FLAG(disconnected_flag);

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	TEST_ASSERT(err == 0, "(2) Failed to unpair (err %d)", err);

	number_of_settings_left = get_settings_list_size();

	stop_settings_record();
	settings_list_cleanup();

	if (number_of_settings_left > 0) {
		TEST_FAIL("'bt_unpair' did not clear the settings properly.");
	}

	TEST_PASS("server");
}
