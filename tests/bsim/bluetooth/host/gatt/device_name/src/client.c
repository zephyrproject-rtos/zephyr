/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

#include "testlib/adv.h"
#include "testlib/att.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

LOG_MODULE_REGISTER(client, LOG_LEVEL_DBG);

static DEFINE_FLAG(client_security_changed_flag);

static struct bt_conn_cb client_conn_cb;

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	TEST_ASSERT(err == 0, "Security update failed: %s level %u err %d", addr_str, level, err);

	LOG_DBG("Security changed: %s level %u", addr_str, level);
	SET_FLAG(client_security_changed_flag);
}

static void init_client_conn_callbacks(void)
{
	int err;

	client_conn_cb.connected = NULL;
	client_conn_cb.disconnected = NULL;
	client_conn_cb.security_changed = security_changed;

	err = bt_conn_cb_register(&client_conn_cb);
	TEST_ASSERT(err == 0, "Failed to set client conn callbacks (err %d)", err);
}

void find_characteristic(struct bt_conn *conn, const struct bt_uuid *svc,
			 const struct bt_uuid *chrc, uint16_t *chrc_value_handle)
{
	int err;
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_end_handle;

	err = bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn, svc,
					       BT_ATT_FIRST_ATTRIBUTE_HANDLE,
					       BT_ATT_LAST_ATTRIBUTE_HANDLE);
	TEST_ASSERT(err == 0, "Failed to discover service: %d", err);

	LOG_DBG("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	err = bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle, NULL,
						      conn, chrc, (svc_handle + 1), svc_end_handle);
	TEST_ASSERT(err == 0, "Failed to get value handle: %d", err);

	LOG_DBG("chrc_value_handle: %u, chrc_end_handle: %u", *chrc_value_handle, chrc_end_handle);
}

void client_procedure(void)
{
	int err;
	struct bt_conn *conn;
	uint16_t handle;

	char server_new_name[CONFIG_BT_DEVICE_NAME_MAX];

	NET_BUF_SIMPLE_DEFINE(attr_value_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

	generate_name(server_new_name, CONFIG_BT_DEVICE_NAME_MAX);

	TEST_START("client");

	bk_sync_init();

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Cannot enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	init_client_conn_callbacks();

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, ADVERTISER_NAME);
	TEST_ASSERT(err == 0, "Failed to start connectable advertising (err %d)", err);

	err = bt_testlib_att_exchange_mtu(conn);
	TEST_ASSERT(err == 0, "Failed to update MTU (err %d)", err);

	find_characteristic(conn, BT_UUID_GAP, BT_UUID_GAP_DEVICE_NAME, &handle);

	err = bt_testlib_att_read_by_handle_sync(&attr_value_buf, NULL, NULL, conn,
						 BT_ATT_CHAN_OPT_UNENHANCED_ONLY, handle, 0);
	TEST_ASSERT(err == 0, "Failed to read characteristic (err %d)", err);

	LOG_DBG("Device Name of the server: %.*s", attr_value_buf.len, attr_value_buf.data);

	err = bt_testlib_att_write(conn, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, handle, server_new_name,
				   sizeof(server_new_name));
	TEST_ASSERT(err == BT_ATT_ERR_SUCCESS, "Got ATT error: %d", err);

	bk_sync_send();

	TEST_PASS("client");
}
